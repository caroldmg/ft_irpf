// Pruebas unitarias de IrcMessage::parse — sin servidor, compila solo con IrcMessage.cpp
#include <iostream>
#include <string>
#include <cassert>
#include "IrcMessage.hpp"

static int g_pass = 0;
static int g_fail = 0;

static void check(const char *name, bool ok)
{
	if (ok)
	{
		std::cout << "  [PASS] " << name << "\n";
		++g_pass;
	}
	else
	{
		std::cout << "  [FAIL] " << name << "\n";
		++g_fail;
	}
}

// ─── helpers ──────────────────────────────────────────────────────────────────

static void test_empty_line()
{
	IrcMessage m;
	check("linea vacia → false", !IrcMessage::parse("", m));
}

static void test_simple_command()
{
	IrcMessage m;
	bool ok = IrcMessage::parse("PING", m);
	check("PING — parsea ok",           ok);
	check("PING — command correcto",    m.command == "PING");
	check("PING — sin params",          m.params.empty());
	check("PING — sin prefix",          m.prefix.empty());
}

static void test_uppercase_normalization()
{
	IrcMessage m;
	IrcMessage::parse("nick alice", m);
	check("nick → NICK (upcase)",       m.command == "NICK");
	check("nick alice — param correcto", m.params.size() == 1 && m.params[0] == "alice");
}

static void test_trailing_param()
{
	IrcMessage m;
	IrcMessage::parse("PRIVMSG #chan :hello world", m);
	check("trailing — command",         m.command == "PRIVMSG");
	check("trailing — 2 params",        m.params.size() == 2);
	check("trailing — target",          m.params[0] == "#chan");
	check("trailing — texto con espacio", m.params[1] == "hello world");
}

static void test_prefix()
{
	IrcMessage m;
	IrcMessage::parse(":nick!user@host QUIT :bye", m);
	check("prefix — extraido",          m.prefix == "nick!user@host");
	check("prefix — command",           m.command == "QUIT");
	check("prefix — trailing param",    m.params.size() == 1 && m.params[0] == "bye");
}

static void test_pass_command()
{
	IrcMessage m;
	IrcMessage::parse("PASS secreto", m);
	check("PASS — command",             m.command == "PASS");
	check("PASS — param password",      m.params.size() == 1 && m.params[0] == "secreto");
}

static void test_user_command()
{
	// USER alice 0 * :Nombre Real
	IrcMessage m;
	IrcMessage::parse("USER alice 0 * :Nombre Real", m);
	check("USER — command",             m.command == "USER");
	check("USER — 4 params",            m.params.size() == 4);
	check("USER — username",            m.params[0] == "alice");
	check("USER — mode",                m.params[1] == "0");
	check("USER — unused",              m.params[2] == "*");
	check("USER — realname con espacio", m.params[3] == "Nombre Real");
}

static void test_cap_command()
{
	IrcMessage m;
	IrcMessage::parse("CAP LS 302", m);
	check("CAP — command",              m.command == "CAP");
	check("CAP — 2 params",            m.params.size() == 2);
	check("CAP — subcommand",          m.params[0] == "LS");
	check("CAP — version",             m.params[1] == "302");
}

static void test_multiple_spaces()
{
	IrcMessage m;
	IrcMessage::parse("NICK   alice", m);
	check("multi-espacio — command",    m.command == "NICK");
	check("multi-espacio — param",      m.params.size() == 1 && m.params[0] == "alice");
}

static void test_trailing_only_colon()
{
	IrcMessage m;
	IrcMessage::parse("QUIT :", m);
	check("trailing vacio — command",   m.command == "QUIT");
	check("trailing vacio — param",     m.params.size() == 1 && m.params[0] == "");
}

static void test_tostring_roundtrip()
{
	IrcMessage m;
	IrcMessage::parse("PRIVMSG #chan :hola mundo", m);
	std::string s = m.toString();
	check("toString — contiene PRIVMSG", s.find("PRIVMSG") != std::string::npos);
	check("toString — contiene #chan",   s.find("#chan") != std::string::npos);
	check("toString — contiene trailing", s.find(":hola mundo") != std::string::npos);
}

static void test_prefix_without_params()
{
	IrcMessage m;
	bool ok = IrcMessage::parse(":only_prefix", m);
	check("prefix sin command → false", !ok);
}

// ─── main ─────────────────────────────────────────────────────────────────────

int main()
{
	std::cout << "\n=== IrcMessage parser — unit tests ===\n\n";

	test_empty_line();
	test_simple_command();
	test_uppercase_normalization();
	test_trailing_param();
	test_prefix();
	test_pass_command();
	test_user_command();
	test_cap_command();
	test_multiple_spaces();
	test_trailing_only_colon();
	test_tostring_roundtrip();
	test_prefix_without_params();

	std::cout << "\nResultado: " << g_pass << " pass, " << g_fail << " fail\n\n";
	return (g_fail == 0 ? 0 : 1);
}
