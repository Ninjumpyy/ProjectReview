/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Fd.cpp                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rpandipe <rpandie@student.42luxembourg.    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/04/22 14:22:02 by rpandipe          #+#    #+#             */
/*   Updated: 2025/04/24 07:28:04 by rpandipe         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "utils/Fd.hpp"
#include <unistd.h>
#include <iostream>

webserv::Fd::Fd(int fd): m_fd(fd), m_refCount(new int(1))
{
	std::cerr << "Fd: Created with fd " << fd << " (refCount: " << *m_refCount << ")" << std::endl;
}

webserv::Fd::Fd(const Fd& other): m_fd(other.m_fd), m_refCount(other.m_refCount)
{
	if (m_refCount)
	{
		++(*m_refCount);
		std::cerr << "Fd: Copied fd " << m_fd << " (refCount: " << *m_refCount << ")" << std::endl;
	}
}

webserv::Fd& webserv::Fd::operator=(const Fd& other)
{
	if (this != &other)
	{
		release();
		m_fd = other.m_fd;
		m_refCount = other.m_refCount;
		if (m_refCount)
		{
			++(*m_refCount);
			std::cerr << "Fd: Assigned fd " << m_fd << " (refCount: " << *m_refCount << ")" << std::endl;
		}
	}
	return *this;
}

webserv::Fd::~Fd()
{
	release();
}

int webserv::Fd::getfd() const
{
	return (m_fd);
}

bool webserv::Fd::isValid() const
{
	return (m_fd >= 0);
}

void webserv::Fd::release()
{
	if (m_refCount)
	{
		std::cerr << "Fd: Releasing fd " << m_fd << " (refCount: " << *m_refCount << ")" << std::endl;
		if (--(*m_refCount) == 0)
		{
			if (m_fd >= 0)
			{
				::close(m_fd);
				std::cerr << "Fd: Closed fd " << m_fd << std::endl;
			}
			delete m_refCount;
			m_refCount = NULL;
		}
	}
}