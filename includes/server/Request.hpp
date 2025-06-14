/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Request.hpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: tle-moel <tle-moel@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/05/12 17:28:45 by thomas            #+#    #+#             */
/*   Updated: 2025/06/13 17:26:05 by tle-moel         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef __REQUEST_HPP__
# define __REQUEST_HPP__


#include <map>
#include <vector>
#include <set>
#include <algorithm>
#include <cctype>
#include <limits>
#include "server/Parser.hpp"
#include "server/Upload.hpp"
#include "utils/utils.hpp"
#include "utils/HttpError.hpp"

#ifndef __PARSERCONFIG_HPP__
namespace webserv
{
	class Config
	{
		public:
			struct Server;
			struct LocationConfig;
	};
}
#endif


namespace webserv
{
	class Request
	{
		public:
			enum Method {GET, POST, DELETE, HEAD, UNSET};

			Request(std::vector<const webserv::Config::Server*> &config, Upload &upload);
			~Request();

			const std::string getMethod(void) const;
			const std::string &getPath(void) const;
			const std::string &getQuery(void) const;
			const std::string &getVersion(void) const;
			const std::string &getBody(void) const;
			bool			getKeepAlive(void) const;
			const std::map<std::string, std::vector<std::string> > &getHeaders(void) const;
			const webserv::Config::Server* getServerBlock(void) const;
			const webserv::Config::LocationConfig* getLocationBlock(void) const;

			void setMethod(Method method);
			void setPath(std::string path);
			void setQuery(std::string query);
			void setVersion(std::string version);
			void setBody(std::string body);
			void setHeader(std::string key, std::string value);
			void setKeepAlive(bool);
			void setServerBlock(const webserv::Config::Server* serverBlock);
			void setLocationBlock(const webserv::Config::LocationConfig* locationBlock);
			void setCookie(const std::string& name, const std::string& val);
			std::string getCookie(const std::string& name) const;

			void clearForNextRequest(void);
			void validateRequest(void);
			void selectServerBlock();
			void selectLocationBlock();

			bool parseFixedBody(int clientFD, ssize_t maxBodySize, size_t content_length);
			bool parseChunkedBody(int clientFD, ssize_t maxBodySize, size_t content_length);
			size_t getExpectedLength() const;
			
		private:
			Upload		&m_upload;
			Method 		m_method;
			std::string m_path;
			std::string m_query;
			std::string m_version;
			std::string m_body;
			std::string m_buffer;
			std::map<std::string, std::vector<std::string> > m_headers;
			const webserv::Config::Server* m_serverBlock;
			const webserv::Config::LocationConfig *m_locationBlock;
			std::vector<const webserv::Config::Server*> &m_config;
			std::map<std::string,std::string> m_cookies;

			bool		m_keepAlive;

			Request(const Request&);
			Request& operator=(const Request&);
	};
}

#endif