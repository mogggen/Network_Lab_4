#include <iostream>
#include <ctime>
#include <string>
#include <cstring>
#include <WS2tcpip.h>
#include <thread>
#pragma comment(lib, "ws2_32.lib")

//Enums och constants
#define MAXNAMELEN  32

enum ObjectDesc {
	Human,
	NonHuman,
	Vehicle,
	StaticObject
};

enum ObjectForm {
	Cube,
	Sphere,
	Pyramid,
	Cone
};

struct Coordinate {
	int x;
	int y;
};

enum MsgType {
	Join,           //Client joining game at server
	Leave,          //Client leaving game
	Change,         //Information to clients
	Event,          //Information from clients to server
	TextMessage     //Send text messages to one or all
};
//MESSAGE HEAD, Included first in all messages   
struct MsgHead {
	unsigned int length;     //Total length for whole message   
	unsigned int seqNo;      //Sequence number
	unsigned int id;         //Client ID or 0;
	MsgType type;            //Type of message
};
//JOIN MESSAGE (CLIENT->SERVER)
struct JoinMsg {
	MsgHead head;
	ObjectDesc desc;
	ObjectForm form;
	char name[MAXNAMELEN];   //null terminated!,or empty
};
//LEAVE MESSAGE (CLIENT->SERVER)
struct LeaveMsg {
	MsgHead head;
};
//CHANGE MESSAGE (SERVER->CLIENT)
enum ChangeType {
	NewPlayer,
	PlayerLeave,
	NewPlayerPosition
};
//Included first in all Change messages
struct ChangeMsg {
	MsgHead head;
	ChangeType type;
};

struct NewPlayerMsg {
	ChangeMsg msg;          //Change message header with new client id
	ObjectDesc desc;
	ObjectForm form;
	char name[MAXNAMELEN];  //nullterminated!,or empty
};
struct PlayerLeaveMsg {
	ChangeMsg msg;          //Change message header with new client id
};
struct NewPlayerPositionMsg {
	ChangeMsg msg;          //Change message header
	Coordinate pos;         //New object position
	Coordinate dir;         //New object direction
};
//EVENT MESSAGE (CLIENT->SERVER)
enum EventType { Move };
//Included first in all Event messages
struct EventMsg {
	MsgHead head;
	EventType type;
};
//Variantions of EventMsg
struct MoveEvent {
	EventMsg event;
	Coordinate pos;         //New object position
	Coordinate dir;         //New object direction
};
//TEXT MESSAGE
struct TextMessageMsg {
	MsgHead head;
	char text[1];   //NULL-terminerad array of chars.
};

int x, y;
bool isFinished = false;

void ConnectGUI(SOCKET sock)
{

}

void move(SOCKET sock, int clientid)
{
	MoveEvent moving =
	{
		{{0, 0, clientid, Event}, Move},
		{x, y},
		{0, 0}
	};
	moving.event.head.length = sizeof(MoveEvent);
	send(sock, (char*)&moving, moving.event.head.length, 0);
}

void leave(SOCKET sock, int clientid)
{
	LeaveMsg leaveMsg =
	{
		{0, 0, clientid, Leave}
	};
	leaveMsg.head.length = sizeof(LeaveMsg);
	send(sock, (char*)&leaveMsg, leaveMsg.head.length, 0);
}

void ConnectServer(SOCKET sock)
{
	char buf[192];
	MsgHead* msgHead = (MsgHead*)buf;

	ChangeMsg* changeMsg = (ChangeMsg*)buf;
	
	NewPlayerMsg* newPlayerMsg = (NewPlayerMsg*)buf;
	PlayerLeaveMsg* playerLeaveMsg = (PlayerLeaveMsg*)buf;
	NewPlayerPositionMsg* newPlayerPositionMsg = (NewPlayerPositionMsg*)buf;
	int id = -1;

	
	while (!isFinished)
	{
		msgHead = (MsgHead*)buf;

		changeMsg = (ChangeMsg*)buf;

		newPlayerMsg = (NewPlayerMsg*)buf;
		playerLeaveMsg = (PlayerLeaveMsg*)buf;
		newPlayerPositionMsg = (NewPlayerPositionMsg*)buf;

		int count = recv(sock, buf, sizeof(buf), 0);
		using namespace std;
		while (msgHead <= (MsgHead*)(buf + sizeof(buf) / sizeof(*buf)))
		{
			switch (changeMsg->type)
			{
			case NewPlayer:
				std::cout << '[' << newPlayerMsg->msg.head.seqNo << "]:\t" <<
					newPlayerMsg->name << " joined\tId=" << newPlayerMsg->msg.head.id << " " << endl;
				//for each (client c in clientList)
				if (id == -1)
				{
					id = newPlayerMsg->msg.head.id;
					SetConsoleTitle((string(newPlayerMsg->name) + string(": Id=") + to_string(id)).c_str());
					move(sock, id); // dummy command to recive the player coordinates
				}
				break;

			case PlayerLeave:
				std::cout << '[' << playerLeaveMsg->msg.head.seqNo << "]:\tleft server\tId=" << playerLeaveMsg->msg.head.id << endl;
				if (playerLeaveMsg->msg.head.id == id) isFinished = true;
				break;

			case NewPlayerPosition:
				std::cout << '[' << newPlayerPositionMsg->msg.head.seqNo << "]:\tpos:(";

				std::cout << newPlayerPositionMsg->pos.x << ";";
				std::cout << newPlayerPositionMsg->pos.y << ")\tId=" << newPlayerPositionMsg->msg.head.id << endl;
				if (newPlayerPositionMsg->msg.head.id == id)
				{
					x = newPlayerPositionMsg->pos.x;
					y = newPlayerPositionMsg->pos.y;
				}
				break;

			default:
				std::cout << "pause debugger..." << endl;
				cin.get();
				break;
			}

			msgHead = (MsgHead*)(msgHead + msgHead->length);

			newPlayerMsg = (NewPlayerMsg*)msgHead;
			playerLeaveMsg = (PlayerLeaveMsg*)msgHead;
			newPlayerPositionMsg = (NewPlayerPositionMsg*)msgHead;
		}
	}
}

using namespace std;
int main()
{
	string ipAddress = "130.240.40.25";	// IP Address of the server
	int LinuxPort = 49152; 54000; 		// Linux Server port

	/*ipAddress = "127.0.0.1";
	LinuxPort = 9002;*/

	// Initialize WinSock
	WSAData data;
	WORD ver = MAKEWORD(2, 2);
	int wsResult = WSAStartup(ver, &data);
	if (wsResult != 0)
	{
		cerr << "Can't start Winsock, Err #" << wsResult << endl;
		return 0;
	}

	// Create socket
	SOCKET listening = socket(AF_INET, SOCK_STREAM, 0);

	// Fill in a hint structure
	sockaddr_in sockaddr_in;
	sockaddr_in.sin_family = AF_INET;
	sockaddr_in.sin_port = htons(LinuxPort);
	inet_pton(AF_INET, ipAddress.c_str(), &sockaddr_in.sin_addr);

	// Error Handeling
	int conRes = connect(listening, (sockaddr*)&sockaddr_in, sizeof(sockaddr_in));
	if (conRes == SOCKET_ERROR)
	{
		string prompt = "Can't connect to server, Error: ";
		int error = WSAGetLastError();
		if (error == 10061)
			cerr << prompt << "timeout" << endl;
		else
			cerr << prompt << '#' << error << endl;
		closesocket(listening);
		WSACleanup();
		return 0;
	}

	char buf[MAXNAMELEN];
	char command[MAXNAMELEN];

	JoinMsg joining
	{
		{0, 0, 0, Join},
		Human,
		Pyramid,
		"Lisa"
	};
	joining.head.length = sizeof(joining);

	send(listening, (char*)&joining, joining.head.length, 0);
	recv(listening, buf, sizeof(buf), 0);
	MsgHead* msgHead = (MsgHead*)buf;

	int clientid = msgHead->id;
	joining.head.id = clientid;
		
	// Byt ut mot Non-blocking IO
	// if (FD_ISSET) är användbart ;)
	// fd_set
	// FD_CLR
	// FD_ZERO
	// FD_SET
	thread listen(ConnectServer, listening);

	Sleep(1000);
	// while loop to send data
	while (!isFinished)
	{
		//send to linux server
		scanf_s("%s", command, 16); // take from java key input
		for (char i = 0; i < sizeof(command) / sizeof(char); i++)
		{
			command[i] = tolower(command[i]);
		}

		if (!strcmp(command, "moveu"))
		{
			y++;
			move(listening, clientid);
		}
		else if (!strcmp(command, "moved"))
		{
			y--;
			move(listening, clientid);
		}
		else if (!strcmp(command, "movel"))
		{
			x--;
			move(listening, clientid);
		}
		else if (!strcmp(command, "mover"))
		{
			x++;
			move(listening, clientid);
		}

		else if (!strcmp(command, "leave"))
		{
			//leave(sock, clientid);
			isFinished = true;
		}
		else
		{
			cout << "Unknown command: \'" << command << "\'\n";
		}
		//recive from linux server

		//send to java client
	}

	// Close everything
	listen.join();
	Sleep(0.5);
	closesocket(listening);
	WSACleanup();
	return 0;
}
