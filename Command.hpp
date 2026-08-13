#ifndef COMMAND_HPP
#define COMMAND_HPP

#define SERVER_NAME "ircserv"

#define RPL_WELCOME(nickname, username, host) (":" SERVER_NAME " 001 " + nickname + " :Welcome to the IRC Network " + nickname + "!" + username + "@" + host + "\r\n")
#define RPL_CHANNELMODEIS(nickname, channel, mode) (":" SERVER_NAME " 324 " + nickname + " " + channel + " " + mode + "\r\n")
#define RPL_NOTOPIC(nickname, channel) (":" SERVER_NAME " 331 " + nickname + " " + channel + " :No topic is set\r\n")
#define RPL_TOPIC(nickname, channel, topic) (":" SERVER_NAME " 332 " + nickname + " " + channel + " :" + topic + "\r\n")
#define RPL_INVITING(nickname, invited, channel) (":" SERVER_NAME " 341 " + nickname + " " + invited + " " + channel + "\r\n")
#define RPL_NAMREPLY(nickname, channel, names) (":" SERVER_NAME " 353 " + nickname + " = " + channel + " :" + names + "\r\n")
#define RPL_ENDOFNAMES(nickname, channel) (":" SERVER_NAME " 366 " + nickname + " " + channel + " :End of /NAMES list\r\n")
#define ERR_NOSUCHNICK(nickname, target) (":" SERVER_NAME " 401 " + nickname + " " + target + " :No such nickname/channel\r\n")
#define ERR_NOSUCHCHANNEL(nickname, channel) (":" SERVER_NAME " 403 " + nickname + " " + channel + " :No such channel\r\n")
#define ERR_CANNOTSENDTOCHAN(nickname, channel) (":" SERVER_NAME " 404 " + nickname + " " + channel + " :Cannot send to channel\r\n")
#define ERR_NOORIGIN(nickname) (":" SERVER_NAME " 409 " + nickname + " :No origin specified\r\n")
#define ERR_NORECIPIENT(nickname, cmd) (":" SERVER_NAME " 411 " + nickname + " :No recipient given (" + cmd + ")\r\n")
#define ERR_NOTEXTTOSEND(nickname) (":" SERVER_NAME " 412 " + nickname + " :No text to send\r\n")
#define ERR_UNKNOWNCOMMAND(nickname, cmd) (":" SERVER_NAME " 421 " + nickname + " " + cmd + " :Unknown command\r\n")
#define ERR_NONICKNAMEGIVEN(nickname) (":" SERVER_NAME " 431 " + nickname + " :No nickname given\r\n")
#define ERR_ERRONEUSNICKNAME(nickname, bad_nickname) (":" SERVER_NAME " 432 " + nickname + " " + bad_nickname + " :Erroneous nickname\r\n")
#define ERR_NICKNAMEINUSE(nickname, bad_nickname) (":" SERVER_NAME " 433 " + nickname + " " + bad_nickname + " :Nickname is already in use\r\n")
#define ERR_USERNOTINCHANNEL(nickname, target, channel) (":" SERVER_NAME " 441 " + nickname + " " + target + " " + channel + " :They aren't on that channel\r\n")
#define ERR_NOTONCHANNEL(nickname, channel) (":" SERVER_NAME " 442 " + nickname + " " + channel + " :You're not on that channel\r\n")
#define ERR_USERONCHANNEL(nick, target, channel) (":" SERVER_NAME " 443 " + nick + " " + target + " " + channel + " :is already on channel\r\n")
#define ERR_NOTREGISTERED(nickname) (":" SERVER_NAME " 451 " + nickname + " :You have not registered\r\n")
#define ERR_NEEDMOREPARAMS(nickname, cmd) (":" SERVER_NAME " 461 " + nickname + " " + cmd + " :Not enough parameters\r\n")
#define ERR_ALREADYREGISTERED(nickname) (":" SERVER_NAME " 462 " + nickname + " :You may not reregister\r\n")
#define ERR_PASSWDMISMATCH(nickname) (":" SERVER_NAME " 464 " + nickname + " :Password incorrect\r\n")
#define ERR_UNKNOWNMODE(nickname, mode) (":" SERVER_NAME " 472 " + nickname + " " + mode + " :is unknown mode char to me\r\n")
#define ERR_CHANNELISFULL(nickname, channel) (":" SERVER_NAME " 471 " + nickname + " " + channel + " :Cannot join channel (+l)\r\n")
#define ERR_INVITEONLYCHAN(nickname, channel) (":" SERVER_NAME " 473 " + nickname + " " + channel + " :Cannot join channel (+i)\r\n")
#define ERR_BADCHANNELKEY(nickname, channel) (":" SERVER_NAME " 475 " + nickname + " " + channel + " :Cannot join channel (+k)\r\n")
#define ERR_CHANOPRIVSNEEDED(nickname, channel) (":" SERVER_NAME " 482 " + nickname + " " + channel + " :You're not channel operator\r\n")

#endif