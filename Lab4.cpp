#include <iostream>
#include <ctime>
#include <string>
#include <sstream>
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
bool loop = true;

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

	
	while (loop)
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
					SetConsoleTitle((string(newPlayerMsg->name) + string(": id=") + to_string(id)).c_str());
					move(sock, id); // dummy command to recive the player coordinates
				}
				break;

			case PlayerLeave:
				std::cout << '[' << playerLeaveMsg->msg.head.seqNo << "]:\tleft server\tId=" << playerLeaveMsg->msg.head.id << endl;
				if (playerLeaveMsg->msg.head.id == id) loop = true;
				break;

			case NewPlayerPosition:
				std::cout << '[' << newPlayerPositionMsg->msg.head.seqNo << "]:\tpos:(";

				std::cout << newPlayerPositionMsg->pos.x << ";";
				std::cout << newPlayerPositionMsg->pos.y << ")\tid=" << newPlayerPositionMsg->msg.head.id << endl;
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
	string javaAddress = "127.0.0.1";
	int javaPort = 4999;

	string LinuxIpAddress = "130.240.40.7";	// IP Address of the server
	int LinuxPort = 54000;// 49152;				// Linux Server port

	LinuxIpAddress = "127.0.0.1";
	LinuxPort = 9002;

	WSAData data;
	WORD ver = MAKEWORD(2, 2);
	int wsResult = WSAStartup(ver, &data);
	if (wsResult != 0)
	{
		cerr << "Can't start Winsock, Err #" << wsResult << endl;
		return 0;
	}
	
	// Create socket
	SOCKET Linux_listening = socket(AF_INET, SOCK_STREAM, 0);
	
	// Fill in a Linux hint structure
	sockaddr_in Linux_sockaddr_in;
	Linux_sockaddr_in.sin_family = AF_INET;
	Linux_sockaddr_in.sin_port = htons(LinuxPort);
	inet_pton(AF_INET, LinuxIpAddress.c_str(), &Linux_sockaddr_in.sin_addr);

	// Error Handeling
	int conRes = connect(Linux_listening, (sockaddr*)&Linux_sockaddr_in, sizeof(Linux_sockaddr_in));
	if (conRes == SOCKET_ERROR)
	{
		string prompt = "Can't connect to server, Error: ";
		int error = WSAGetLastError();
		if (error == 10061)
			cerr << prompt << "timeout" << endl;
		else
			cerr << prompt << '#' << error << endl;
		closesocket(Linux_listening);
		WSACleanup();
		return 0;
	}

	// Create socket
	SOCKET java_listening = socket(AF_INET, SOCK_STREAM, 0);
	
	sockaddr_in java_sockaddr_in;
	java_sockaddr_in.sin_family = AF_INET;
	java_sockaddr_in.sin_port = htons(LinuxPort);
	inet_pton(AF_INET, LinuxIpAddress.c_str(), &java_sockaddr_in.sin_addr);

	// Error Handeling
	int conRes = connect(java_listening, (sockaddr*)&java_sockaddr_in, sizeof(java_sockaddr_in));
	if (conRes == SOCKET_ERROR)
	{
		string prompt = "Can't connect to server, Error: ";
		int error = WSAGetLastError();
		if (error == 10061)
			cerr << prompt << "timeout" << endl;
		else
			cerr << prompt << '#' << error << endl;
		closesocket(java_listening);
		WSACleanup();
		return 0;
	}

	


	char Linux_buf[MAXNAMELEN];
	char java_buf[MAXNAMELEN];
	char command[MAXNAMELEN];

	JoinMsg joining
	{
		{0, 0, 0, Join},
		Human,
		Pyramid,
		"Lisa"
	};
	joining.head.length = sizeof(joining);

	send(Linux_listening, (char*)&joining, joining.head.length, 0);
	recv(Linux_listening, Linux_buf, sizeof(Linux_buf), 0);
	MsgHead* msgHead = (MsgHead*)Linux_buf;

	int clientid = msgHead->id;
	joining.head.id = clientid;

	send(java_listening, (char*)&joining, joining.head.length, 0);
	recv(java_listening, java_buf, sizeof(java_buf), 0);

	// Byt ut mot Non-blocking IO
	// if (FD_ISSET) är anv�ndbart ;)
	// fd_set
	// FD_ZERO
	// FD_CLR
	// FD_SET
	fd_set Linux_master;
	FD_ZERO(&Linux_master);
	fd_set java_master;
	FD_ZERO(&java_master);
	//thread listen(ConnectServer, listening);

	Sleep(1000);
	while (!loop)
	{
		fd_set Linux_copy = Linux_master;

		int Linux_socketCount = select(0, &Linux_copy, nullptr, nullptr, nullptr);

		for (int i = 0; i < Linux_socketCount; i++)
		{
			SOCKET sock = Linux_copy.fd_array[i];

			if (sock == Linux_listening)
			{
				SOCKET client = accept(Linux_listening, nullptr, nullptr);

				// Add the new connection to the list of connected clients
				FD_SET(client, &Linux_master);

				string welcomeMsg = "Welcome to the Awesome Chat Server!\r\n";
				send(client, welcomeMsg.c_str(), welcomeMsg.size() + 1, 0);
			}
			else
			{
				char buf[4096];
				ZeroMemory(buf, 4096);

				int bytesIn = recv(sock, buf, 4096, 0);
				if (bytesIn <= 0)
				{
					closesocket(sock);
					FD_CLR(sock, &Linux_master);
				}

				for (int i = 0; i < Linux_master.fd_count; i++)
				{
					SOCKET outSock = Linux_master.fd_array[i];
					if (outSock != Linux_listening && outSock != sock)
					{
						ostringstream ss;
						ss << "SOCKET #" << sock << ": " << buf << "\r\n";
						string strOut = ss.str();

						send(outSock, strOut.c_str(), strOut.size() + 1, 0);
					}
				}
			}
		}
		
		fd_set java_copy = java_master;

		int java_socketCount = select(0, &java_copy, nullptr, nullptr, nullptr);

		for (int i = 0; i < java_socketCount; i++)
		{
			SOCKET sock = java_copy.fd_array[i];

			if (sock == java_listening)
			{
				SOCKET client = accept(java_listening, nullptr, nullptr);

				// Add the new connection to the list of connected clients
				FD_SET(client, &java_master);

				string welcomeMsg = "Welcome to the Awesome Chat Server!\r\n";
				send(client, welcomeMsg.c_str(), welcomeMsg.size() + 1, 0);
			}
			else
			{
				char buf[4096];
				ZeroMemory(buf, 4096);

				int bytesIn = recv(sock, buf, 4096, 0);
				if (bytesIn <= 0)
				{
					closesocket(sock);
					FD_CLR(sock, &java_master);
				}

				for (int i = 0; i < java_master.fd_count; i++)
				{
					SOCKET outSock = java_master.fd_array[i];
					if (outSock != java_listening && outSock != sock)
					{
						ostringstream ss;
						ss << "SOCKET #" << sock << ": " << buf << "\r\n";
						string strOut = ss.str();

						send(outSock, strOut.c_str(), strOut.size() + 1, 0);
					}
				}
			}
		}

		//send to linux server
		scanf_s("%s", command, 16);
		for (char i = 0; i < sizeof(command) / sizeof(char); i++)
		{
			command[i] = tolower(command[i]);
		}

		if (!strcmp(command, "moveu"))		//0
		{
			y++;
			move(Linux_listening, clientid);
		}
		else if (!strcmp(command, "moved"))	//1
		{
			y--;
			move(Linux_listening, clientid);
		}
		else if (!strcmp(command, "movel"))	//2
		{
			x--;
			move(Linux_listening, clientid);
		}
		else if (!strcmp(command, "mover"))	//3
		{
			x++;
			move(Linux_listening, clientid);
		}

		else if (!strcmp(command, "leave"))
		{
			//leave(sock, clientid);
			loop = false;
		}
		else
		{
			cout << "Unknown command: \'" << command << "\'\n";
		}
		//recive from linux server

		//send to java client
	}

	// Close everything
	Sleep(500);
	closesocket(Linux_listening);
	closesocket(java_listening);
	WSACleanup();
	return 0;
}
