/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Poller.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rpandipe <rpandipe.student.42luxembourg    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/04/22 14:59:59 by rpandipe          #+#    #+#             */
/*   Updated: 2025/05/28 12:53:39 by rpandipe         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef __POLLER_HPP__
# define __POLLER_HPP__

#include "server/IEventHandler.hpp"
#include "vector"
#include <poll.h>
#include <exception>

namespace webserv
{
class Poller
{
	public:
		Poller();
		~Poller();
		
		static Poller& getInstance();
		void add(int fd, short events, IEventHandler* h);
		void modify(int fd, short newevent);
		void remove(int fd);
		void run();

		class FdErrorException: public std::exception
		{
			public:
				const char* what() const throw();
		};
		
	private:
		std::vector<pollfd> m_pollfd;
		std::vector<IEventHandler *> m_handlers;
		bool m_running;
		
		int findIndex(int fd) const;
};
}

#endif