/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   AcceptHandler.hpp                                  :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rpandipe <rpandipe.student.42luxembourg    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/04/22 16:11:42 by rpandipe          #+#    #+#             */
/*   Updated: 2025/05/28 17:42:36 by rpandipe         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef __ACCEPTHANDLER_HPP__
#define __ACCEPTHANDLER_HPP__

#include "server/IEventHandler.hpp"
#include "server/Poller.hpp"
#include "utils/Fd.hpp"
#include "config/ParseConfig.hpp"
#include <exception>

namespace webserv
{
class ClientHandler;

class AcceptHandler: public IEventHandler
{
	public:
		AcceptHandler(Fd &listenfd, Poller &poller, std::vector<const webserv::Config::Server*> &config);
		virtual ~AcceptHandler();
		
		virtual void onEvent(short revents);
		class EventErrorException: public std::exception
		{
			public:
				const char* what() const throw();
		};

	private:
		Fd &m_fd;
		Poller &m_poller;
		std::vector<const webserv::Config::Server*> &m_config;
		AcceptHandler();
		AcceptHandler(AcceptHandler &);
		AcceptHandler operator=(AcceptHandler &);
};
}

#endif