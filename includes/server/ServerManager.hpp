/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ServerManager.hpp                                  :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rpandipe <rpandie@student.42luxembourg.    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/05/28 11:26:25 by rpandipe          #+#    #+#             */
/*   Updated: 2025/06/14 08:52:21 by rpandipe         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef __SERVERMANAGER_HPP__
#define __SERVERMANAGER_HPP__

#include "server/Socket.hpp"
#include "config/ParseConfig.hpp"
#include <vector>

namespace webserv
{
class ServerManager
{
	public:
		ServerManager();
		~ServerManager();
		void setupsockets(const std::vector<webserv::Config::Server> &server);
		void startserver();
		
	private:
		std::vector<webserv::Socket*> m_sockets;

		ServerManager(const ServerManager &other);
		ServerManager& operator=(const ServerManager &other);
		static void handleSignal(int signum);
};
}
#endif