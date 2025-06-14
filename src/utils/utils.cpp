/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   utils.cpp                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: thomas <thomas@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/05/21 15:31:25 by tle-moel          #+#    #+#             */
/*   Updated: 2025/06/13 09:20:21 by thomas           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "utils/utils.hpp"

int stringtoint(std::string str)
{
	if (str.find_first_not_of("0123456789") != std::string::npos)
		throw webserv::HttpError(400);
	std::istringstream iss(str);
    int res;
    if (!(iss >> res) || !iss.eof()) 
        throw webserv::HttpError(400);
    return res;
}

int hexastringtoint(std::string str)
{
	if (str.find_first_not_of("0123456789abcdefABCDEF") != std::string::npos)
		throw webserv::HttpError(400);
	std::istringstream iss(str);
	int res;
	if (!(iss >> std::hex >> res) || !iss.eof())
		throw webserv::HttpError(400);
	return res;
}

std::string to_string(int nb)
{
	if (nb == 0)
		return "0";

	bool negative = nb < 0;
	if (negative)
		nb *= -1;
	
	std::string res;
	while (nb > 0)
	{
		char digit = '0' + (nb % 10);
		res.push_back(digit);
		nb /= 10;
	}
	if (negative)
		res.push_back('-');
	std::reverse(res.begin(), res.end());
	return res;
}

std::string finet_ntop(in_addr sin_addr)
{
	uint32_t ip = ntohl(sin_addr.s_addr);
	int    b1 = (ip >> 24) & 0xFF;
	int    b2 = (ip >> 16) & 0xFF;
	int    b3 = (ip >>  8) & 0xFF;
	int    b4 = (ip      ) & 0xFF;
	std::string ipStr = to_string(b1) + "." 
                  + to_string(b2) + "." 
                  + to_string(b3) + "." 
                  + to_string(b4);
	return ipStr;
}

void trimOWS(std::string& value)
{
	//trim whitespace from both ends
	size_t start = value.find_first_not_of(" \t");
	size_t end = value.find_last_not_of(" \t");
	if (start == std::string::npos)
		value.clear();
	else
		// substr from start, length = end-start+1
		value = value.substr(start, end - start + 1);
}