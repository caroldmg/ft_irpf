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
			  $(SRCDIR)/network/IrcMessage.cpp \
			  $(SRCDIR)/commands/Cmd_register.cpp \
			  $(SRCDIR)/commands/Cmd_channel.cpp

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
	rm -f tests/unit/test_parser

re: fclean all

# ── Tests ──────────────────────────────────────────────────────────────────────

TEST_UNIT_BIN = tests/unit/test_parser
TEST_UNIT_SRC = tests/unit/test_parser.cpp $(SRCDIR)/network/IrcMessage.cpp

# Compila solo el binario de unit tests (sin servidor)
test_build: $(TEST_UNIT_BIN)

$(TEST_UNIT_BIN): $(TEST_UNIT_SRC)
	$(CXX) $(CXXFLAGS) $(TEST_UNIT_SRC) -o $(TEST_UNIT_BIN)

# Solo unit tests (sin servidor)
test_unit: test_build
	@echo ""
	@echo "=== IrcMessage parser — unit tests ==="
	./$(TEST_UNIT_BIN)

# Solo integration tests (requiere que el servidor compile)
test_integration: all
	@chmod +x tests/integration/common.sh tests/integration/test_*.sh
	@for t in tests/integration/test_*.sh; do bash $$t; done

# Suite completa
test: all test_build
	@chmod +x tests/run_all.sh tests/integration/common.sh tests/integration/test_*.sh
	@bash tests/run_all.sh

.PHONY: all clean fclean re test test_build test_unit test_integration
