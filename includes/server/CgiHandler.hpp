/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   CgiHandler.hpp                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rpandipe <rpandipe.student.42luxembourg    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/06/03 03:41:43 by rpandipe          #+#    #+#             */
/*   Updated: 2025/06/04 16:44:19 by rpandipe         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef __CGIHANDLER_HPP__
#define __CGIHANDLER_HPP__
#include "server/IEventHandler.hpp"
#include "server/Poller.hpp"

#include "utils/Fd.hpp"

namespace webserv
{
	class CgiProcess;
	
	class CgiHandler : public IEventHandler
	{
		public:
			CgiHandler(const Fd &listenfd, webserv::CgiProcess &cgi, bool owns_process);
			virtual ~CgiHandler();


			virtual void onEvent(short revents);

		private:
			Poller& m_poller;
			CgiProcess &m_cgi;
			Fd m_listenfd;
			bool m_owns_process;
			
			CgiHandler();
			CgiHandler(CgiHandler &);
			CgiHandler operator=(CgiHandler &);
	};
};

#endif