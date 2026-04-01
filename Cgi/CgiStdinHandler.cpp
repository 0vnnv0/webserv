/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   CgiStdinHandler.cpp                                :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: ilazar <ilazar@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/02 14:15:15 by ilazar            #+#    #+#             */
/*   Updated: 2026/01/09 13:38:37 by ilazar           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "CgiStdinHandler.hpp"
#include "Cgi.hpp"
#include "Client.hpp"
#include "Epoll.hpp"
#include "Logger.hpp"
#include <unistd.h>
#include <cstring>
#include <iostream>

CgiStdinHandler::CgiStdinHandler(Cgi *cgi, Client *client, Epoll &ep)
    : _cgi(cgi), _client(client), _ep(ep), _closedFlag(false),
    _expireAt(CGI_TIMEOUT), _timedout(false) {
}

CgiStdinHandler::~CgiStdinHandler() {}

void CgiStdinHandler::handleWrite(time_t now) {
    (void) now;
    if (!_cgi || !_client) {
        handleError();
        return;
    }
    if (_closedFlag)
        return;

    const std::string &body = _cgi->getRequestBody();
    size_t bytesWritten = _cgi->getStdinBytesWritten();
    
    // Check if we've written everything
    if (bytesWritten >= body.size()) {
        _closedFlag = true;
        eraseHandler();
        return;
    }

    // Write as much as we can
    ssize_t written = write(_cgi->getStdinWriteFd(), body.c_str() + bytesWritten, body.size() - bytesWritten);
    if (written < 0) {
        return;
    }
    if (written == 0) {
        handleError();
        return;
    }
    _client->addActivityToQueue(this, now);
    _cgi->setStdinBytesWritten(bytesWritten + written);
    if (_cgi->getStdinBytesWritten() >= body.size()) {
        // All data written, close stdin and remove from epoll
        _closedFlag = true;
        eraseHandler();
    }
}

// Stdin should not attempt read
void CgiStdinHandler::handleRead(time_t now) {
    (void) now;
    handleError();
}

bool CgiStdinHandler::handleError(uint32_t events) {
    (void) events;
    return (handleError());
}

bool CgiStdinHandler::handleError() {
    Logger::error("[fd " + Logger::toString(getFd()) + "] Stdin Cgi triggered EPOLLERR/HUP");
    _closedFlag = true;
    eraseHandler();
    return true;
}

//Remove from epoll, close the stdin pipe and erase from handlers map
void CgiStdinHandler::eraseHandler() {
    int fd = getFd();
    if (fd != -1) {
        _ep.delFd(fd);
        Logger::info("[fd " + Logger::toString(getFd()) + "] Cgi stdin has closed");
        close(fd);
        _cgi->setStdinWriteFd(-1);
    }
    _client->deleteCgiStd(this, fd);
}

void     CgiStdinHandler::updateExpireTime(time_t now) {
    _expireAt = (now + CGI_TIMEOUT);
}

void     CgiStdinHandler::onTimeout() {
    Logger::warn("[fd " + Logger::toString(getFd()) + "] Timeout");
    eraseHandler();
    _cgi->onTimeout();
    _client->cgiTimeout();
}

time_t   CgiStdinHandler::getExpireAt() const {
    return _expireAt;
}

int      CgiStdinHandler::getFd() const {
    return _cgi ? _cgi->getStdinWriteFd() : -1;
}

bool     CgiStdinHandler::isTimedout() {
    return _timedout;
}

void     CgiStdinHandler::setTimedout(bool val) {
    _timedout = val;
}