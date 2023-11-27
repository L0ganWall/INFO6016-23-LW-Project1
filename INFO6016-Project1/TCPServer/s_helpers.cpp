#include "s_helpers.h"
bool s_helpers::IsSenderInTheRoom(std::vector<sRoom*> rooms, const SOCKET& sender, int index) {
	for (int idx = 0; idx < rooms[index]->r_ClientList.size(); idx++) {
		if (rooms[index]->r_ClientList[idx] == sender) {
			return true;
		}
	}
	return false;
}

//helper to get the room index the client sent it in, it returns the room's index
int s_helpers::GetRoomIndexForMessage(std::vector<sRoom*> rooms, std::string message) {
	//a string delimiter to find the room at the start of the message
	std::string delimiter = ":";
	std::string roomName = message.substr(0, message.find(delimiter));

	for (int idx = 0; idx < rooms.size(); idx++) {
		if (rooms[idx]->r_Name == roomName) {
			return idx;
		}
	}
	return -1;
}

//helper to get the room name the message sent it in, it returns the room's index
std::string s_helpers::GetRoomIndexForMessage(std::string message) {
	//a string delimiter to find the room at the start of the message
	std::string delimiter = ":";
	std::string roomName = message.substr(0, message.find(delimiter));

	return roomName;
}

//helper to join a room
bool s_helpers::JoinRoom(std::vector<sRoom*> rooms, std::string roomName, const SOCKET& sender) {
	for (int idx = 0; idx < rooms.size(); idx++) {
		//also checking to make sure the client is not already in the room
		if (rooms[idx]->r_Name == roomName && !IsSenderInTheRoom(rooms, sender, idx)) {
			rooms[idx]->r_ClientList.push_back(sender);
			return true;
		}
	}
	return false;
}

//helper to leave a room
bool s_helpers::LeaveRoom(std::vector<sRoom*> rooms, std::string roomName, const SOCKET& sender) {
	//finds the correct room
	for (int idx = 0; idx < rooms.size(); idx++) {
		if (rooms[idx]->r_Name == roomName) {
			//finds the client in that room
			for (int clientIdx = 0; clientIdx < rooms[idx]->r_ClientList.size(); clientIdx++) {
				if (rooms[idx]->r_ClientList[clientIdx] == sender) {
					rooms[idx]->r_ClientList.erase(rooms[idx]->r_ClientList.begin() + clientIdx);
					return true;
				}
			}
		}
	}
	return false;
}

//Helper to create the ChatMessage format
ChatMessage s_helpers::CreateChatMessage(std::string s_Message, int messageType) {
	ChatMessage message;
	message.message = s_Message;
	message.messageLength = message.message.length();
	message.header.messageType = messageType;
	message.header.packetSize =
		message.message.length()
		+ sizeof(message.messageLength)
		+ sizeof(message.header.messageType)
		+ sizeof(message.header.packetSize);

	return message;
}

//Helper method to create and return the buffer
Buffer s_helpers::CreateBuffer(ChatMessage message) {

	//create the buffer
	const int bufSize = 512;
	Buffer newBuffer(bufSize);

	// Write our packet to the buffer
	newBuffer.WriteUInt32LE(message.header.packetSize);
	newBuffer.WriteUInt32LE(message.header.messageType);
	newBuffer.WriteUInt32LE(message.messageLength);
	newBuffer.WriteString(message.message);

	return newBuffer;
}

bool s_helpers::SendMessageToRoom(std::vector<sRoom*> rooms, int index, const SOCKET& sender, ChatMessage message, addrinfo* info) {
	//check and make sure the sender is connected to that room
	if (!IsSenderInTheRoom(rooms, sender, index)) {
		return false;
	}

	Buffer newBuffer = CreateBuffer(message);

	//sends message to everyone but the sender
	for (int idx = 0; idx < rooms[index]->r_ClientList.size(); idx++) {
		if (rooms[index]->r_ClientList[idx] != sender) {
			int result = send(rooms[index]->r_ClientList[idx], (const char*)(&newBuffer.m_BufferData[0]), message.header.packetSize, 0);
			if (result == SOCKET_ERROR) {
				printf("send failed with error %d\n", WSAGetLastError());
				closesocket(rooms[index]->r_ClientList[idx]);
				freeaddrinfo(info);
				WSACleanup();
			}
		}
	}
	return true;
}

//A helper function to announce that a client has left, it's different because the client sending it is no longer in the room
bool s_helpers::LeavingAnnouncement(std::vector<sRoom*> rooms, int index, const SOCKET& sender, ChatMessage message, addrinfo* info) {
	Buffer newBuffer = CreateBuffer(message);

	//send the message to everyone in the room
	for (int idx = 0; idx < rooms[index]->r_ClientList.size(); idx++) {
		if (rooms[index]->r_ClientList[idx] != sender) {
			int result = send(rooms[index]->r_ClientList[idx], (const char*)(&newBuffer.m_BufferData[0]), message.header.packetSize, 0);
			if (result == SOCKET_ERROR) {
				printf("send failed with error %d\n", WSAGetLastError());
				closesocket(rooms[index]->r_ClientList[idx]);
				freeaddrinfo(info);
				WSACleanup();
			}
		}
	}
	return true;
}

void s_helpers::SendErrorMessageToSender(const SOCKET& sender, ChatMessage errorMessage, addrinfo* info) {
	//create the buffer
	Buffer newBuffer = CreateBuffer(errorMessage);

	int result = send(sender, (const char*)(&newBuffer.m_BufferData[0]), errorMessage.header.packetSize, 0);
	if (result == SOCKET_ERROR) {
		printf("send failed with error %d\n", WSAGetLastError());
		closesocket(sender);
		freeaddrinfo(info);
		WSACleanup();
	}
}

std::string s_helpers::ReadIncomingMessage(SOCKET& serverSocket, Buffer& buffer, addrinfo* info, std::vector<sRoom*> rooms) {
	//return string;
	std::string msg;

	int result = recv(serverSocket, (char*)&buffer.m_BufferData[0], 512, 0);
	if (buffer.m_BufferData[0] != NULL) {
		if (result == SOCKET_ERROR) {
			//an error other than no data received has occured
			if (WSAGetLastError() != 10035) {
				printf("recv failed with error %d\n", WSAGetLastError());
				closesocket(serverSocket);
				freeaddrinfo(info);
				WSACleanup();
			}
		}

		//need to the request Id to send the message back to the sender
		std::string temp;
		int index = 0;
		while (true) {
			if (buffer.m_BufferData[index] != NULL || buffer.m_BufferData[index + 1] != NULL) {
				temp.push_back(buffer.m_BufferData[index]);
				std::cout << buffer.m_BufferData[index] << std::endl;
			}
			else {
				break;
			}
			index++;
		}
		auth::AuthenticateWebSuccess authSuccess;
		auth::AuthenticateWebFailure authFailure;
		auth::CreateAccountWebFailure createFailure;
		auth::CreateAccountWebSuccess createSuccess;
		bool test = createFailure.ParseFromString(temp + '\0');
		//finding out which type of message to send back to the client
		long requestId = 0;
		std::string successOrFailure = "";
		std::string msg = "";
		if (authSuccess.ParseFromString(temp)) {
			successOrFailure = "success";
			requestId = authSuccess.requestid();
			 msg = successOrFailure + "\nChat Rooms: ";

			for (int idx = 0; idx < rooms.size(); idx++) {
				msg += (rooms[idx]->r_Name + ", ");
			}
		}
		else if (authFailure.ParseFromString(temp)) {
			successOrFailure = "Failure: ";
			if (authFailure.failurereason() == 0) {
				successOrFailure += "Invalid credentials";
			}
			else if (authFailure.failurereason() == 1) {
				successOrFailure += "Internal server error";
			}
			requestId = authFailure.requestid();
			msg = successOrFailure;
		}
		else if (createSuccess.ParseFromString(temp)) {
			successOrFailure = "success";
			requestId = createSuccess.requestid();
			msg = successOrFailure + "\nChat Rooms: ";

			for (int idx = 0; idx < rooms.size(); idx++) {
				msg += (rooms[idx]->r_Name + ", ");
			}
		}
		else if (test) {
			successOrFailure = "Failure: ";
			if (createFailure.failurereason() == 0) {
				successOrFailure += "Account already exists";
			}
			else if (createFailure.failurereason() == 2) {
				successOrFailure += "Internal server error";
			}
			requestId = createFailure.requestid();
			msg = successOrFailure;
		}
		
		ChatMessage c_msg = s_helpers::CreateChatMessage(msg, 1);
		Buffer newBuffer = s_helpers::CreateBuffer(c_msg);

		SOCKET tempSocket = rooms[0]->clientRequests[requestId];
		int result = send(tempSocket, (char*)(&newBuffer.m_BufferData[0]), newBuffer.m_BufferData.size(), 0);
		if (result == SOCKET_ERROR) {
			printf("send failed with error %d\n", WSAGetLastError());
			closesocket(rooms[0]->clientRequests[requestId]);
			freeaddrinfo(info);
			WSACleanup();
		}
		else {

		}
		return msg;
	}
	return "";
}

//helper function to handle the recieved messages and input
void s_helpers::HandleClientInput(std::vector<sRoom*> rooms, Buffer& buffer, const SOCKET& sender, const SOCKET& auth, addrinfo* info) {

	//using a namespace for this function
	using namespace s_helpers;


	uint32_t packetSize = buffer.ReadUInt32LE();
	uint32_t messageType = buffer.ReadUInt32LE();

	if (buffer.m_BufferData.size() >= packetSize)
	{
		// We can finally handle our message
	}
	if (messageType == 1)
	{
		//Get the message out
		uint32_t messageLength = buffer.ReadUInt32LE();
		std::string msg = buffer.ReadString(messageLength);

		//Get the proper room
		int roomIndex = GetRoomIndexForMessage(rooms, msg);
		if (roomIndex == -1) {
			CreateChatMessage("The entered room does not exist, please enter a room from the list above", 1);
		}

		//Get the ChatMessage
		ChatMessage c_msg = CreateChatMessage(msg, 1);

		//Finally send the message to all the rooms but the sender and the error handling
		if (!SendMessageToRoom(rooms, roomIndex, sender, c_msg, info)) {
			//sends a message to the sender only
			SendErrorMessageToSender(sender, CreateChatMessage("Error: you are not connected to this room, until you do you send messages to it", 1), info);
		}
	}
	else if (messageType == 2) {
		//We know this message is for joining a room

		//Getting the name of the room
		uint32_t messageLength = buffer.ReadUInt32LE();
		std::string msg = buffer.ReadString(messageLength);

		//getting the room name and client names out of the string
		std::string delimiter = ":";
		std::string roomName = msg.substr(0, msg.find(delimiter));
		std::string name = msg.substr(msg.find(delimiter) + 1);

		//Joining the room
		if (!JoinRoom(rooms, roomName, sender)) {
			SendErrorMessageToSender(sender, CreateChatMessage("Error: you are already connected that this room", 1), info);
		}
		else {
			//Send the message after the person has successfully joined the room

			//Get the proper room
			int roomIndex = GetRoomIndexForMessage(rooms, roomName);
			if (roomIndex == -1) {
				CreateChatMessage("The entered room does not exist, please enter a room from the list above", 1);
			}

			//Create the message
			ChatMessage c_msg = CreateChatMessage(roomName + ":" + name + " has joined the room!", 1);

			//adding the room joined message before the person actual joins the room
			SendMessageToRoom(rooms, roomIndex, sender, c_msg, info);
			printf("Room joined successfully");
		}
	}
	else if (messageType == 3) {
		//This message type is for leaving rooms

		//Getting the room to leave
		uint32_t messageLength = buffer.ReadUInt32LE();
		std::string msg = buffer.ReadString(messageLength);

		std::string delimiter = ":";
		std::string roomName = msg.substr(0, msg.find(delimiter));
		std::string name = msg.substr(msg.find(delimiter) + 1);

		//Leaving the room
		if (!LeaveRoom(rooms, roomName, sender)) {
			SendErrorMessageToSender(sender, CreateChatMessage("Error: you were not connected to that room", 1), info);
		}
		else {
			//Send the message after the person has successfully left the room

			//Get the proper room
			int roomIndex = GetRoomIndexForMessage(rooms, roomName);
			if (roomIndex == -1) {
				CreateChatMessage("The entered room does not exist, please enter a room from the list above", 1);
			}

			//Create the message
			ChatMessage c_msg = CreateChatMessage(roomName + ":" + name + " has left the room!", 1);
			LeavingAnnouncement(rooms, roomIndex, sender, c_msg, info);
			//adding the room joined message before the person actual joins the room
			printf("Left room successfully");
		}

	}
	else if (messageType == 4) {
		uint32_t messageLength = buffer.ReadUInt32LE();
		std::string msg = buffer.ReadString(messageLength);

		std::string delimiter = ":";
		std::string sClientId = msg.substr(0, msg.find(delimiter));
		int clientId = std::stoi(sClientId);
		delimiter = " ";

		if (rooms[0]->clientRequests.find(clientId) == rooms[0]->clientRequests.end()) {
			rooms[0]->clientRequests.insert({ clientId, sender });
		}

		auth::CreateAccountWeb new_account;
		new_account.set_requestid(clientId);
		new_account.set_email(msg.substr(msg.find(delimiter), msg.find_last_of(delimiter) - msg.find(delimiter)));
		new_account.set_plaintextpassword(msg.substr(msg.find_last_of(delimiter) + 1));

		std::cout << "Data received: " << new_account.requestid() << " " << new_account.email() << " " << new_account.plaintextpassword() << std::endl;

		std::string serializedAuthMessage;
		new_account.SerializeToString(&serializedAuthMessage);

		Buffer sendBuffer;

		for (int i = 0; i < serializedAuthMessage.size(); i++) {
			sendBuffer.m_BufferData[i] = serializedAuthMessage[i];
		}

		int result = send(auth, (char*)(&sendBuffer.m_BufferData[0]), serializedAuthMessage.length(), 0);
		if (result == SOCKET_ERROR) {
			printf("send failed with error %d\n", WSAGetLastError());
			closesocket(sender);
			freeaddrinfo(info);
			WSACleanup();
		}
	}
	else if (messageType == 5) {
		uint32_t messageLength = buffer.ReadUInt32LE();
		std::string msg = buffer.ReadString(messageLength);

		std::string delimiter = ":";
		int clientId = std::stoi(msg.substr(0, msg.find(delimiter)));
		delimiter = " ";

		if (rooms[0]->clientRequests.find(clientId) == rooms[0]->clientRequests.end()) {
			rooms[0]->clientRequests.insert({ clientId, sender });
		}

		auth::AuthenticateWeb login;
		login.set_requestid(clientId);
		login.set_email(msg.substr(msg.find(delimiter), msg.find_last_of(delimiter) - msg.find(delimiter)));
		login.set_plaintextpassword(msg.substr(msg.find_last_of(delimiter) + 1));

		std::cout << "Data received: " << login.requestid() << " " << login.email() << " " << login.plaintextpassword() << std::endl;

		std::string serializedAuthMessage;
		login.SerializeToString(&serializedAuthMessage);


		int result = send(auth, serializedAuthMessage.c_str(), serializedAuthMessage.length(), 0);
		if (result == SOCKET_ERROR) {
			printf("send failed with error %d\n", WSAGetLastError());
			closesocket(sender);
			freeaddrinfo(info);
			WSACleanup();
		}
	}
}

void s_helpers::CloseClientSockets(std::vector<SOCKET>& client, addrinfo* info) {
	for (int index = 0; index < client.size(); index++) {
		closesocket(client[index]);
		freeaddrinfo(info);
		WSACleanup();
	}
}