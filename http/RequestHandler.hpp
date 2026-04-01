/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   RequestHandler.hpp                                 :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: nrauh <nrauh@student.42berlin.de>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/10/24 13:59:30 by anschmit          #+#    #+#             */
/*   Updated: 2026/01/09 14:54:53 by nrauh            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once
#include <string>
#include "httpResponse.hpp"
#include "httpRequest.hpp"
#include "ConfigWrapper.hpp"
#include "Cgi.hpp"

class Client;
struct cgiResponse;

enum CgiDetectResult {
    CGI_NO_MATCH,
    CGI_MATCH,
    CGI_NOT_FOUND,
    CGI_FORBIDDEN
};

struct RequestResult {
    enum Type { FULL_RESPONSE, CGI_PENDING, ERROR } type;
	HttpRequest		request;
	HttpResponse	response;
	std::string		rawResponse;

    RequestResult(Type t = FULL_RESPONSE) : type(t) {}
};

class RequestHandler {

    private:
        const ConfigWrapper   &_config;
        const ServerConf      &_serverConf;
        Client                &_client;

        //disable copies
	    RequestHandler          (const RequestHandler &other);
        RequestHandler	        &operator=(const RequestHandler &other);
        
        HttpResponse	        &setErrorResponse(HttpResponse &response, int status, const std::string &reason, const std::string &body, const std::string &contenType = "text/plain");
		HttpResponse	        handleMultipartUpload(const HttpRequest &request, Location location, const std::string &path, const std::string &contentType);
		HttpResponse	        handleRawUpload(const HttpRequest &request, const std::string &contentType);
		HttpResponse	        saveFile(const std::string &filepath, const std::string &content, const std::string &contentType);
        RequestResult		    createErrorResult(int status, const std::string &reason, const std::string &body, const std::string &contentType);
		std::string		        getMIMEtype(const std::string &path);
        std::string		        getPrefferedType(const HttpRequest &request) const;
        bool			        checkAndResolveUploadPath(const std::string &requestPath, std::string &filename, HttpResponse &response, const HttpRequest &request);
		bool			        validateRequestSize(const HttpRequest &request, HttpResponse &response);
        bool                    detectCgi(const Location &location, const std::string &path);

	public:
        RequestHandler(const ConfigWrapper &config, const ServerConf &serverConf, Client &client);
        ~RequestHandler();
        RequestResult		    handle(const std::string &rawRequest);
		HttpResponse		    setRedirect(const HttpRequest &request);
		HttpResponse		    setGet(const HttpRequest &request);
		HttpResponse		    setPost(const HttpRequest &request);
		HttpResponse		    setDelete(const HttpRequest &request);
		HttpResponse			getDirectoryListing(const std::string &filename);
        std::string             getErrorResp(int page);
        static HttpResponse	    httpResponseFromCgi(const cgiResponse &cgi);
    };



