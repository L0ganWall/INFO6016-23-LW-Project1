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

//helper function to handle the recieved messages and input
void s_helpers::HandleClientInput(std::vector<sRoom*> rooms, Buffer& buffer, const SOCKET& sender, addrinfo* info) {

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
}

void s_helpers::CloseClientSockets(std::vector<SOCKET>& client, addrinfo* info) {
	for (int index = 0; index < client.size(); index++) {
		closesocket(client[index]);
		freeaddrinfo(info);
		WSACleanup();
	}
}