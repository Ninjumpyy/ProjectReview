/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   SessionManager.hpp                                 :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rpandipe <rpandipe.student.42luxembourg    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/06/13 02:49:02 by rpandipe          #+#    #+#             */
/*   Updated: 2025/06/13 17:07:59 by rpandipe         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef __SESSIONMANAGER_HPP__
#define __SESSIONMANAGER_HPP__

#include <map>
#include <string>
#include <ctime>
#include <cstdlib>

namespace webserv
{
	class SessionManager
	{
		public:
			struct SessionData { std::map<std::string,std::string> data; };

			static std::string createSession();
			static bool exists(const std::string& sid);
			static SessionData& getSession(const std::string& sid);
	};
	static std::map<std::string, SessionManager::SessionData> sessions;
}

#endif