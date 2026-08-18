NAME		= ircserv

CXX			= c++
CXXFLAGS	= -Wall -Wextra -Werror -std=c++98
DEPFLAGS	= -MMD -MP

SRCS		= main.cpp Server.cpp Client.cpp Authentication.cpp \
				join.cpp Channel.cpp PRIVMSG.cpp kick.cpp INVITE.cpp\
				TOPIC.cpp

OBJS		= $(SRCS:.cpp=.o)
DEPS		= $(OBJS:.o=.d)

all: $(NAME)

$(NAME): $(OBJS)
	$(CXX) $(CXXFLAGS) $(OBJS) -o $(NAME)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(DEPFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(DEPS)

fclean: clean
	rm -f $(NAME)

re: fclean all

-include $(DEPS)

.PHONY: all clean fclean re
