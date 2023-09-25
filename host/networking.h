#pragma once
#include "common.h"

constexpr uint16 DEFAULT_PORT = 9001;
constexpr uint16 MAX_PACKET_SIZE = 1024;
constexpr uint16 BACKLOG = 512;
constexpr uint16 CHUNCK_SIZE = 1024;

WSADATA _wsa;

extern std::atomic<bool> thread_running(true);
CRITICAL_SECTION mtx;

// T uses a seperate socket file descriptor for accepting clients as to not cause race conditions
DWORD WINAPI net_subroutine(LPVOID clients);

typedef struct {
	char name[256]; //yeah it's wastefull but saves me alot of time
	int clock_speed_ghz;
	int core_count;
}cpu_specs;

typedef struct {
	float size;
	char name[256];
}hdd_specs;

typedef struct {
	char name[256];
	float v_ram;
}gpu_specs;

typedef struct {
	cpu_specs cpu;
	gpu_specs gpu;
	hdd_specs hdd;


	float ram;		// virtual and physical ~35 gb
} client_specs;

struct user {
	SOCKET fd;
	client_specs specs;
	char username[256];
};

struct data_chunck
{
	char content[CHUNCK_SIZE];
	unsigned long iteration;
};

class networking
{
public:
	std::vector<user> client;

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
			0, //default stack size allocated for thread
			net_subroutine,
			&client,
			0,
			NULL);

		if (T == NULL)
			throw std::runtime_error("Error creating thread");
	}

	int send_text(SOCKET fd, std::string buffer) // default return value = 0
	{
		int bytes_recv = send(fd, buffer.c_str(), buffer.size(), 0);
		if (bytes_recv == SOCKET_ERROR)
		{
			return -1;
		}

		return 0;
	}

	int recv_text(SOCKET fd, std::string& buffer) // default return value = 0
	{
		char buf[MAX_PACKET_SIZE]{ 0 };
		int res = recv(fd, buf, sizeof(buf), 0);
		switch (res)
		{
		case SOCKET_ERROR:
			return -1;

		case 0:
			return 0;

		default:
			buffer = buf;
			return 0;
		}
	}

	std::string recv_all_f(SOCKET Dst)
	{
		std::string res;
		struct data_chunck buffer;

		while (true)
		{
			recv(Dst, reinterpret_cast<char*>(&buffer), sizeof(data_chunck), 0);
			if (buffer.iteration == 0)
			{
				res += buffer.content;
				break;
			}
			res += buffer.content;
		}

		return res;
	}

	SOCKET sock_retr(std::string Dst)
	{
		char* cmp = (char*)Dst.c_str();
		for (int i = 0; i < client.size(); i++)
		{
			if (strcmp(client[i].username, cmp) == 0)
				return client[i].fd;
		}

		std::cout << "[*] Unable to retrieve requested file descriptor" << std::endl;
		return NULL;
	}

	void exit()
	{
		thread_running = false;
		if (client.size() != 0)
		{
			for (int i = 0; i < client.size(); i++)
				closesocket(client[i].fd);
		}

		CloseHandle(T);
		DeleteCriticalSection(&mtx);
	}

private:
	//threading should be more efficient as it wont be executing anything remotely
	//computationally heavy
	HANDLE T;
};

DWORD WINAPI net_subroutine(LPVOID clients)
{
	std::cout << "\n[+]Thread entry\n";
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

	std::vector<user>* client_ptr = static_cast<std::vector<user>*>(clients);

	while (thread_running)
	{
		buf = accept(socket_fd, (struct sockaddr*)&addr, &addrlen);
		if (buf != INVALID_SOCKET)
		{
			user res;
			recv(buf, reinterpret_cast<char*>(&res), sizeof(user), 0);
			res.fd = buf;

			EnterCriticalSection(&mtx);
			client_ptr->push_back(res);
			LeaveCriticalSection(&mtx);
		}
	}
	return 0;
}
