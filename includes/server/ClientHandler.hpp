/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ClientHandler.hpp                                  :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: tle-moel <tle-moel@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/04/22 17:10:43 by rpandipe          #+#    #+#             */
/*   Updated: 2025/06/17 15:21:26 by tle-moel         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef __CLIENTHANDLER_HPP__
# define __CLIENTHANDLER_HPP__

#include <cstdio>
#include <string>
#include <sys/socket.h>
#include <sys/stat.h>
#include <errno.h>
#include <fcntl.h>
#include <dirent.h>
#include "server/IEventHandler.hpp"
#include "utils/Fd.hpp"
#include "server/Poller.hpp"
#include "config/ParseConfig.hpp"
#include "server/Request.hpp"
#include "server/Parser.hpp"
#include "server/Response.hpp"
#include "server/Upload.hpp"
#include "server/CgiProcess.hpp"
#include "utils/HttpError.hpp"
#include "server/SessionManager.hpp"

namespace webserv
{
	class ClientHandler: public IEventHandler
	{
		public:
			enum Status {PARSING_HEADER, PARSING_BODY, REDIRECTION, EXPECT_CONTINUE, EXPECT_FAILED, CGI, COMPLETE};

			ClientHandler(const Fd &listenfd, Poller &poller, std::vector<const webserv::Config::Server*> &config);
			virtual ~ClientHandler();

			virtual void onEvent(short revents);
		
		private:
			Fd m_fd;
			Poller &m_poller;
			Upload m_upload;
			Request m_request;
			Response m_response;
			Parser m_parser;
			std::vector<const webserv::Config::Server*> &m_config;

			Status m_status;
			size_t m_headerOffset;
			int	m_fileFD;
			std::string m_bodyBuffer;
			bool m_bodyToSend;
			bool isCGI;
			
			ClientHandler();
			ClientHandler(ClientHandler &);
			ClientHandler operator=(ClientHandler &);
			void cleanup();

			void checkServerAndLocation();
			void evaluateTransition(void);
			
			void prepareResponse();
			void computePath();
			bool serveDefaultFile();
			void generateAutoIndex();
			void findUploadStore();
			void findTypeOfUpload(std::string &value);
	};
}


#endif