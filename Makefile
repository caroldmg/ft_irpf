NAME		= ircserv

CXX			= c++
CXXFLAGS	= -Wall -Wextra -Werror -std=c++98 -Iinc

SRCDIR		= src
OBJDIR		= obj

SRCS		= $(SRCDIR)/main.cpp \
			  $(SRCDIR)/core/Server.cpp \
			  $(SRCDIR)/core/Client.cpp \
			  $(SRCDIR)/core/Channel.cpp \
			  $(SRCDIR)/network/Server_connect.cpp \
			  $(SRCDIR)/network/Server_recv.cpp \
			  $(SRCDIR)/network/IrcMessage.cpp

OBJS		= $(SRCS:$(SRCDIR)/%.cpp=$(OBJDIR)/%.o)

all: $(NAME)

$(NAME): $(OBJS)
	$(CXX) $(CXXFLAGS) $(OBJS) -o $(NAME)

$(OBJDIR)/%.o: $(SRCDIR)/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -rf $(OBJDIR)

fclean: clean
	rm -f $(NAME)

re: fclean all

.PHONY: all clean fclean re
