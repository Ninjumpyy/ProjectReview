/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ClientHandler.cpp                                  :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: tle-moel <tle-moel@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/04/22 17:15:00 by rpandipe          #+#    #+#             */
/*   Updated: 2025/06/17 12:08:31 by tle-moel         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "server/ClientHandler.hpp"
#include <errno.h>
#include <sstream>
#include <iostream>
#include <unistd.h>

webserv::ClientHandler::ClientHandler(const Fd &listenfd, Poller &poller, std::vector<const webserv::Config::Server*> &config): 
			m_fd(listenfd), m_poller(poller), m_upload(), m_request(config, m_upload), m_response(m_request), 
			m_parser(m_request), m_config(config), m_status(PARSING_HEADER), m_headerOffset(0), m_bodyOffset(0), 
			m_fileFD(-1), m_bodyToSend(false), isCGI(false)
{
	std::cerr << "Client handler Details " << m_config[0]->name.size() << std::endl;
}

webserv::ClientHandler::~ClientHandler() 
{
	std::cerr << "ClientHandler: Destructor called for fd " << m_fd.getfd() << std::endl;
}

void webserv::ClientHandler::onEvent(short revents)
{
	if (!m_fd.isValid())
	{
		std::cerr << "ClientHandler: Invalid fd, cleaning up" << std::endl;
		cleanup();
		return;
	}
	std::cerr << "ClientHandler: Event received for fd " << m_fd.getfd() << ": " << revents << std::endl;
	if (revents & (POLLERR | POLLNVAL)) 
	{
		std::cerr << "ClientHandler: Error event received, cleaning up" << std::endl;
        cleanup();
        return;
    }
	if (revents & POLLHUP)
	{
		std::cerr << "ClientHandler: remote hang-up, cleaning up" << std::endl;
        cleanup();
        return;
	}
	if (revents & POLLIN)
	{
		try
		{
			if (m_status == PARSING_HEADER)
			{
				char buf[1024];
				ssize_t bytesRead = recv(m_fd.getfd(), buf, sizeof(buf), 0);
				if (bytesRead > 0)
				{
					if (!m_parser.feed(buf, bytesRead)) 
					return ; // Need more data to parse headers
				
					// Check server and location
					m_request.selectServerBlock();
					m_request.selectLocationBlock();
					checkServerAndLocation();
					std::string sid = m_request.getCookie("SESSIONID");
					if (sid.empty() || !SessionManager::exists(sid)) 
					{
						sid = SessionManager::createSession();
						m_response.addCookie("SESSIONID", sid);
					}
					SessionManager::getSession(sid);
					if (m_status != REDIRECTION)
						evaluateTransition(); // Set m_status based on request
					std::cerr << "POLLIN tasks successful" << std::endl;
				}	
				else
				{
					std::cerr << "ClientHandler: Read error or connection closed: " << std::endl;
					cleanup();
					return;
				}
			}
			if (m_status == EXPECT_CONTINUE)
			{
				std::cerr << "ClientHandler: catch Expect-Continue" << std::endl;
				m_response.buildErrorMessage(100); //Expect Continue
				m_poller.modify(m_fd.getfd(), POLLOUT);
			}
			if (m_status == EXPECT_FAILED)
			{
				std::cerr << "ClientHandler: catch Expection-Failed" << std::endl;
				m_response.buildErrorMessage(417); //Expect Error
				m_poller.modify(m_fd.getfd(), POLLOUT);
			}
			if (m_status == REDIRECTION)
			{
				std::cerr << "ClientHandler: Redirection" << std::endl;
				m_response.buildRedirection();
				m_poller.modify(m_fd.getfd(), POLLOUT);
			}
			if (m_status == PARSING_BODY)
			{
				std::cerr << "ClientHandler: Parsing body" << std::endl;


				if (isCGI)
				{
					std::cerr << "ClientHandler: CGI" << std::endl;
					if (m_upload.isChunked())
					{
						if (m_request.parseChunkedBody(m_fd.getfd(), m_upload.getMaxBodySize(), m_upload.getContentLength()))
						{
							CgiProcess * process = new CgiProcess(m_request.getServerBlock()->cgi[0], m_request.getPath(), m_request, m_response);
							process->spawn();
							m_poller.modify(m_fd.getfd(), POLLOUT);
						}
						return ;
					}
					if (m_upload.isFixed())
					{
						if (m_request.parseFixedBody(m_fd.getfd(), m_upload.getMaxBodySize(), m_upload.getContentLength()))
						{
							CgiProcess * process = new CgiProcess(m_request.getServerBlock()->cgi[0], m_request.getPath(), m_request, m_response);
							process->spawn();
							m_poller.modify(m_fd.getfd(), POLLOUT);
						}
						return ;
					}
				}
				else if (m_request.getMethod() == "POST")
				{
					if (m_upload.getUploadStore().empty())
						findUploadStore();
					if (m_upload.handlePost(m_fd.getfd()))
					{
						std::cerr << "ClientHandler: Upload completed" << std::endl;

						m_response.buildPostResponse(m_upload.getUploadStore());						
						m_poller.modify(m_fd.getfd(), POLLOUT);
					}
				}
				else if (m_request.getMethod() == "DELETE")
				{
					if (m_upload.handleDelete(m_fd.getfd()))
					{
						std::cerr << "ClientHandler: DELETE request completed" << std::endl;
						m_status = COMPLETE;
					}
				}
			}
			if (m_status == COMPLETE)
			{
				if (isCGI)
				{
					CgiProcess * process = new CgiProcess(m_request.getServerBlock()->cgi[0], m_request.getPath(), m_request, m_response);
					process->spawn();
					m_poller.modify(m_fd.getfd(), POLLOUT);
				}
				else
				{
					prepareResponse();
					m_poller.modify(m_fd.getfd(), POLLOUT);
				}
			}
		}
		catch (const HttpError& err)
		{
			m_response.buildErrorMessage(err.code());
			m_poller.modify(m_fd.getfd(), POLLOUT);
		}
	}
	if (revents & POLLOUT)
	{
		if (!m_response.getResponseBuffer().empty())
		{
			while (m_headerOffset < m_response.getResponseBuffer().size())
			{
				ssize_t sent = ::send(m_fd.getfd(), m_response.getResponseBuffer().c_str() + m_headerOffset, m_response.getResponseBuffer().size() - m_headerOffset, 0);
				if (sent > 0)
				{
					std::cerr << "ClientHandler: Header Sent " << sent << " bytes" << std::endl;
					m_headerOffset += static_cast<size_t>(sent);
				}
				else //non-blocking soket -> send <= 0 is fatal
				{
					std::cerr << "Non-blocking soket send <= 0 is fatal" << std::endl;
					cleanup();
					return;
				}
			}
			
			// *****
			std::cerr << "ClientHandler: Header Sent :" << std::endl;
			std::cerr << m_response.getResponseBuffer() << std::endl << std::endl;
			// *****
			m_response.getResponseBuffer().clear(); // Clear the response buffer after sending
		}
		if (m_bodyToSend)
		{
			char buf[16*1024];
			ssize_t nbytes = read(m_fileFD, buf, sizeof(buf));
			if (nbytes > 0)
			{
				while (m_bodyOffset < static_cast<size_t>(nbytes))
				{
					ssize_t sent = ::send(m_fd.getfd(), buf + m_bodyOffset, nbytes - m_bodyOffset, 0);
					if (sent > 0)
					{
						std::cerr << "ClientHandler: Body Sent " << sent << " bytes" << std::endl;
						m_bodyOffset += static_cast<size_t>(sent);
					}
					else //non-blocking soket send <= 0 is fatal
					{
						std::cerr << "Non-blocking soket send <= 0 is fatal" << std::endl;
						cleanup();
						return;
					}
				}

				// *****
				std::cerr << "ClientHandler: Body Chunk Sent :" << std::endl;
				std::cerr << buf << std::endl << std::endl;
				// *****

				m_bodyOffset = 0; //sent total chunk, reset for next chunk
			}
			else
			{
				//EOF or error
				::close(m_fileFD);
				m_bodyToSend = false;
			}
		}
		if (m_response.getResponseBuffer().empty() && !m_bodyToSend)
		{
			std::cerr << "Response has been sent!" << std::endl;
			if (!m_request.getKeepAlive())
			{
				std::cerr << "ClientHandler: Close connection after sending response" << std::endl;
				cleanup();
			}
			else if (m_request.getLocationBlock() && m_request.getLocationBlock()->hasRedirect)
			{
				std::cerr << "ClientHandler: Close connection after redirection" << std::endl;
				cleanup();
			}
			else
			{
				std::cerr << "ClientHandler: Keep connection alive with the client" << std::endl;
				std::cerr << "ClientHandler: Clear for next request" << std::endl;
				//Clear for next request
				m_parser.clearBuffer();
				m_request.clearForNextRequest();
				m_response.setResponseBuffer("");
				m_upload.reset();
				m_status = PARSING_HEADER;
				m_headerOffset = 0;
				m_bodyOffset = 0;
				m_bodyToSend = false;
				m_fileFD = -1;
				m_poller.modify(m_fd.getfd(), POLLIN);
			}
		}
		
	}
}


void webserv::ClientHandler::cleanup()
{
	if (m_fd.isValid())
	{
		int fd = m_fd.getfd();
		std::cerr << "ClientHandler: Cleaning up fd " << fd << std::endl;
		m_poller.remove(fd);
	}
}

void webserv::ClientHandler::checkServerAndLocation()
{
	if (!m_request.getLocationBlock() && m_request.getServerBlock()->root.empty())
		throw HttpError(400); // Bad Request, no root set for server
	if (m_request.getServerBlock()->max_body_size)
		m_upload.setMaxBodySize(m_request.getServerBlock()->max_body_size);
		
	if (m_request.getLocationBlock() && m_request.getLocationBlock()->hasRedirect)
		m_status = REDIRECTION;
	else
	{
		if (m_request.getLocationBlock() && !m_request.getLocationBlock()->methods.empty())
		{
			std::vector<std::string>::const_iterator it;
			for (it = m_request.getLocationBlock()->methods.begin(); it != m_request.getLocationBlock()->methods.end(); ++it)
			{
				if (*it == m_request.getMethod())
					break;
			}
			if (it == m_request.getLocationBlock()->methods.end())
				throw HttpError(405); //Method not allowed
		}
		if (m_request.getMethod() == "POST") // POST need a locationBlock with an uploadStore set
		{
			if (!m_request.getLocationBlock() || m_request.getLocationBlock()->uploadStore.empty())
				throw HttpError(405); //Method not allowed
		}
	}
	std::cerr << "out of check server and location" << std::endl;
}

void webserv::ClientHandler::evaluateTransition(void)
{
	std::map<std::string, std::vector<std::string> > headers = m_request.getHeaders();
	bool CL = headers.count("content-length");
	bool TE = headers.count("transfer-encoding");
	bool EX = headers.count("expect");


	if (m_request.getMethod() == "GET" || m_request.getMethod() == "HEAD")
	{
		// GET must not have a body → so Content-Length or Transfer-Encoding or Expect are not allowed
		if (CL || TE || EX)
			throw HttpError(400);
		m_status = COMPLETE;
	}
	else if (m_request.getMethod() == "POST" || m_request.getMethod() == "DELETE")
	{
		if (!(CL ^ TE))
			throw HttpError(400);
		if (!(CL || TE)) // There is no body to read
		{
			if (EX)
				m_status = EXPECT_FAILED;
			else
				m_status = COMPLETE;
		}
		else
		{
			//There is body to read but Client expect a response before
			if (EX)
			{
				// Check if it is a valid Expect header
				if (headers["expect"].size() != 1 || headers["expect"][0] != "100-continue")
					throw HttpError(400);
				m_status = EXPECT_CONTINUE;
			}
			else //Client don't expect a response, body is ready to be read
				m_status = PARSING_BODY;
				
			if (TE)
				m_upload.setChunked(true);
		}
	}
	else
		throw HttpError(405); //Method not allowed

	// Check content-type in case of POST
	if (m_request.getMethod() == "POST")
	{
		if (!headers.count("content-type"))
			throw HttpError(415);
		findTypeOfUpload(headers["content-type"][0]);
	}

	//CGI preparation before processing
	if (m_request.getLocationBlock() && !m_request.getLocationBlock()->cgiPass.empty())
	{
		std::string scriptName;
		std::string pathInfo;

		if (m_request.getLocationBlock()->prefix[0] == '/')
		{
			computePath(); // compute the filesystem path based on the request path and location prefix
			std::string uri = m_request.getPath();

			std::string candidate;
			for (size_t i = 1; i <= uri.size(); ++i)
			{
				if (i < uri.size() && uri[i] != '/')
					continue;

				candidate = uri.substr(0, i);
				struct stat s;
				if (stat(candidate.c_str(), &s) == 0 && S_ISREG(s.st_mode))
				{
					scriptName = candidate;
				}
			}
			if (scriptName.empty())
				throw HttpError(404); // Not Found, no CGI script found
			pathInfo = uri.substr(scriptName.size());
		}
		else if (m_request.getLocationBlock()->prefix[0] == '.') // Regex
		{
			std::string uri = m_request.getPath();
			std::string extension = m_request.getLocationBlock()->cgiextension;
			size_t pos = uri.find(extension);
			if (pos == std::string::npos)
				throw HttpError(404); // Not Found, no CGI script found
			scriptName = uri.substr(0, pos + extension.size());
			pathInfo = uri.substr(pos + extension.size());
		}
		else
		{
			std::cerr << "ClientHandler: Location prefix is not valid" << std::endl;
			throw HttpError(400); // Bad Request, invalid location prefix
		}
		isCGI = true;
		m_request.setScriptName(scriptName); // scriptName is the CGI script path (URL)
		m_request.setPathInfo(pathInfo); // pathInfo is the PATH_INFO
	}
}


void webserv::ClientHandler::prepareResponse()
{
	std::cerr << "Starting to prepare response" << std::endl;
	if (m_request.getMethod() == "GET" || m_request.getMethod() == "HEAD")
	{
		computePath();
		struct stat st;
		if (stat(m_request.getPath().c_str(), &st) < 0)
		{
			if (errno == ENOENT) 
				m_response.buildErrorMessage(404); //Not Found
			else if (errno == EACCES) 
				m_response.buildErrorMessage(403); //Forbidden
			else 
				m_response.buildErrorMessage(500); //Server error
			return ;
		}
		else if (S_ISREG(st.st_mode))
		{
			std::cerr << "ClientHandler: Target is a regular file" << std::endl;
			m_fileFD = open(m_request.getPath().c_str(), O_RDONLY);
			if (m_fileFD < 0)
			{
				m_response.buildErrorMessage(403); //Forbidden
				return ;
			}
			m_response.buildGetResponse(m_request.getPath(), static_cast<size_t>(st.st_size));
			if (m_request.getMethod() == "GET")
				m_bodyToSend = true;
		}
		else if (S_ISDIR(st.st_mode))
		{
			std::cerr << "ClientHandler: Target is a directory" << std::endl;
			if (serveDefaultFile())
			{
				std::cerr << "There is a default file" << std::endl;
				m_fileFD = open(m_request.getPath().c_str(), O_RDONLY);
				if (m_fileFD < 0)
				{
					m_response.buildErrorMessage(403); //Forbidden
					return ;
				}
				struct stat st;
				stat(m_request.getPath().c_str(), &st);
				m_response.buildGetResponse(m_request.getPath().c_str(), static_cast<size_t>(st.st_size));
				if (m_request.getMethod() == "GET")
					m_bodyToSend = true;
			}
			else
			{
				std::cerr << "There is no default file" << std::endl;
				if (m_request.getLocationBlock() && m_request.getLocationBlock()->autoindex)
					generateAutoIndex();
				else
					m_response.buildErrorMessage(404);
				return ;
			}	
		}
		else
		{
			m_response.buildErrorMessage(403); //Forbidden
			return ;
		}

	}
	else if (m_request.getMethod() == "DELETE")
	{
		computePath();
		struct stat st;
		if (stat(m_request.getPath().c_str(), &st) < 0)
		{
			if (errno == ENOENT) 
				m_response.buildErrorMessage(404); //Not Found
			else if (errno == EACCES) 
				m_response.buildErrorMessage(403); //Forbidden
			else 
				m_response.buildErrorMessage(500); //Server error
			return ;
		}
		else if (!S_ISREG(st.st_mode)) // Only regular files can be deleted
		{
			m_response.buildErrorMessage(405); //Method not allowed
			return ;
		}
		else if (std::remove(m_request.getPath().c_str()) != 0)
		{
			m_response.buildErrorMessage(500); //Internal Server Error
			return ;
		}
		m_response.buildDeleteResponse();
	}
	else
		m_response.buildErrorMessage(405); //Method not allowed, POST is handled in onevent()
}

void webserv::ClientHandler::computePath()
{
	const webserv::Config::LocationConfig* locationBlock = m_request.getLocationBlock();
	if (locationBlock)
		std::cerr << "CH : Location Block is " << locationBlock->prefix << std::endl;
	else
		std::cerr << "CH : Location Block is empty" << std::endl;
	std::string filesystemPath;
	std::string reqPath = m_request.getPath();
	std::string remaining;

	if (locationBlock)
	{
		std::cerr << "Location Block is " << locationBlock->prefix << " root is " << (locationBlock->root.empty() ? true : false)  << " reqpath is " << reqPath << std::endl;
		std::string prefix = locationBlock->prefix;
		if (reqPath == prefix) // prefix exactly matches the request path (so we look for an index file under the loc alias/root)
			remaining = "/";
		else if (prefix == "/") // prefix matches everything, so remaining should be the full path
			remaining = reqPath;
		else // remaining = the substring _after_ the prefix, including the leading slash
			remaining = reqPath.substr(prefix.size());
	}
	else
		remaining = reqPath;
	
	if (locationBlock && !locationBlock->root.empty())
		filesystemPath = locationBlock->root + remaining;
	else if (locationBlock && !locationBlock->alias.empty())
		filesystemPath = locationBlock->alias + remaining;
	else
		filesystemPath = m_request.getServerBlock()->root + remaining;
		
	m_request.setPath(filesystemPath);
	std::cerr << "Path has been set" << std::endl;
	std::cerr << "Path is: " << filesystemPath << std::endl;
}

bool webserv::ClientHandler::serveDefaultFile()
{
	std::cerr << "Enter serveDefaultFile" << std::endl;
	const webserv::Config::LocationConfig* locationBlock = m_request.getLocationBlock();
	const webserv::Config::Server* serverBlock = m_request.getServerBlock();
	std::string path = m_request.getPath();

	if (path[path.size() - 1] != '/') // Ensure the dir_path end with '/'
    	path += '/';

	if (locationBlock) //check index file in location block
	{
		for (size_t i = 0; i < locationBlock->indexFiles.size(); ++i)
		{
			struct stat st;
			std::string newPath = path + locationBlock->indexFiles[i];
			if (stat(newPath.c_str(), &st) == 0)
			{
				m_request.setPath(newPath);
				return true;
			}
		}
	}
	for (size_t i = 0; i < serverBlock->index_files.size(); ++i) //check index file in server block
	{
		struct stat st;
		std::string newPath = path + serverBlock->index_files[i];
		if (stat(newPath.c_str(), &st) == 0)
		{
			m_request.setPath(newPath);
			return true;
		}
	}
	return false;
}

void webserv::ClientHandler::generateAutoIndex()
{
	std::cerr << "Enter generateAutoIndex" << std::endl;
	DIR* dirp = opendir(m_request.getPath().c_str());
	if (dirp == NULL)
	{
		m_response.buildErrorMessage(403);
		return ;
	}
	
	std::ostringstream autoIndex;
	std::ostringstream body;

	body << "<!DOCTYPE html>\n"
		<< "<html><head><title>Index of " << m_request.getPath() << "</title></head>\n"
		<< "<body>\n"
		<< "<h1>Index of " << m_request.getPath() << "</h1>\n"
		<< "<ul>\n";

	struct dirent* entry;
	while ((entry = readdir(dirp)) != NULL)
	{
		std::string name = entry->d_name;
		if (name == "." || name == "..")
			continue ;

		std::string href = m_request.getPath();
		if (href != "/" && href[href.size() - 1] != '/')
			href += '/';
		href += name;

		body << "<li><a href=\"" << href << "\">" << name << "</a></li>\n"; 
	}
	closedir(dirp);

	body << "</ul>\n"
		<< "<hr><address>Webserv</address>\n"
		<< "</body></html>";
	
	autoIndex << "HTTP/1.1 200 OK\r\n"
		<< "Content-Type: text/html; charset=utf-8\r\n"
		<< "Content-Length: " << body.str().size() << "\r\n\r\n"
		<< body.str();

	m_response.setResponseBuffer(autoIndex.str());
}

void webserv::ClientHandler::findUploadStore()
{
	std::string uploadStore = m_request.getLocationBlock()->uploadStore; //if arrive here, there is a location and upload store is not empty.
	// Create the directory path to store the uploaded file(s)
	if (uploadStore[0] != '/')
	{
		if (!m_request.getLocationBlock()->alias.empty())
		{
			if (m_request.getLocationBlock()->alias[m_request.getLocationBlock()->alias.size() - 1] != '/')
				uploadStore = m_request.getLocationBlock()->alias + "/" + uploadStore;
			else
				uploadStore = m_request.getLocationBlock()->alias + uploadStore;
		}
		else if (!m_request.getLocationBlock()->root.empty())
		{
			if (m_request.getLocationBlock()->root[m_request.getLocationBlock()->root.size() - 1] != '/')
				uploadStore = m_request.getLocationBlock()->root + "/" + uploadStore;
			else
				uploadStore = m_request.getLocationBlock()->root + uploadStore;
		}
		else
		{
			if (m_request.getServerBlock()->root[m_request.getServerBlock()->root.size() - 1] != '/')
				uploadStore = m_request.getServerBlock()->root + "/" + uploadStore;
			else
				uploadStore = m_request.getServerBlock()->root + uploadStore;
		}
	}

	//Check directory exists and is writable
	struct stat st;
	if (stat(uploadStore.c_str(), &st) < 0 || !S_ISDIR(st.st_mode) || access(uploadStore.c_str(), W_OK) < 0) // The upload directory should exist when the server is running
		throw HttpError(500);
	m_upload.setUploadStore(uploadStore);
}

void webserv::ClientHandler::findTypeOfUpload(std::string &value)
{
	std::string boundary;
	
	if (value.empty())
		throw HttpError(415); //Unsupported Media Type
	else if (value == "application/octet-stream")
	{
		m_upload.setRaw(true);
		return ;
	}
	if (value.compare(0, 19, "multipart/form-data") == 0)
	{
		size_t pos = 19; // Length of "multipart/form-data"
		while (value[pos] == ' ' || value[pos] == '\t') // Skip spaces and tabs
			++pos;
		if (value[pos] != ';')
			throw HttpError(415); //Unsupported Media Type
		++pos; // Skip the semicolon
		while (value[pos] == ' ' || value[pos] == '\t') // Skip spaces and tabs
			++pos;
		if (value.compare(pos, 8, "boundary") != 0)
			throw HttpError(415); //Unsupported Media Type
		pos += 8; // Skip "boundary"
		while (value[pos] == ' ' || value[pos] == '\t') // Skip spaces and tabs
			++pos;
		if (value[pos] != '=')
			throw HttpError(415); //Unsupported Media Type
		++pos; // Skip the equal sign
		while (value[pos] == ' ' || value[pos] == '\t') // Skip spaces and tabs
			++pos;
		size_t endPos = value.find_first_of(" \t\r\n", pos);
		if (endPos == std::string::npos)
			endPos = value.size();
		boundary = value.substr(pos, endPos - pos);
		if (boundary.empty())
			throw HttpError(415); // Unsupported Media Type
		m_upload.setMultipart(true);
		m_upload.setBoundary(boundary);
		return ;
	}
	else
		throw HttpError(415); // Unsupported Media Type
}
