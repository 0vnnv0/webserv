/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   httpRequest.cpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: anschmit <anschmit@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/10/24 14:00:01 by anschmit          #+#    #+#             */
/*   Updated: 2026/01/09 14:33:01 by anschmit         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "httpRequest.hpp"
#include <sstream>
#include <stdlib.h>
#include <iostream>

HttpRequest::HttpRequest() : _method(), _path(), _version(), _headers(), _body(), _queryString(), _seenContentLength(false), _seenContentType(false), _seenHost(false),
      _seenTransferEncoding(false), _expectContinue(false) {}

HttpRequest::HttpRequest(const HttpRequest &other) : _method(other._method), _path(other._path), _version(other._version), _headers(other._headers), _body(other._body), _queryString(other._queryString), _expectContinue(other._expectContinue) {}

HttpRequest &HttpRequest::operator=(const HttpRequest &other) {
	if (this != &other) {
		_method = other._method;
		_path = other._path;
		_version = other._version;
		_headers = other._headers;
		_body = other._body;
		_queryString = other._queryString;
		_expectContinue = other._expectContinue;
	}
	return *this;
}

HttpRequest::~HttpRequest() {}

void HttpRequest::reset() {
    _method.clear();
    _path.clear();                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                 
    _version.clear();
    _headers.clear();
    _body.clear();

    _seenContentLength = false;
    _seenContentType = false;
    _seenHost = false;
    _seenTransferEncoding = false;

	_expectContinue = false;
}

bool HttpRequest::parseRequestLine(const std::string &line) {
	std::istringstream iss(line);

	if (!(iss >> _method >> _path >> _version)) {
		return false;
	}

	if (_version.empty()) {
		return false;
	}
	size_t questionMark = _path.find('?');
	if (questionMark != std::string::npos) {
		_queryString = _path.substr(questionMark + 1);
		_path = _path.substr(0, questionMark);
	}
	if (_path.find('\0') != std::string::npos) {
		_path = "";
	}
	if (_version.size() != 8 || _version.substr(0,5) != "HTTP/" || !isdigit(_version[5]) || _version[6] != '.' || !isdigit(_version[7])) {
		return false;
	}
	return true;
}

std::string HttpRequest::trim(const std::string &s) const {
	const std::string	ws = " \t";
	size_t				start = s.find_first_not_of(ws);

	if (start == std::string::npos) { //no whitespaces found
		return ""; //empty string
	}
	size_t end = s.find_last_not_of(ws);
	return s.substr(start, end - start + 1);
}

bool HttpRequest::parseHeaders(const std::string &headers_block) {
    std::istringstream stream(headers_block);
    std::string line;

    while (std::getline(stream, line)) { // removes the \n
        if (!line.empty() && line[line.size() - 1] == '\r') {
            line.erase(line.size() - 1);
        }
        if (line.empty()) { // end of headers
            break;
        }

        size_t colon = line.find(':');
        if (colon == std::string::npos) {
            return false;
        }

        std::string name = trim(line.substr(0, colon));
        std::string value = trim(line.substr(colon + 1));

        // Normalize header name: lowercase + underscores → dashes
        for (size_t i = 0; i < name.size(); ++i) {
            name[i] = std::tolower(name[i]);
            if (name[i] == '_') name[i] = '-';
        }
		if (!check_duplicate(name))
			return false;
        _headers[name] = value;
    }
    return true;
}

bool HttpRequest::check_duplicate(std::string& name) {

	if (name == "content-length") {
		if (_seenContentLength) {
			return false;
		}
		_seenContentLength = true;
	}
	else if (name == "content-type") {
		if (_seenContentType) {
			return false;
		}
		_seenContentType = true;
	}
	else if (name == "host") {
		if (_seenHost){
			return false;
		}
		_seenHost = true;
	}
	else if (name == "transfer-encoding") {
		if (_seenTransferEncoding){
			return false;
		}
		_seenTransferEncoding = true;
	}
	return true;
}

bool HttpRequest::parseChunkedBody(const std::string &body_block) {
	size_t pos = 0;
	while (pos < body_block.size()) {
		size_t rn = body_block.find("\r\n", pos);
		if (rn == std::string::npos){
				return false;
			}

		std::string size_str = body_block.substr(pos, rn - pos);
		std::istringstream iss(size_str);
		int chunk_size;
		iss >> std::hex >> chunk_size;
		if (chunk_size < 0 || iss.fail()){
				return false;
			}
		pos = rn + 2;
		if (chunk_size == 0) {
			if (pos + 2 <= body_block.size() && body_block.substr(pos, 2) == "\r\n") 
				return true;
			else {
				return false;
			}
		}

		if (pos + static_cast<size_t>(chunk_size) > body_block.size()){
				return false;
			}

		std::string chunk_data = body_block.substr(pos, chunk_size);
		_body += chunk_data;
		pos += chunk_size;

		if (pos + 2 > body_block.size() || body_block.substr(pos, 2) != "\r\n")
			return false;
		pos += 2;
	}
	return false;
}

bool HttpRequest::parseBody(const std::string &body_block) {

	std::string clHeader = getHeader("Content-Length");
	std::string teHeader = getHeader("Transfer-Encoding");
	// Reject ONLY if Transfer-Encoding is chunked AND Content-Length is present
	if (!clHeader.empty() && !teHeader.empty()) {
    std::string teLower = teHeader;
    for (size_t i = 0; i < teLower.size(); ++i)
        teLower[i] = std::tolower(teLower[i]);
    if (teLower == "chunked")
        return false;
	}

	std::map<std::string, std::string>::const_iterator te = _headers.find("transfer-encoding");
	if (te != _headers.end() && te->second == "chunked") {
		return parseChunkedBody(body_block);
	}
	std::map<std::string, std::string>::const_iterator it = _headers.find("content-length");
	if (it == _headers.end()) {
		_body.clear();
		return true;
	}
	int length = std::atoi(it->second.c_str());
	if (length < 0) {
		return false; // invalid Content-Length
    }

	if (static_cast<size_t>(length) > body_block.size()) {
        return false; // incomplete body
    }
	_body = body_block.substr(0, length);
	return true;
}

void	HttpRequest::checkExpectContinue() {
	std::string expect = getHeader("Expect");
	if (!expect.empty()) {
		for (size_t i = 0; i < expect.size(); ++i)
			expect[i] = std::tolower(expect[i]);

		if (expect == "100-continue") {
			_expectContinue = true;
		}
    }
}

bool HttpRequest::parse(const std::string &raw_request) {
	size_t pos = raw_request.find("\r\n\r\n");
	if (pos == std::string::npos) {
		return false;
	}
	std::string head = raw_request.substr(0, pos);
	std::string body = raw_request.substr(pos + 4); //wir skippen \r\n\r\n
	std::istringstream	stream(head);
	std::string			request_line;
	if (!std::getline(stream, request_line)) {
		return false;
	}
	if (!request_line.empty() && request_line[request_line.size() - 1] == '\r') {
		request_line.erase(request_line.size() - 1);
	}
	if (!parseRequestLine(request_line)) {
		return false;
	}
	std::string headers_block;
	std::string line;
	while (std::getline(stream, line)) {
		headers_block += line + "\n";
	}
	if (!parseHeaders(headers_block)) {
		return false;
	}
	checkExpectContinue();
	if (!parseBody(body)) {
		return false;
	}
	return true;
}

std::string HttpRequest::getMethod() const { return _method; }

std::string HttpRequest::getPath() const { return _path; }

std::string HttpRequest::getVersion() const { return _version; }

std::string HttpRequest::getHeader(const std::string &name) const {
    std::string lname = name;
    for (size_t i = 0; i < lname.size(); ++i)
        lname[i] = std::tolower(lname[i]);
    
    for (std::map<std::string,std::string>::const_iterator it = _headers.begin(); it != _headers.end(); ++it) {
        std::string key = it->first;
        for (size_t i = 0; i < key.size(); ++i)
            key[i] = std::tolower(key[i]);
        if (key == lname)
            return it->second;
    }
    return "";
}


std::string HttpRequest::getBody() const { return _body; }

std::string HttpRequest::getQueryString() const {return _queryString; }

const std::map<std::string, std::string>& HttpRequest::getHeaders() const {
    return _headers;
}

bool HttpRequest::getExpectContinue() const {
	return _expectContinue;
}
