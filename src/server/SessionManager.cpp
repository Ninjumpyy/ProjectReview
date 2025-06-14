/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   SessionManager.cpp                                 :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rpandipe <rpandie@student.42luxembourg.    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/06/13 03:01:16 by rpandipe          #+#    #+#             */
/*   Updated: 2025/06/13 03:07:14 by rpandipe         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "server/SessionManager.hpp"

std::string webserv::SessionManager::createSession()
{
    // Seed the RNG for session ID generation
    srand(static_cast<unsigned>(time(NULL)) ^ rand());
    static const char hexchars[] = "0123456789abcdef";
    std::string sid;
    sid.reserve(32);
    for (int i = 0; i < 32; ++i) {
        sid += hexchars[rand() % 16];
    }
    // Store a new, empty session
    sessions[sid] = SessionData();
    return sid;
}


bool webserv::SessionManager::exists(const std::string& sid) 
{
    return sessions.find(sid) != sessions.end();
}

webserv::SessionManager::SessionData& webserv::SessionManager::getSession(const std::string& sid) 
{
    return sessions[sid];
}