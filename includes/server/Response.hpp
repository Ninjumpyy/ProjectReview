/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Response.hpp                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: thomas <thomas@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/05/26 15:41:03 by tle-moel          #+#    #+#             */
/*   Updated: 2025/06/13 12:57:09 by thomas           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef __RESPONSE_HPP__
# define __RESPONSE_HPP__

#include <iostream>
#include <algorithm>
#include <unistd.h>
#include <fcntl.h>
#include "utils/utils.hpp"
#include <sys/stat.h>
#include <map>
#include <vector>
#include <string>

namespace webserv
{
	class Request;
	
	class Response
	{
		public:
			Response(Request& request);
			~Response();

			std::string& getResponseBuffer(void);
			void setResponseBuffer(std::string response);

			void buildGetResponse(const std::string& path, size_t fileSize);
			void buildPostResponse(const std::string& path);
			void buildDeleteResponse(void);
			void buildErrorMessage(int code);
			void buildRedirection(void);
			void setStatusLine(int code, const std::string& reason);
			void addHeader(const std::string& name, const std::string& value);
			void appendCRLF();
			void appendBody(const std::string& data);
			void addCookie(const std::string& name, const std::string& value, const std::string& opts ="; Path=/; HttpOnly");


		private:
			Request 	&m_request;
			std::string m_responseBuffer;
			static const std::map<std::string, std::string> s_mimeType;
			std::vector<std::string> m_setcookies;

			std::string detectMimeType(const std::string& path);
			bool customErrorPage(int code);

			std::string reasonPhrase(int code);
			static std::map<std::string, std::string> initializeMimeType(void);

			Response();
			Response(const Response&);
			const Response& operator=(const Response&);
		
	};
}

#endif