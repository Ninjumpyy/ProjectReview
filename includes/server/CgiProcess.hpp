/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   CgiProcess.hpp                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rpandipe <rpandipe.student.42luxembourg    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/06/02 13:47:38 by rpandipe          #+#    #+#             */
/*   Updated: 2025/06/04 16:42:04 by rpandipe         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef __CGIPROCESS_HPP__
#define __CGIPROCESS_HPP__

#include <string>
#include <vector>
#include "server/Response.hpp"
#include "server/Poller.hpp"
#include "utils/Fd.hpp"
#include "server/CgiHandler.hpp"
#include <unistd.h>
#include <sstream>

namespace webserv
{
	class Request;
	
	class CgiProcess
	{
		public:
			struct CgiMapping {
				std::string extension;
				std::string interpreter;
			};

			CgiProcess(const CgiMapping&  mapping, const std::string& scriptpath, const webserv::Request& request, webserv::Response& response);
			~CgiProcess();
			void spawn();
			void sendRequestBody();
			void readResponse();
			bool getsendingstatus() const;
			bool getreadingstatus() const;
			pid_t getPid() const;
			void builderrorresponse();

		private:
			const CgiMapping &m_cgimap;
			const std::string &m_script;
			const webserv::Request &m_request;
			webserv::Response &m_response;
			webserv::Poller &m_poller;
			pid_t m_pid;
			webserv::Fd m_cgi_inread;
			webserv::Fd m_cgi_inwrite;
			webserv::Fd m_cgi_outread;
			webserv::Fd m_cgi_outwrite;
			size_t m_sentbody;
			bool m_finishedsending;
			bool m_finishedreading;
			std::string m_cgi_output;

			CgiProcess();
			CgiProcess(const CgiProcess&);
			CgiProcess& operator=(const CgiProcess&);
	};
} 

#endif