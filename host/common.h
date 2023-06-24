#pragma once
#define WIN32_LEAN_AND_MEAN
#define uint16 unsigned short
#define uint64 unsigned long

#include <iostream>
#include <vector>
#include <fstream>
#include <tchar.h>
#include <string>

#include <Windows.h>
#include <atomic>
#include <mutex>

#include <winSock2.h>
#include <WS2tcpip.h>
#pragma comment(lib, "ws2_32.lib")

std::string read_f(const char *path)
{
	std::ifstream fd(path);
	std::string res;

	if (fd.is_open())
	{
		fd >> res;
		fd.close();
	}
	return res;
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