/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Client.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: ilazar <ilazar@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/07 15:28:19 by ilazar            #+#    #+#             */
/*   Updated: 2026/01/09 13:37:18 by ilazar           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Client.hpp"
#include "Logger.hpp"
#include "RequestHandler.hpp"
#include "CgiStdinHandler.hpp"
#include "CgiStdoutHandler.hpp"
#include <sys/epoll.h>
#include <iostream>
#include <sys/socket.h>
#include <unistd.h>
#include <algorithm>

//constructors
Client::Client (int clientFd, Epoll &ep,
    const ConfigWrapper &config, const ServerConf &serverConf, ServerManager &mgr) : 
                _clientFd(clientFd), _ep(ep),
                _config(config), _serverConf(serverConf),
                _mgr(mgr),
                _cgi(NULL), _cgiPending(false),
                _cgiIn(NULL), _cgiOut(NULL),
                _closedFlag(false), _keepAlive(false),
                _expireAt(CLIENT_TIMEOUT), _timedout(false) {};

Client::~Client() {
    if (_cgi || _cgiIn || _cgiOut) {
        cleanCgi();
    } 
    closeClient();
};

//handle write event flow
void    Client::handleWrite(time_t now) {
    bool    activity;
    
    activity = writeEvent();
    if (controlClose())
        return;
    if (isRespBufEmpty())
        _ep.setModNonWritable(_clientFd, this);
    if (isKeepAlive() && activity)
        addActivityToQueue(this, now);
}

//send response data to client returns true if activity happend.
//if (n < 0) stop writing for now. if error - will be caught in main loop
bool    Client::writeEvent() {
    Logger::info("[fd " + Logger::toString(_clientFd) + "] sends Response");
    ssize_t n = send(_clientFd, _respBuf.data(), _respBuf.size(), 0);
    if (n > 0) {
        Logger::debug("[fd " + Logger::toString(_clientFd) + "] Sent: ------>\n" + _respBuf.substr(0, n));
        _respBuf.erase(0, n);
        return true;
    } else if (n == 0) {   //client closed gracefully
        _closedFlag = true;
        return false;
    } else // n < 0
        return false; 
}

// Handle read event flow
void    Client::handleRead(time_t now) {
    bool    completeRequest;
    bool    activity = false;
    
    if (!_closedFlag)
        activity = readEvent();
    if (activity)
        Logger::debug("[fd " + Logger::toString(_clientFd) + "] Read: --- >\n" + _reqBuf);
    completeRequest = ProcessRequest(now);
    if (controlClose()) 
        return;
    if (isKeepAlive())
        if ((activity || completeRequest || _cgiPending))
            addActivityToQueue(this, now);
    if (!isRespBufEmpty()) {
        _ep.setModWritable(_clientFd, this);
    }
}

//read data from client and appends it to the buffer to be later sent
//if n < 0 and there's a real error, socket will trigger EPOLLERR/EPOLLHUP in main loop
bool    Client::readEvent()  {
    char buffer[READ_BUFF_SIZE];
    ssize_t n = recv(_clientFd, buffer, sizeof(buffer), 0);
    if (n > 0) {
        _reqBuf.append(buffer, n);
        return true;
    } else if (n == 0) {
        Logger::debug("[fd " + Logger::toString(_clientFd) + "] sent a FIN");
        _closedFlag = true;
        return false;
    } else // n < 0
        return false;
}


// Closes the client according to received FIN or keepAlive status
// FIN overrides keepAlive
bool   Client::controlClose() {
	if (!_reqBuf.empty() || !isRespBufEmpty())
        return false; 
    if (isRespBufEmpty() && !isCgiPending()) {
        if ( _closedFlag || !isKeepAlive()) {
            closeClient();
            return true;
        }
    }
    return false;
}

//keeps marked closed client in epoll if theres buffer to send - false
//or closes the client - true
bool    Client::handleError() {
    Logger::error("[fd " + Logger::toString(_clientFd) + "] triggered EPOLLERR/HUP");
    if (_respBuf.empty()) {
        _closedFlag = true; //flag as closed and ensure EPOLLOUT
        _ep.setModWritable(_clientFd, this);
        return false;
    }
    _mgr.setToDelete(this);
    return true;
}

//Try to extract full requests and pass them to the Http handler.
//returns true if full request was proccessed
bool    Client::ProcessRequest(time_t now) {
    RequestHandler handler(_config, _serverConf, *this);

    if (_reqBuf.empty())
        return false;
    if (_cgiPending)
            return false;
    bool processedAny = false;
    while (1) {
        if (!checkHugeContent())
            return false;
        std::string newRequest = RequestBuffer::extractFullRequest(_reqBuf, _closedFlag);
        if (newRequest.empty()) {
            break;
        }
        RequestResult res = handler.handle(newRequest);
        switch (res.type) {
        case RequestResult::FULL_RESPONSE:
            Logger::debug("[fd " + Logger::toString(_clientFd) + "] FULL request received");
            // Logger::debug("\n" + newRequest);
            setKeepAlive(res.request);
            _respBuf += res.rawResponse;
            processedAny = true;
            break;
        case RequestResult::ERROR:
            Logger::debug("[fd " + Logger::toString(_clientFd) + "] ERROR request received");
            _respBuf += res.rawResponse;
            _keepAlive = false;
            _reqBuf.clear();
            processedAny = true;
            break;
        case RequestResult::CGI_PENDING:
            Logger::debug("[fd " + Logger::toString(_clientFd) + "] CGI pending request received");
            startCgi(res, now);
            setKeepAlive(res.request);
            processedAny = true;
            return true;
        }
    }
    return processedAny;
}

//remove from epoll, from handlersId list and closes the fd
void    Client::closeClient(void) {
    if (_clientFd >= 0) {
        _mgr.setToDelete(this);
        cleanCgi();
        _ep.delFd(_clientFd);
        _mgr.removeHandlersId(_clientFd);
        Logger::info("[fd " + Logger::toString(_clientFd) + "] has closed");
        close(_clientFd);
        _clientFd = -1;
    }
}

//check HTTP version and connection type. Set keepAlive status accordingly
//return true for keep alive. false for close.
bool   Client::setKeepAlive(HttpRequest  req) {
        std::string version = req.getVersion();
        std::string conType = req.getHeader("Connection");
        // default based on HTTP version
        if (version.find("1.1") != std::string::npos)
            _keepAlive = true;
        else
            _keepAlive = false;
        // check Connection header if it exists
        if (!conType.empty()) {
            std::transform(conType.begin(), conType.end(), conType.begin(), ::tolower);
            if (conType.find("alive") != std::string::npos)
                _keepAlive = true;
            else if (conType.find("close") != std::string::npos)
                _keepAlive = false;
        }
        // if (_keepAlive)
            // Logger::debug("[fd " + Logger::toString(_clientFd) + "] set alive");
        return _keepAlive;
}

//for the special case of Expect continue for example, add the proper respond to the buffer
void    Client::interimResp(const std::string &resp) {
    _respBuf += resp;
    _ep.setModWritable(_clientFd, this); 
}

//Check Content-Length BEFORE extracting full request
//retrun false when aborting, true when content length is ok
bool    Client::checkHugeContent() {
    RequestHandler reqHan(_config, _serverConf, *this);
    size_t headers_end = _reqBuf.find("\r\n\r\n");
    if (headers_end != std::string::npos) {
        std::string headers = _reqBuf.substr(0, headers_end + 4);
        size_t content_length = RequestBuffer::extractContentLength(headers);
        if (content_length > static_cast<size_t>(_serverConf.clientMaxBodySize)) {
            _respBuf += reqHan.getErrorResp(413);
            _keepAlive = false;
            _reqBuf.clear();
            return false;
        }
    }
    return true;
}

//Start cgi process
void    Client::startCgi(const RequestResult &res, time_t now) {
    if (_cgi) {
        Logger::warn("CGI already running");
        return;
    }
    _cgi = new Cgi(res, _ep, _config, _serverConf);
    int status = _cgi->checkCgiFile();
    if (status == 404) {
        cgiError(404);
        return;
    } else if (status == 403) {
        cgiError(403);
        return;
    }
    
    if (!_cgi->start()) {
         Logger::error("Failed to start CGI");
        cgiError(500);
        cleanCgi();
        return;
    }
    _cgiIn = new CgiStdinHandler(_cgi, this, _ep);
    _cgiOut = new CgiStdoutHandler(_cgi, this, _ep);
    _mgr.addHandlersId(_cgiIn, _cgiIn->getFd());
    _mgr.addHandlersId(_cgiOut, _cgiOut->getFd());
    if (!_ep.addFd(_cgi->getStdinWriteFd(), EPOLLOUT, _cgiIn)) {
        cgiError(500);
        cleanCgi();
        return;
    }
    if (!_ep.addFd(_cgi->getStdoutReadFd(), EPOLLIN, _cgiOut)) {
        cgiError(500);
        cleanCgi();
        return;
    }
    addActivityToQueue(_cgiIn, now);
    addActivityToQueue(_cgiOut, now);
    _cgiPending = true;
}


void Client::handleCgiOutput(const std::string& output) {
    cgiResponse cgiRes;
    
    _cgiPending = false;
    if (_cgi && !_cgi->checkChildStatus())
        cgiError(502);
    else {
        cgiRes = Cgi::parseRespone(cgiRes, output);
        if (cgiRes.headers.empty() && cgiRes.body.empty()) {
            cgiError(500);
        } else {
            HttpResponse httpRes = RequestHandler::httpResponseFromCgi(cgiRes);
            _respBuf += httpRes.toString();
        }
    }
    if (controlClose())
        return;
    if (!_respBuf.empty()) {
        _ep.setModWritable(_clientFd, this);
    }
}

void    Client::stdinDelete(int id) {
    _mgr.removeHandlersId(id);
    if (_cgiIn) {
        _mgr.setToDelete(_cgiIn);
        _cgiIn = NULL;
    }
}

// Remove cgi handler from id map, and set it to the deletion list
void    Client::deleteCgiStd(IEventHandler *handler, int id) {
    _mgr.removeHandlersId(id);
    if (handler == _cgiOut && _cgiOut != NULL) {
        _mgr.setToDelete(_cgiOut);
        _cgiOut = NULL;
    }
    else if (handler == _cgiIn && _cgiIn != NULL) {
        _mgr.setToDelete(_cgiIn);
        _cgiIn = NULL;
    }
    if (!_cgiIn && !_cgiOut)
        _cgiPending = false;
}

void    Client::stdoutDelete(int id) {
    _mgr.removeHandlersId(id);
    if (_cgiOut) {
        _mgr.setToDelete(_cgiOut);
        _cgiOut = NULL;
    }
}

// Deleting a cgi object if exsits
void Client::cleanCgi(void) {
    if (_cgiIn)
    _cgiIn->eraseHandler();
    if (_cgiOut)
    _cgiOut->eraseHandler();
    if (_cgi && !_cgiIn && !_cgiOut) {
        delete _cgi;
        _cgi = NULL;
    }
    _cgiPending = false;
}

//When cgi timesout, clean cgi and send proper response
void    Client::cgiTimeout() {    
    cleanCgi();
    cgiError(504);
}

bool    Client::handleError(uint32_t events) {
    (void) events;
    return (handleError());
}

void    Client::addActivityToQueue(IEventHandler *h, time_t now) {
    _mgr.addActivityToQueue(h, now);
}


void    Client::cgiError(int page) {
    RequestHandler reqHan(_config, _serverConf, *this);
    
    _respBuf += reqHan.getErrorResp(page);
    _ep.setModWritable(_clientFd, this);
    _cgiPending = false;
}

// Update every time client implies it's still alive
void    Client::updateExpireTime(time_t now) {
    _expireAt = (now + CLIENT_TIMEOUT);
}

void    Client::onTimeout() {
    RequestHandler reqHan(_config, _serverConf, *this);
    
    Logger::warn("[fd " + Logger::toString(_clientFd) + "] Timeout");
    _respBuf += reqHan.getErrorResp(408);
    _ep.setModWritable(_clientFd, this);
    _reqBuf.clear();
    _closedFlag = true;
}

//Setters Getters
bool    Client::closedFlag (void) const {
    return (_closedFlag);
}

void    Client::setClosed (bool val) {
    _closedFlag = val;
}

bool    Client::isKeepAlive(void) const {
    return _keepAlive;
}

std::string&    Client::reqBuf(void) {
    return _reqBuf;
}

std::string&    Client::respBuf(void) {
    return _respBuf;
}

bool    Client::isRespBufEmpty(void) const {
    return _respBuf.empty();
}

int     Client::getFd(void) const {
    return _clientFd;
}

time_t  Client::getExpireAt(void) const {
    return _expireAt;
}

bool    Client::isCgiPending() {
    return _cgiPending;
}

void    Client::setToDelete(IEventHandler *handler) {
    _mgr.setToDelete(handler);
}

bool    Client::isTimedout() {
    return _timedout;
}

void    Client::setTimedout(bool val) {
    _timedout = val;
}
