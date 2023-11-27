#pragma once

// WinSock2 Windows Sockets
#define WIN32_LEAN_AND_MEAN

#include <Windows.h>
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <stdlib.h>
#include <stdio.h>

#include "buffer.h"

#include "../gen/authentication.pb.h"

// Need to link Ws2_32.lib
#pragma comment(lib, "Ws2_32.lib")

// First, make it work (messy), then organize

#define DEFAULT_PORT "8412"
#define AUTH_PORT "8413"

//struct to handle the rooms
struct sRoom {
	std::vector<SOCKET> r_ClientList;
	std::string r_Name;
	std::map<int, SOCKET> clientRequests;
};
namespace s_helpers {

	//checks to ensure the sender is in the room before allowing them to send a message to it
	bool IsSenderInTheRoom(std::vector<sRoom*> rooms, const SOCKET& sender, int index);

	//helper to get the room index the client sent it in, it returns the room's index
	int GetRoomIndexForMessage(std::vector<sRoom*> rooms, std::string message);

	//helper to get the room name the message sent it in, it returns the room's index
	std::string GetRoomIndexForMessage(std::string message);

	//helper to join a room
	bool JoinRoom(std::vector<sRoom*> rooms, std::string roomName, const SOCKET& sender);

	//helper to leave a room
	bool LeaveRoom(std::vector<sRoom*> rooms, std::string roomName, const SOCKET& sender);

	//Helper to create the ChatMessage format
	ChatMessage CreateChatMessage(std::string s_Message, int messageType);

	//Helper method to create and return the buffer
	Buffer CreateBuffer(ChatMessage message);
	
	//sends a message to everyone but the sender in the selected room
	bool SendMessageToRoom(std::vector<sRoom*> rooms, int index, const SOCKET& sender, ChatMessage message, addrinfo* info);

	//A helper function to announce that a client has left, it's different because the client sending it is no longer in the room
	bool LeavingAnnouncement(std::vector<sRoom*> rooms, int index, const SOCKET& sender, ChatMessage message, addrinfo* info);

	//sends an error message to the sender
	void SendErrorMessageToSender(const SOCKET& sender, ChatMessage errorMessage, addrinfo* info);

	//read message from auth server
	std::string ReadIncomingMessage(SOCKET& serverSocket, Buffer& buffer, addrinfo* info, std::vector<sRoom*> rooms);

	//this function handles all the client input and determines what to do with it
	void HandleClientInput(std::vector<sRoom*> rooms, Buffer& buffer, const SOCKET& sender, const SOCKET& auth, addrinfo* info);

	//closes all client sockets
	void CloseClientSockets(std::vector<SOCKET>& client, addrinfo* info);
};

