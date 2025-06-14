/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   HttpError.hpp                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: tle-moel <tle-moel@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/05/27 16:06:18 by tle-moel          #+#    #+#             */
/*   Updated: 2025/05/27 16:25:20 by tle-moel         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef HTTPERROR_HPP
# define HTTPERROR_HPP

#include <exception>

namespace webserv
{
	class HttpError : public std::exception
	{
		public:
			HttpError(int statusCode): m_code(statusCode) {}

			int code() const throw()
			{
				return m_code;
			}	
			
			const char* what() const throw()
			{
				return "HTTP error";
			}

		private:
			int m_code;
	};
}

#endif