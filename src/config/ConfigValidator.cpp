/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ConfigValidator.cpp                                :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: tle-moel <tle-moel@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/05/27 10:41:51 by rpandipe          #+#    #+#             */
/*   Updated: 2025/05/29 15:06:47 by tle-moel         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "config/ConfigValidator.hpp"
#include <iostream>
#include <sstream>
#include <set>
#include <algorithm>

webserv::ConfigValidator::ConfigValidator(const std::vector<webserv::Config::Server> &server) : m_server(server) {}

webserv::ConfigValidator::~ConfigValidator(){};

void webserv::ConfigValidator::printConfigurations()
{
	std::cout << "--------------------------------------------------------------------" << std::endl;
	std::cout << "Printing Server Configurations :" << std:: endl;
	for (std::vector<webserv::Config::Server>::const_iterator it = m_server.begin(); it != m_server.end(); it++)
	{
		std::cout << "Host is " << it->hostname << std::endl;
		std::cout << "Port is " << it->port << std::endl;
		std::cout << "Servername : " << std::endl;
		for(size_t i = 0; i < it->name.size(); i++)
			std::cout << "		" << i << ") " << it->name[i] << std::endl;
		std::cout << "Root is : " << it->root << std::endl;
		std::cout << "Index files : " << std::endl;
		for(size_t i = 0; i < it->index_files.size(); i++)
			std::cout << "		" << i << ") " << it->index_files[i] << std::endl;
		std::cout << "Error Pages : " << std::endl;
		for (std::map<int,std::string>::const_iterator jt = it->error_pages.begin(); jt != it->error_pages.end(); jt++)
			std::cout << "		- " << jt->first << " → " << jt->second << std::endl;
		std::cout << "Max Body Size : " << it->max_body_size << std::endl;
		for (std::vector<webserv::Config::LocationConfig>::const_iterator loc = it->locations.begin(); loc != it->locations.end(); loc++)
		{
			std::cout << "Location " << loc->prefix << std::endl;
			std::cout << "	Methods :";
			for (size_t i = 0; i < loc->methods.size(); i++)
				std::cout << " " << loc->methods[i];
			std::cout << std::endl;
			std::cout << "Redirection : " << loc->hasRedirect << std::endl;
			std::cout << "Redirection code : " << loc->redirectCode << std::endl;
			std::cout << "Redirection Target : " << loc->redirectTarget << std::endl;
			std::cout << "Root is " << loc->root << std::endl;
			std::cout << "Alias is " << loc->alias << std::endl;
			std::cout << "Autoindex : " << loc->autoindex << std::endl;
			std::cout << "Index Files :" << std::endl;
			for (size_t i = 0; i < loc->indexFiles.size(); i++)
				std::cout <<"	" << i << ") " << loc->indexFiles[i] << std::endl;
			std::cout << "CGI Pass : " << loc->cgiPass << std::endl;
			std::cout << "Upload Path : " << loc->uploadStore << std::endl;
		}
	}
	std::cout << "--------------------------------------------------------------------" << std::endl;
}

void webserv::ConfigValidator::validateConfig()
{
	validateListen();
	validateRoot();
	validateLocation();
}

void webserv::ConfigValidator::validateListen()
{
	std::map<HostPort, std::vector< std::vector<std::string> > > groups;

	for (size_t i = 0; i < m_server.size(); ++i)
	{
		if (m_server[i].hostname.empty() || m_server[i].port == 0)
		{
			std::ostringstream oss;
			oss << "Server #" << (i + 1) << " is missing a valid listen directive (hostname or port)";
			throw std::runtime_error(oss.str());
		}
		HostPort key(m_server[i].hostname, m_server[i].port);
		groups[key].push_back(m_server[i].name);
	}
	for (std::map<HostPort, std::vector< std::vector<std::string> > >::const_iterator it = groups.begin(); it != groups.end(); it++)
	{
		std::vector< std::vector<std::string> > names = it->second;
		std::set<std::string> seennames;
		for (size_t i = 0; i < names.size(); i++)
		{
			if (i > 0 && names[i].empty())
			{
				std::ostringstream oss;
				oss << "Server #" << (i + 1)
					<< " on (" << it->first.first << ":" << it->first.second
					<< ") must have at least one server_name";
				throw std::runtime_error(oss.str());
			}
			for (size_t j = 0; j < names[i].size(); j++)
			{
				if (!seennames.insert(names[i][j]).second)
				{
					std::ostringstream oss;
					oss << "Duplicate server_name '" << names[i][j]
						<< "' on (" << it->first.first << ":" << it->first.second << ")";
					throw std::runtime_error(oss.str());
				}
			}
		}
	}
}

void webserv::ConfigValidator::validateRoot()
{
	for (size_t i = 0; i < m_server.size(); ++i)
	{
		if (m_server[i].root.empty())
		{
			for(std::vector<webserv::Config::LocationConfig>::const_iterator it = m_server[i].locations.begin(); it != m_server[i].locations.end(); it++)
			{
				if (it->root.empty() && it->alias.empty())
				{
					std::ostringstream oss;
					oss << "Server #" << (i + 1)
						<< " does not define a root directive, and at least one location is missing both root and alias.";
					throw std::runtime_error(oss.str());
				}
			}
		}	
	}
}

void webserv::ConfigValidator::validateLocation()
{
	for (size_t i = 0; i < m_server.size(); ++i)
	{
		for(std::vector<webserv::Config::LocationConfig>::const_iterator it = m_server[i].locations.begin(); it != m_server[i].locations.end(); it++)
		{
			std::set<std::string> seenmethods;
			for (size_t j = 0; j < it->methods.size(); j++)
			{
				if (!seenmethods.insert(it->methods[j]).second)
				{
					std::ostringstream oss;
					oss << "Duplicate method '" << it->methods[j] << "' in location '" << it->prefix << "'";
					throw std::runtime_error(oss.str());
				}
			}
			if (!it->root.empty() && !it->alias.empty())
			{
				std::ostringstream oss;
				oss << "Server #" << (i + 1)
					<< " Conflict between root and alias.";
				throw std::runtime_error(oss.str());
			}
			if (it->hasRedirect && (!it->alias.empty() || !it->root.empty() || !it->cgiPass.empty() || !it->uploadStore.empty()))
			{
				std::ostringstream oss;
				oss << "Location '" << it->prefix << "' defines return but also uses root/alias/cgi_pass/upload_store";
				throw std::runtime_error(oss.str());
			}
			if (!it->uploadStore.empty())
			{
				const std::vector<std::string>& methods = it->methods;
				if (std::find(methods.begin(), methods.end(), "POST") == methods.end() &&
					std::find(methods.begin(), methods.end(), "PUT") == methods.end())
				{
					std::ostringstream oss;
					oss << "Location '" << it->prefix << "' uses upload_store but does not allow POST or PUT";
					throw std::runtime_error(oss.str());
				}
			}
			if (!it->cgiPass.empty())
			{
				const std::vector<std::string>& methods = it->methods;
				if (std::find(methods.begin(), methods.end(), "GET") == methods.end() &&
					std::find(methods.begin(), methods.end(), "POST") == methods.end())
				{
					std::ostringstream oss;
					oss << "Location '" << it->prefix << "' uses cgi_pass but does not allow GET or POST";
					throw std::runtime_error(oss.str());
				}
			}
		}
	}
}