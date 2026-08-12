// hen@localhost:8666

#ifndef PARSER_HPP
#define PARSER_HPP

#include <cstddef>
#include <string>
#include <vector>
typedef struct s_command
{
    std::string command; // PASS 
    std::vector<std::string> params; // #sdiji
    std::string trailing; // Hello, world!
} t_command;

#endif
