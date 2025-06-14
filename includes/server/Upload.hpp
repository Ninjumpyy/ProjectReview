/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Upload.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rpandipe <rpandipe.student.42luxembourg    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/06/10 17:10:24 by thomas            #+#    #+#             */
/*   Updated: 2025/06/13 17:03:30 by rpandipe         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef __UPLOAD_HPP__
# define __UPLOAD_HPP__

#include <iostream>
#include <string>
#include <sys/socket.h>
#include <unistd.h>
#include <fcntl.h>
#include <sstream>
#include <cstdio>
#include "utils/HttpError.hpp"
#include "utils/utils.hpp"

namespace webserv
{
	class Upload
	{
		public:
			enum UploadState { UP_UNSET,            // initial state
							UP_RAW_STREAMING,     // for application/octet-stream
							UP_MULTIPART_HEADERS, // in multipart, reading the little part headers
							UP_MULTIPART_BODY,    // in multipart, writing part data to disk
							UP_DELETION,         // if DELETE method, reading the body to delete it
							UP_DONE };          // upload finished, ready to send response

			enum ChunkedState { CHUNKED_UNSET, 
							CHUNKED_PART, 
							CHUNKED_TRAILERS,
							CHUNKED_DONE };

			Upload();
			~Upload();

			std::string getUploadStore() const;
			bool isFixed(void) const;
			bool isChunked(void) const;
			ssize_t getMaxBodySize(void) const;
			size_t getContentLength(void) const;

			void setFixed(bool is_fixed_length);
			void setChunked(bool is_chunked);
			void setRaw(bool is_raw);
			void setMultipart(bool is_multipart);
			void setContentLength(size_t contentLength);
			void setBoundary(const std::string& boundary);
			void setUploadStore(const std::string& store);
			void setMaxBodySize(ssize_t size);
			void reset(void);
			
			bool handlePost(int clientFd); // returns true if upload is done, false if more data is needed
			bool handleDelete(int clientFd); // returns true if the full body has been drained, false if more data is needed


		private:
			bool 		m_is_fixed; // if content-length is set
			bool 		m_is_chunked; // if transfer-encoding is set to chunked
			bool 		m_is_raw; // if content-type is application/octet-stream
			bool 		m_is_multipart; // if content-type is multipart/form-data

			UploadState   m_upState;
			ChunkedState  m_chunkState;

			int           m_tempFd;       // current file’s FD or -1
			ssize_t        m_contentLength; // for fixed body length
			std::string   m_boundary;     // for multipart
			std::string   m_tail;         // leftover bytes between reads
			std::string	  m_chunkBuffer; 
			ssize_t        m_bytesWritten; // track size limit
			std::string   m_currentFilename; // for multipart
			
			std::string   m_uploadStore; // where to store the uploaded files
			ssize_t 	  m_maxBodySize; // max body size for the upload

			void 		initializeUpload(void);
			void 		initializeDelete(void);
			std::string createTempFileName(std::string dir);
			void 		renameFile(const std::string& newName);
			void 		deleteTempFile(void);

			bool		handleFixedBody(const char* data, size_t length, bool flag);
			void		parseMultipartHeaders(std::string headers);
			void		parseHeaderLine(const std::string& line);
			void		handleChunkedBody(void);
	};
}

#endif