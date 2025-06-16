/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ParseConfig.cpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: tle-moel <tle-moel@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/05/22 15:30:25 by rpandipe          #+#    #+#             */
/*   Updated: 2025/06/16 15:11:41 by tle-moel         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "config/ParseConfig.hpp"
#include <exception>
#include <fstream>
#include <sstream>
#include <iostream>
#include <cstdlib>

webserv::Config::Config(std::string path) : m_path(path), m_pos(0) {}

webserv::Config::~Config() {}

const char* webserv::Config::getHost() const
{
	return m_server[0].hostname.c_str();	
}

int webserv::Config::getPort() const
{
	return m_server[0].port;
}

const std::vector<webserv::Config::Server>& webserv::Config::getServers() const
{
	return m_server;
}

void webserv::Config::parseConfig()
{
	std::ifstream in(m_path.c_str());
	if (!in)
		throw std::runtime_error("Error: Cannot Open config file");
	std::ostringstream ss;
	ss << in.rdbuf();
	m_source = ss.str();
	std::cerr << "entering tokenize" << std::endl;
	tokenize();
	printTokens();
	parseConfigTokens();
}

void webserv::Config::tokenize()
{
	int line = 1;
	size_t len = m_source.size();
	for (size_t i = 0; i < len; )
	{
		char c = m_source[i];
		if (c == '\n')
		{
			i++;
			line++;
			continue ;
		}
		if (isspace(c)) 
		{
			i++;
			continue ;
		}
		if ((c == '/' && i + 1 < len && m_source[i + 1] == '/' &&  
			(i == 0 || m_source[i-1] != ':')) || c == '#')
		{
			std::cerr << "Skipping comments in line " << line << std::endl;
			i += 2;
			while (i < len && m_source[i] != '\n')
				i++;
			continue ;
		}
		if (c =='~')
		{
			m_token.push_back((Token(T_REGEX, "~", line)));
			size_t start = i + 1;
			while (start < len && !isspace(m_source[start]) && m_source[start] != '{' && m_source[start] != ';')
        		++start;
    		m_token.push_back(Token(T_REGEX_STRING,
                            m_source.substr(i + 1, start - i - 1),
                            line));
			i = start;
   			continue;
		}
		if (isalnum(c) || c == '/')
		{
			std::cerr << "Check for keywords" << std::endl;
			int end = i;
			while (i < len && (isalnum(m_source[i]) 
								|| m_source[i] == '.' 
								|| m_source[i] == '-'
								|| m_source[i] == '_'
								|| m_source[i] == '/'))
				i++;
			std::string word = m_source.substr(end, i - end);
			std::cerr << "Current word is " << word << std::endl;
			TokenType tt;
			if (word == "server") tt = T_SERVER;
			else if (word == "listen") tt = T_LISTEN;
			else if (word == "server_name") tt = T_SERVER_NAME;
			else if (word == "root") tt = T_ROOT;
			else if (word == "index") tt = T_INDEX;
			else if (word == "error_page") tt = T_ERROR_PAGE;
			else if (word == "client_max_body_size") tt = T_MAX_BODY_SIZE;
			else if (word == "location") tt = T_LOCATION;
			else if (word == "methods") tt = T_METHOD;
			else if (word == "return") tt = T_RETURN;
			else if (word == "alias") tt = T_ALIAS;
			else if (word == "autoindex") tt = T_AUTOINDEX;
			else if (word == "upload_store") tt = T_UPLOAD_STORE;
			else if (word == "cgi_pass") tt = T_CGI_PASS;
			else 	
			{
				bool allnum  = 1;
				for (size_t j = 0; j < word.size(); j++)
				{
					if (!isdigit(word[j]))
					{
						allnum = 0;
						break ;
					}
				}
				tt = allnum ? T_NUMBER :T_IDENTIFIER;
			}
			m_token.push_back(Token(tt, word, line));
			continue ;
		}
		switch (c)
		{
			case '{' : m_token.push_back(Token(T_LBRACE, "{", line)); break;
			case '}' : m_token.push_back(Token(T_RBRACE, "}", line)); break;
			case ';' : m_token.push_back(Token(T_SEMICOLON, ";", line)); break;
			case ':' : m_token.push_back(Token(T_COLON, ":", line)); break;
			default:
				m_token.push_back(Token(T_ERROR, std::string(1,c), line));
		}
		i++;
	}
	m_token.push_back(Token(T_END, "", line));
	std::cerr << "exiting tokenize" << std::endl;
}

void webserv::Config::printTokens()
{
	std::vector<Token>::iterator it = m_token.begin();
	while (it < m_token.end())
	{
		std::cout << "type is " << it->type << " text is " << it->text << " and line number is " << it->line_nbr << std::endl;
		it++;
	}
}

const webserv::Config::Token& webserv::Config::peek() const
{
	if (m_pos >= m_token.size())
		throw std::out_of_range("No more tokens to peek");
	return m_token[m_pos];
}

webserv::Config::Token& webserv::Config::get()
{
	if (m_pos >= m_token.size())
		throw std::out_of_range("No more tokens to get");
	return m_token[m_pos++];
}

void webserv::Config::expect(TokenType type)
{
	if (peek().type != type)
		throw std::runtime_error("Expected token type does not match in line :" +
							 peek().text);
	get();
}

void webserv::Config::parseConfigTokens()
{
	std::cerr << "entering parseTokens" << std::endl;
	std::cerr << "Initial Token is " << peek().type << std::endl;
	while (peek().type != T_END)
	{
		if (peek().type == T_SERVER)
			parseServer();
		else
			throw std::runtime_error("Unexpected token in line : " +
									peek().text);
	}
	std::cerr << "exiting parseTokens" << std::endl;
}

void webserv::Config::parseServer()
{
	std::cerr << "entering parseServer" << std::endl;
	expect(T_SERVER);
	expect(T_LBRACE);
	Server server;
	server.max_body_size = 1024 * 1024;
	while (peek().type != T_RBRACE)
	{
		TokenType tt = peek().type;
		switch (tt)
		{
			case (T_LISTEN) : parseListen(server); break;
			case (T_SERVER_NAME) : parseServerName(server); break;
			case (T_ROOT) : parseRoot(server); break;
			case (T_INDEX) : parseIndex(server); break;
			case (T_ERROR_PAGE) : parseErrorPage(server); break;
			case (T_MAX_BODY_SIZE) : parseBodySize(server); break;
			case (T_LOCATION) : parseLocation(server); break;
				
			default:
				throw std::runtime_error("Unexpected token in line :" +
									peek().text);
		}	
	}
	expect(T_RBRACE);
	
	// Resolve DNS in IP address
	struct addrinfo hints;
	struct addrinfo *res, *p;
	std::memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET; // only IPv4
	hints.ai_socktype = SOCK_STREAM;

	if(::getaddrinfo(server.hostname.c_str(), NULL, &hints, &res) !=0)
		throw std::runtime_error ("Cannot resolve :" +  server.hostname);

	for (p = res; p != NULL; p = p->ai_next)
	{
		sockaddr_in* sin = reinterpret_cast<sockaddr_in*>(p->ai_addr);
		std::string ip = finet_ntop(sin->sin_addr);
		
		Server clone = server;
		clone.hostname = ip;
		m_server.push_back(clone);
	}
	freeaddrinfo(res);

	std::cerr << "exiting parseServer" << std::endl;
}

void webserv::Config::parseListen(Server &server)
{
	std::cerr << "entering parseListen" << std::endl;
	expect(T_LISTEN);
	if (peek().type != T_IDENTIFIER && peek().type != T_NUMBER)
		throw std::runtime_error("Expected hostname or port in line : " +
								 peek().text);
	server.hostname = peek().text;
	if (server.hostname.empty())
		throw std::runtime_error("Hostname cannot be empty in line " +
								 peek().text);
	get();
	expect(T_COLON);
	if (peek().type != T_NUMBER)
		throw std::runtime_error("Expected port number in line : " +
								 peek().text);
	server.port = ::atoi(peek().text.c_str());
	if (server.port < 0 || server.port > 65535)
	{
		std::ostringstream err;
    	err << "Port number out of range in line "
			<< peek().line_nbr
			<< ": "
			<< server.port;
   		throw std::runtime_error(err.str());
	}
	get();
	expect(T_SEMICOLON);
	std::cerr << "exiting parseListen" << std::endl;
}

void webserv::Config::splitLabels(std::string &name, std::vector<std::string> &labels)
{
	size_t start = 0;
	size_t pos = 0;
	while ((pos = name.find('.', start)) != std::string::npos)
	{
		labels.push_back(name.substr(start, pos - start));
		start = pos + 1;
	}
	labels.push_back(name.substr(start));
}

void webserv::Config::parseServerName(Server& server)
{
	std::cerr << "entering parseServerName" << std::endl;
	expect(T_SERVER_NAME);
	if (peek().type != T_IDENTIFIER)
		throw std::runtime_error("Expected server name in line :" +
									peek().text);
	while(peek().type == T_IDENTIFIER)
	{
		std::string name = get().text;
		std::vector<std::string> labels;
		if (name.empty() || name.size() > 255 || name.find('_') != std::string::npos)
			throw std::runtime_error("Invalid server name :" +
									name);
		splitLabels(name, labels);
		for (size_t i = 0; i < labels.size(); i++)
		{
			int size = labels[i].size();
			if (size == 0 
				|| size > 63
				|| labels[i][0] == '-'
				|| labels[i][size -1] == '-')
				throw std::runtime_error("Invalid server name :" +
									name);	
		}
		labels.clear();
		for (size_t i = 0; i < name.size(); i++)
			name[i] = std::tolower(static_cast<unsigned char>(name[i]));
		server.name.push_back(name);
	}
	expect(T_SEMICOLON);
	std::cerr << "exiting parseServerName" << std::endl;
}

void webserv::Config::parseRoot(Server &server)
{
	std::cerr << "entering parseRoot" << std::endl;
	expect(T_ROOT);
	if (peek().type != T_IDENTIFIER)
		throw std::runtime_error("Expected root :" +
									peek().text);
	std::string root = get().text;
	if (root[0] != '/' || root.find("..") != std::string::npos)
    	throw std::runtime_error("Server root must be absolute and must not  contain '..'");
	expect(T_SEMICOLON);
	server.root = root;
	std::cerr << "exiting parseRoot" << std::endl;
}

void webserv::Config::parseIndex(Server &server)
{
	std::cerr << "entering parseIndex" << std::endl;
	expect(T_INDEX);
	if (peek().type != T_IDENTIFIER)
		throw std::runtime_error("Expected index :" +
									peek().text);
	while (peek().type == T_IDENTIFIER)
	{
		std::string index = get().text;
		size_t len = index.size();
		if (len == 0 || len > 255 || index[0] == '.' || index.find("..") != std::string::npos || index.find('/') != std::string::npos)
			throw std::runtime_error("Invalid index file names :" +
									index);
		server.index_files.push_back(index);
	}
	expect(T_SEMICOLON);
	std::cerr << "exiting parseIndex" << std::endl;
}

void webserv::Config::parseErrorPage(Server& server)
{
	std::cerr << "entering parseErrorPage" << std::endl;
	expect(T_ERROR_PAGE);
	if (peek().type != T_NUMBER)
		throw std::runtime_error("Expected error code in line :" +
									peek().text);
	std::vector<int> error_code;
	while (peek().type == T_NUMBER)
	{
		int code = ::atoi(peek().text.c_str());
		if (code < 400 || code > 599)
			throw std::runtime_error("Invalid error code in line :" +
									peek().text);
		error_code.push_back(code);
		get();
	}
	if (peek().type != T_IDENTIFIER)
		throw std::runtime_error("Expected error page in line :" +
									peek().text);
	std::string error_page = get().text;
	if (error_page.empty() || error_page.size() > 255 || error_page[0] == '.' || error_page.find("..") != std::string::npos)
		throw std::runtime_error("Invalid error page in line :" +
									peek().text);
	for (size_t i = 0; i < error_code.size(); i++)
		server.error_pages[error_code[i]] = error_page;
	expect(T_SEMICOLON);
	std::cerr << "exiting parseErrorPage" << std::endl;
}

void webserv::Config::parseBodySize(Server &server)
{
	std::cerr << "entering parseBodySize" << std::endl;
	expect(T_MAX_BODY_SIZE);
	if (peek().type != T_NUMBER)
		throw std::runtime_error("Expected max body size in line :" +
									peek().text);
	const std::string txt = get().text;
	char* endptr = NULL;
	unsigned long val = strtoul(txt.c_str(), &endptr, 10);
	if (*endptr != '\0') 
	{
		throw std::runtime_error("Invalid number in client_max_body_size: “" 
								+ txt);
	}
	size_t max_size = static_cast<size_t>(val);
	if (max_size > (10 * 1024 * 1024))
		throw std::runtime_error("Max body size out of range in line :" +
									peek().text);
	expect(T_SEMICOLON);
	server.max_body_size = max_size;
	std::cerr << "exiting parseBodySize" << std::endl;
}

void webserv::Config::parseLocation(Server &server)
{
	std::cerr << "entering parseLocation" << std::endl;
	expect(T_LOCATION);
	if (peek().type != T_IDENTIFIER && peek().type != T_REGEX)
		throw std::runtime_error("Expected location prefix in line :" +
									peek().text);
	LocationConfig loc;
	bool isRegex = (peek().type == T_REGEX);
	if (isRegex)
	{
		expect (T_REGEX);
		if (peek().type != T_REGEX_STRING)
			throw std::runtime_error("Expected regex string in line :" +
									 peek().text);
		loc.prefix = get().text;
		if (loc.prefix.empty() || loc.prefix[0] != '\\' || loc.prefix[loc.prefix.size() - 1] != '$')
			throw std::runtime_error("Invalid location regex in line :" +
										peek().text);
		loc.prefix = loc.prefix.substr(1, loc.prefix.size() - 2); // remove leading \ and trailing $
	}
	else if (peek().type == T_IDENTIFIER)
	{
		loc.prefix = get().text;
		if (loc.prefix.empty() || loc.prefix[0] != '/')
			throw std::runtime_error("Invalid location prefix in line :" +
										peek().text);
	}
	expect(T_LBRACE);
	while (peek().type != T_RBRACE)
	{
		TokenType tt = peek().type;
		switch (tt)
		{
			case (T_METHOD) :
			{
				expect(T_METHOD);
				if (peek().type != T_IDENTIFIER)
					throw std::runtime_error("Expected method in line :" +
											 peek().text);
				while (peek().type == T_IDENTIFIER)
				{
					std::string method = get().text;
					if (method != "GET" && method != "POST" && method != "DELETE")
						throw std::runtime_error("Invalid method in line :" +
												 peek().text);
					loc.methods.push_back(method);
				}
				expect(T_SEMICOLON);
				break;
			}
			case (T_RETURN) :
			{
				expect(T_RETURN);
				if (peek().type != T_NUMBER)
					throw std::runtime_error("Expected redirect code in line :" +
											 peek().text);
				loc.hasRedirect = true;
				loc.redirectCode = ::atoi(get().text.c_str());
				if (loc.redirectCode < 300 || loc.redirectCode > 399)
					throw std::runtime_error("Invalid redirect code in line :" +
											 peek().text);
				if (peek().type != T_IDENTIFIER)
					throw std::runtime_error("Expected redirect target in line :" +
											 peek().text);
				std::string target = get().text;
				while (peek().type != T_SEMICOLON)
					target += get().text;
				if (target.find("..") != std::string::npos)
					throw std::runtime_error("Invalid redirect target in line :" +
											 target);
				expect(T_SEMICOLON);
				loc.redirectTarget = target;
				break;
			}
			case (T_ROOT) :
			{
				expect(T_ROOT);
				if (peek().type != T_IDENTIFIER)
					throw std::runtime_error("Expected root :" +
												peek().text);
				std::string root = get().text;
				expect(T_SEMICOLON);
				loc.root = root;
				break;
			}
			case (T_ALIAS) :
			{
				expect(T_ALIAS);
				if (peek().type != T_IDENTIFIER)
					throw std::runtime_error("Expected alias in line :" +
											 peek().text);
				std::string alias = get().text;
				if (alias[0] != '/' || alias.find("..") != std::string::npos)
					throw std::runtime_error("Invalid alias in line :" +
											 alias);
				expect(T_SEMICOLON);
				loc.alias = alias;
				break;
			}
			case (T_AUTOINDEX) :
			{
				expect(T_AUTOINDEX);
				if (peek().type != T_IDENTIFIER)
					throw std::runtime_error("Expected autoindex in line :" +
											 peek().text);
				std::string autoindex = get().text;
				if (autoindex != "on" && autoindex != "off")
					throw std::runtime_error("Invalid autoindex in line :" +
											 autoindex);
				loc.autoindex = (autoindex == "on");
				expect(T_SEMICOLON);
				break;
			}
			case (T_INDEX) :
			{
				expect(T_INDEX);
				if (peek().type != T_IDENTIFIER)
					throw std::runtime_error("Expected index in line :" +
											 peek().text);
				while (peek().type == T_IDENTIFIER)
				{
					std::string index = get().text;
					if (index.size() > 255 || index[0] == '.' || index.find("..") != std::string::npos)
						throw std::runtime_error("Invalid index file names in line :" +
												index);
					loc.indexFiles.push_back(index);
				}
				expect(T_SEMICOLON);
				break;
			}
			case (T_CGI_PASS) :
			{
				expect(T_CGI_PASS);
				if (peek().type != T_IDENTIFIER)
					throw std::runtime_error("Expected cgi_pass in line :" +
											 peek().text);
				std::string cgiPass = get().text;
				if (cgiPass.find("..") != std::string::npos)
					throw std::runtime_error("Invalid cgi_pass in line :" +
											 cgiPass);
				expect(T_SEMICOLON);
				loc.cgiPass = cgiPass;
				webserv::CgiProcess::CgiMapping cgi;
				if (isRegex)
					cgi.extension = loc.prefix;
				else
				{
					size_t pos = loc.cgiPass.find_last_of('.');
					cgi.extension = loc.cgiPass.substr(pos + 1);
				}
				std::cerr << "Adding CGI mapping for extension: " << cgi.extension << " interpreter is " << cgi.interpreter << std::endl;
				server.cgi.push_back(cgi);
				break;
			}
			case (T_UPLOAD_STORE) :
			{
				expect(T_UPLOAD_STORE);
				if (peek().type != T_IDENTIFIER)
					throw std::runtime_error("Expected upload_store in line :" +
											 peek().text);
				std::string uploadStore = get().text;
				if (uploadStore.find("..") != std::string::npos)
					throw std::runtime_error("Invalid upload_store in line :" +
											 uploadStore);
				expect(T_SEMICOLON);
				loc.uploadStore = uploadStore;
				break;
			}
			default:
      			throw std::runtime_error("Unknown location directive: " + peek().text);
		}
	}
	expect(T_RBRACE);
	std::cerr << "exiting parseLocation" << std::endl;
	server.locations.push_back(loc);
}

