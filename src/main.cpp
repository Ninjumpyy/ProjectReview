/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rpandipe <rpandipe.student.42luxembourg    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/04/21 13:24:06 by rpandipe          #+#    #+#             */
/*   Updated: 2025/05/28 15:52:42 by rpandipe         ###   ########.fr       */
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
		if (argc  == 2)
		{
			webserv::Config config(argv[1]);
			config.parseConfig();
			const std::vector<webserv::Config::Server> server = config.getServers();
			webserv::ConfigValidator valid(server);
			valid.validateConfig();
			valid.printConfigurations();
			webserv::ServerManager manager;
			manager.setupsockets(server);
			manager.startserver();
		}
	}
	catch (const std::exception &e)
	{
		std::cerr << "Error : " << e.what() << std::endl;
		return (1);
	}
	return (0);
}