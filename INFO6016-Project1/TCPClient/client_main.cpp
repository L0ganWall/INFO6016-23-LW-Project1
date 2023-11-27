#pragma once
#include "c_helpers.h"
#include <ctime>

int main(int arg, char** argv)
{
	//Create a variable to store this client's name
	std::string clientName = "temp";
	srand(time(0));
	int clientId = rand();

	//creating a room vector, it stores the messages of each room that this client is in, since this client joined the room
	std::vector<cRoom*> rooms;


	// Initialize WinSock
	WSADATA wsaData;
	int result;

	// Set version 2.2 with MAKEWORD(2,2)
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


	result = getaddrinfo("127.0.0.1", DEFAULT_PORT, &hints, &info);
	if (result != 0) {
		printf("getaddrinfo failed with error %d\n", result);
		WSACleanup();
		return 1;
	}
	printf("getaddrinfo successfully!\n");

	// Socket
	SOCKET serverSocket = socket(info->ai_family, info->ai_socktype, info->ai_protocol);
	if (serverSocket == INVALID_SOCKET) {
		printf("socket failed with error %d\n", WSAGetLastError());
		freeaddrinfo(info);
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
	result = connect(serverSocket, info->ai_addr, (int)info->ai_addrlen);
	if (serverSocket == INVALID_SOCKET) {
		printf("connect failed with error %d\n", WSAGetLastError());
		closesocket(serverSocket);
		freeaddrinfo(info);
		WSACleanup();
		return 1;
	}
	printf("Connect to the server successfully!\n");

	Sleep(10);

	printf("Enter the command: REGISTER your_email_here your_password_here to create a new account\n");
	printf("Enter the command: AUTHENTICATE your_email_here your_password_here to login to an existing account\n");

	Buffer newBuffer(512);

	std::string u_Input = "";
	std::string chatRooms = "";
	while (true) {
		std::getline(std::cin, u_Input);
		if (u_Input != "") {
			std::string delimiter = " ";
			std::string command = "";
			std::string email = "";
			std::string password = "";

			int commandNum = 0;

			int temp2 = u_Input.find_last_of(delimiter);
			int temp = u_Input.find(delimiter);

			command = u_Input.substr(0, temp);
			email = u_Input.substr(temp, temp2 - temp);
			password = u_Input.substr(temp2 + 1);

			ChatMessage message;

			if (command == "REGISTER") {
				commandNum = 4;
			}
			else if (command == "AUTHENTICATE") {
				commandNum = 5;
			}
			else {
				continue;
			}

			std::string joinMessage = std::to_string(clientId) + ":" + u_Input;
			message = c_helpers::CreateChatMessage(joinMessage, commandNum);
			Buffer buffer = c_helpers::CreateBuffer(message);

			result = send(serverSocket, (char*)(&buffer.m_BufferData[0]), message.header.packetSize, 0);
			if (result == SOCKET_ERROR) {
				printf("send failed with error %d\n", WSAGetLastError());
				closesocket(serverSocket);
				freeaddrinfo(info);
				WSACleanup();
				return 1;
			}
		}
		chatRooms = c_helpers::ReadIncomingMessage(serverSocket, newBuffer, info);
		std::string start = chatRooms.substr(0, '\n');
		//the message back will always start with s if it succeded
		if (chatRooms[0] == 's') {
			break;
		}
	}


	//a bool to keep track if a message should be sent
	bool sendMsg = false;


	while (true) {
		Buffer readBuffer(512);
		std::string receviedMessage = c_helpers::ReadIncomingMessage(serverSocket, readBuffer, info);
		if (receviedMessage.size() > 0) {
			std::string delimiter = ":";
			std::string roomName = receviedMessage.substr(0, receviedMessage.find(delimiter));
			std::string savedMessage = receviedMessage.substr(receviedMessage.find(delimiter) + 1);
			//add this message to the specific rooms message list
			for (int idx = 0; idx < rooms.size(); idx++) {
				if (rooms[idx]->roomName == roomName) {
					rooms[idx]->messages.push_back(savedMessage);
				}
			}
		}

		//getting user input without blocking all the time
		//using conio's _getch leads to an issue where an message can be recieved inside the message your typing
		//while it has no impact on the message that's being sent, it's inconvient, hello wbob has joined the roomorld
		if (_kbhit()) {
			std::getline(std::cin, u_Input);
		}

		//Ensuring there's input before doing this
		if (u_Input != "") {
			//checking if join or leave were called
			std::string delimiter = " ";
			std::string command = "";
			std::string roomName = "";
			command = u_Input.substr(0, u_Input.find(delimiter));
			roomName = u_Input.substr(u_Input.find(delimiter) + 1);


			//the chat message to hold the message whatever it is
			ChatMessage message;

			if (command == "join") {
				//add this room to the client's list
				//checking to make sure this room isn't added twice
				if (rooms.size() == 0) {
					rooms.push_back(new cRoom(roomName));
				}
				else {
					for (int idx = 0; idx < rooms.size(); idx++) {
						if (rooms[idx]->roomName != roomName) {
							rooms.push_back(new cRoom(roomName));
						}
					}
				}

				//create the message for the server
				std::string joinMessage = roomName + ":" + clientName;
				message = c_helpers::CreateChatMessage(joinMessage, 2);
				Buffer buffer = c_helpers::CreateBuffer(message);

				// Write
				result = send(serverSocket, (char*)(&buffer.m_BufferData[0]), message.header.packetSize, 0);
				if (result == SOCKET_ERROR) {
					printf("send failed with error %d\n", WSAGetLastError());
					closesocket(serverSocket);
					freeaddrinfo(info);
					WSACleanup();
					return 1;
				}
			}
			else if (command == "leave") {
				//Leave the client's list
				for (int idx = 0; idx < rooms.size(); idx++) {
					if (rooms[idx]->roomName == roomName) {
						rooms[idx] = nullptr;
						rooms.erase(rooms.begin() + idx);
					}
				}

				std::string leaveMessage = roomName + ":" + clientName;
				message = c_helpers::CreateChatMessage(leaveMessage, 3);
				Buffer buffer = c_helpers::CreateBuffer(message);

				// Write
				result = send(serverSocket, (char*)(&buffer.m_BufferData[0]), message.header.packetSize, 0);
				if (result == SOCKET_ERROR) {
					printf("send failed with error %d\n", WSAGetLastError());
					closesocket(serverSocket);
					freeaddrinfo(info);
					WSACleanup();
					return 1;
				}
			}
			else if (command == "Exit") {
				break;

			}	else if (command != "") {
				//creating the message to send
				std::string chatMessage = u_Input.insert(u_Input.find(":"), ":" + clientName);
				std::string delimiter = ":";
				std::string roomName = chatMessage.substr(0, chatMessage.find(delimiter));
				std::string savedMessage = chatMessage.substr(chatMessage.find(delimiter) + 1);

				//creating ChatMessage and Buffer
				message = c_helpers::CreateChatMessage(chatMessage, 1);
				Buffer buffer = c_helpers::CreateBuffer(message);

				// Write
				result = send(serverSocket, (char*)(&buffer.m_BufferData[0]), message.header.packetSize, 0);
				if (result == SOCKET_ERROR) {
					printf("send failed with error %d\n", WSAGetLastError());
					closesocket(serverSocket);
					freeaddrinfo(info);
					WSACleanup();
					return 1;
				}

				//add this message to the specific rooms message list
				for (int idx = 0; idx < rooms.size(); idx++) {
					if (rooms[idx]->roomName == roomName) {
						rooms[idx]->messages.push_back(savedMessage);
					}
				}
			}
		}
		u_Input = "";
		c_helpers::PrintClient(chatRooms, rooms);
		Sleep(50);
	}

	// Close
	result = shutdown(serverSocket, SD_SEND);
	if (result == SOCKET_ERROR) {
		printf("shutdown failed: %d\n", WSAGetLastError());
		closesocket(serverSocket);
		WSACleanup();
		return 1;
	}
	freeaddrinfo(info);
	closesocket(serverSocket);
	WSACleanup();

	return 0;
}
