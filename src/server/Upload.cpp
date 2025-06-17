/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Upload.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: tle-moel <tle-moel@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/06/10 17:11:30 by thomas            #+#    #+#             */
/*   Updated: 2025/06/17 14:30:09 by tle-moel         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "server/Upload.hpp"

webserv::Upload::Upload(): m_is_fixed(false), m_is_chunked(false), m_is_raw(false), m_is_multipart(false),
				m_upState(UP_UNSET), m_chunkState(CHUNKED_UNSET), m_tempFd(-1), m_maxBodySize(1024 * 1024) {}

webserv::Upload::~Upload() {}

std::string webserv::Upload::getUploadStore() const
{
	return m_uploadStore;
}

bool webserv::Upload::isFixed(void) const
{
	return (m_is_fixed);
}

bool webserv::Upload::isChunked(void) const
{
	return (m_is_chunked);
}

ssize_t webserv::Upload::getMaxBodySize(void) const
{
	return (m_maxBodySize);
}

size_t webserv::Upload::getContentLength(void) const
{
	return (m_contentLength);
}

void webserv::Upload::setFixed(bool is_fixed_length)
{
	m_is_fixed = is_fixed_length;
}

void webserv::Upload::setChunked(bool is_chunked)
{
	m_is_chunked = is_chunked;
}

void webserv::Upload::setRaw(bool is_raw)
{
	m_is_raw = is_raw;
}

void webserv::Upload::setMultipart(bool is_multipart)
{
	m_is_multipart = is_multipart;
}

void webserv::Upload::setContentLength(size_t contentLength)
{
	m_contentLength = contentLength;
}

void webserv::Upload::setBoundary(const std::string& boundary)
{
	m_boundary = boundary;
}

void webserv::Upload::setUploadStore(const std::string& store)
{
	m_uploadStore = store;
}

void webserv::Upload::setMaxBodySize(ssize_t size)
{
	m_maxBodySize = size;
}

void webserv::Upload::reset(void)
{
	m_is_fixed = false;
	m_is_chunked = false;
	m_is_raw = false;
	m_is_multipart = false;
	m_upState = UP_UNSET;
	m_chunkState = CHUNKED_UNSET;
	m_tempFd = -1;
	m_contentLength = 0;
	m_boundary.clear();
	m_tail.clear();
	m_chunkBuffer.clear();
	m_bytesWritten = 0;
	m_currentFilename.clear();
	m_uploadStore.clear();
	m_maxBodySize = 1024 * 1024;
}

bool webserv::Upload::handlePost(int clientFd)
{
	std::cerr << "Handling POST request for upload" << std::endl;

	if (m_upState == UP_UNSET) 
		initializeUpload();

	char buf[1024];
	ssize_t bytesRead = recv(clientFd, buf, sizeof(buf), 0);
	if (bytesRead <= 0)
	{
		deleteTempFile();
		throw HttpError(500); // Internal Server Error
	}
	if (m_is_fixed)
	{
		m_bytesWritten += bytesRead;
		if (m_bytesWritten > m_contentLength || m_bytesWritten > m_maxBodySize)
		{
			deleteTempFile();
			throw HttpError(413); // Payload Too Large
		}

		return (handleFixedBody(buf, bytesRead, true)); // returns true if upload is done, false if more data is needed
	}
	else if (m_is_chunked)
	{
		m_chunkBuffer.append(buf, bytesRead);
		size_t pos;
		int	nbytes;

		if (m_chunkState == CHUNKED_PART)
		{
			while (true)
			{
				pos = m_chunkBuffer.find("\r\n");
				if (pos == std::string::npos)
					return false; // more data needed

				nbytes = hexastringtoint(m_chunkBuffer.substr(0, pos));
				if (m_chunkBuffer.size() < pos + 2 + nbytes + 2) // I need -> size \r\n + nbytes \r\n
					return false; // more data needed
				if (nbytes != 0)
				{
					m_bytesWritten += nbytes;
					if (m_bytesWritten > m_maxBodySize)
					{
						deleteTempFile();
						throw HttpError(413); // Payload Too Large
					}
					m_chunkBuffer.erase(0, pos + 2);
					m_tail.append(m_chunkBuffer.data(), nbytes);
					m_chunkBuffer.erase(0, nbytes + 2);
					handleChunkedBody();
				}
				else
				{
					m_chunkBuffer.erase(0, 1); // remove the size = 0 but let \r\n to detect \r\n\r\n
					m_chunkState = CHUNKED_TRAILERS; // move to trailers state
				}
			}
		}
		if (m_chunkState == CHUNKED_TRAILERS)
		{
			size_t pos = m_chunkBuffer.find("\r\n\r\n");
			if (pos != std::string::npos)
				return false;
			m_chunkBuffer.erase(0, pos + 4); //do not handle trailers for now
			m_chunkState = CHUNKED_DONE;
		}
		if (m_chunkState == CHUNKED_DONE)
		{
			m_upState = UP_DONE; // upload is done
			close(m_tempFd);
			return true;
		}
	}
	else
	{
		deleteTempFile();
		throw HttpError(400); // Bad Request, unsupported upload type
	}
	return false;
}



// Send the http response in ClientHandler.cpp

void webserv::Upload::initializeUpload(void)
{
	std::cerr << "Initializing upload" << std::endl;

	if (m_is_raw)
		m_upState = UP_RAW_STREAMING;
	else if (m_is_multipart)
		m_upState = UP_MULTIPART_HEADERS;
	else
		throw HttpError(400);

	if (m_is_chunked)
		m_chunkState = CHUNKED_PART;
	
	while (true)
	{
		createTempFileName(m_uploadStore);
		if (access(m_currentFilename.c_str(), F_OK) == -1) // if file does not exist
		{
			m_tempFd = open(m_currentFilename.c_str(), O_CREAT | O_WRONLY | O_TRUNC, 0644); // create if missing, write-only, discard any existing content
			if (m_tempFd < 0)
				throw HttpError(500); // Internal Server Error
			break; // we have a valid temp file descriptor
		}
	}
}

void webserv::Upload::initializeDelete(void)
{
	std::cerr << "Initializing delete upload" << std::endl;

	m_upState = UP_DELETION;
	if (m_is_chunked)
		m_chunkState = CHUNKED_PART;
}

std::string webserv::Upload::createTempFileName(std::string dir)
{
	std::cerr << "Creating temporary file name in directory: " << dir << std::endl;

	static unsigned uniqueCounter = 0;
	++uniqueCounter;
	
	std::ostringstream oss;
	oss << dir;
	if (dir[dir.size() - 1] != '/')
		oss << '/';
	oss << ".upload_" << uniqueCounter << ".tmp";
	m_currentFilename = oss.str();
	return (oss.str());
}

void webserv::Upload::renameFile(const std::string& newName)
{
	std::cerr << "Renaming file to " << newName << std::endl;

	std::string newPath = m_uploadStore;
	if (newPath[newPath.size() - 1] != '/')
		newPath += '/';
	newPath += newName;
	if (access(newPath.c_str(), F_OK) == 0) // if file exist already
	{
		deleteTempFile();
		throw HttpError(409); // Conflict
	}
	if (::rename(m_currentFilename.c_str(), newPath.c_str()) < 0)
	{
		deleteTempFile();
		throw HttpError(500); // Internal Server Error
	}
	m_currentFilename = newPath; // Update the current filename to the new one
}

void webserv::Upload::deleteTempFile(void)
{
	std::cerr << "Deleting temporary file" << std::endl;

	if (m_tempFd != -1) 
    {
        close(m_tempFd);
        if (!m_currentFilename.empty())
            unlink(m_currentFilename.c_str()); // Delete the partial file
        m_tempFd = -1;
    }
}

bool webserv::Upload::handleFixedBody(const char* data, size_t length, bool flag)
{
	std::cerr << "Handling fixed body upload" << std::endl;

	if (m_is_raw)
	{
		size_t offset = 0;
		while (offset < length)
		{
			ssize_t sent = write(m_tempFd, data + offset, length - offset);
			if (sent < 0)
			{
				deleteTempFile();
				throw HttpError(500); // Internal Server Error
			}
			offset += static_cast<size_t>(sent);
		}
		if (m_bytesWritten == m_contentLength)
		{
			m_upState = UP_DONE;
			close(m_tempFd);
			return true; // Upload is done
		}
	}
	if (m_is_multipart)
	{
		std::string boundary = "--" + m_boundary;
		std::string closingBoundary = boundary + "--";
		if (flag) // If flag is true, we are handling the first part of the body
			m_tail.append(data, length);
		if (m_upState == UP_MULTIPART_HEADERS)
		{
			size_t pos = m_tail.find("\r\n\r\n"); // Find the end of headers
			if (pos != std::string::npos)
			{
				std::string headers = m_tail.substr(0, pos + 4);
				m_tail.erase(0, pos + 4); // Remove headers from tail
				parseMultipartHeaders(headers);
				m_upState = UP_MULTIPART_BODY;
			}
			return false; // More data needed
		}
		if (m_upState == UP_MULTIPART_BODY)
		{
			if (m_tail.empty())
				return false; // No data to process yet
			size_t pos = m_tail.find(boundary);
			if (pos == std::string::npos) // No boundary found, just write the data to the temp file
			{
				ssize_t sent = write(m_tempFd, m_tail.data(), m_tail.size());
				if (sent < 0)
				{
					deleteTempFile();
					throw HttpError(500); // Internal Server Error
				}
				m_tail.clear(); // Clear the tail after writing
				return false; // More data needed
			}
			else // We found a boundary, write up to that point
			{
				ssize_t sent = write(m_tempFd, m_tail.data(), pos);
				if (sent < 0)
				{
					deleteTempFile();
					throw HttpError(500); // Internal Server Error
				}
				if (m_tail.compare(pos, closingBoundary.size(), closingBoundary) == 0)
				{
					m_tail.erase(0, pos + closingBoundary.size()); // Remove closing boundary
					if (!m_tail.empty())
					{
						deleteTempFile();
						throw HttpError(400); // Bad Request, unexpected data after closing boundary
					}
					m_upState = UP_DONE;
					close(m_tempFd); // Close the temp file
					return true; // Upload done
				}
				else
				{
					m_tail.erase(0, pos + boundary.size()); // Remove the boundary
					m_upState = UP_MULTIPART_HEADERS; // Go back to headers state to parse the next part
					return (handleFixedBody(m_tail.data(), m_tail.size(), false)); // Continue processing the remaining data
				}
			}
		}
	}
	return false;
}

void webserv::Upload::parseMultipartHeaders(std::string headers)
{
	while (true)
	{
		size_t pos = headers.find("\r\n");
		if (pos == 0) // End of headers -> we got \r\n\r\n
			return;
		std::string headerLine = headers.substr(0, pos);
		parseHeaderLine(headerLine);
		headers.erase(0, pos + 2);
	}

}

void webserv::Upload::parseHeaderLine(const std::string& line)
{
	size_t pos = line.find(':');
	if (pos == std::string::npos)
	{
		deleteTempFile();
		throw HttpError(400); // Bad Request
	}

	std::string key = line.substr(0, pos);
	if (key.find(' ') != std::string::npos || key.find('\t') != std::string::npos)
	{
		deleteTempFile();
		throw HttpError(400); // Bad Request
	}
	for (size_t i = 0; i < key.size(); i++)
		key[i] = std::tolower(static_cast<unsigned char>(key[i]));

	std::string value = line.substr(pos + 1);
	trimOWS(value);
	if (value.empty())
	{
		deleteTempFile();
		throw HttpError(400); // Bad Request
	}

	if (key == "content-disposition")
	{
		size_t filenamePos = value.find("filename=");
		if (filenamePos != std::string::npos)
		{
			filenamePos += 9; // Skip "filename="
			size_t endPos = value.find_first_of("; \t", filenamePos);
			if (endPos == std::string::npos)
				endPos = value.size();
			std::string newName = value.substr(filenamePos, endPos - filenamePos);
			if (newName.size() > 255 || newName.empty()) 
			{
				deleteTempFile();
				throw HttpError(413); // Payload Too Large
			}
			for (size_t i = 0; i < newName.size(); i++)
			{
				if (!isalnum(newName[i]) && newName[i] != '_' && newName[i] != '-')
				{
					deleteTempFile();
					throw HttpError(400); // Bad Request
				}
			}
			renameFile(newName);
		}
	}
}

void webserv::Upload::handleChunkedBody(void)
{
	std::cerr << "Handling chunked body upload" << std::endl;

	if (m_is_raw)
	{
		size_t offset = 0;
		while (offset < m_tail.size())
		{
			ssize_t sent = write(m_tempFd, m_tail.data() + offset, m_tail.size() - offset);
			if (sent < 0)
			{
				deleteTempFile();
				throw HttpError(500); // Internal Server Error
			}
			offset += static_cast<size_t>(sent);
		}
	}
	if (m_is_multipart)
	{
		std::string boundary = "--" + m_boundary;
		std::string closingBoundary = boundary + "--";
		if (m_upState == UP_MULTIPART_HEADERS)
		{
			size_t pos = m_tail.find("\r\n\r\n"); // Find the end of headers
			if (pos != std::string::npos)
			{
				std::string headers = m_tail.substr(0, pos + 4);
				m_tail.erase(0, pos + 4); // Remove headers from tail
				parseMultipartHeaders(headers);
				m_upState = UP_MULTIPART_BODY;
			}
			return; // More data needed
		}
		if (m_upState == UP_MULTIPART_BODY)
		{
			if (m_tail.empty())
				return; // No data to process yet
			size_t pos = m_tail.find(boundary);
			if (pos == std::string::npos) // No boundary found, just write the data to the temp file
			{
				ssize_t sent = write(m_tempFd, m_tail.data(), m_tail.size());
				if (sent < 0)
				{
					deleteTempFile();
					throw HttpError(500); // Internal Server Error
				}
				m_tail.clear(); // Clear the tail after writing
				return; // More data needed
			}
			else // We found a boundary, write up to that point
			{
				ssize_t sent = write(m_tempFd, m_tail.data(), pos);
				if (sent < 0)
				{
					deleteTempFile();
					throw HttpError(500); // Internal Server Error
				}
				if (m_tail.compare(pos, closingBoundary.size(), closingBoundary) == 0)
				{
					m_tail.erase(0, pos + closingBoundary.size()); // Remove closing boundary
					if (!m_tail.empty())
					{
						deleteTempFile();
						throw HttpError(400); // Bad Request, unexpected data after closing boundary
					}
					return; // Should be finished, but we need to check if we get the zero-length chunk
				}
				else
				{
					m_tail.erase(0, pos + boundary.size()); // Remove the boundary
					m_upState = UP_MULTIPART_HEADERS; // Go back to headers state to parse the next part
					return (handleChunkedBody()); // Continue processing the remaining data
				}
			}
		}
	}
}

bool webserv::Upload::handleDelete(int clientFd) // Purpose: drain the body of a DELETE request
{
	std::cerr << "Handling DELETE request for upload" << std::endl;
	
	if (m_upState == UP_UNSET) 
		initializeDelete();

	char buf[1024];
	ssize_t bytesRead = recv(clientFd, buf, sizeof(buf), 0);
	
	if (m_is_fixed)
	{
		if (bytesRead <= 0)
			throw HttpError(500); // Internal Server Error
		m_bytesWritten += bytesRead;
		if (m_bytesWritten > m_contentLength || m_bytesWritten > m_maxBodySize)
			throw HttpError(413); // Payload Too Large
		if (m_bytesWritten == m_contentLength)
			return true; // DELETE operation is done, we have drained the body
		return false; // More data needed
	}
	if (m_is_chunked)
	{
		if (m_chunkState == CHUNKED_PART)
		{
			m_chunkBuffer.append(buf, bytesRead);
			size_t pos;
			int	nbytes;
			
			while (true)
			{
				pos = m_chunkBuffer.find("\r\n");
				if (pos == std::string::npos)
					return false; // more data needed

				nbytes = hexastringtoint(m_chunkBuffer.substr(0, pos));
				if (m_chunkBuffer.size() < pos + 2 + nbytes + 2) // I need -> size \r\n + nbytes \r\n
					return false; // more data needed
				if (nbytes != 0)
				{
					m_bytesWritten += nbytes;
					if (m_bytesWritten > m_maxBodySize)
						throw HttpError(413); // Payload Too Large
					m_chunkBuffer.erase(0, pos + 2 + nbytes + 2);
				}
				else
				{
					m_chunkBuffer.erase(0, 1); // remove the size = 0 but let \r\n to detect \r\n\r\n
					m_chunkState = CHUNKED_TRAILERS; // move to trailers state
				}
			}
		}
		if (m_chunkState == CHUNKED_TRAILERS)
		{
			size_t pos = m_chunkBuffer.find("\r\n\r\n");
			if (pos != std::string::npos)
				return false;
			m_chunkBuffer.erase(0, pos + 4); //do not handle trailers for now
			m_chunkState = CHUNKED_DONE;
		}
		if (m_chunkState == CHUNKED_DONE)
		{
			m_upState = UP_DONE; // upload is done
			return true;
		}
	}
	return false;
}
