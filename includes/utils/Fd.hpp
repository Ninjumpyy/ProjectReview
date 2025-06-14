/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Fd.hpp                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rpandipe <rpandie@student.42luxembourg.    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/04/22 14:12:46 by rpandipe          #+#    #+#             */
/*   Updated: 2025/04/24 07:28:41 by rpandipe         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef __FD_HPP__
#define __FD_HPP__

namespace webserv
{
class Fd
{
	public:
		explicit Fd(int fd);
		Fd(const Fd &other);
		Fd& operator=(const Fd &other); 
		~Fd();

		int getfd() const;
		bool isValid() const;
	
	private:
		int m_fd;
		int* m_refCount;

		Fd();
		void release();

};
}

#endif