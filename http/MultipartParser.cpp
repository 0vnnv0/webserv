/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   MultipartParser.cpp                                :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: ilazar <ilazar@student.42.de>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/26 16:32:43 by anschmit          #+#    #+#             */
/*   Updated: 2026/01/06 16:09:23 by ilazar           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "MultipartParser.hpp"

MultipartParser::MultipartParser() {}

MultipartParser::MultipartParser(const MultipartParser &other) {(void)&other;}

MultipartParser &MultipartParser::operator=(const MultipartParser &other) {
	(void)other;
	return *this;
}

MultipartParser::~MultipartParser() {}

MultipartParser::MultipartParser(const std::string &contentType, const std::string &body) :_body(body) {
	extractBoundary(contentType);
}

bool MultipartParser::extractBoundary(const std::string &contentType) {
    size_t pos = contentType.find("boundary=");
    if (pos == std::string::npos)
        return false;

    pos += 9; // move past "boundary="

    size_t end = contentType.find(';', pos);
    std::string boundary = contentType.substr(pos, end - pos);

    // Remove optional quotes
    if (!boundary.empty() && boundary[0] == '"')
        boundary.erase(0, 1);
    if (!boundary.empty() && boundary[boundary.size() - 1] == '"')
        boundary.erase(boundary.size() - 1);

    _boundary = "--" + boundary;
    return true;
}


bool MultipartParser::extractFilename(size_t start, std::string &filename) const {
	size_t pos = _body.find("filename=\"", start);
	if (pos == std::string::npos)
		return false;
	size_t end = _body.find("\"", pos + 10);
	if (end == std::string::npos)
		return false;
	filename = _body.substr(pos + 10, end - (pos + 10));
	return true;
}

bool MultipartParser::extractFileContent(size_t start, std::string &fileContent) const {
	size_t headerEnd = _body.find("\r\n\r\n", start);
	if (headerEnd == std::string::npos)
		return false;
	headerEnd += 4;
	size_t end = _body.find(_boundary, headerEnd);
	if (end == std::string::npos)
		end = _body.size();
	size_t contentEnd = end;
	if (contentEnd >= 2 && _body.substr(contentEnd - 2, 2) == "\r\n")
		contentEnd -= 2;

	fileContent = _body.substr(headerEnd, contentEnd - headerEnd);	return true;
}

bool MultipartParser::parse(std::string &filename, std::string &fileContent) const {
    if (_boundary.empty())
        return false;

    size_t start = _body.find(_boundary);
    if (start == std::string::npos) {
        start = _body.find("\r\n" + _boundary);
        if (start == std::string::npos)
            return false;
        start += 2; 
    }

    size_t lineEnd = _body.find("\r\n", start);
    if (lineEnd == std::string::npos)
        return false;
    start = lineEnd + 2;

    if (!extractFilename(start, filename))
        return false;

    if (!extractFileContent(start, fileContent))
        return false;

    return true;
}
