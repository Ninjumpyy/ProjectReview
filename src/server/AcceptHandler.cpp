/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   AcceptHandler.cpp                                  :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rpandipe <rpandie@student.42luxembourg.    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/04/22 16:22:13 by rpandipe          #+#    #+#             */
/*   Updated: 2025/06/17 02:29:48 by rpandipe         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "server/AcceptHandler.hpp"
#include "server/ClientHandler.hpp"
#include <netdb.h>
#include <errno.h>
#include <fcntl.h>
#include <iostream>

webserv::AcceptHandler::AcceptHandler(Fd &listenfd, Poller &poller, std::vector<const webserv::Config::Server*> &config): m_fd(listenfd), m_poller(poller), m_config(config)
{
	m_poller.add(m_fd.getfd(), POLLIN, this);
	std::cerr << "Accept handler Details " << m_config[0]->name.size() << std::endl;
}

webserv::AcceptHandler::~AcceptHandler()
{
	std::cerr << "AcceptHandler: Destructor called" << std::endl;
	//m_poller.remove(m_fd.getfd());
}

void webserv::AcceptHandler::onEvent(short revents)
{
	if (revents & (POLLNVAL | POLLERR | POLLHUP))
		throw(webserv::AcceptHandler::EventErrorException());
	if (revents & POLLIN)
	{
		std::cerr << "Accept handler: Ready to accept new connection" << std::endl;
		while (true)
		{
			int client_socket = ::accept(m_fd.getfd(), NULL, NULL);
			if (client_socket < 0)
			{
				if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    break;
                }
				std::cerr << "Accept handler: Error accepting connection: " << std::endl;
				throw(webserv::AcceptHandler::EventErrorException());
			}
			std::cerr << "Accept handler: Accepted new connection on socket " << client_socket << std::endl;

			Fd clientfd(client_socket);
			int flag = fcntl(client_socket, F_GETFL, 0);
			if (fcntl(client_socket, F_SETFL, flag | O_NONBLOCK) < 0)
				throw(webserv::AcceptHandler::EventErrorException());
			
			std::cerr << "CLientfd is " << clientfd.getfd();
			
			webserv::IEventHandler *handler = new webserv::ClientHandler(clientfd, m_poller, m_config);
			m_poller.add(clientfd.getfd(), POLLIN,  handler);
		}
	}
}
const char * webserv::AcceptHandler::EventErrorException::what() const throw()
{
	return ("Poll Failed");
}
