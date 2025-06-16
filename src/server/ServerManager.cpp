/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ServerManager.cpp                                  :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rpandipe <rpandie@student.42luxembourg.    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/05/28 11:35:26 by rpandipe          #+#    #+#             */
/*   Updated: 2025/06/14 08:55:16 by rpandipe         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "server/ServerManager.hpp"
#include "server/Socket.hpp"
#include "server/Parser.hpp"
#include <set>
#include <utility>
#include <signal.h>

webserv::ServerManager::ServerManager() 
{
	::signal(SIGINT,  &ServerManager::handleSignal);
    ::signal(SIGTERM, &ServerManager::handleSignal);
}

webserv::ServerManager::~ServerManager()
{
	for (size_t i = 0; i < m_sockets.size(); i++)
		delete m_sockets[i];
}

void webserv::ServerManager::handleSignal(int signum)
{
    std::cerr << "Received signal " << signum << ", shutting down server...\n";
    webserv::Poller::getInstance().stop();
	std::cerr << "Server shutdown successful." << std::endl;
}

void webserv::ServerManager::setupsockets(const std::vector<webserv::Config::Server> &server)
{
	std::set<std::pair<std::string, int> > unique_sockets;
	// Create unique hostname-port instance
	for (std::vector<webserv::Config::Server>::const_iterator it = server.begin(); it != server.end(); it++)	
		unique_sockets.insert(std::make_pair(it->hostname, it->port));
	
	// Create a new socket, giving access to all the server block corresponding to this hostname-port
	for (std::set<std::pair<std::string, int> >::iterator it = unique_sockets.begin(); it != unique_sockets.end(); it++)
	{
		std::vector<const webserv::Config::Server*> related;
		for (std::vector<webserv::Config::Server>::const_iterator sit = server.begin(); sit != server.end(); ++sit)
		{
			if (sit->hostname == it->first && sit->port == it->second)
			{
				related.push_back(&(*sit));
				std::cerr << "Found the related host and port " << sit->hostname << "  " << sit->port << sit->name.size() << std::endl;
			}
		}
		webserv::Socket *socket = new webserv::Socket(it->first.c_str(), it->second, related);
		socket->run();
		m_sockets.push_back(socket);
	}
}

void webserv::ServerManager::startserver()
{
	webserv::Poller::getInstance().run();
}