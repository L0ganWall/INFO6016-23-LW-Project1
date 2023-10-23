#include "c_helpers.h"
ChatMessage c_helpers::CreateChatMessage(std::string s_Message, int messageType) {
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
Buffer c_helpers::CreateBuffer(ChatMessage message) {

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

std::string c_helpers::ReadIncomingMessage(SOCKET& serverSocket, Buffer& buffer, addrinfo* info) {
	//return string;
	std::string msg;

	int result = recv(serverSocket, (char*)(&buffer.m_BufferData[0]), 512, 0);
	if (result == SOCKET_ERROR) {
		//an error other than no data received has occured
		if (WSAGetLastError() != 10035) {
			printf("recv failed with error %d\n", WSAGetLastError());
			closesocket(serverSocket);
			freeaddrinfo(info);
			WSACleanup();
		}
	}
	else if (result > 0) {
		uint32_t packetSize = buffer.ReadUInt32LE();
		uint32_t messageType = buffer.ReadUInt32LE();


		uint32_t messageLength = buffer.ReadUInt32LE();
		msg = buffer.ReadString(messageLength);

		printf("%s\n", msg.c_str());
	}
	return msg;
}

void c_helpers::PrintClient(const std::string& input, const std::vector<cRoom*>& rooms) {
	system("cls");
	printf("%s\n", input.c_str());
	std::cout << "Controls:\ntype \'join\' or \'leave\' then the room name to join or leave that room if you haven't already" << std::endl;
	std::cout << "Ex. join A would join room A if you're not already in it" << std::endl;
	std::cout << "To send a message to a room type the room\'s name followed by a : then the message you want to send" << std::endl;
	std::cout << "Ex. A: Hello everyone! would send Hello everyone! to anyone in room A other than you" << std::endl;
	std::cout << "Exit will close the application" << std::endl;
	if (rooms.size() > 0) {
		for (int idx = 0; idx < rooms.size(); idx++) {
			rooms[idx]->PrintRoom();
		}
	}
}