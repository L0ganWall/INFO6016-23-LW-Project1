#include "Buffer.h"

Buffer::Buffer(uint32 size) {
	m_BufferData.resize(size);
	m_WriteIndex = 0;
	m_ReadIndex = 0;
}

Buffer::Buffer() {
	m_BufferData.resize(512);
	m_WriteIndex = 0;
	m_ReadIndex = 0;
}

Buffer::~Buffer() {}

//void Buffer::WriteUInt32LE(size_t index, uint32 value) {
//	if (index + sizeof(value) < m_BufferData.size()) {
//		m_BufferData[index] = value;
//		m_BufferData[index + 1] = value >> 8;
//		m_BufferData[index + 2] = value >> 16;
//		m_BufferData[index + 3] = value >> 24;
//	}
//	else {
//		//grow buffer
//		m_BufferData.resize(m_BufferData.size() + 4);
//	}
//}

void Buffer::WriteUInt32LE(uint32 value) {
	//grow buffer by 4 if needed
	if (m_WriteIndex + sizeof(value) > m_BufferData.size()) {
		m_BufferData.resize(m_BufferData.size() + 4);
	}
	m_BufferData[m_WriteIndex] = value >> 24;
	m_BufferData[m_WriteIndex++] = value >> 16;
	m_BufferData[m_WriteIndex++] = value >> 8;
	m_BufferData[m_WriteIndex++] = value;
}
//
//uint32 Buffer::ReadUInt32LE(size_t index) {
//	uint32 newValue = 0;
//	newValue |= m_BufferData[index];
//	newValue |= m_BufferData[index + 1] << 8;
//	newValue |= m_BufferData[index + 2] << 16;
//	newValue |= m_BufferData[index + 3] << 24;
//	return newValue;
//}

uint32 Buffer::ReadUInt32LE() {
	uint32 newValue = 0;
	newValue |= m_BufferData[m_ReadIndex] << 24;
	newValue |= m_BufferData[m_ReadIndex++] << 16;
	newValue |= m_BufferData[m_ReadIndex++] << 8;
	newValue |= m_BufferData[m_ReadIndex++];
	return newValue;
}

void Buffer::WriteUShort16LE(uint16 value) {
	//if growing the buffer is needed
	if (m_WriteIndex + sizeof(value) > m_BufferData.size()) {
		m_BufferData.resize(m_BufferData.size() + 2);
	}
	m_BufferData[m_WriteIndex] = value >> 8;
	m_BufferData[m_WriteIndex++] = value;
}

uint16 Buffer::ReadUShort16LE() {
	uint16 newValue = 0;
	newValue |= m_BufferData[m_ReadIndex] << 8;
	newValue |= m_BufferData[m_ReadIndex++];
	return newValue;
}

void Buffer::WriteString(std::string value) {
	//grows buffer if needed
	if (m_WriteIndex + sizeof(value) > m_BufferData.size()) {
		m_BufferData.resize(m_BufferData.size() + sizeof(value));
	}
	for (int idx = 0; idx < value.length(); idx++) {
		m_BufferData[m_WriteIndex++] = value[idx];
	}
}

std::string Buffer::ReadString(uint32 length) {
	std::string returnString = "";

	for (int i = 0; i < length; i++)
	{
		returnString.push_back(m_BufferData[m_ReadIndex++]);
	}
	return returnString;
}