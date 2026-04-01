/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   httpRequest.hpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: ilazar <ilazar@student.42.de>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/10/24 13:59:53 by anschmit          #+#    #+#             */
/*   Updated: 2026/01/06 15:43:50 by ilazar           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once
#include <string>
#include <map>

class HttpRequest {

	private:
		std::string _method;
		std::string _path;
		std::string _version;
		std::map<std::string, std::string> _headers; //associative container from the STL
		std::string _body;
		std::string _queryString;
		bool _seenContentLength;
		bool _seenContentType;
		bool _seenHost;
		bool _seenTransferEncoding;
		bool _expectContinue;
	
	public:
		HttpRequest();
		HttpRequest(const HttpRequest &other);
		HttpRequest &operator=(const HttpRequest &other);
		~HttpRequest();
		
		void reset();
		bool parseRequestLine(const std::string &line);
		bool parseHeaders(const std::string &headers_block);
		bool parseBody(const std::string &body_block);
		bool parseChunkedBody(const std::string &body_block);
		bool parse(const std::string &raw_request);
		bool check_duplicate(std::string& name);
		void checkExpectContinue();

		
		std::string trim(const std::string &s) const;

		std::string getMethod() const;
		std::string getPath() const;
		std::string getVersion() const;
		std::string getHeader (const std::string &name) const;
		std::string getBody() const;
		std::string getQueryString() const;
		const std::map<std::string, std::string>& getHeaders() const;
		bool 		getExpectContinue() const;
};
