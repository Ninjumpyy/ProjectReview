/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ParseConfig.hpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: tle-moel <tle-moel@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/05/22 15:07:44 by rpandipe          #+#    #+#             */
/*   Updated: 2025/06/16 13:45:09 by tle-moel         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef __PARSERCONFIG_HPP__
#define __PARSERCONFIG_HPP__

#include <string>
#include <vector>
#include <map>
#include "utils/utils.hpp"
#include <netdb.h>
#include <cstring>
#include <netinet/in.h> //sockaddr_in
#include "server/CgiProcess.hpp"


namespace webserv
{	
	class Config
	{
		public:
			struct LocationConfig {
				std::string    prefix;
				std::vector<std::string> methods;
				bool           hasRedirect;
				int            redirectCode;
				std::string    redirectTarget;
				std::string    root;
				std::string    alias;
				bool           autoindex;
				std::vector<std::string> indexFiles;
				std::string    cgiPass;
				std::string    uploadStore;

				LocationConfig()
					: methods()
					, hasRedirect(false)
					, redirectCode(0)
					, redirectTarget()
					, root()
					, alias()
					, autoindex(false)
					, indexFiles()
					, cgiPass()
					, uploadStore()
					{}
			};

			struct Server{
				std::string hostname;
				std::vector<std::string> name;
				int port;
				std::string root;
				std::vector<std::string> index_files;
				std::map<int, std::string> error_pages;
				size_t max_body_size;
				std::vector<LocationConfig> locations;
				std::vector<webserv::CgiProcess::CgiMapping> cgi;
			};

			enum TokenType{
				T_SERVER, T_LISTEN,
				T_LBRACE, T_RBRACE,
				T_SEMICOLON, T_COLON,
				T_IDENTIFIER, T_NUMBER,
				T_END, T_ERROR, 
				T_SERVER_NAME, T_ROOT,
				T_INDEX, T_ERROR_PAGE,
				T_MAX_BODY_SIZE, T_LOCATION,
				T_METHOD, T_RETURN,
				T_ALIAS, T_AUTOINDEX,
				T_UPLOAD_STORE, T_CGI_PASS,
				T_REGEX, T_REGEX_STRING
			};

			struct Token{
				TokenType type;
				std::string text;
				int line_nbr;

				Token(TokenType t, const std::string& tx, int ln) : type(t), text(tx), line_nbr(ln)
				{};
			};

			explicit Config(std::string path);
			~Config();
			void parseConfig();
			const char* getHost() const;
			int getPort() const;
			const std::vector<Server>& getServers() const;
		
		private:
			Config();
			Config(const Config &other);
			Config operator=(const Config &other);
			void tokenize();
			const Token& peek() const;
			Token& get();
			void expect(TokenType type);
			void parseConfigTokens();
			void parseServer();
			void parseListen(Server &server);
			void parseServerName(Server &server);
			void parseRoot(Server &server);
			void parseIndex(Server &server);
			void parseErrorPage(Server& server);
			void parseBodySize(Server &server);
			void parseLocation(Server &server);
			void printTokens();
			void splitLabels(std::string &name, std::vector<std::string> &labels);
			
			std::string m_path;
			std::vector<Token> m_token;
			std::vector<Server> m_server;
			std::string m_source;
			size_t m_pos;
	};
}
#endif