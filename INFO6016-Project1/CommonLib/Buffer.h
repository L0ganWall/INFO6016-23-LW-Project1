#pragma once
#include "Common.h"

class Buffer
{
public:
	Buffer(uint32 size);
	~Buffer();

	//void WriteUInt32LE(size_t index, uint32 value);
	void WriteUInt32LE(uint32 value);
	//uint32 ReadUInt32LE(size_t index);
	uint32 ReadUInt32LE();

	void WriteUShort16LE(uint16 value);
	uint16 ReadUShort16LE();

	void WriteString(std::string value);
	std::string ReadString(uint32 length);

	// this stores all of the data within the buffer
	std::vector<uint8> m_BufferData;

	// The index to write the next byte of data in the buffer
	uint32 m_WriteIndex;

	// The index to read the next byte of data from the buffer
	uint32 m_ReadIndex;
};
