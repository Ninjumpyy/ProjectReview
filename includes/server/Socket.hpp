/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Socket.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rpandipe <rpandie@student.42luxembourg.    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/04/19 04:05:04 by rpandipe          #+#    #+#             */
/*   Updated: 2025/06/17 02:47:36 by rpandipe         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef __SOCKET_HPP__
# define __SOCKET_HPP__

#include <string>
#include <sys/socket.h>
#include <netinet/in.h>
#include <exception>
#include "utils/Fd.hpp"
#include "server/Poller.hpp"
#include "server/AcceptHandler.hpp"
#include "config/ParseConfig.hpp"

namespace webserv 
{
	class Socket
	{
		public:
			Socket(const char *host, int port, std::vector<const webserv::Config::Server*> config);
			~Socket();

			void listen(int backlog = 128);
			void run();
			int getSock() const;
			
			class SocketNotCreatedException: public std::exception
			{
				public:
					const char* what() const throw();
			};

		private:
			webserv::Fd m_sock;
			std::vector<const webserv::Config::Server*> m_config;
			struct sockaddr_in m_address;
			webserv::AcceptHandler *m_acceptor;

			Socket();
			Socket(const Socket &);
			Socket& operator=(const Socket &);
	};
}

#endif