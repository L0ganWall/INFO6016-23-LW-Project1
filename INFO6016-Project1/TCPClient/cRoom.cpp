#include "cRoom.h"

cRoom::cRoom(std::string roomName) {
	this->roomName = roomName;
}
cRoom::~cRoom() {}

void cRoom::PrintRoom() {
	printf("Room: %s\n", this->roomName.c_str());
	printf("-----------------------------------------------\n");
	for (int idx = 0; idx < messages.size(); idx++) {
		printf("%s\n", messages[idx].c_str());
	}
	printf("-----------------------------------------------\n");
}