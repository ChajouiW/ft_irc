#include "Server.hpp"

#include <cstdlib>

#include <iostream>
int main (int ac, char **argv)
{
	if (ac != 3)
	{
		std::cerr << "Usage: " << argv[0] << " <port> <password>" << std::endl;
		return 1;
	}
	int port = std::atoi(argv[1]);
	std::string pass = argv[2];
	Server server(port, pass);
	try
	{
		server.setup();
	}
	catch (const std::runtime_error &e)
	{
		std::cerr << "Error: " << e.what() << std::endl;
		return 1;
	}
	std::cout << "Server setup successfully on port " << port << std::endl;
	server.Run();
	return 0;
}