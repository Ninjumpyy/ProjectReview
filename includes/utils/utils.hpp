/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   utils.hpp                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: thomas <thomas@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/05/21 15:28:14 by tle-moel          #+#    #+#             */
/*   Updated: 2025/06/13 09:20:34 by thomas           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef __UTILS_HPP__
# define __UTILS_HPP__

# include <string>
# include <sstream>
# include <netinet/in.h> // defines struct in_addr, in_addr_t
# include <algorithm>
# include "utils/HttpError.hpp"

int stringtoint(std::string);
int hexastringtoint(std::string);
std::string to_string(int nb);
std::string finet_ntop(in_addr sin_addr);
void trimOWS(std::string& value);

#endif