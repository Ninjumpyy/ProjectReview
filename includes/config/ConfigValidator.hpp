/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ConfigValidator.hpp                                :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rpandipe <rpandipe.student.42luxembourg    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/05/27 10:41:56 by rpandipe          #+#    #+#             */
/*   Updated: 2025/05/27 18:04:26 by rpandipe         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef __CONFIGVALIDATOR_HPP__
#define __CONFIGVALIDATOR_HPP__

#include "ParseConfig.hpp"

namespace webserv
{
class ConfigValidator
{
	public:
		ConfigValidator(const std::vector<webserv::Config::Server> &server);
		~ConfigValidator();
		void printConfigurations();
		void validateConfig();

		typedef std::pair<std::string, int> HostPort;
		
	private:
		ConfigValidator();
		ConfigValidator(const ConfigValidator &other);
		ConfigValidator& operator=(const ConfigValidator &other);
		void validateListen();
		void validateRoot();
		void validateLocation();

		const std::vector<webserv::Config::Server> m_server;
};
}
#endif