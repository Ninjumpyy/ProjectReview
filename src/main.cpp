/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: tle-moel <tle-moel@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/04/21 13:24:06 by rpandipe          #+#    #+#             */
/*   Updated: 2025/06/17 12:17:06 by tle-moel         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "server/ServerManager.hpp"
#include "config/ParseConfig.hpp"
#include "config/ConfigValidator.hpp"
#include <iostream>
#include <string>
#include <unistd.h>
#include <vector>
#include <poll.h>

int main(int argc, char** argv)
{
	try
	{
		if (argc <= 2)
		{
			std::string configFileName;
			if (argc == 1) // Use default config file
			{
				std::cerr << "No config file provided, using default: webserv.conf" << std::endl;
				configFileName = "webserv.conf";
			}
			else if (argc == 2)
				configFileName = argv[1];

			webserv::Config config(configFileName);
			config.parseConfig();
			const std::vector<webserv::Config::Server> server = config.getServers();
			webserv::ConfigValidator valid(server);
			valid.validateConfig();
			valid.printConfigurations();
			webserv::ServerManager manager;
			manager.setupsockets(server);
			manager.startserver();
		}
		else
		{
			std::cerr << "Usage: " << argv[0] << " <config_file>" << std::endl;
			return (1);
		}
	}
	catch (const std::exception &e)
	{
		std::cerr << "Error : " << e.what() << std::endl;
		return (1);
	}
	return (0);
}