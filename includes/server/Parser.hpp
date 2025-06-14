/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Parser.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: thomas <thomas@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/05/19 10:49:52 by tle-moel          #+#    #+#             */
/*   Updated: 2025/06/13 09:21:14 by thomas           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef __PARSER_HPP__
# define __PARSER_HPP__

#include <string>
#include "config/ParseConfig.hpp"
#include "server/Request.hpp"
#include "utils/utils.hpp"
#include <iostream>

namespace webserv
{
	class Request;
	
	class Parser
	{
		public:
			//enum Step {HEADERS, FIXED_BODY, CHUNKED_BODY, TRAILERS, DONE};
			//enum Status {NEED_DATA, PARSING_HEADERS, PARSING_BODY, EXPECT_CONTINUE, EXPECT_FAILED, REDIRECTION, COMPLETE};
			
			Parser(Request& request);
			~Parser();
			
			bool 					feed(const char* data, size_t length);
			void 					clearBuffer(void);
			std::string 			getBuffer(void);

		private:
			static const size_t MAX_EMPTY_SLOTS = 5;
			static const size_t DEFAULT_MAX_BUFFER = 8 * 1024; // 8 KiB for entire request‐line + headers
			
			Request		&m_request;
			std::string m_buffer;
			
			void parseStartLine(std::string startLine);
			void parseTarget(const std::string& target);
			void parsePathAndQuery(std::string raw_path);
			void decodePercentage(std::string& token);
			void parseHeaders(std::string headers);
			void parseHeaderLine(const std::string& line);
			void normalizeCommaListedValue(std::string& key, std::string& value);
			bool isCommaListHeader(const std::string header);
	};
}

#endif