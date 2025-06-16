/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Request.cpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: tle-moel <tle-moel@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/05/12 17:28:07 by thomas            #+#    #+#             */
/*   Updated: 2025/06/16 13:02:36 by tle-moel         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "config/ParseConfig.hpp"
#include "server/Request.hpp"

webserv::Request::Request(std::vector<const webserv::Config::Server*> &config, Upload& upload): 
			m_upload(upload), m_method(UNSET), m_serverBlock(NULL), m_locationBlock(NULL), m_config(config), m_keepAlive(true) 
{
	std::cerr << "Request Details " << config[0]->name.size() << std::endl;
}

webserv::Request::~Request() {}

const std::string webserv::Request::getMethod(void) const
{
	if (m_method == webserv::Request::GET) return "GET";
	else if (m_method == webserv::Request::POST) return "POST";
	else if (m_method == webserv::Request::DELETE) return "DELETE";
	else return "";
}

const std::string& webserv::Request::getPath(void) const
{
	return (m_path);
}
 
const std::string& webserv::Request::getQuery(void) const
{
	return (m_query);
}

const std::string& webserv::Request::getVersion(void) const
{
	return (m_version);
}

const std::string& webserv::Request::getBody(void) const
{
	return (m_body);
}

const std::map<std::string, std::vector<std::string> >& webserv::Request::getHeaders(void) const
{
	return (m_headers);
}

bool	webserv::Request::getKeepAlive(void) const
{
	return (m_keepAlive);
}

const webserv::Config::Server* webserv::Request::getServerBlock(void) const
{
	return (m_serverBlock);
}

const webserv::Config::LocationConfig* webserv::Request::getLocationBlock(void) const
{
	return (m_locationBlock);
}

void webserv::Request::setMethod(Method method)
{
	m_method = method;
}

void webserv::Request::setPath(std::string path)
{
	m_path = path;
}

void webserv::Request::setQuery(std::string query)
{
	m_query = query;
}

void webserv::Request::setVersion(std::string version)
{
	m_version = version;
}

void webserv::Request::setHeader(std::string key, std::string value)
{
	m_headers[key].push_back(value);
}

void webserv::Request::setKeepAlive(bool att)
{
	m_keepAlive = att;
}

void webserv::Request::setServerBlock(const webserv::Config::Server* serverBlock)
{
	m_serverBlock = serverBlock;
}

void webserv::Request::setLocationBlock(const webserv::Config::LocationConfig* locationBlock)
{
	m_locationBlock = locationBlock;
}
size_t webserv::Request::getExpectedLength() const
{
	return m_upload.getContentLength();	
}

bool webserv::Request::parseFixedBody(int clientFd, ssize_t maxBodySize, size_t content_length)
{
	char buf[1024];
	ssize_t bytesRead = recv(clientFd, buf, sizeof(buf), 0);
	if (bytesRead <= 0)
		throw HttpError(500); // Internal server error
	m_body.append(buf, bytesRead);
	if (m_body.size() > static_cast<size_t>(maxBodySize) || m_body.size() > content_length)
		throw HttpError(413); // too large
	if (m_body.size() == content_length)
		return true;
	return false;
}

bool webserv::Request::parseChunkedBody(int clientFd, ssize_t maxBodySize, size_t content_length)
{
	char buf[1024];
	ssize_t bytesRead = recv(clientFd, buf, sizeof(buf), 0);
	if (bytesRead <= 0)
		throw HttpError(500); // Internal server error
	m_buffer.append(buf, bytesRead);
	
	size_t pos;
	int	nbytes;
	while (true)
	{
		pos = m_buffer.find("\r\n");
		if (pos == std::string::npos)
			return false; // more data needed

		nbytes = hexastringtoint(m_buffer.substr(0, pos));
		if (m_buffer.size() < pos + 2 + nbytes + 2) // I need -> size \r\n + nbytes \r\n
			return false; // more data needed
		if (nbytes != 0)
		{
			m_buffer.erase(0, pos + 2);
			m_body.append(m_buffer.data(), nbytes);
			m_buffer.erase(0, nbytes + 2);

			if (m_body.size() > static_cast<size_t>(maxBodySize) || m_body.size() > content_length)
				throw HttpError(413); // too large
		}
		else
		{
			m_buffer.clear(); //CHECK TRAILERS LATER
			return true; // move to trailers state
		}
	}
}

void webserv::Request::clearForNextRequest(void)
{
	m_method = UNSET;
	m_path.clear();
	m_query.clear();
	m_version.clear();
	m_body.clear();
	m_buffer.clear();
	m_headers.clear();
	m_serverBlock = NULL;
	m_locationBlock = NULL;
}

void webserv::Request::validateRequest(void)
{
	// Host is always required
	if (!m_headers.count("host") || m_headers["host"][0].empty())
		throw HttpError(400);
	
	// Validate Content-Length if any
	if (m_headers.count("content-length"))
	{
		std::string contentLength = m_headers["content-length"][0];
		if (contentLength.empty())
			throw HttpError(400);
		m_upload.setFixed(true);
		m_upload.setContentLength(stringtoint(contentLength)); //if not valid integer, throw exception
	}

	// Validate Tranfer-Encoding if any
	if (m_headers.count("transfer-encoding"))
	{
		if (m_headers["transfer-encoding"].size() != 1 || m_headers["transfer-encoding"][0] != "chunked") // Verify that the only transfer-coding present is chunked
			throw HttpError(400);
	}
	
	// Update Connection
	if (m_headers.count("connection"))
	{
		std::map<std::string,std::vector<std::string> >::const_iterator it = m_headers.find("connection");
		const std::vector<std::string>& vals = it->second;
		if (std::find(vals.begin(), vals.end(), "close") != vals.end()) 
        	m_keepAlive = false;
	}
}

void webserv::Request::selectServerBlock()
{
	std::map<std::string,std::vector<std::string> >::const_iterator it = m_headers.find("host");
	std::string host = it->second[0]; //I'm sure from parsing that there is at least one host header, with only one entry for the value
	std::cerr << "host:" << host << std::endl;
	size_t colon = host.find(':'); //Take just the host part
	if (colon != std::string::npos)
		host.resize(colon);
	for (size_t i = 0; i < host.size(); i++)
		host[i] = std::tolower(static_cast<unsigned char>(host[i]));


	for (std::vector<const webserv::Config::Server*>::const_iterator it = m_config.begin(); it != m_config.end(); ++it)
	{
		std::cerr << "1st loop and size is " << (*it)->name.size() << std::endl;
		for (std::vector<std::string>::const_iterator sit = (*it)->name.begin(); sit != (*it)->name.end(); ++sit)
		{
			std::cerr << "2nd loop" << std::endl;
			
			if (*sit == host)
			{
				m_serverBlock = *it;
				return ;
			}
		}
	}
	m_serverBlock = m_config.front();
}

void webserv::Request::selectLocationBlock()
{
	const webserv::Config::LocationConfig* locationBlock = NULL;
	size_t bestLen = 0;

	for (size_t i = 0; i < m_serverBlock->locations.size(); ++i)
	{
		const std::string & prefix = m_serverBlock->locations[i].prefix;
		size_t prefixLen = prefix.size();
		std::cerr << "Path is " << m_path << " prefix is " << prefix <<std::endl; 
		if (m_path.size() < prefixLen)
			continue;
		
		if (m_path.compare(0, prefixLen, prefix) == 0)
		{
			if (m_path.size() == prefixLen)
			{
				locationBlock = &m_serverBlock->locations[i];
				break ;
			}
			if (prefix == "/" || m_path[prefixLen] == '/' || prefix[prefixLen - 1] == '/')
			{
				if (prefixLen > bestLen)
				{
					locationBlock = &m_serverBlock->locations[i];
					bestLen = prefixLen;
				}
				
			}
		}
	}
	m_locationBlock = locationBlock;
	if (locationBlock == NULL)
		std::cerr << "Location block  is null" << std::endl;
	else
		std::cerr << "Location block is " << locationBlock->prefix << std::endl;
		
}


void webserv::Request::setCookie(const std::string& name, const std::string& val)
{
	m_cookies[name] = val;
}

std::string webserv::Request::getCookie(const std::string& name) const
{
	std::map<std::string, std::string>::const_iterator it = m_cookies.find(name);
	if (it != m_cookies.end())
		return it->second;
	return "";
}
