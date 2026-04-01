/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   RequestHandler.cpp                                 :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: ilazar <ilazar@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/10/24 13:59:18 by anschmit          #+#    #+#             */
/*   Updated: 2026/01/09 15:21:41 by ilazar           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "RequestHandler.hpp"
#include "httpRequest.hpp"
#include "httpResponse.hpp"
#include "MultipartParser.hpp"
#include "Cgi.hpp"
#include "Client.hpp"
#include <iostream>
#include <unistd.h>
#include <sys/socket.h>
#include <fstream>
#include "MultipartParser.hpp"
#include <fcntl.h>
#include <dirent.h>

RequestHandler::RequestHandler(const ConfigWrapper &config, const ServerConf &serverConf, Client &client) : 
_config(config), _serverConf(serverConf), _client(client) {}

RequestHandler::~RequestHandler()  {}

HttpResponse RequestHandler::setGet(const HttpRequest &request) {
    HttpResponse response;
	
    std::string path = request.getPath();
    Location location = _config.findLocation(_serverConf, path);
    std::string root = _config.getRoot(_serverConf, location);
    std::string filename = _config.buildFilePath(_serverConf, location, path);
	
	if (detectCgi(location, request.getPath())) {
        response.setCgiState(true);
        return response;
    }
	
	struct stat st;
	if (stat(filename.c_str(), &st) == 0 && S_ISDIR(st.st_mode)) {
		if (path[path.length() - 1] != '/') {
			response.setStatus(301);
			response.setReason("Moved Permanently");
			response.setHeader("Location", path + "/");
			response.setBody("Redirect");
			response.setHeader("Content-Length", "8");
			response.toString();
			return response;
		}

    std::string index = _config.getIndex(_serverConf, location);
    if (!index.empty()) {
        std::string indexPath = filename;
        if (indexPath[indexPath.length() - 1] != '/')
            indexPath += "/";
        indexPath += index;

        std::ifstream indexFile(indexPath.c_str(), std::ios::binary);
        if (indexFile.is_open()) {
            std::stringstream buffer;
            buffer << indexFile.rdbuf();
            response.setStatus(200);
            response.setReason("OK");
            response.setHeader("Content-Type", getMIMEtype(indexPath));
            response.setBody(buffer.str());
            return response;
        }
    }
	if (_config.getAutoindex(location)) {
		return getDirectoryListing(filename);
	}
    return setErrorResponse(response, 403, "Forbidden", "Directory listing not allowed", getPrefferedType(request));
	}

    std::ifstream file(filename.c_str(), std::ios::binary);
    if (!file.is_open())
        return setErrorResponse(response, 404, "Not Found", "File not found", getPrefferedType(request));

    std::stringstream buffer;
    buffer << file.rdbuf();
    response.setStatus(200);
    response.setReason("OK");
    response.setHeader("Content-Type", getMIMEtype(filename));
    response.setBody(buffer.str());
    return response;
}

HttpResponse RequestHandler::setPost(const HttpRequest &request) {
    HttpResponse response;
    std::string contentType = getPrefferedType(request);
    std::string path = request.getPath();
    Location location = _config.findLocation(_serverConf, path);

    if (std::find(location.methods.begin(), location.methods.end(), "POST") == location.methods.end())
        return setErrorResponse(response, 405, "Method Not Allowed", "POST not allowed", contentType);

    if (detectCgi(location, request.getPath())) {
        response.setCgiState(true);
        return response;
    }
    
    std::string uploadPath = _config.getUploadPath(location);
    if (!uploadPath.empty()) {
        std::string requestContentType = request.getHeader("Content-Type");
        
        if (requestContentType.find("multipart/form-data") != std::string::npos)
            return handleMultipartUpload(request, location, path, contentType);
        
        return handleRawUpload(request, contentType);
    }

    response.setStatus(200);
    response.setReason("OK");
    response.setHeader("Content-Type", "text/plain");
    response.setBody("POST received");
    return response;
}

HttpResponse RequestHandler::handleMultipartUpload(const HttpRequest &request, Location location, const std::string &path, const std::string &contentType) {
    HttpResponse response;
    std::string requestContentType = request.getHeader("Content-Type");
    
    MultipartParser parser(requestContentType, request.getBody());
    
    std::string targetFilename = _config.buildFilePath(_serverConf, location, path);
    std::string uploadFilename;
	std::string fileContent;
    if (!parser.parse(uploadFilename, fileContent))
    return setErrorResponse(response, 400, "Bad Request", "Invalid multipart data", contentType);
    
    struct stat st;
    if (stat(targetFilename.c_str(), &st) == 0 && S_ISDIR(st.st_mode)) {
        if (!targetFilename.empty() && targetFilename[targetFilename.length() - 1] != '/')
            targetFilename += "/";
        targetFilename = targetFilename + uploadFilename;
    }
    if (stat(targetFilename.c_str(), &st) == 0) {
            // File exists - check if we can write to it
        if (access(targetFilename.c_str(), W_OK) != 0) {
            return setErrorResponse(response, 403, "Forbidden", "No permission to write file", contentType);
        }
	}
	HttpResponse saveResponse = saveFile(targetFilename, fileContent, contentType);
    
    if (saveResponse.getCode() == 201)
        saveResponse.setBody("File uploaded successfully: " + targetFilename);
    
    return saveResponse;
}

HttpResponse RequestHandler::handleRawUpload(const HttpRequest &request,
                                              const std::string &contentType) {
    HttpResponse response;
    std::string filename;
    
    if (!checkAndResolveUploadPath(request.getPath(), filename, response, request)) {
        return response;
    }
    
	// Check if the resolved path is a directory
	struct stat s;
    if (stat(filename.c_str(), &s) == 0 && S_ISDIR(s.st_mode)) {
        if (!filename.empty() && filename[filename.length() - 1] != '/') {
            filename += "/";
        }
        filename += "raw_chunked_upload.txt"; 
    }
    return saveFile(filename, request.getBody(), contentType);
}

HttpResponse RequestHandler::saveFile(const std::string &filepath, const std::string &content, const std::string &contentType) {
    HttpResponse response;
    struct stat st;
    int fd;

    // Check if file exists
    if (stat(filepath.c_str(), &st) == 0) {
        //check write permissions first
        if (!(st.st_mode & S_IWUSR)) {
            return setErrorResponse(response, 403, "Forbidden", "No permission to write file", contentType);
        }
        
        // File exists → open without O_CREAT or O_TRUNC
        fd = open(filepath.c_str(), O_WRONLY);
        if (fd < 0) {
            if (errno == EACCES){
				std::cout << "TESTSTETTTS 3333" << std::endl;
				return setErrorResponse(response, 403, "Forbidden", "No permission to write file", contentType);
			}
			return setErrorResponse(response, 500, "Internal Server Error", "Cannot open file", contentType);
        }
    } else {
        // File does not exist → create it
        fd = open(filepath.c_str(), O_WRONLY | O_CREAT, 0666);
        if (fd < 0)
            return setErrorResponse(response, 500, "Internal Server Error", "Cannot create file", contentType);
    }

    // Write content to file
    ssize_t written = write(fd, content.c_str(), content.size());
    close(fd);

    if (written < 0 || (size_t)written != content.size()) {
        return setErrorResponse(response, 500, "Internal Server Error", "Failed to write file", contentType);
	}
	
    response.setStatus(201);
    response.setReason("Created");
    response.setHeader("Content-Type", "text/plain");
    response.setBody("File uploaded successfully");
    return response;
}

HttpResponse RequestHandler::setDelete(const HttpRequest &request) {
    HttpResponse response;
    std::string contentType = getPrefferedType(request);
    std::string path = request.getPath();
    Location location = _config.findLocation(_serverConf, path);
    
    // 1. ZUERST: Methode prüfen
    if (std::find(location.methods.begin(), location.methods.end(), "DELETE") == location.methods.end())
        return setErrorResponse(response, 405, "Method Not Allowed", "DELETE not allowed", contentType);
    
    // 2. DANACH: Pfad konstruieren und Datei prüfen
    std::string filename = _config.buildFilePath(_serverConf, location, path);

    // 3. Path traversal prüfen
    if (path.find("..") != std::string::npos) {
		return setErrorResponse(response, 403, "Forbidden", "Invalid path", contentType);
	}
    // 4. Datei existiert?
    std::ifstream file(filename.c_str());
    if (!file.good())
        return setErrorResponse(response, 404, "Not Found", "File does not exist", contentType);
    file.close();

    // 5. Löschen
    if (std::remove(filename.c_str()) != 0)
        return setErrorResponse(response, 500, "Internal Server Error", "Failed to delete file", contentType);

    response.setStatus(204);
    response.setReason("No Content");
    return response;
}

HttpResponse RequestHandler::getDirectoryListing(const std::string &filename) {
	HttpResponse response;

	DIR *dir = opendir(filename.c_str());
	if (dir == NULL) {
		return setErrorResponse(response, 403, "Forbidden", "Cannot open directory", "text/html");
	}
	
	std::stringstream body;
	body << "<html><body><pre>\n";

	
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        std::string name(entry->d_name);

        if (name == "." || name == "..")
            continue;

        body << name << "\n";
    }

    closedir(dir);

    body << "</pre></body></html>\n";

    response.setStatus(200);
    response.setReason("OK");
    response.setHeader("Content-Type", "text/html");
    response.setBody(body.str());

    return response;
}

bool RequestHandler::validateRequestSize(const HttpRequest &request, HttpResponse &response) {
    // Check Content-Length header
    std::string contentLengthStr = request.getHeader("Content-Length");
    if (!contentLengthStr.empty()) {
        std::stringstream ss(contentLengthStr);
        unsigned long contentLength;
        ss >> contentLength;
        if (contentLength > static_cast<unsigned long>(_serverConf.clientMaxBodySize)) {
            response = setErrorResponse(response, 413, "Payload Too Large", "Payload too large", getPrefferedType(request));
            return false;
        }
    }
    
    // Also check actual body size (for chunked or if Content-Length was wrong)
    if (request.getBody().size() > static_cast<size_t>(_serverConf.clientMaxBodySize)) {
		response = setErrorResponse(response, 413, "Payload Too Large", "Payload too large", getPrefferedType(request));
        return false;
    }
    return true;
}

HttpResponse RequestHandler::setRedirect(const HttpRequest &request) {
	HttpResponse response;
	std::string contentType = getPrefferedType(request);
    std::string path = request.getPath();
    Location loc = _config.findLocation(_serverConf, path);
	
    std::string redirectUrl = _config.getReturnTarget(loc);
    int redirectCode = _config.getLocationStatusCode(loc);

    if (redirectCode == 0 || redirectCode < 300 || redirectCode >= 400)
        redirectCode = 301;

    response.setStatus(redirectCode);
	response.setReason("Moved Permanently");
    response.setHeader("Location", redirectUrl);
	response.setHeader("Content-Type", "text/html");
    response.setBody("Redirect");
    response.setHeader("Content-Length", "8");
    return response;
}

RequestResult RequestHandler::handle(const std::string &rawRequest) {
    RequestResult result;
	
	result.request.reset();
    if (!result.request.parse(rawRequest)) {
		return createErrorResult(400, "Bad Request", "Malformed request", "text/html");
    }

	if (!validateRequestSize(result.request, result.response)) {
		result.type = RequestResult::ERROR;
		result.rawResponse = result.response.toString();
		return result;
	}
       
	if (result.request.getVersion() != "HTTP/1.1" && result.request.getVersion() != "HTTP/1.0") {
		return createErrorResult(505, "HTTP Version Not Supported", "HTTP Version not supported", "text/html");
	}
    std::string teHeader = result.request.getHeader("Transfer-Encoding");
    for (size_t i = 0; i < teHeader.size(); ++i) {
        teHeader[i] = std::tolower(teHeader[i]);
    }
    bool hasTE = !teHeader.empty();
    bool isChunked = false;
    if (hasTE) {
        if (teHeader.find(',') != std::string::npos){
            return createErrorResult(501, "Not Implemented", "Multiple Transfer-Encoding values not supported", "text/html");
		}
        if (teHeader != "chunked"){
            return createErrorResult(501, "Not Implemented", "Unsupported Transfer-Encoding", "text/html");
		}
        isChunked = true;
    }
    // POST must have Content-Length or chunked
	const std::string &method = result.request.getMethod();
    if (method == "POST") {
        std::string cl = result.request.getHeader("Content-Length");
        if ((cl.empty() || cl == "0") && !isChunked) {
			return createErrorResult(411, "Length Required", "POST requests must include Content-Length or Tranfer-Encoding: chunked", "text/html");
        }
    }

    if (result.request.getExpectContinue()) {
        _client.interimResp("HTTP/1.1 100 Continue\r\n\r\n");
    }
    
	Location loc = _config.findLocation(_serverConf, result.request.getPath());
	if (!loc.isValid) {
		return createErrorResult(404, "Not Found", "Not found", "text/html");
	}
	std::string redirectUrl = _config.getReturnTarget(loc);
    if (!redirectUrl.empty()) {
        result.response = setRedirect(result.request);
        result.rawResponse = result.response.toString();
        return result;
    }
	
    if (method != "GET" && method != "POST" && method != "DELETE") {
		return createErrorResult(501, "Not Implemented", "Not implemented", "text/html");
    }
	
	std::vector<std::string> allowed = _config.getHTTPMethods(loc);
    if (!allowed.empty() &&
        std::find(allowed.begin(), allowed.end(), result.request.getMethod()) == allowed.end()) {
        result.response.setStatus(405);
        result.response.setReason("Method Not Allowed");
        result.response.setHeader("Content-Type", "text/plain");

        std::string allowHeader;
        for (size_t i = 0; i < allowed.size(); ++i) {
            if (i) allowHeader += ", ";
            allowHeader += allowed[i];
        }
        result.response.setHeader("Allow", allowHeader);
        result.response.setBody("Method not allowed");
        result.type = RequestResult::ERROR;
        result.rawResponse = result.response.toString();
        return result;
    }
    // const std::string &method = result.request.getMethod();
    if (method == "GET")
        result.response = setGet(result.request);
    else if (method == "POST")
        result.response = setPost(result.request);
    else if (method == "DELETE")
        result.response = setDelete(result.request);

    if (result.response.getCgiState())
        result.type = RequestResult::CGI_PENDING;
    else
        result.rawResponse = result.response.toString();
    return result;
}

std::string RequestHandler::getMIMEtype(const std::string &path) {
	size_t dot = path.find_last_of('.');
	
	if (dot == std::string::npos)
		return "application/octet-stream";

	std::string end = path.substr(dot + 1);
	if (end == "html") 
		return ("text/html");
	if (end == "css")
		return("text/css");
	if (end == "png")
		return ("image/png");
	if (end == "jpeg")
		return ("image/jpeg");
	if (end == "jpg")
		return ("image/jpg");
	if (end == "js")
		return ("application/javascript");
	if (end == "pdf")
		return ("application/pdf");
	if (end == "mp3")
		return ("audio/mpeg");
	if (end == "txt")
		return ("text/plain");
	if (end  == "json")
		return ("application/json");
	if (end == "ico")
		return ("image/x-icon");
	return ("application/octet-stream");
}

std::string RequestHandler::getPrefferedType(const HttpRequest &request) const {
	std::string accept = request.getHeader("Accept");
	std::string agent = request.getHeader("User-Agent");

	if (accept.find("text/html") != std::string::npos || 
		accept.find("application/xhtml+xml") != std::string::npos)
		return ("text/html");

	if (agent.find("Mozilla") != std::string::npos || 
		agent.find("Chrome") != std::string::npos || 
		agent.find("Edge") != std::string::npos)
		return ("text/html");

	if (accept.find("application/json") != std::string::npos)
		return ("application/json");

	return "text/plain";
}

RequestResult RequestHandler::createErrorResult(int status, const std::string &reason, const std::string &body, const std::string &contentType) {
	RequestResult result;
	setErrorResponse(result.response, status, reason, body, contentType);
	result.type = RequestResult::ERROR;
	result.rawResponse = result.response.toString();
	return result;	
}

HttpResponse &RequestHandler::setErrorResponse(HttpResponse &response, int status, const std::string &reason, const std::string &body, const std::string &contentType) {
	response.setStatus(status);
	response.setReason(reason);
	response.setHeader("Content-Type", contentType);

	std::string errorPagePath = _config.findErrorPage(_serverConf, status);
	if (!errorPagePath.empty()) {
		std::ifstream file(errorPagePath.c_str(), std::ios::binary);
		if (file.is_open()) {
			std::stringstream buffer;
			buffer << file.rdbuf();
			response.setBody(buffer.str());
			response.setHeader("Content-Type", getMIMEtype(errorPagePath));
			return response;
		}
	}
	if (contentType == "text/html") {
		std::stringstream html;      
		html << "<!DOCTYPE html>\n"
		<< "<html>\n"
		<< "<head>\n"
		<< "<title>" << status << " " << reason << "</title>\n"
		<< "</head>\n"
		<< "<body>\n"
        << "<center><h1>" << status << " " << reason << "</h1></center>\n"
        << "<hr>\n"
        << "<center>" << body << "</center>\n"
        << "</body>\n"
        << "</html>\n";
		response.setBody(html.str());
	}
	else {
		response.setBody(body);
	}
	return response;
}

bool RequestHandler::checkAndResolveUploadPath(const std::string &requestPath,
                                               std::string &resolvedFilename,
                                               HttpResponse &response,
                                               const HttpRequest &request)
{
    Location location = _config.findLocation(_serverConf, requestPath);
    std::string uploadDir = _config.getUploadPath(location);

    if (uploadDir.empty()) {
        resolvedFilename.clear();
        response = setErrorResponse(response, 400, "Bad Request", "No upload directory configured", getPrefferedType(request));
        return false;
    }

    std::string filename = _config.buildFilePath(_serverConf, location, requestPath);
	
    if (filename.find("..") != std::string::npos) {
			std::cout << "TESTSTETTTS 6666" << std::endl;

        response = setErrorResponse(response, 403, "Forbidden", "Invalid filename", getPrefferedType(request));
        return false;
    }

    resolvedFilename = filename;
    return true;
}


HttpResponse RequestHandler::httpResponseFromCgi(const cgiResponse &cgi)
{
    HttpResponse response;
    response.setVersion("HTTP/1.1");

    std::map<std::string, std::string>::const_iterator it =
        cgi.headers.find("Status");

    if (it != cgi.headers.end())
    {
        std::istringstream iss(it->second);
        int code;
        std::string reason;

        iss >> code;
        std::getline(iss, reason);
        if (!reason.empty() && reason[0] == ' ')
            reason.erase(0, 1);

        response.setStatus(code);
        response.setReason(reason);
    }
    else
    {
        response.setStatus(200);
        response.setReason("OK");
    }

    for (std::map<std::string, std::string>::const_iterator h = cgi.headers.begin();
         h != cgi.headers.end(); ++h)
    {
        if (h->first == "Status" || h->first == "Content-Length")
            continue;

        response.setHeader(h->first, h->second);
    }
    response.setBody(cgi.body);
    response.setHeader("Content-Type", "text/plain");
    std::ostringstream len;
    len << cgi.body.size();
    return response;
}

//returns true if cgi was found
bool RequestHandler::detectCgi(const Location &location, const std::string &path)
{
    const std::string &cgiExt = _config.getCGIExtension(location);
    if (cgiExt.empty())
        return false;

    size_t pos = path.find(cgiExt);
    if (pos == std::string::npos)
        return false;

    if (pos + cgiExt.length() == path.length() || path[pos + cgiExt.length()] == '/')
        return true;
    return false;
}

std::string   RequestHandler::getErrorResp(int page) {
    std::string rawResp;
    HttpResponse resp;
    
    switch (page) {
        case 100:
            rawResp = "HTTP/1.1 100 Continue\r\n\r\n";
            return rawResp;
        case 413:
            resp = setErrorResponse(resp ,413, "Payload Too Large", "413 Payload Too Large", "text/plain");
            break;
        case 403:
            resp = setErrorResponse(resp ,403, "Forbidden", "403 Forbidden", "text/html");
            break;
        case 404:
            resp = setErrorResponse(resp ,404, "Not Found", "404 Not Found", "text/html");
            break;
        case 502:
            resp = setErrorResponse(resp ,502, "Bad Gateway", "502 Bad Gateway", "text/html");
            break;
        case 408:
            resp = setErrorResponse(resp ,408, "Request Timeout", "408 Request Timeout", "text/html");
            break;
        case 504:
            resp = setErrorResponse(resp ,504, "Gateway Timeout", "504 Gateway Timeout", "text/html");
            break;
        default:
            resp = setErrorResponse(resp ,500, "Internal Server Error", "500 Internal Server Error", "text/html");
        }
    return resp.toString();
}
