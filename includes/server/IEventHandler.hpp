/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   IEventHandler.hpp                                  :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rpandipe <rpandipe.student.42luxembourg    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/04/22 14:52:57 by rpandipe          #+#    #+#             */
/*   Updated: 2025/04/22 18:12:57 by rpandipe         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef __IEVENTHANDLER_HPP__
#define __IEVENTHANDLER_HPP__

namespace webserv
{
class IEventHandler
{
	public:
		virtual ~IEventHandler() {};
		virtual void onEvent(short revents) = 0;
};
}

#endif