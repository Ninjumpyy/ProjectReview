/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Socket.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rpandipe <rpandie@student.42luxembourg.    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/04/19 04:04:58 by rpandipe          #+#    #+#             */
/*   Updated: 2025/06/17 02:47:45 by rpandipe         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "server/Socket.hpp"
#include <netdb.h>
#include <cstring>
#include <unistd.h>
#include <fcntl.h>
#include <iostream>

webserv::Socket::Socket(const char *host, int port, std::vector<const webserv::Config::Server*> config):
	m_sock(::socket(AF_INET, SOCK_STREAM, 0)), m_config(config)
{	
	std::cerr << "Socket: Creating socket" << std::endl;
	std::cerr << "Server Details " << config[0]->name.size() << std::endl;
	if (!m_sock.isValid())
		throw(webserv::Socket::SocketNotCreatedException());
	int flag = fcntl(m_sock.getfd(), F_GETFL, 0);
	if (fcntl(m_sock.getfd(), F_SETFL, flag | O_NONBLOCK) < 0)
		throw(webserv::Socket::SocketNotCreatedException());
	int optval = 1;
	if(::setsockopt(m_sock.getfd(), SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) < 0)
		throw(webserv::Socket::SocketNotCreatedException());
	std::memset(&m_address, 0, sizeof(m_address));
	if (std::strcmp(host, "*") == 0)
	{
		m_address.sin_family = AF_INET;
		m_address.sin_addr.s_addr= INADDR_ANY;
	}
	else
	{
		struct addrinfo hints; 
		struct addrinfo	*res = NULL;
		std::memset(&hints, 0, sizeof(hints));
		hints.ai_family = AF_INET;
		hints.ai_socktype = SOCK_STREAM;
		if(::getaddrinfo(host, NULL, &hints, &res) !=0)
			throw(webserv::Socket::SocketNotCreatedException());
		m_address = *reinterpret_cast<sockaddr_in *>(res->ai_addr);
		::freeaddrinfo(res);
	}
	m_address.sin_port = htons(port);
	std::cerr << "Socket: Binding to " << host << std::endl;
	if(::bind(m_sock.getfd(), reinterpret_cast<const sockaddr*>(&m_address), sizeof(m_address)) < 0 )
			throw(webserv::Socket::SocketNotCreatedException());
	std::cerr << "Socket: Successfully bound to port " << port << std::endl;
	m_acceptor = new webserv::AcceptHandler(m_sock, webserv::Poller::getInstance(), m_config);
}

webserv::Socket::~Socket()
{
	std::cerr << "Socket: Destructor called" << std::endl;
}

void webserv::Socket::listen(int backlog)
{
	std::cerr << "Socket: Starting to listen with backlog " << backlog << std::endl;
	if (::listen(m_sock.getfd(),backlog) < 0)
		throw(webserv::Socket::SocketNotCreatedException());
	std::cerr << "Socket: Successfully listening" << std::endl;
}

void webserv::Socket::run()
{
	try {
		this->listen();
	} catch (const std::exception& e) {
		std::cerr << "Socket: Error in run: " << e.what() << std::endl;
	}
}

int webserv::Socket::getSock() const
{
	return (m_sock.getfd());
}

const char* webserv::Socket::SocketNotCreatedException::what() const throw()
{
	return ("Failed to create the socket.");
}