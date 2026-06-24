*This project has been created as part of the 42 curriculum by cde-migu, mcaro-ro, asaiz-lo.*

 
## Table of Contents
 
- [Description](#description)
- [Features](#features)
- [Supported Commands](#supported-commands)
- [Channel Modes](#channel-modes)
- [Bonus](#bonus-ircbot)
- [Project Structure](#project-structure)
- [Instructions](#instructions)
- [Usage Example](#usage-example)
- [Tests](#tests)
- [Technical Choices](#technical-choices)
- [Resources](#resources)
---
 
## Description
 
`ft_irc` is an implementation of an **IRC (Internet Relay Chat) server** written from
scratch in **C++98**. Its goal is to build a server able to handle multiple clients
**simultaneously and without ever blocking**, communicating over TCP/IP and respecting
the subset of the IRC standard required to connect with a real reference client such as
**HexChat**, **WeeChat** or **irssi**.
 
All input/output is multiplexed through a **single `poll()`** call: there is no forking,
no threading and no blocking read/write operations, so the server never hangs waiting on
a client. The project covers low-level network programming concepts — sockets, event
handling, message framing by `\r\n`, user registration, channels, channel operators and
message broadcasting.
 
A connection always follows the same flow:
 
1. The client sends the server password with `PASS`.
2. It identifies itself with `NICK` (nickname) and `USER` (username + real name).
3. Once the handshake is complete, the server replies with the welcome numerics
   (`001`, `002`, `003`, `004`) and the client can join channels and chat.
---
 
## Features
 
- **Single `poll()`** for every connection (listening socket + all clients).
- **Per-client buffer**: partial messages are accumulated and only processed once a full
  line terminated by `\r\n` is received (correct handling of fragmented TCP streams).
- **Signal handling** (`SIGINT`, `SIGTERM`) for a clean shutdown, and `SIGPIPE` ignored.
- **Argument validation**: port in the `1–65535` range and a non-empty password.
- Full user **registration** flow with password authentication.
- **Channels** with operators, topics, invitations and the mandatory channel modes.
- Private and channel **messaging** (`PRIVMSG` / `NOTICE`).
- Compatible with a **reference IRC client**.
- Builds with `-Wall -Wextra -Werror` and **no external libraries**.
---
 
## Supported Commands
 
### Registration & connection
 
| Command | Description |
|---------|-------------|
| `PASS`  | Send the server password |
| `NICK`  | Set or change the nickname |
| `USER`  | Register username and real name |
| `CAP`   | Capability negotiation (handshake with modern clients) |
| `PING` / `PONG` | Connection keep-alive check |
| `QUIT`  | Close the session and disconnect the client |
 
### Channels & messaging
 
| Command | Description |
|---------|-------------|
| `JOIN`    | Join (or create) a channel |
| `PART`    | Leave a channel |
| `PRIVMSG` | Send a message to a user or channel |
| `NOTICE`  | Send a notice (no automatic error reply) |
| `TOPIC`   | View or change the channel topic |
| `KICK`    | Eject a user from a channel (operator) |
| `INVITE`  | Invite a user to a channel (operator) |
| `MODE`    | Change channel modes (operator) |
 
---
 
## Channel Modes
 
The `MODE` command implements the modes required by the subject:
 
| Mode | Effect |
|:----:|--------|
| `+i` / `-i` | Invite-only channel |
| `+t` / `-t` | Only operators may change the `TOPIC` |
| `+k` / `-k` | Set or remove a channel password (key) |
| `+o` / `-o` | Grant or revoke operator privilege to a user |
| `+l` / `-l` | Set or remove the channel user limit |
 
---
 
## Bonus: `ircbot`
 
The bonus part includes **PPBot**, a standalone IRC client that connects to the server
(or to any reference IRC server) and replies automatically:
 
- Reacts to the `ppsize` command both in **direct PRIVMSG** and in **channel messages**.
- **Auto-joins** a channel when it is invited (`INVITE`).
- Replies to the server's `PING` with `PONG` to keep the connection alive.
```bash
make bonus
./ircbot <host> <port> <password> [nick]
```
 
---
 
## Project Structure
 
```
ft_irc/
├── inc/                     # Server headers
│   ├── Server.hpp
│   ├── Client.hpp
│   ├── Channel.hpp
│   ├── IrcMessage.hpp
│   └── Replies.hpp
├── src/
│   ├── main.cpp             # Argument parsing, signals and startup
│   ├── core/                # Domain logic
│   │   ├── Server.cpp
│   │   ├── Client.cpp
│   │   └── Channel.cpp
│   ├── network/             # Network layer
│   │   ├── Server_connect.cpp
│   │   ├── Server_recv.cpp
│   │   └── IrcMessage.cpp   # Protocol parser
│   └── commands/            # Command implementations
│       ├── Cmd_register.cpp
│       └── Cmd_channel.cpp
├── bonus/                   # IRC bot (standalone binary)
│   ├── inc/bot.hpp
│   └── src/{bot.cpp,main.cpp}
├── tests/                   # Test suite (unit + integration)
│   ├── unit/test_parser.cpp
│   ├── integration/*.sh
│   ├── run_all.sh
│   └── Tests.md
└── Makefile
```
 
---
 
## Instructions
 
### Build
 
```bash
git clone https://github.com/caroldmg/ft_irpf.git
cd ft_irpf
make
```
 
Available Makefile rules:
 
| Rule | Action |
|------|--------|
| `make` / `make all` | Build the `ircserv` server |
| `make bonus`        | Build the `ircbot` bot |
| `make clean`        | Remove object files (`obj/`) |
| `make fclean`       | Remove objects and binaries |
| `make re`           | `fclean` + `all` |
| `make test`         | Run the full test suite |
| `make test_unit`    | Run only the parser unit tests |
| `make test_integration` | Run only the integration tests |
 
### Run
 
```bash
./ircserv <port> <password>
```
 
- `<port>`: listening port (integer between 1 and 65535).
- `<password>`: password clients must send to connect.
Example:
 
```bash
./ircserv 6667 mypassword
```
 
Then connect with your favourite client (e.g. HexChat) pointing to `localhost:6667`
with that same password, or test quickly with `netcat`:
 
```bash
nc localhost 6667
```
 
---
 
## Usage Example
 
```text
PASS mypassword
NICK alice
USER alice 0 * :Alice Doe
JOIN #42
TOPIC #42 :Welcome to the channel
PRIVMSG #42 :Hello everyone!
MODE #42 +it
INVITE bob #42
```
 
The server replies with the welcome numerics (`001`–`004`), confirms the `JOIN`,
broadcasts the `TOPIC` and the `PRIVMSG`, and applies the channel modes.
 
---
 
## Tests
 
The project ships with its own test battery validating both the parser and the
end-to-end behaviour of the server:
 
```bash
make test              # full suite (unit + integration)
make test_unit         # IRC message parser only
make test_integration  # handshake, errors, JOIN/PART/PRIVMSG, modes, KICK...
```
 
The integration tests cover, among others: the registration handshake, error numerics
(`464`, `433`, `451`), `PING/PONG`, `CAP`, `JOIN`/`PART`/`PRIVMSG`, `NOTICE`/`TOPIC`,
and `KICK`/`MODE`/`INVITE`. See [`tests/Tests.md`](tests/Tests.md) for the manual
testing guide using real clients.
 
---

 
### Use of AI
 
AI tools were used in a supporting role, never to write the core server logic:
 
- **Test documentation**: the manual testing guide [`tests/Tests.md`](tests/Tests.md)
  was drafted with an LLM and then reviewed and adjusted by the team.
- **README**: this file was structured and drafted with AI assistance from the actual
  project sources, then verified against the code.
- **Debugging & explanations**: used occasionally to clarify parts of the IRC RFCs,
  understand specific error numerics, and review edge cases in the message parser.
All AI-assisted output was read, tested and validated by the authors before delivery.
 
