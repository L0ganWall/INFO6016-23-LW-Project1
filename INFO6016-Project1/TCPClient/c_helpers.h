#pragma once

// WinSock2 Windows Sockets
#define WIN32_LEAN_AND_MEAN

#include <Windows.h>
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <stdlib.h>
#include <stdio.h>




#include "buffer.h"
#include "cRoom.h";

// Need to link Ws2_32.lib
#pragma comment(lib, "Ws2_32.lib")

// First, make it work (messy), then organize

#define DEFAULT_PORT "8412"
namespace c_helpers
{
//Helper to create the ChatMessage format
	ChatMessage CreateChatMessage(std::string s_Message, int messageType);

//Helper method to create and return the buffer
	Buffer CreateBuffer(ChatMessage message);

	std::string ReadIncomingMessage(SOCKET& serverSocket, Buffer& buffer, addrinfo* info);

	void PrintClient(const std::string& input, const std::vector<cRoom*>& rooms);
};

