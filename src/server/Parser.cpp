/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Parser.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: tle-moel <tle-moel@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/05/19 10:49:40 by tle-moel          #+#    #+#             */
/*   Updated: 2025/06/13 17:26:08 by tle-moel         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "server/Parser.hpp"

webserv::Parser::Parser(Request& request): m_request(request) {}

webserv::Parser::~Parser() {}

void webserv::Parser::clearBuffer(void)
{
	m_buffer.clear();
}
std::string webserv::Parser::getBuffer(void)
{
	return (m_buffer);
}

bool webserv::Parser::feed(const char* data, size_t length)
{	
	// ****
	std::cerr << std::endl << "ENTERING PARSING FEED" << std::endl;

	m_buffer.append(data, length);
	if (m_buffer.size() > DEFAULT_MAX_BUFFER)
		throw HttpError(413); //Payload Too Large
	
	size_t pos = m_buffer.find("\r\n\r\n");
	if (pos != std::string::npos)
	{
		// PARSING STARTING LINE
		size_t start = m_buffer.find("\r\n");
		std::string startLine = m_buffer.substr(0, start);
		parseStartLine(startLine);
		m_buffer.erase(0, start + 2); // remove start line

		// *****
		std::cerr << "STARTING LINE PARSED" << std::endl;
		std::cerr << "Method: " << m_request.getMethod() << std::endl;
		std::cerr << "Path: " << m_request.getPath() << std::endl;
		std::cerr << "Query: " << m_request.getQuery() << std::endl;
		std::cerr << "Version: " << m_request.getVersion() << std::endl;
		// *****

		// PARSING HEADERS
		size_t end = m_buffer.find("\r\n\r\n");
		if (end != std::string::npos)
		{
			std::string headers = m_buffer.substr(0, end + 4);
			parseHeaders(headers);
			m_buffer.erase(0, pos + 4); // remove headers
			m_request.validateRequest();
			
			std::cerr << "HEADERS PARSED SUCCESSFULLY" << std::endl;
		}
		else
			throw HttpError(400);
	}
	else
		return false;
	return true;
}

void webserv::Parser::parseStartLine(std::string startLine)
{
	std::istringstream iss(startLine);
	std::string token;
	std::vector<std::string> parts;

	while (std::getline(iss, token, ' '))
	{
		if (token.empty())
			throw HttpError(400);
		parts.push_back(token);
	}
	if (parts.size() != 3)
		throw HttpError(400);
	
	if (parts[0] == "GET")
		m_request.setMethod(webserv::Request::GET);
	else if (parts[0] == "POST")
		m_request.setMethod(webserv::Request::POST);
	else if (parts[0] == "DELETE")
		m_request.setMethod(webserv::Request::DELETE);
	else if (parts[0] == "HEAD")
		m_request.setMethod(webserv::Request::HEAD);
	else		
		throw HttpError(400);

	parseTarget(parts[1]);
	
	if (parts[2] != "HTTP/1.1")
		throw HttpError(505); //HTTP Version Not Supported 
	m_request.setVersion(parts[2]);
}

void webserv::Parser::parseTarget(const std::string& target)
{
	if (target[0] == '/')
	{
		parsePathAndQuery(target);
		return;
	}
	
	size_t pos = target.find("://");
	std::string scheme = target.substr(0, pos);
	if (scheme != "http" && scheme != "https")
		throw HttpError(400);
	size_t delim = target.find_first_of('/', pos + 3);
	if (delim == std::string::npos)
		parsePathAndQuery("/");
	else
		parsePathAndQuery(target.substr(delim));
}

void webserv::Parser::parsePathAndQuery(std::string raw_path)
{
	size_t pos = raw_path.find('?');
	if (pos != std::string::npos)
	{
		m_request.setQuery(raw_path.substr(pos + 1));
		raw_path = raw_path.substr(0, pos);
	}
	if (raw_path == "/")
	{
		m_request.setPath("/");
		return ;
	}

	std::istringstream iss(raw_path);
	std::string token;
	std::vector<std::string> directories;
	while (std::getline(iss, token, '/'))
	{
		if (token.find('%') != std::string::npos)
			decodePercentage(token);
		if (token.empty() || token == ".")
			continue;
		else if (token == "..")
		{
			if (directories.empty())
				throw HttpError(400);
			directories.pop_back();
		}
		else
			directories.push_back(token);
	}
	
	std::string path;
	for (std::vector<std::string>::iterator it = directories.begin(); it != directories.end(); it++)
		path.append("/" + *it);
	m_request.setPath(path);
	
}

void webserv::Parser::decodePercentage(std::string& token)
{
	std::string new_token;
	new_token.reserve(token.size());

	for (size_t i = 0; i < token.size(); i++)
	{
                if (token[i] == '%')
                {
                        if (i + 2 >= token.size())
                                throw HttpError(400);
                        char c = hexastringtoint(token.substr(i + 1, 2));
			new_token.push_back(c);
			i += 2;
		}
		else
			new_token.push_back(token[i]);
	}
	token = new_token;
}

void webserv::Parser::parseHeaders(std::string headers)
{
	while (true)
	{
		size_t pos = headers.find("\r\n");
		if (pos == 0) // End of headers -> we got \r\n\r\n
		{
			std::cerr << "Parser parsed each line of headers" << std::endl;
			return;
		}
		std::string headerLine = headers.substr(0, pos);
		parseHeaderLine(headerLine);
		headers.erase(0, pos + 2);
	}
}

void webserv::Parser::parseHeaderLine(const std::string& line)
{
	// *****
	std::cerr << "Parsing line : " << line << std::endl;
	// *****
	
	size_t pos = line.find(':');
	if (pos == std::string::npos)
		throw HttpError(400);

	std::string key = line.substr(0, pos);
	if (key.find(' ') != std::string::npos || key.find('\t') != std::string::npos)
		throw HttpError(400);
	for (size_t i = 0; i < key.size(); i++)
		key[i] = std::tolower(static_cast<unsigned char>(key[i]));

	std::string value = line.substr(pos + 1);
	trimOWS(value);
	if (key == "cookies")
	{
		std::istringstream iss(value);
		std::string pair;
		while (std::getline(iss, pair, ','))
		{
			trimOWS(pair);
			size_t eq  = pair.find('=');
			if (eq != std::string::npos)
				m_request.setCookie(pair.substr(0, eq), pair.substr(eq + 1));
		}
		return;
	}
	pos = value.find(',');
	if (pos != std::string::npos && key != "set-cookie")
	{
		if (!isCommaListHeader(key))
			throw HttpError(400);
		normalizeCommaListedValue(key, value);
	}
	else
		m_request.setHeader(key, value);
	std::cerr << "Line parsed successfully" << std::endl;
	std::cerr << "key: " << key << " -> value: " << value << std::endl;
}

void webserv::Parser::normalizeCommaListedValue(std::string& key, std::string& value)
{
	bool atLeastOne = false;
	size_t empties = 0;
	size_t start = 0;
	
	while (true)
	{
		std::string elem;
		size_t pos = value.find(',', start);
		if (pos == std::string::npos)
			elem = value.substr(start);
		else
			elem = value.substr(start, pos - start);
		trimOWS(elem);
		if (elem.empty())
		{
			if (++empties > MAX_EMPTY_SLOTS)
				throw HttpError(400);
		}
		else
		{
			atLeastOne = true;
			m_request.setHeader(key, elem);
		}
		if (pos == std::string::npos)
			break;
		start = pos + 1;
	}
	if (!atLeastOne)
		throw HttpError(400);
}

bool webserv::Parser::isCommaListHeader(const std::string header)
{
	if (header == "accept" || header == "accept-charset" ||
		header == "accept-encoding" || header == "accept-language" ||
		header == "cache-control" || header == "connection" ||
		header == "content-encoding" ||	header == "content-language" ||
		header == "expect" || header == "if-match" ||
		header == "if-none-match" || header == "pragma" || header == "priority" ||
		header == "proxy-authenticate" || header ==  "te" ||
		header == "trailer" || header == "transfer-encoding" ||
		header == "upgrade" || header == "vary" ||
		header == "via" || header == "warning")
			return true;
		return false;
}