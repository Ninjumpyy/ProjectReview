/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Response.cpp                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: tle-moel <tle-moel@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/05/26 15:41:15 by tle-moel          #+#    #+#             */
/*   Updated: 2025/06/16 13:00:08 by tle-moel         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "server/Response.hpp"
#include "server/Request.hpp"

webserv::Response::Response(Request& request): m_request(request) {}

webserv::Response::~Response() {}

const std::map<std::string, std::string> webserv::Response::s_mimeType = initializeMimeType();

void webserv::Response::buildGetResponse(const std::string& path, size_t fileSize)
{
	setStatusLine(200, reasonPhrase(200));
	addHeader("Content-Type", detectMimeType(path));
	addHeader("Content-Length", to_string(fileSize));
	if (!m_request.getKeepAlive())
		addHeader("Connection", "close");
	for (size_t i = 0; i < m_setcookies.size(); ++i)
		addHeader("Set-Cookie", m_setcookies[i]);
	m_setcookies.clear();
	appendCRLF(); // end of headers
}

void webserv::Response::buildPostResponse(const std::string& path)
{
	setStatusLine(201, reasonPhrase(201));
	addHeader("Location", path);
	addHeader("Content-Length", "0");
	if (!m_request.getKeepAlive())
		addHeader("Connection", "close");
	appendCRLF(); // end of headers
}

void webserv::Response::buildDeleteResponse(void)
{
	setStatusLine(204, reasonPhrase(204));
	addHeader("Content-Length", "0");
	if (!m_request.getKeepAlive())
		addHeader("Connection", "close");
	for (size_t i = 0; i < m_setcookies.size(); ++i)
		addHeader("Set-Cookie", m_setcookies[i]);
	m_setcookies.clear();
	appendCRLF(); // end of headers
}

void webserv::Response::buildErrorMessage(int code)
{
	std::string defaultErrorPage = "<!DOCTYPE html>"
  		"<html><head><title>" +	to_string(code) + " " + reasonPhrase(code) + "</title></head>"
  		"<body><h1>" + to_string(code) + " " + reasonPhrase(code) + "</h1>"
  		"<p>The server returned an error.</p></body></html>";
		
	setStatusLine(code, reasonPhrase(code));
	
	std::string ErrorPage;
	if (customErrorPage(code))
	{
		int errorFD = open(m_request.getPath().c_str(), O_RDONLY);
		if (errorFD < 0)
		{
			addHeader("Content-Type", "text/html; charset=utf-8");
			ErrorPage = defaultErrorPage;
		}
		else
		{
			char buf[16*1024];
			ssize_t nbytes = read(errorFD, buf, sizeof(buf));
			while (nbytes > 0)
			{
				ErrorPage.append(buf, nbytes);
				nbytes = read(errorFD, buf, sizeof(buf));
			}
			close(errorFD);
			addHeader("Content-Type", detectMimeType(m_request.getPath()));
		}
	}
	else
	{
		addHeader("Content-Type", "text/html; charset=utf-8");
		ErrorPage = defaultErrorPage;
	}

	addHeader("Content-Length", to_string(ErrorPage.size()));

	if (code == 405 && m_request.getLocationBlock() && !m_request.getLocationBlock()->methods.empty())
	{
		std::string allowedMethods;
		for (std::vector<std::string>::const_iterator it = m_request.getLocationBlock()->methods.begin(); it != m_request.getLocationBlock()->methods.end(); it++)
		{
			allowedMethods += *it;
			if (it + 1 != m_request.getLocationBlock()->methods.end())
				allowedMethods += ", ";
		}
		addHeader("Allow", allowedMethods);
	}
	
	if (code >= 400)
		m_request.setKeepAlive(false);
	if (!m_request.getKeepAlive())
		addHeader("Connection", "close");
	
	for (size_t i = 0; i < m_setcookies.size(); ++i)
		addHeader("Set-Cookie", m_setcookies[i]);
	m_setcookies.clear();
	appendCRLF(); // end of headers
	appendBody(ErrorPage);
}

void webserv::Response::buildRedirection(void)
{
	int code = m_request.getLocationBlock()->redirectCode;
	setStatusLine(code, reasonPhrase(code));
	addHeader("Location", m_request.getLocationBlock()->redirectTarget);
	for (size_t i = 0; i < m_setcookies.size(); ++i)
		addHeader("Set-Cookie", m_setcookies[i]);
	m_setcookies.clear();
	appendCRLF(); //end of response
}

std::string& webserv::Response::getResponseBuffer(void)
{
	return (m_responseBuffer);
}

void webserv::Response::setResponseBuffer(std::string response)
{
	m_responseBuffer = response;
}

void webserv::Response::setStatusLine(int code, const std::string& reason)
{
	
	m_responseBuffer.append("HTTP/1.1 ");
	m_responseBuffer.append(to_string(code));
	m_responseBuffer.append(" ");
	m_responseBuffer.append(reason);
	appendCRLF();
}

void webserv::Response::addHeader(const std::string& name, const std::string& value)
{
	m_responseBuffer.append(name);
	m_responseBuffer.append(": ");
	m_responseBuffer.append(value);
	appendCRLF();
}

void webserv::Response::appendCRLF()
{
	m_responseBuffer.append("\r\n");
}

void webserv::Response::appendBody(const std::string& data)
{
	m_responseBuffer.append(data);
}

std::string webserv::Response::detectMimeType(const std::string& path)
{
	std::string ext;
	size_t dot = path.find_last_of('.');
	if (dot == std::string::npos)
		ext = "";
	else
		ext = path.substr(dot + 1);
	for (size_t i = 0; i < ext.size(); i++)
		ext[i] = std::tolower(static_cast<unsigned char>(ext[i]));
	std::map<std::string, std::string>::const_iterator it = s_mimeType.find(ext);
	if (it != s_mimeType.end())
		return (it->second);
	else
		return "application/octet-stream"; //fallback
}

bool webserv::Response::customErrorPage(int code)
{
	const webserv::Config::Server* server = m_request.getServerBlock();
	if (server == NULL || server->error_pages.empty())
		return false;
	if (server->error_pages.count(code))
	{
		std::map<int,std::string>::const_iterator it = server->error_pages.find(code);
		std::string errorPage = it->second;
		struct stat st;

		std::string errorPagePath = server->root;
		if (errorPagePath[errorPagePath.size() - 1] == '/' && errorPage[0] == '/')
			errorPagePath.resize(errorPagePath.size() - 1);
		else if (errorPagePath[errorPagePath.size() - 1] != '/' && errorPage[0] != '/')
			errorPagePath += "/";
		errorPagePath += errorPage;

		if (stat(errorPagePath.c_str(), &st) == 0)
		{
			m_request.setPath(errorPagePath);
			return true;
		}
	}
	return false;
}


void webserv::Response::addCookie(const std::string& name, const std::string& value, const std::string& opts)
{
	std::string cookie = name + "=" + value + opts;
	m_setcookies.push_back(cookie);
}

std::string webserv::Response::reasonPhrase(int code)
{
	switch (code)
	{
		case 100: return "Continue";
		case 101: return "Switching Protocols";
		case 200: return "OK";
		case 201: return "Created";
		case 202: return "Accepted";
		case 203: return "Non-Authoritative Information";
		case 204: return "No Content";
		case 205: return "Reset Content";
		case 300: return "Multiple Choices";
		case 301: return "Moved Permanently";
		case 302: return "Found";
		case 303: return "See Other";
		case 305: return "Use Proxy";
		case 307: return "Temporary Redirect";
		case 400: return "Bad Request";
		case 402: return "Payment Required";
		case 403: return "Forbidden";
		case 404: return "Not Found";
		case 405: return "Method Not Allowed";
		case 406: return "Not Acceptable";
		case 408: return "Request Timeout";
		case 409: return "Conflict";
		case 410: return "Gone";
		case 411: return "Length Required";
		case 413: return "Payload Too Large";
		case 414: return "URI Too Long";
		case 415: return "Unsupported Media Type";
		case 417: return "Expectation Failed";
		case 426: return "Upgrade Required";
		case 500: return "Internal Server Error";
		case 501: return "Not Implemented";
		case 502: return "Bad Gateway";
		case 503: return "Service Unavailable";
		case 504: return "Gateway Timeout";
		case 505: return "HTTP Version Not Supported";
		default: return "Unknown";
	}
}

std::map<std::string, std::string> webserv::Response::initializeMimeType(void)
{
	std::map<std::string, std::string> m;
	m["html"] = "text/html";
	m["htm"] = "text/html";
	m["shtml"] = "text/html";
	m["css"] = "text/css";
	m["xml"] = "text/xml";
	m["mml"] = "text/mathml";
	m["txt"] = "text/plain";
	m["jad"] = "text/vnd.sun.j2me.app-descriptor";
	m["wnl"] = "text/vnd.wap.wml";
	m["htc"] = "text/x-component";

	m["jpg"] = "image/jpeg";
	m["jpeg"] = "image/jpeg";
	m["gif"] = "image/gif";
	m["png"] = "image/png";
	m["tif"] = "image/tiff";
	m["tiff"] = "image/tiff";
	m["wbmp"] = "image/vnd.wap.wbmp";
	m["ico"] = "image/x-icon";
	m["jng"] = "image/x-jng";
	m["bmp"] = "image/x-ms-bmp";
	m["svg"] = "image/xsvg+xml";
	m["svgz"] = "image/xsvg+xml";
	m["webp"] = "image/webp";

	m["mp4"] = "video/mp4";
	m["3gpp"] = "video/3gpp";
	m["3gp"] = "video/3gpp";
	m["mpeg"] = "video/mpeg";
	m["mpg"] = "video/mpeg";
	m["mov"] = "video/quicktime";
	m["webm"] = "video/webm";
	m["flv"] = "video/x-flv";
	m["m4v"] = "video/x-m4v";
	m["mng"] = "video/x-mng";
	m["asx"] = "video/x-ms-asf";
	m["asf"] = "video/x-ms-asf";
	m["wmv"] = "video/x-ms-wmv";
	m["avi"] = "video/x-msvideo";

	m["mp3"] = "audio/mpeg";
	m["mid"] = "audio/midi";
	m["kar"] = "audio/midi";
	m["midi"] = "audio/midi";
	m["ogg"] = "audio/ogg";
	m["m4a"] = "audio/x-m4a";
	m["ra"] = "audio/x-realaudio";

	m["js"] = "application/javascript";
	m["atom"] = "application/atom+xml";
	m["rss"] = "application/rss+xml";
	m["jar"] = "application/java-archive";
	m["war"] = "application/java-archive";
	m["ear"] = "application/java-archive";
	m["hqx"] = "application/max-binhex40";
	m["doc"] = "application/msword";
	m["pdf"] = "application/pdf";
	m["ps"] = "application/postscript";
	m["eps"] = "application/postscript";
	m["ai"] = "application/postscript";
	m["rtf"] = "application/rtf";
	m["xls"] = "application/vnd.ms-excel";
	m["ppt"] = "application/vnd.ms-powerpoint";
	m["wmlc"] = "application/vnd.wap.wmlc";
	m["kml"] = "application/vnd.google-earth.kml+xml";
	m["kmz"] = "application/vnd.google-earth.kmz";
	m["7z"] = "application/x-7z-compressed";
	m["cco"] = "application/x-cocoa";
	m["jardiff"] = "application/java-archive-diff";
	m["jnlp"] = "application/java-jnlp-file";
	m["run"] = "application/x-makeself";
	m["pl"] = "application/x-perl";
	m["pm"] = "application/x-perl";
	m["prc"] = "application/x-pilot";
	m["pdb"] = "application/x-pilot";
	m["rar"] = "application/x-rar-compressed";
	m["rpm"] = "application/x-redhat-package-manager";
	m["sea"] = "application/x-sea";
	m["swf"] = "application/x-shockwave-flash";
	m["sit"] = "application/x-stuffit";
	m["tcl"] = "application/x-x-tcl";
	m["tk"] = "application/x-x-tcl";
	m["der"] = "application/x-x509-ca-cert";
	m["pem"] = "application/x-x509-ca-cert";
	m["crt"] = "application/x-x509-ca-cert";
	m["xpi"] = "application/x-xpinstall";
	m["xhtml"] = "application/xhtml+xml";
	m["zip"] = "application/zip";

	m["bin"] = "application/octet-stream";
	m["exe"] = "application/octet-stream";
	m["dll"] = "application/octet-stream";
	m["deb"] = "application/octet-stream";
	m["dmg"] = "application/octet-stream";
	m["eot"] = "application/octet-stream";
	m["iso"] = "application/octet-stream";
	m["img"] = "application/octet-stream";
	m["msi"] = "application/octet-stream";
	m["msp"] = "application/octet-stream";
	m["msm"] = "application/octet-stream";
	return (m);
}