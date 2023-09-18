#pragma once

#define WIN32_LEAN_AND_MEAN
#define uint8 unsigned char
#define uint16 unsigned short
#define uint64 unsigned long

#include <iostream>
#include <vector>
#include <fstream>
#include <tchar.h>
#include <sstream>
#include <string>

#include <Windows.h>
#include <atomic>
#include <mutex>

#include <winSock2.h>
#include <WS2tcpip.h>
#pragma comment(lib, "ws2_32.lib")

// should only be called once
extern WSADATA _wsa;
inline void init_wsa() {
	if (WSAStartup(MAKEWORD(2, 2), &_wsa) != 0)
		throw std::runtime_error("Unable to initialize WSA\n");
}

inline void exit_wsa() {
	WSACleanup();
}

// For the love of god never change this or binary file reading will be fucked
std::string read_f(const char *path) {
	std::ifstream file(path, std::ios::binary);

	if (!file.is_open()) 
		throw std::runtime_error("Unable to open file:");

	std::string buffer;
	unsigned char byte;

	while (file.read(reinterpret_cast<char*>(&byte), sizeof(byte))) {
		buffer.push_back(byte);
	}

	file.close();

	return buffer;
}

bool write_f(std::string content, const char* path) 
{
	std::ofstream fd(path);
	if (fd.is_open())
	{
		fd << content;
		fd.close();
		
		return true;
	}

	return false;
}

// mainly used as utility for packet chunking
inline std::string hexify(long val) {
	std::stringstream stream;
	stream << std::hex << val;
	return stream.str();
}
