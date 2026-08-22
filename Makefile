NAME		= ircserv

CXX			= c++
CXXFLAGS	= -Wall -Wextra -Werror -std=c++98
INCFLAGS	= -I includes

SRC_DIR		= srcs

SRCS		= $(SRC_DIR)/main.cpp \
				$(SRC_DIR)/core/Server.cpp \
				$(SRC_DIR)/core/Client.cpp \
				$(SRC_DIR)/core/Channel.cpp \
				$(SRC_DIR)/commands/Authentication.cpp \
				$(SRC_DIR)/commands/join.cpp \
				$(SRC_DIR)/commands/PRIVMSG.cpp \
				$(SRC_DIR)/commands/kick.cpp \
				$(SRC_DIR)/commands/INVITE.cpp \
				$(SRC_DIR)/commands/TOPIC.cpp \
				$(SRC_DIR)/commands/MODE.cpp

OBJS		= $(SRCS:.cpp=.o)

all: $(NAME)

$(NAME): $(OBJS)
	$(CXX) $(CXXFLAGS) $(OBJS) -o $(NAME)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(INCFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS)

fclean: clean
	rm -f $(NAME)

re: fclean all

.PHONY: all clean fclean re
