/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   CgiHandler.cpp                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rpandipe <rpandipe.student.42luxembourg    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/06/03 04:18:17 by rpandipe          #+#    #+#             */
/*   Updated: 2025/06/04 16:44:25 by rpandipe         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "server/CgiHandler.hpp"
#include "server/CgiProcess.hpp"
#include <signal.h>
#include <sys/wait.h>

webserv::CgiHandler::CgiHandler(const Fd &listenfd, CgiProcess &cgi, bool owns_process)
	: m_poller(webserv::Poller::getInstance()), m_cgi(cgi), m_listenfd(listenfd), m_owns_process(owns_process)
{
	if (owns_process)
		m_poller.add(m_listenfd.getfd(), POLLIN, this);
	else
		m_poller.add(m_listenfd.getfd(), POLLOUT, this);
}

webserv::CgiHandler::~CgiHandler()
{
	if (m_owns_process)
		delete &m_cgi;
}

void webserv::CgiHandler::onEvent(short revents)
{
	if (!m_owns_process && (revents & POLLOUT))
	{
		m_cgi.sendRequestBody();
		if (m_cgi.getsendingstatus())
			m_poller.remove(m_listenfd.getfd());
	}
	else if (m_owns_process && (revents & POLLIN))
	{
		m_cgi.readResponse();
		if (m_cgi.getreadingstatus())
			m_poller.remove(m_listenfd.getfd());
	}
	else if (revents & (POLLNVAL | POLLERR | POLLHUP))
	{
		m_cgi.builderrorresponse();
		::kill(m_cgi.getPid(), SIGKILL);
        int status;
        ::waitpid(m_cgi.getPid(), &status, 0);
		m_poller.remove(m_listenfd.getfd());
	}
}
