/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Poller.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rpandipe <rpandie@student.42luxembourg.    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/04/22 15:12:40 by rpandipe          #+#    #+#             */
/*   Updated: 2025/06/17 02:29:20 by rpandipe         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "server/Poller.hpp"
#include <algorithm>
#include <iostream>
#include <cerrno>
#include <ctime>
#include <map>

webserv::Poller::Poller():m_running(true) {}

webserv::Poller::~Poller() 
{
	std::cerr << "Poller: Destructor called" << std::endl;
	for (size_t i = 0; i < m_handlers.size(); ++i)
	{
		if (m_handlers[i])
		{
			delete m_handlers[i];
			m_handlers[i] = NULL;
		}
	}
	m_handlers.clear();
	m_pollfd.clear();
}

webserv::Poller& webserv::Poller::getInstance()
{
	static Poller instance;
	return instance;
}

void webserv::Poller::add(int fd, short events, IEventHandler* h)
{	
	std::cerr << "Poller: Adding fd " << fd << " with events " << events << std::endl;
	int index = findIndex(fd);
	if (index == -1)
	{
		pollfd fds;
		fds.events = events;
		fds.fd = fd;
		fds.revents = 0;
		m_pollfd.push_back(fds);
		m_handlers.push_back(h);
		std::cerr << "Poller: Successfully added fd " << fd << std::endl;
	}
	else
		throw (webserv::Poller::FdErrorException());
}

void webserv::Poller::modify(int fd, short newevent)
{
	std::cerr << "Poller: Modifying fd " << fd << " with new events " << newevent << std::endl;
	int index = findIndex(fd);
	if (index == -1)
		throw (webserv::Poller::FdErrorException());
	m_pollfd[index].events = newevent;
	std::cerr << "Poller: Successfully modified fd " << fd << std::endl;
}

void webserv::Poller::remove(int fd)
{
	int index = findIndex(fd);
	if (index != -1)
	{
		IEventHandler *handler = m_handlers[index];
		// I think we can remove this line
		//m_handlers[index] = NULL;
		m_pollfd.erase(m_pollfd.begin() + index);
		m_handlers.erase(m_handlers. begin() + index);
		if (handler)
			delete (handler);
		std::cerr << "Poller: Removed fd " << fd << std::endl;
	}
}

int webserv::Poller::findIndex(int fd) const
{
	for(size_t i = 0; i < m_pollfd.size(); i++)
	{
		if (m_pollfd[i].fd == fd)
			return (i);
	}
	return (-1);
}

const char* webserv::Poller::FdErrorException::what() const throw()
{
	return ("Fatal Error in Poller");
}

void webserv::Poller::run()
{
	static const int  POLL_INTERVAL_MS  = 500;
    static const int  REQUEST_TIMEOUT_S = 30; 
	std::cerr << "Poller: Starting event loop" << std::endl;
	std::map<int, time_t> last_active;
	while (m_running)
	{
		time_t now = ::time(NULL);
		if (m_pollfd.empty())  
		{
			std::cerr << "Poller: No fds to poll" << std::endl;
			::poll(NULL, 0, POLL_INTERVAL_MS);
		}
		else
		{
			std::cerr << "Poller: Polling " << m_pollfd.size() << " fds" << std::endl;
			int ready = ::poll(&m_pollfd[0], m_pollfd.size(), POLL_INTERVAL_MS);
			if (ready < 0)
			{
				if (errno == EINTR)
				{
					std::cerr << "Poller: Poll interrupted by signal" << std::endl;
					break;
				}
				std::cerr << "Poll Failed" << std::endl;
				break;
			}
			for (size_t i = 0; i < m_pollfd.size(); i++)
			{
				struct pollfd &pfd = m_pollfd[i];
				short re = pfd.revents;
				if (re == 0)
					continue;
				last_active[pfd.fd] = now;
				std::cerr << "Poller: Event on fd " << pfd.fd << " with revents " << re << std::endl;
				pfd.revents = 0;
				IEventHandler* h = m_handlers[i];
				if (h)
				{
					try {
						h->onEvent(re);
					} catch (const std::exception& e) {
						std::cerr << "Poller: Error handling event: " << e.what() << std::endl;
						remove(pfd.fd);
						last_active.erase(pfd.fd);
					}
				}
			}
		}
		for (std::map<int, time_t>::iterator it = last_active.begin(); it != last_active.end(); /* no increment */)
		{
			if (now - it->second > REQUEST_TIMEOUT_S)
			{
				int fd = it->first;
				std::cerr << "Poller: Timing out fd " << fd << std::endl;
				remove(fd);
				last_active.erase(it++);
			}
			else
				++it;
		}
	}
	std::cerr << "Poller: Event loop ended" << std::endl;
}

void webserv::Poller::stop()
{
	std::cerr << "Poller: Stopping event loop" << std::endl;
	m_running = false;
}