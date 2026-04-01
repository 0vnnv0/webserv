/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   RequestBuffer.hpp                                  :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: ilazar <ilazar@student.42.de>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/10/20 13:33:26 by ilazar            #+#    #+#             */
/*   Updated: 2026/01/02 12:51:50 by ilazar           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

/*
Tries to extract a complete request after the client reads bytes from recv().
If successful, passes the full request to the HTTP parser.
*/
#pragma once
#include <string>
#include <iostream>

#define MAX_BUFF_SIZE 8192 //8kb

class RequestBuffer {

    public:
		static size_t extractContentLength(const std::string& headers);
        static std::string  extractFullRequest(std::string& buffer, bool closedFlag);    
    
    private:
        static size_t       findRequestStart(const std::string& buffer);
        static bool         isChunkedEncoding(const std::string& headers);
        static size_t       findChunkedEnd(const std::string& buffer, size_t bodyStart);
		static std::string	decodeChunkedBody(const std::string& chunkedData, size_t bodyStart);
		static std::string	removeTransferEncoding(const std::string& headers);
		static std::string	addContentLength(const std::string& headers, size_t length);
};