/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   CgiStdoutHandler.cpp                               :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: ilazar <ilazar@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/02 15:00:23 by ilazar            #+#    #+#             */
/*   Updated: 2026/01/08 14:42:47 by ilazar           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "CgiStdoutHandler.hpp"
#include "Cgi.hpp"
#include "Client.hpp"
#include "Epoll.hpp"
#include "Logger.hpp"
#include <unistd.h>
#include <cstring>
#include <iostream>
#include <sys/epoll.h>


CgiStdoutHandler::CgiStdoutHandler(Cgi *cgi, Client *client, Epoll &ep)
    : _cgi(cgi), _client(client), _ep(ep), _closedFlag(false),
    _expireAt(CGI_TIMEOUT), _timedout(false)  {}


CgiStdoutHandler::~CgiStdoutHandler() {}

void CgiStdoutHandler::handleRead(time_t now) {
    if (!_cgi || !_client) {
        handleError();
        return;
    }
    if (_closedFlag)
        return;
    char buffer[CGI_BUFF_SIZE];
    ssize_t bytesRead = read(_cgi->getStdoutReadFd(), buffer, sizeof(buffer));
    _client->addActivityToQueue(this, now);
    if (bytesRead < 0) {
        return;
    }
    if (bytesRead == 0) {
        receiveEOF();
        return;
    }
    _cgi->appendOutput(buffer, bytesRead);
}


// No write should happen on the stdout pipe
void CgiStdoutHandler::handleWrite(time_t now) {
    (void) now;
    handleError();
}

// Log error and set handler for erasing
void CgiStdoutHandler::handleError(void) {
    if (_closedFlag)
        return;
    _closedFlag = true;
    Logger::error("[fd " + Logger::toString(getFd()) + "] Stdout Cgi error");
    eraseHandler();
}

// Epollerr - report real error and erase. Epollhup - act as EOF 
bool CgiStdoutHandler::handleError(uint32_t events) {
    if (events & (EPOLLERR)) {
        _closedFlag = true;
        Logger::error("[fd " + Logger::toString(getFd()) + "] Stdout Cgi triggered EPOLLERR");
        eraseHandler();    
    }
    else
        receiveEOF();
    return true;
}

// Close the stdout pipe. delete from epoll. erase from handlers.
void CgiStdoutHandler::eraseHandler() {
    int fd = getFd();
    if (fd != -1) {
        _ep.delFd(fd);
        Logger::info("[fd " + Logger::toString(fd) + "] Cgi stdout has closed");
        close(fd);
        _cgi->setStdoutReadFd(-1);
    }
    _client->deleteCgiStd(this, fd);
}

// All bytes were received, finish cgi process.
void    CgiStdoutHandler::receiveEOF() {
    _client->handleCgiOutput(_cgi->getOutput());
    _closedFlag = true;
    eraseHandler();
    _client->cleanCgi();
}

void     CgiStdoutHandler::updateExpireTime(time_t now) {
    _expireAt = (now + CGI_TIMEOUT);
}

void     CgiStdoutHandler::onTimeout() {
    Logger::warn("[fd " + Logger::toString(getFd()) + "] Timeout");
    eraseHandler();
    _cgi->onTimeout();
    _client->cgiTimeout();
}

time_t   CgiStdoutHandler::getExpireAt() const {
    return _expireAt;
}

int      CgiStdoutHandler::getFd() const {
    return _cgi ? _cgi->getStdoutReadFd() : -1;
}

bool     CgiStdoutHandler::isTimedout() {
    return _timedout;
}

void     CgiStdoutHandler::setTimedout(bool val) {
    _timedout = val;
}