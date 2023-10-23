#pragma once
#include <Common.h>

class cRoom
{
public:
	cRoom(std::string roomName);
	~cRoom();

	void PrintRoom();

	std::vector<std::string> messages;
	std::string roomName;
};

