#pragma once

#include "s_helpers.h";


int main(int arg, char** argv)
{

	//Creating Room vector
	std::vector<sRoom*> Rooms;

	sRoom* RoomA = new sRoom;
	RoomA->r_Name = 'A';

	Rooms.push_back(RoomA);

	sRoom* RoomB = new sRoom;
	RoomB->r_Name = 'B';

	Rooms.push_back(RoomB);

	//creating a bool to hold if a client has connected
	bool atLeastOneConnection = false;


	// Initialize WinSock
	WSADATA wsaData;
	int result;

	// Set version 2.2 with MAKEWORD(2,2)
	// https://learn.microsoft.com/en-us/windows/win32/api/winsock2/nf-winsock2-wsastartup
	result = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (result != 0) {
		printf("WSAStartup failed with error %d\n", result);
		return 1;
	}
	printf("WSAStartup successfully!\n");

	struct addrinfo* info = nullptr;
	struct addrinfo hints;
	ZeroMemory(&hints, sizeof(hints));	// ensure we don't have garbage data 
	hints.ai_family = AF_INET;			// IPv4
	hints.ai_socktype = SOCK_STREAM;	// Stream
	hints.ai_protocol = IPPROTO_TCP;	// TCP
	hints.ai_flags = AI_PASSIVE;

	// https://learn.microsoft.com/en-us/windows/win32/api/ws2tcpip/nf-ws2tcpip-getaddrinfo
	result = getaddrinfo(NULL, DEFAULT_PORT, &hints, &info);
	if (result != 0) {
		printf("getaddrinfo failed with error %d\n", result);
		WSACleanup();
		return 1;
	}
	printf("getaddrinfo successfully!\n");

	// Socket
	// https://learn.microsoft.com/en-us/windows/win32/api/winsock2/nf-winsock2-socket
	SOCKET listenSocket = socket(info->ai_family, info->ai_socktype, info->ai_protocol);
	if (listenSocket == INVALID_SOCKET) {
		printf("socket failed with error %d\n", WSAGetLastError());
		freeaddrinfo(info);
		WSACleanup();
		return 1;
	}
	printf("socket created successfully!\n");

	// Bind
	// https://learn.microsoft.com/en-us/windows/win32/api/winsock2/nf-winsock2-bind
	result = bind(listenSocket, info->ai_addr, (int)info->ai_addrlen);
	if (result == SOCKET_ERROR) {
		printf("bind failed with error %d\n", WSAGetLastError());
		closesocket(listenSocket);
		freeaddrinfo(info);
		WSACleanup();
		return 1;
	}
	printf("bind was successful!\n");

	// Listen
	// https://learn.microsoft.com/en-us/windows/win32/api/winsock2/nf-winsock2-listen
	result = listen(listenSocket, SOMAXCONN);
	if (result == SOCKET_ERROR) {
		printf("listen failed with error %d\n", WSAGetLastError());
		closesocket(listenSocket);
		freeaddrinfo(info);
		WSACleanup();
		return 1;
	}
	printf("listen successful\n");
	
	//auth client stuff

	struct addrinfo* authInfo = nullptr;
	struct addrinfo authHints;
	ZeroMemory(&authHints, sizeof(authHints));	// ensure we don't have garbage data 
	authHints.ai_family = AF_INET;			// IPv4
	authHints.ai_socktype = SOCK_STREAM;	// Stream
	authHints.ai_protocol = IPPROTO_TCP;	// TCP
	authHints.ai_flags = AI_PASSIVE;


	result = getaddrinfo("127.0.0.1", AUTH_PORT, &authHints, &authInfo);
	if (result != 0) {
		printf("getaddrinfo failed with error %d\n", result);
		WSACleanup();
		return 1;
	}
	printf("getaddrinfo successfully!\n");

	// Socket
	SOCKET serverSocket = socket(authInfo->ai_family, authInfo->ai_socktype, authInfo->ai_protocol);
	if (serverSocket == INVALID_SOCKET) {
		printf("socket failed with error %d\n", WSAGetLastError());
		freeaddrinfo(authInfo);
		WSACleanup();
		return 1;
	}
	printf("socket created successfully!\n");

	//setting the socket as non-blocking
	u_long iMode = 1;

	result = ioctlsocket(serverSocket, FIONBIO, &iMode);
	if (result != NOERROR) {
		printf("ioctlsocket failed with error: %ld\n", result);
	}

	// Connect
	result = connect(serverSocket, authInfo->ai_addr, (int)authInfo->ai_addrlen);
	if (serverSocket == INVALID_SOCKET) {
		printf("connect failed with error %d\n", WSAGetLastError());
		closesocket(serverSocket);
		freeaddrinfo(authInfo);
		WSACleanup();
		return 1;
	}
	printf("Connect to the server successfully!\n");

	Sleep(10);


	std::vector<SOCKET> activeConnections;

	FD_SET activeSockets;				// List of all of the clients ready to read.
	FD_SET socketsReadyForReading;		// List of all of the connections

	FD_ZERO(&activeSockets);			// Initialize the sets
	FD_ZERO(&socketsReadyForReading);

	// Use a timeval to prevent select from waiting forever.
	timeval tv;
	tv.tv_sec = 1;
	tv.tv_usec = 0;

	while (true)
	{
		//a way to shut the server down
		std::string s_Input;
		if (_kbhit()) {
			std::getline(std::cin, s_Input);
		}
		if (s_Input == "shutdown") {
			break;
		}

		Buffer gBuffer;
		std::string receviedMessage = s_helpers::ReadIncomingMessage(serverSocket, gBuffer, authInfo, Rooms);
		
		// Reset the socketsReadyForReading
		FD_ZERO(&socketsReadyForReading);

		// Add our listenSocket to our set to check for new connections
		// This will remain set if there is a "connect" call from a 
		// client to our server.
		FD_SET(listenSocket, &socketsReadyForReading);

		// Add all of our active connections to our socketsReadyForReading
		// set.
		for (int i = 0; i < activeConnections.size(); i++)
		{
			FD_SET(activeConnections[i], &socketsReadyForReading);
		}

		// https://learn.microsoft.com/en-us/windows/win32/api/winsock2/nf-winsock2-select
		int count = select(0, &socketsReadyForReading, NULL, NULL, &tv);
		if (count == 0)
		{
			// Timevalue expired
			//continue;
		}
		if (count == SOCKET_ERROR)
		{
			// Handle an error
		//	printf("select had an error %d\n", WSAGetLastError());
			continue;
		}

		// Loop through socketsReadyForReading
		//   recv
		for (int i = 0; i < activeConnections.size(); i++)
		{
			SOCKET socket = activeConnections[i];
			if (FD_ISSET(socket, &socketsReadyForReading))
			{
				// Handle receiving data with a recv call
				//char buffer[bufSize];
				const int bufSize = 512;
				Buffer buffer(bufSize);

				// https://learn.microsoft.com/en-us/windows/win32/api/winsock2/nf-winsock2-recv
				// result 
				//		-1 : SOCKET_ERROR (More info received from WSAGetLastError() after)
				//		0 : client disconnected
				//		>0: The number of bytes received.
				int result = recv(socket, (char*)(&buffer.m_BufferData[0]), bufSize, 0);
				if (result == SOCKET_ERROR) {
					printf("recv failed with error %d\n", WSAGetLastError());
					closesocket(listenSocket);
					freeaddrinfo(info);
					WSACleanup();
					break;
				}

				//printf("Received %d bytes from the client!\n", result);

			
				s_helpers::HandleClientInput(Rooms, buffer, socket, serverSocket, info);

				//// https://learn.microsoft.com/en-us/windows/win32/api/winsock2/nf-winsock2-send
				//result = send(socket, buffer, 512, 0);
				//if (result == SOCKET_ERROR) {
				//	printf("send failed with error %d\n", WSAGetLastError());
				//	closesocket(listenSocket);
				//	freeaddrinfo(info);
				//	WSACleanup();
				//	break;
				//}

				FD_CLR(socket, &socketsReadyForReading);
				count--;
			}
		}

		// Handle any new connections, if count is not 0 then we have 
		// a socketReadyForReading that is not an active connection,
		// which means we have a 'connect' request from a client.
		//   accept
		if (count > 0)
		{
			if (FD_ISSET(listenSocket, &socketsReadyForReading))
			{
				// https://learn.microsoft.com/en-us/windows/win32/api/winsock2/nf-winsock2-accept
				SOCKET newConnection = accept(listenSocket, NULL, NULL);
				if (newConnection == INVALID_SOCKET)
				{
					// Handle errors
					printf("accept failed with error: %d\n", WSAGetLastError());
				}
				else
				{
					//// io = input, output, ctl = control
					//// input output control socket
					//// https://learn.microsoft.com/en-us/windows/win32/api/winsock/nf-winsock-ioctlsocket
					////-------------------------
					//// Set the socket I/O mode: In this case FIONBIO
					//// enables or disables the blocking mode for the 
					//// socket based on the numerical value of iMode.
					//// If iMode = 0, blocking is enabled; 
					//// If iMode != 0, non-blocking mode is enabled.
					// unsigned long NonBlock = 1;
					// ioctlsocket(newConnection, FIONBIO, &NonBlock);

					// Handle successful connection
					activeConnections.push_back(newConnection);
					FD_SET(newConnection, &activeSockets);
					FD_CLR(listenSocket, &socketsReadyForReading);

					printf("Client connected with Socket: %d\n", (int)newConnection);

					//set the atLeastOneConnectionBool to true
					atLeastOneConnection = false;

				}
			}
		}
	}

	// Cleanup
	// https://learn.microsoft.com/en-us/windows/win32/api/ws2tcpip/nf-ws2tcpip-freeaddrinfo
	freeaddrinfo(info);

	// https://learn.microsoft.com/en-us/windows/win32/api/winsock2/nf-winsock2-closesocket
	closesocket(listenSocket);

	// TODO Close connection for each client socket
	s_helpers::CloseClientSockets(activeConnections, info);
	// https://learn.microsoft.com/en-us/windows/win32/api/winsock2/nf-winsock2-wsacleanup
	WSACleanup();

	return 0;
}