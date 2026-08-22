#include "Server.hpp"

#include <cstdlib>

volatile sig_atomic_t	g_stop = 0;

/* katse3 ghir flag — signal handler khass ykoun async-signal-safe */
static void	handleSignal(int sig)
{
	(void)sig;
	g_stop = 1;
}

int main (int ac, char **argv)
{
	if (ac != 3)
	{
		std::cerr << "Usage: " << argv[0] << " <port> <password>" << std::endl;
		return 1;
	}
	signal(SIGINT, handleSignal);
	signal(SIGTERM, handleSignal);
	signal(SIGPIPE, SIG_IGN);

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