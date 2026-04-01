/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   RequestBuffer.cpp                                  :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: ilazar <ilazar@student.42.de>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/07 15:28:50 by ilazar            #+#    #+#             */
/*   Updated: 2026/01/06 12:18:59 by ilazar           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "RequestBuffer.hpp"
#include <algorithm>
#include <cctype>
#include <iostream>
#include "Logger.hpp"
#include <sstream>

template <typename T>
std::string toStr(const T& value) {
    std::ostringstream oss;
    oss << value;
    return oss.str();
}

// Extracts Content-Length from the headers (if any)
size_t RequestBuffer::extractContentLength(const std::string& headers) {
    std::string lower = headers;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
    const std::string key = "content-length:";
    size_t pos = lower.find(key);
    if (pos == std::string::npos)
        return 0;
    pos += key.size();
    while (pos < lower.size() && std::isspace(lower[pos]))
        pos++;
    size_t end = pos;
    while (end < lower.size() && std::isdigit(lower[end]))
        end++;
    return std::atoi(lower.substr(pos, end - pos).c_str());
}

std::string RequestBuffer::extractFullRequest(std::string& buffer, bool closedFlag) {
	size_t start = findRequestStart(buffer);
    if (start == std::string::npos) {
        if (buffer.size() > MAX_BUFF_SIZE)
            buffer.erase(0, buffer.size() / 2);
        return ""; // no possible request start yet
    }
    if (start > 0)
        buffer.erase(0, start); // remove junk before request
    size_t headers_end = buffer.find("\r\n\r\n");
    if (headers_end == std::string::npos)
        return "";
    // Extract headers to compute body size
    std::string headers = buffer.substr(0, headers_end + 4);
    size_t bodyStart = headers_end + 4;


   if (isChunkedEncoding(headers)) {
        // edge case: empty chunked body (body == "0\r\n\r\n")
        if (buffer.compare(bodyStart, 5, "0\r\n\r\n") == 0) {
            size_t chunkEnd = bodyStart + 5;
            std::string request = buffer.substr(0, chunkEnd);
            buffer.erase(0, chunkEnd);
            return request;
        }
    
        size_t chunkEnd = buffer.find("\r\n0\r\n\r\n", bodyStart);
        if (chunkEnd != std::string::npos) {
            chunkEnd += 7;
            std::string request = buffer.substr(0, chunkEnd);
            buffer.erase(0, chunkEnd);
            return request;
        }
        if (closedFlag) {
            std::string request = buffer;
            buffer.clear();
            return request;
        }
        return "";
    }

    //Content length
    size_t content_length = extractContentLength(headers);
    size_t full_length = bodyStart + content_length;
    if (buffer.size() < full_length) {
        return "";
	}
    // Extract full request
    std::string request = buffer.substr(0, full_length);
    buffer.erase(0, full_length); // remove it from the buffer
    return request;
}

size_t RequestBuffer::findRequestStart(const std::string& buffer) {
    // Look for start of line with uppercase letter (HTTP method)
    size_t start = 0;
    while (start < buffer.size() && (buffer[start] == '\r' || buffer[start] == '\n'))
        start++;
    if (start >= buffer.size())
        return std::string::npos;
    if (!std::isupper(buffer[start]))
        return std::string::npos;
    size_t spacePos = buffer.find(' ', start);
    if (spacePos == std::string::npos)
        return std::string::npos;
    
    return start;
}

size_t RequestBuffer::findChunkedEnd(const std::string& buffer, size_t bodyStart) {
    size_t end = buffer.find("0\r\n\r\n", bodyStart);
    if (end == std::string::npos)
        return std::string::npos;
    return end + 5;
}

bool RequestBuffer::isChunkedEncoding(const std::string& headers) {
    std::string lower = headers;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
    size_t tePos = lower.find("transfer-encoding:");
    if (tePos == std::string::npos)
        return false;
    size_t endLine = lower.find("\r\n", tePos);
    std::string teVal = lower.substr(tePos, endLine - tePos);
    return (teVal.find("chunked") != std::string::npos);
}

std::string RequestBuffer::decodeChunkedBody(const std::string& chunkedData, size_t bodyStart) {
	std::string decoded;
	size_t pos = bodyStart;
	while (pos < chunkedData.size()) {
		size_t lineEnd = chunkedData.find("\r\n", pos);
		if (lineEnd == std::string::npos)
			break;
		std::string sizeStr = chunkedData.substr(pos, lineEnd - pos);
		char* endPtr;
		long chunkSize = std::strtol(sizeStr.c_str(), &endPtr, 16);
		if (chunkSize == 0)
			break;
		pos = lineEnd + 2;
		if (pos + chunkSize <= chunkedData.size()) {
			decoded.append(chunkedData.substr(pos, chunkSize));
		}
		pos += chunkSize + 2;
	}
	return decoded;	
}

std::string RequestBuffer::removeTransferEncoding(const std::string& headers) {
	std::string result = headers;
	std::string lower = headers;
	std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
	
	size_t pos = lower.find("transfer-encoding:");
	if (pos != std::string::npos) {
		size_t lineEnd = headers.find("\r\n", pos);
		if (lineEnd != std::string::npos) {
			result.erase(pos, lineEnd - pos + 2);
		}
	}
	return result;
}

std::string RequestBuffer::addContentLength(const std::string& headers, size_t length) {
	size_t endPos = headers.find("\r\n\r\n");
	if (endPos == std::string::npos)
		return headers;
	std::string result = headers.substr(0, endPos + 2);
    std::ostringstream oss;
    oss << "Content-Length: " << length << "\r\n\r\n";
    result += oss.str();
	return result;
}

