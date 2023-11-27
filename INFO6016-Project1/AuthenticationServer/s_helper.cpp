#include "s_helper.h"
#include "SHA256.h"

//I know this isn't probably a good way to create a salt, but I'm running low on time
std::string CreateSalt() {
	std::string charset = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ!@#$%^&*0123456789";
	std::string salt;
	for (int i = 0; i < 15; i++) {
		srand(time(0) + i);
		salt.push_back(rand() % (charset.length() - 1));
	}

	return salt;

}


int AddUser(std::string email, std::string salt, std::string password, MySQLUtil* myDb)
{
	//check to make sure that there are no duplicate emails
		//check to make sure that there are no duplicate emails
	sql::PreparedStatement* getEmails = myDb->PrepareStatement(("SELECT email FROM web_auth WHERE email = ?"));
	getEmails->setString(1, email);
	sql::ResultSet* result = getEmails->executeQuery();

	std::string testEmail = "";
	while (result->next()) {
		testEmail = result->getString("email");
	}
	if (testEmail != "") {
		return -1;
	}
	salt = CreateSalt();
	password = sha256(password + salt);
	sql::PreparedStatement* pStmt = myDb->PrepareStatement(("INSERT INTO web_auth (email, salt, hashed_password) VALUES (?, ?, ?)"));
	pStmt->setString(1, email);
	pStmt->setString(2, salt);
	pStmt->setString(3, password);
	int count = pStmt->executeUpdate();

	if (count != 0) {
		sql::PreparedStatement* userStmt = myDb->PrepareStatement(("INSERT INTO user (last_login, creation_date) VALUES (CURRENT_TIMESTAMP(), NOW())"));

		int userCount = userStmt->executeUpdate();
		if (userCount != 0) {
			sql::PreparedStatement* getId = myDb->PrepareStatement(("SELECT id FROM user ORDER BY id DESC LIMIT 1"));
			sql::ResultSet* result = getId->executeQuery();

			std::string id = "";
			while (result->next()) {
				 id = result->getString("id");
			}
			if (id != "") {
				sql::PreparedStatement* updateUserId = myDb->PrepareStatement(("UPDATE web_auth SET userId = ? ORDER BY id DESC limit 1"));
				updateUserId->setInt(1, std::stoi(id));

				int count = updateUserId->executeUpdate();
				return std::stoi(id);
			}
		}
	}
	return -1;
}

int AuthenticateUser(std::string email, std::string password, MySQLUtil* myDb) {
	sql::PreparedStatement* pStmt = myDb->PrepareStatement(("SELECT * FROM web_auth WHERE email = ?"));
	pStmt->setString(1, email);
	sql::ResultSet* result = pStmt->executeQuery();
	std::string hashedPassword = "";
	std::string salt = "";
	int userId = 0;
	while (result->next()) {
		hashedPassword = result->getString("hashed_password");
		salt = result->getString("salt");
		userId = result->getInt("userId");
	}
	if (sha256(password + salt) == hashedPassword) {
		sql::PreparedStatement* login = myDb->PrepareStatement(("UPDATE user SET last_login = CURRENT_TIMESTAMP() WHERE id = ?"));
		login->setInt(1, userId);
		int count = login->executeUpdate();

		return userId;

	}
	return -1;
}




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
void s_helpers::HandleClientInput(Buffer& buffer, const SOCKET& sender, addrinfo* info, MySQLUtil* myDb) {

	//using a namespace for this function
	using namespace s_helpers;

	std::string temp;
	int index = 0;
	while (true) {
		if (buffer.m_BufferData[index] != NULL) {
			temp.push_back(buffer.m_BufferData[index]);
		}
		else {
			break;
		}
		index++;
	}

	auth::AuthenticateWeb authWeb;
	bool authTest = authWeb.ParseFromString(temp);

	auth::CreateAccountWeb createWeb;
	bool createTest = createWeb.ParseFromString(temp);

	if (authTest) {
		std::cout << "Data received: " << authWeb.requestid() << " " << authWeb.email() << " " << authWeb.plaintextpassword() << std::endl;
		int userId = AuthenticateUser(authWeb.email(), authWeb.plaintextpassword(), myDb);
		if (userId != -1) {
			auth::AuthenticateWebSuccess success;
			success.set_requestid(authWeb.requestid());
			success.set_userid(userId);
			success.set_creationdate("placeholder");

			std::string serializedAuthMessage;
			success.SerializeToString(&serializedAuthMessage);

			Buffer sendBuffer;

			for (int i = 0; i < serializedAuthMessage.size(); i++) {
				sendBuffer.m_BufferData[i] = serializedAuthMessage[i];
			}
			int result = send(sender, (char*)(&sendBuffer.m_BufferData[0]), serializedAuthMessage.length(), 0);
			if (result == SOCKET_ERROR) {
				printf("send failed with error %d\n", WSAGetLastError());
				closesocket(sender);
				freeaddrinfo(info);
				WSACleanup();
			}
		}
		else {
			auth::AuthenticateWebFailure failure;
			failure.set_requestid(authWeb.requestid());
			failure.set_failurereason(auth::AuthenticateWebFailure_reason_INVALID_CREDENTIALS);

			std::string serializedAuthMessage;
			failure.SerializeToString(&serializedAuthMessage);

			Buffer sendBuffer;

			for (int i = 0; i < serializedAuthMessage.size(); i++) {
				sendBuffer.m_BufferData[i] = serializedAuthMessage[i];
			}
			int result = send(sender, (char*)(&sendBuffer.m_BufferData[0]), serializedAuthMessage.length(), 0);
			if (result == SOCKET_ERROR) {
				printf("send failed with error %d\n", WSAGetLastError());
				closesocket(sender);
				freeaddrinfo(info);
				WSACleanup();
			}
		}
	}
	else if (createTest) {
		std::cout << "Data received: " << createWeb.requestid() << " " << createWeb.email() << " " << createWeb.plaintextpassword() << std::endl;
		int userId = AddUser(createWeb.email(), "", createWeb.plaintextpassword(), myDb);
		if (userId != -1) {
			auth::CreateAccountWebSuccess success;
			success.set_requestid(createWeb.requestid());
			success.set_userid(userId);

			std::string serializedAuthMessage;
			success.SerializeToString(&serializedAuthMessage);

			Buffer sendBuffer;

			for (int i = 0; i < serializedAuthMessage.size(); i++) {
				sendBuffer.m_BufferData[i] = serializedAuthMessage[i];
			}
			int result = send(sender, (char*)(&sendBuffer.m_BufferData[0]), serializedAuthMessage.length(), 0);
			if (result == SOCKET_ERROR) {
				printf("send failed with error %d\n", WSAGetLastError());
				closesocket(sender);
				freeaddrinfo(info);
				WSACleanup();
			}
		}
		else {
			auth::CreateAccountWebFailure failure;
			failure.set_requestid(createWeb.requestid());
			failure.set_failurereason(auth::CreateAccountWebFailure_reason_ACCOUNT_ALREADY_EXISTS);

			std::string serializedAuthMessage;
			failure.SerializeToString(&serializedAuthMessage);

			Buffer sendBuffer;

			for (int i = 0; i < serializedAuthMessage.size(); i++) {
				sendBuffer.m_BufferData[i] = serializedAuthMessage[i];
			}
			int result = send(sender, (char*)(&sendBuffer.m_BufferData[0]), serializedAuthMessage.length(), 0);
			if (result == SOCKET_ERROR) {
				printf("send failed with error %d\n", WSAGetLastError());
				closesocket(sender);
				freeaddrinfo(info);
				WSACleanup();
			}
		}
	}
	else {

	}

	std::cout << "Data received: " << authWeb.requestid() << " " << authWeb.email() << " " << authWeb.plaintextpassword() << std::endl;
	
}

void s_helpers::CloseClientSockets(std::vector<SOCKET>& client, addrinfo* info) {
	for (int index = 0; index < client.size(); index++) {
		closesocket(client[index]);
		freeaddrinfo(info);
		WSACleanup();
	}
}