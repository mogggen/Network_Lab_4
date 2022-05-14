#include <iostream>
#include <ctime>
#include <string>
#include <sstream>
#include <algorithm>
#include <cstring>
#include <vector>
#include <thread>
#ifdef _WIN32
#include <WS2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#elif __linux__
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <errno.h>
#endif

#ifdef __linux
#define SOCKET int
#endif

const int first_avalible_port(std::vector<int>& h)
{
	sort(h.begin(), h.end());
	for (size_t i = 0; i < h.size() - 1; i++)
	{
		if (h[i] + 1 != h[i + 1])
		{
			h.push_back(h[i] + 1);
			return h[i] + 1;
		}
	}
	// no gaps
	h.push_back(h[h.size() - 1] + 1);
	return h[h.size() - 1];
}

SOCKET* setup(const std::string& ipAddress, const int& port)
{
#ifdef _WIN32
	WSAData data;
	WORD ver = MAKEWORD(2, 2);
	int wsResult = WSAStartup(ver, &data);
	if (wsResult != 0)
	{
		std::cerr << "Can't start Winsock, Err #" << wsResult << std::endl;
		return nullptr;
	}
#endif

#ifndef errno
#define errno WSAGetLastError()
#endif

	SOCKET listening = socket(AF_INET, SOCK_STREAM, 0);

	sockaddr_in socketAddr_in;
	socketAddr_in.sin_family = AF_INET;
	socketAddr_in.sin_port = htons(port);
	inet_pton(AF_INET, ipAddress.c_str(), &socketAddr_in.sin_addr);

	int conRes = connect(listening, (sockaddr*)&socketAddr_in, sizeof(socketAddr_in));

#ifdef _WIN32
	if (conRes == SOCKET_ERROR)
	{
		std::string prompt = "Can't connect to server, Error: ";
		int error = errno;
		std::cerr << prompt << '#' << error << std::endl;
		closesocket(listening);
		WSACleanup();
		return nullptr;
	}
#endif
	return &listening;
}

void sending(const SOCKET& listening)
{
	char sendbuf[] = "Hello World!";
	while (true)
	{
		Sleep(1000);
		send(listening, sendbuf, sizeof(sendbuf), 0);
	}
}
void receiving(const SOCKET& listening)
{
	char recvbuf[1024];
	while (true)
	{
#ifdef __linux__
		ssize_t r = recv(listening, recvbuf, sizeof(recvbuf), 0);
		if (r == -1 && errno == EWOULDBLOCK)
		{
			printf("recv failed with error: EWOULDBLOCK\n");
		}
#elif _WIN32
		recv(listening, recvbuf, sizeof(recvbuf), 0);
#endif
		// handle the received data here
		printf("%s", recvbuf);
		/* code */
	}

// closes the socket after receiving a afk ping
#ifdef _WIN32
		closesocket(listening);
		WSACleanup();
#elif __linux__
		close(listening);
#endif
}



int main()
{
	std::string ipAddress = "127.0.0.1";
	std::vector<int> starting_port;
	starting_port.push_back(4999); // linux_port
	// the client leaving should send which port they used so it could be removed from the vector.
	// count ports until one that isn't in use appears.

	std::vector<std::thread> threads;
	threads.resize(4);
	
	SOCKET java_connection = *setup(ipAddress, starting_port[0]);

	threads[0] = std::thread(sending, java_connection);
	threads[1] = std::thread(receiving, java_connection);

	std::cin.get();

	SOCKET linux_connection = *setup(ipAddress, first_avalible_port(starting_port));
	
	threads[2] = std::thread(sending, linux_connection);
	threads[3] = std::thread(receiving, linux_connection);
	
	while(true)
	{
		Sleep(1000);
	}
}