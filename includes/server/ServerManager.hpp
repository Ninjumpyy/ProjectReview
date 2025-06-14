/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ServerManager.hpp                                  :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rpandipe <rpandipe.student.42luxembourg    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/05/28 11:26:25 by rpandipe          #+#    #+#             */
/*   Updated: 2025/05/28 17:44:05 by rpandipe         ###   ########.fr       */
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
};
}
#endif