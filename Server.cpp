#include "Server.hpp"
#include "Command.hpp"

#include <cctype>
#include <cstring>
#include <cerrno>
#include <stdexcept>
#include <sstream>
#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>

Server::Server(int	port, const std::string &pass): _lfd(-1), _port(port), _password(pass)
{
	// & o Server:: jouj wajibin: smiya dial member ma katwalich pointer b rasha
	_handlers["PASS"] = &Server::checkPass;
	_handlers["NICK"] = &Server::setNickname;
	_handlers["USER"] = &Server::setUsername;
	_handlers["QUIT"] = &Server::disconnect;
	_handlers["PING"] = &Server::ping;
	// _handlers["KICK"] = &Server::kick;
	// _handlers["PRIVMSG"] = &Server::privmsg;
	// _handlers["JOIN"] = &Server::join;
	// _handlers["PART"] = &Server::part;
	// _handlers["TOPIC"] = &Server::topic;
	// _handlers["INVITE"] = &Server::invite;
	// _handlers["MODE"] = &Server::mode;
}

Server::~Server()
{
	if (_lfd >= 0)
		close(_lfd);
}

void    toUppercase(std::string &str)
{
    for (size_t i = 0; i < str.size(); i++)
        str[i] = std::toupper(str[i]);
}
/*
** ma kansiftou WALO mn hna. kanzido ghir f _writeBuffer dial dak client, o Run()
** ghadi ykhelli POLLOUT f events dialo mli lbuffer machi khawi.
** subject: send/recv 3la chi fd bla ma poll() i9ol belli wajed => note 0.
*/
void    Server::sendToClient(const std::string &message, int fd)
{
    std::map<int, Client>::iterator it = _clients.find(fd);
    if (it == _clients.end())
        return ;
    it->second.appendWriteBuffer(message);
}

/*
** hadi kaytnada ghir mn Run() mli poll() 3tana POLLOUT.
** send() ymken ykhod 9ell mn li 3titih => kanmes7o ghir li mcha b3da, lba9i
** kayb9a f buffer o ytsift f dowra jaya (dak chi 3lach consumeWriteBuffer).
*/
void    Server::flushClient(int fd)
{
    std::map<int, Client>::iterator it = _clients.find(fd);
    if (it == _clients.end() || !it->second.hasPendingWrite())
        return ;
    std::string out = it->second.getWriteBuffer();
    ssize_t n = send(fd, out.c_str(), out.size(), 0);
    if (n > 0)
        it->second.consumeWriteBuffer(static_cast<size_t>(n));
    else if (n < 0)
        it->second.setWriteBuffer(""); // socket khayb: kanhaydo lbuffer bach ma ndoroch f loop bla 7dod 3la POLLOUT
}
void	Server::parse_cmd(const std::string &cmd, int fd)
{
    //instead of using if else if statements, we can use a map to associate commands with their corresponding member function pointers. This will make the code cleaner and more maintainable.
    Command cmds;
    cmds.args = split_cmd(cmd);
    if (cmds.args.empty())
        return ;
    cmds.command = cmds.args[0];
    toUppercase(cmds.command);
    cmds.args.erase(cmds.args.begin()); // remove the command from the args vector
    if (cmd.find(" :") != std::string::npos)
        cmds.trailing = cmd.substr(cmd.find(" :") + 2);
    if (!cmds.trailing.empty())
        cmds.args.push_back(cmds.trailing);
    // std::cout << "TRAILING : " << cmds.trailing << std::endl;
    std::map<std::string, CmdHandler>::iterator it = _handlers.find(cmds.command);
    if (it == _handlers.end())
        {sendToClient(ERR_UNKNOWNCOMMAND(_clients[fd].getNick(), cmds.command), fd); return ;}// command ma kaynach: mn b3d 421 ERR_UNKNOWNCOMMAND

    // parenthese lbarra wajiba: ->* d3if mn () f precedence
    if (cmds.command != "PASS" && cmds.command != "NICK" && cmds.command != "USER" && cmds.command != "QUIT") // hna katchecki wach lclient kayn f _clients o la, ila ma kaynach kayreturni
        if (!_clients[fd].isPass() || !_clients[fd].isRegistered())
            {sendToClient(ERR_NOTREGISTERED(_clients[fd].getNick()), fd); return ;}
    (this->*(it->second))(cmds, fd);
}

bool	Server::isInUse(const std::string &nick)
{
    for (std::map<int, Client>::iterator it = _clients.begin(); it != _clients.end(); ++it)
    {
        if (it->second.getNick() == nick)
            return true;
    }
    return false;
}


void    Server::setup()
{
    sockaddr_in s;
    memset(&s, 0, sizeof(s));
    s.sin_family = AF_INET;
    s.sin_addr.s_addr = INADDR_ANY;
    s.sin_port = htons(_port);
    _lfd = socket(AF_INET, SOCK_STREAM, 0);
    if (_lfd < 0)
        throw std::runtime_error(std::string("Socket creation failed: ") + std::strerror(errno));
    int opt = 1;
    if (setsockopt(_lfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
        throw std::runtime_error(std::string("setsockopt failed: ") + std::strerror(errno));
    if (bind(_lfd, (struct sockaddr *)&s, sizeof(s)) < 0)
        throw std::runtime_error(std::string("Bind failed: ") + std::strerror(errno));
    if (listen(_lfd, 10) < 0)
        throw std::runtime_error(std::string("Listen failed: ") + std::strerror(errno));
    if (fcntl(_lfd, F_SETFL, O_NONBLOCK) < 0)
        throw std::runtime_error(std::string("fcntl failed: ") + std::strerror(errno));
}

void    Server::acceptCline(int &lfd, std::vector<pollfd> &fds)
{
    sockaddr_in	clientAddr;
    socklen_t clientAddrLen = sizeof(clientAddr);
    int incoming_call = accept(lfd, (struct sockaddr *)&clientAddr, &clientAddrLen);
    if (incoming_call < 0)
        return;
    int incoming_call_state = fcntl(incoming_call, F_SETFL,O_NONBLOCK); // non block kat3ni anaha lmain thread matb9ach tsna taidir lfd lakhor chi input b7al kima kna kandiro f minishell cat katsna input hadi la kadesactiviha 
    if (incoming_call_state < 0)												// F_SETF == File discriptor SET FL.AG 
    {
        close(incoming_call);
        return; // ma3ndkch mo3idat azpi skipi talfo9 again;!!!!
    }
    pollfd lmo3idat; // hna kadir lmo3idat lljondi  jdid okat7to ftiara dial free fire 
    lmo3idat.fd = incoming_call;
    lmo3idat.events = POLLIN;
    lmo3idat.revents = 0;
    fds.push_back(lmo3idat);
    _clients.insert(std::make_pair(incoming_call, Client(incoming_call, inet_ntoa(clientAddr.sin_addr)))); // hna katdir lclient jdid okat7to fmap dial clients li kayn f server.h okat3tiha lfd dialo o ip dialo
}

void    Server::existingClient(int &fd, std::vector<pollfd> &fds, size_t &i)
{
    char buffer [1024];
    ssize_t size = recv(fd,buffer,sizeof(buffer) - 1,0);
    if (size <= 0)
    {
        // 0 sdha mn raso or client deconecta saf 
        // < 0 rah kain issue f socket handlih t7wa
        int dead_fd = fd; // khodo 9bel materasi, mn b3d lerase fds[i] rah wa7d akhor
        close(dead_fd);
        _clients.erase(dead_fd);//// HNA KHAS DECONNECT LHZA9
        fds.erase(fds.begin() + i);
        i--;
        return;
    }
    buffer[size] = '\0';
    Client &client = _clients[fd];
    client.appendToBuffer(buffer, size);
    client.print();
    std::vector<std::string> cmds = client.splitBuffer(); // hna kat9ra lcommand li jiti mn client okat7to fbuffer dialo
    for (size_t j = 0; j < cmds.size(); j++)
        this->parse_cmd(cmds[j], fd); // hna katparse lcommand li jiti mn client okat9raha ffunction parse_cmd li kayn f server.cpp
}

void    Server::Run()
{
    std::vector<pollfd> fds;
    pollfd lfd;
    lfd.fd = _lfd;
    lfd.events = POLLIN;
    lfd.revents = 0;
    fds.push_back(lfd);

    std::cout << "waiting clinet input" << std::endl;
    while (true)
    {
        // 9bel kol poll kanwajdo events mn jdid: POLLOUT GHIR ila kayn chi 7aja msnya.
        // socket li send buffer dialo khawi rah DIMA writable, donc ila khellina POLLOUT
        // dima 7adr, poll() irje3 f tnach = loop 3la 100% CPU bla ma tdir walo.
        for (size_t i = 0; i < fds.size(); i++)
        {
            if (fds[i].fd == _lfd)
                continue ;
            fds[i].events = POLLIN;
            std::map<int, Client>::iterator it = _clients.find(fds[i].fd);
            if (it != _clients.end() && it->second.hasPendingWrite())
                fds[i].events |= POLLOUT;
        }
        if (poll(fds.data(), fds.size(), -1) < 0)
        {
            if (errno == EINTR)
                continue;
            throw std::runtime_error("poll failed");
        }
        for (size_t i = 0; i < fds.size();i++)
        {
            // POLLOUT 9bel POLLIN wajib: existingClient ymken imse7 lclient mn fds o idir i--,
            // o mn dak lwe9t fds[i] rah wa7d akhor => ghadi nsiftou lbuffer l client ghalet.
            if (fds[i].revents & POLLOUT)
                flushClient(fds[i].fd);
            if (fds[i].revents & POLLIN)
            {
                if (fds[i].fd == _lfd)
                    acceptCline(_lfd, fds);
                else // hna katchecki mn b3d okatla9a bl jondi li deja pushitih fliteration lwla okat9ol lih yala tla7 ojib lia
                    existingClient(fds[i].fd, fds, i);
            }
        }

    }
}

std::vector<std::string>    Server::split_cmd(const std::string &cmd)
{
    std::vector<std::string> tokens;
    std::istringstream stream(cmd);
    std::string token;
    while (stream >> token)
    {
        if (token[0] == ':')
            break;
        tokens.push_back(token);
        token.clear();
    }
    return tokens;
}
