/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   httpResponse.cpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: anschmit <anschmit@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/10/24 13:59:46 by anschmit          #+#    #+#             */
/*   Updated: 2026/01/09 13:09:04 by anschmit         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "httpResponse.hpp"

HttpResponse::HttpResponse::HttpResponse() : _version(), _code(), _reason(), _headers(), _body(), _cgiState(false){}

HttpResponse::HttpResponse(const HttpResponse &other) : _version(other._version), _code(other._code), _reason(other._reason), _headers(other._headers), _body(other._body), _cgiState(other._cgiState) {}

HttpResponse &HttpResponse::operator=(const HttpResponse &other) {
    if (this != &other)
    {
        _version = other._version;
        _code = other._code;
        _reason = other._reason;
        _headers = other._headers;
        _body = other._body;
        _cgiState = other._cgiState;
    }
    return *this;
}

HttpResponse::~HttpResponse() {}

void HttpResponse::setVersion(const std::string &version) { _version = version; }

void HttpResponse::setStatus(int code) {
    _code = code;
}

void HttpResponse::setReason(const std::string &reason) { _reason = reason; }

void HttpResponse::setHeader(const std::string &name, const std::string &value) { _headers[name] = value; }

void HttpResponse::setBody(const std::string &body) {
    _body = body;
}


std::string HttpResponse::getVersion() const { return _version; }

int HttpResponse::getCode() const { return _code; }

std::string HttpResponse::getReason() const { return _reason; }

std::string HttpResponse::getHeader(const std::string &name) const {
    std::map<std::string, std::string>::const_iterator it = _headers.find(name);
    if (it != _headers.end())
        return it->second;
    return "";
}

std::string HttpResponse::getBody() const { return _body; }

std::string HttpResponse::toString() const {
    std::ostringstream response;

    response << "HTTP/1.1 " << _code << " " << _reason << "\r\n";

	bool hasContentLength = false;
	
    for (std::map<std::string, std::string>::const_iterator it = _headers.begin(); it != _headers.end(); ++it) {
        response << it->first << ": " << it->second << "\r\n";
		if (it->first == "Content-Length")
            hasContentLength = true;
		//response << "Content-Length: " << _body.size() << "\r\n";
	}
	if (!hasContentLength)
        response << "Content-Length: " << _body.size() << "\r\n";
    response << "\r\n";
    response << _body;
    return response.str();
}


void    HttpResponse::setCgiState(bool val) {
    _cgiState = val;
}

bool    HttpResponse::getCgiState() const {
    return _cgiState;
}
