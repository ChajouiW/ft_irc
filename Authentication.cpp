#include "Server.hpp"

void Server::	authenticate(std::string cmd, int fd)
{
    //nick || pass || user || quit
    //register PASS 

    cmd = cmd.substr(4); // remove "PASS "
    size_t pos = cmd.find_first_not_of(" \t\v");// skip whitespace after "PASS "
    if (pos < cmd.size())
        cmd = cmd.substr(pos);
    if (cmd == _password)
    {
        _clients[fd].setAuthenticated(true);
        std::string response = "Password accepted.\r\n";
        send(fd, response.c_str(), response.size(), 0);
    }
    else
    {
        std::string response = "Wrong password!.\r\n";
        send(fd, response.c_str(), response.size(), 0);
    }

}
void Server::	set_nickname(std::string cmd, int fd)
{
    if (!authenticated(fd))
    {
        std::string response = "You must authenticate first.\r\n";
        send(fd, response.c_str(), response.size(), 0);
        return;
    }
    

}
void Server::	set_username(std::vector<std::string> tokens, int fd)
{

}
void Server::	disconnect(std::string cmd, int fd)
{

}
void Server::	disconnect(std::string cmd, int fd)
{

}