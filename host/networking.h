#pragma once
#include "common.h"

constexpr uint16 DEFAULT_PORT = 9001;
constexpr uint16 MAX_PACKET_SIZE = 1024;
constexpr uint16 BACKLOG = 512;

WSADATA _wsa;

extern std::atomic<bool> thread_running(true);
CRITICAL_SECTION mtx;

// T uses a seperate socket file descriptor for accepting clients as to not cause race conditions
DWORD WINAPI net_subroutine(LPVOID clients);

struct user {
	SOCKET fd;
	char username[256];
};

class networking
{
public:
	std::vector<struct user> client;

	//moved the wsa api to the main class to avoid additional boilerplate code
	networking()
	{
		//wsa init
		if (WSAStartup(MAKEWORD(2, 2), &_wsa) != 0)
			throw std::runtime_error("WSA initialization error\n");
		InitializeCriticalSection(&mtx);

		//what fucking tool thought this was a good design choice,
		//i can't even use a goddamn vector properly without casting it
		 T = CreateThread(
			NULL,
			0,
			net_subroutine,
			&client,
			0,
			NULL );
		
		 if (T == NULL)
			 throw std::runtime_error("Error creating thread");
	}

	~networking()
	{
		thread_running = false;
		if (client.size() != 0)
		{
			for (int i = 0; i < client.size(); i++)
				closesocket(client[i].fd);
		}
		
		CloseHandle(T);

		DeleteCriticalSection(&mtx);
		WSACleanup();
	}

private:
	//threading should be more efficient as it wont be executing anything remotely
	//computationally heavy
	HANDLE T;
};

DWORD WINAPI net_subroutine(LPVOID clients)
{
	std::cout << "[+]Thread entry\n";
	SOCKET socket_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (socket_fd == SOCKET_ERROR)
		throw std::runtime_error("unable to initialize socket\n");
	
	sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(DEFAULT_PORT);
	addr.sin_addr.s_addr = INADDR_ANY;
	int addrlen = sizeof(addr);
	
	//size_t->int
	bind(socket_fd, (sockaddr*)&addr, static_cast<int>(sizeof(addr)));
	listen(socket_fd, BACKLOG);
	SOCKET buf;

	unsigned long optval = 1;
	if (ioctlsocket(socket_fd, FIONBIO, &optval) != 0)
		throw std::runtime_error("ioctl initialization error\m");

	std::vector<struct user>* client_ptr = static_cast<std::vector<struct user>*>(clients);

	while (thread_running)
	{
		buf = accept(socket_fd, (struct sockaddr*)&addr, &addrlen);
		if (buf != INVALID_SOCKET)
		{
			struct user tmp = {buf, NULL};
			recv(buf, tmp.username, 255, 0);

			EnterCriticalSection(&mtx);
			client_ptr->push_back(tmp);
			LeaveCriticalSection(&mtx);
		} 
		else
		{
			if (WSAGetLastError() != WSAEWOULDBLOCK)
				throw std::exception("unable to accept connection");
			
			if (!thread_running)
				break;
		}
	}
}