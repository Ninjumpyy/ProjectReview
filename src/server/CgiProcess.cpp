/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   CgiProcess.cpp                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: tle-moel <tle-moel@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/06/02 14:10:02 by rpandipe          #+#    #+#             */
/*   Updated: 2025/06/16 16:23:18 by tle-moel         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "server/CgiProcess.hpp"
#include "server/Request.hpp"
#include <sys/wait.h>

webserv::CgiProcess::CgiProcess(const CgiMapping&  mapping, const std::string& scriptpath, const webserv::Request& request, webserv::Response& response)
: m_cgimap(mapping), m_script(scriptpath), m_request(request), m_response(response), m_poller(Poller::getInstance()), m_pid(-1), m_cgi_inread(-1), m_cgi_inwrite(-1), 
	m_cgi_outread(-1), m_cgi_outwrite(-1), m_sentbody(0), m_finishedsending(false), m_finishedreading(false)
{}

webserv::CgiProcess::~CgiProcess() {}

void webserv::CgiProcess::spawn()
{
	int fds1[2];
	int fds2[2];

	if (pipe(fds1) < 0 || pipe(fds2))
	{
		m_response.buildErrorMessage(500);
		::exit(1) ;
	}
		
	m_cgi_inread = webserv::Fd(fds1[0]);
	m_cgi_inwrite = webserv::Fd(fds1[1]);
	m_cgi_outread = webserv::Fd(fds2[0]);
	m_cgi_outwrite = webserv::Fd(fds2[1]);
	
	m_pid = fork();
	if (m_pid < 0)
	{
		m_response.buildErrorMessage(500);
		::exit(1) ;
	}
	if (m_pid == 0)
	{
		m_cgi_inwrite = webserv::Fd(-1);
		m_cgi_outread = webserv::Fd(-1);
		
		::dup2(m_cgi_inread.getfd(), STDIN_FILENO);
		::dup2(m_cgi_outwrite.getfd(), STDOUT_FILENO);
		m_cgi_inread = webserv::Fd(-1);
		m_cgi_outwrite = webserv::Fd(-1);
		std::vector<std::string> envstr;
		
		std::string method = m_request.getMethod();
	
		envstr.push_back("REQUEST_METHOD=" + method);
		envstr.push_back("QUERY_STRING=" + m_request.getQuery());
		std::ostringstream oss;
		oss << m_request.getExpectedLength();
		envstr.push_back("CONTENT_LENGTH=" + oss.str());
		if (!m_request.getPathInfo().empty())
			envstr.push_back("PATH_INFO=" + m_request.getPathInfo());
		
		const std::map<std::string, std::vector<std::string> > header = m_request.getHeaders();
		std::map<std::string, std::vector<std::string> >::const_iterator it = header.begin();
		for (; it != header.end(); it++)
		{
			const std::string& name   = it->first;
   		 	const std::vector<std::string>& values = it->second;

			if (values.empty())
        		continue;
			
			if (name == "Content-Length" || name == "host")
				continue;

			std::string hname = "HTTP_";
			for (size_t i = 0; i < name.size(); ++i)
			{
				char c = name[i];
				if (c == '-')
					hname.push_back('_');
				else
					hname.push_back(static_cast<char>(std::toupper(c)));
			}
			std::string merged_values;
			for (size_t i = 0; i < values.size(); i++)
			{
				if (i > 0) merged_values += ",";
				merged_values += values[i];
			}
			envstr.push_back(hname + "=" + merged_values);
		}
		std::vector<char*> envp;
		for (size_t i = 0; i < envstr.size(); ++i)
        	envp.push_back(const_cast<char*>(envstr[i].c_str()));

		envp.push_back(NULL);

		std::vector<char *> argv;
		argv.push_back(const_cast<char*>(m_cgimap.interpreter.c_str()));
		argv.push_back(const_cast<char*>(m_script.c_str()));
		argv.push_back(NULL);

		std::string script_dir;
		for(size_t i = m_script.size(); i > 0; i--)
		{
			if (m_script[i] == '/')
			{
				script_dir = m_script.substr(0, i);
				break;
			}
		}
		if (script_dir.empty())
			script_dir = ".";
		
		if (::chdir(script_dir.c_str()) != 0)
		{
			m_response.buildErrorMessage(500);
			::exit(1);
		}

		::execve(m_cgimap.interpreter.c_str(), argv.data(), envp.data());
		m_response.buildErrorMessage(500);
		::exit(1);
	}
	m_cgi_inread = webserv::Fd(-1);
	m_cgi_outwrite = webserv::Fd(-1);
	int flag1 = fcntl(m_cgi_inwrite.getfd(), F_GETFL, 0);
	int flag2 = fcntl(m_cgi_outread.getfd(), F_GETFL, 0);
	if (fcntl(m_cgi_inwrite.getfd(), F_SETFL, flag1 | O_NONBLOCK) < 0 
		|| fcntl(m_cgi_outread.getfd(), F_SETFL, flag2 | O_NONBLOCK) < 0)
	{
		m_response.buildErrorMessage(500);
		return ; // need a way to delete the CgiProcess object
	}

	new CgiHandler(m_cgi_inwrite, *this, false);
	new CgiHandler(m_cgi_outread, *this, true);
}

void webserv::CgiProcess::sendRequestBody()
{
	if (m_finishedsending || !m_cgi_inwrite.isValid())
		return;
	
	const std::string& body = m_request.getBody();
	size_t total = body.size();
	if (m_sentbody >= total)
	{
		m_finishedsending = true;
		m_cgi_inwrite = webserv::Fd(-1);
		return;
	}
	const char* data = body.c_str() + m_sentbody;
	size_t remaining = total - m_sentbody;
	size_t bytes_written = ::write(m_cgi_inwrite.getfd(), data, remaining);
	if (bytes_written > 0)
	{
		m_sentbody += bytes_written;
		if (m_sentbody >= total)
			m_finishedsending = true;
	}
	else if (bytes_written == 0)
		m_finishedsending = true;
	else
	{
		m_response.buildErrorMessage(500);
		m_finishedsending = true;	
	}
	if (m_finishedsending)
		m_cgi_inwrite = webserv::Fd(-1);
}

void webserv::CgiProcess::readResponse()
{

	if (m_finishedreading || !m_cgi_outread.isValid())
		return;
	
	char buffer[4096];
	ssize_t bytes_read = ::read(m_cgi_outread.getfd(), buffer, sizeof(buffer));
	if (bytes_read < 0)
	{
		m_response.buildErrorMessage(500);
		return;
	}
	else if (bytes_read > 0)
	{
		m_cgi_output.append(buffer, bytes_read);
		return;
	}
	else if (bytes_read == 0)
	{
		m_finishedreading = true;
		int status;
		::waitpid(m_pid, &status, 0);
		size_t sep = m_cgi_output.find("\r\n\r\n");
		std::string headers;
		std::string body;
		if (sep != std::string::npos)
		{
			headers = m_cgi_output.substr(0, sep);
			body = m_cgi_output.substr(sep + 4);
		}
		else
			body = m_cgi_output;

		std::istringstream header_stream(headers);
		std::string line;
		bool saw_status_line = false;
		std::string status_line = "HTTP/1.1 200 OK\r\n";
		while (std::getline(header_stream, line))
		{
			if (!line.empty() && line[line.size() - 1] == '\r')
				line.erase(line.size() - 1);

			if (line.rfind("Status: ", 0) == 0)
			{
				std::string rest = line.substr(7);
				if (!rest.empty() && rest[0] == ' ')
					rest.erase(rest.begin());
				std::istringstream ss(rest);
				int code;
				ss >> code;
				std::string reason;
				std::getline(ss, reason);
				if (!reason.empty() && reason[0] == ' ')
					reason.erase(reason.begin());
				m_response.setStatusLine(code, reason);
				saw_status_line = true;
			}
			else if (!line.empty())
			{
				std::string::size_type pos = line.find(':');
				if (pos != std::string::npos)
				{
					std::string name = line.substr(0, pos);
					std::string value = line.substr(pos + 1);
					if (!value.empty() && value[0] == ' ')
						value.erase(value.begin());
					m_response.addHeader(name, value);
				}
			}
		}
		if (!saw_status_line)
			m_response.setStatusLine(200, "OK");
		if (!m_request.getKeepAlive())
        	m_response.addHeader("Connection", "close");
		m_response.appendCRLF();
		if (!body.empty())
			m_response.appendBody(body);
		m_finishedreading = true;
	}
}

bool webserv::CgiProcess::getsendingstatus() const
{
	return m_finishedsending;
}

bool webserv::CgiProcess::getreadingstatus() const
{
	return m_finishedreading;
}

pid_t webserv::CgiProcess::getPid() const
{
	return m_pid;
}

void webserv::CgiProcess::builderrorresponse()
{
	m_response.buildErrorMessage(500);
	m_finishedsending = true;
	m_finishedreading = true;
	m_cgi_inwrite = webserv::Fd(-1);
	m_cgi_outread = webserv::Fd(-1);
}