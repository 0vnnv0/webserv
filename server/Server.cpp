/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: nrauh <nrauh@student.42berlin.de>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/07 15:29:03 by ilazar            #+#    #+#             */
/*   Updated: 2025/12/19 11:13:06 by nrauh            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Server.hpp"
#include "Logger.hpp"
#include "RequestHandler.hpp"
#include <stdexcept>
#include <errno.h>
#include <cstring>
#include <netdb.h>
#include <iostream>
#include <sstream>


// Constructor Destructor
Server::Server(const ServerConf &serverConf, ServerManager* mgr)
: _socketFd(-1), _serverConf(serverConf), _mgr(mgr) {}


Server::~Server() {
    closeSocket();
}


//Functions
void    Server::init(void) {
    struct  addrinfo    hints;
    struct  addrinfo    *res;

    std::memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;       //IPv4
    hints.ai_socktype = SOCK_STREAM; //TCP
    hints.ai_flags = AI_PASSIVE;     //binds "0.0.0.0" if node == null
    int ret = getaddrinfo(_serverConf.host.c_str(), _serverConf.port.c_str(), &hints, &res);
    if (ret != 0) {
        throw std::runtime_error(std::string("getaddrinfo() failed: ") + std::string(gai_strerror(ret)));
    }
    
    _socketFd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (_socketFd < 0) {
        freeaddrinfo(res);
        throw std::runtime_error(std::string("socket() failed: ") + strerror(errno));
    }
    
    // SO_REUSEADDR no TIME_WAIT state, allows socket to bind again immediatly
    int opt = 1;
    setsockopt(_socketFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    if (bind(_socketFd, res->ai_addr, res->ai_addrlen) < 0) {
        freeaddrinfo(res);
        throw std::runtime_error(std::string("bind() failed: ") + strerror(errno));
    }

    freeaddrinfo(res);
    
    if (listen(_socketFd, SOMAXCONN) < 0) { // Max allowed connections 
        throw std::runtime_error(std::string("listen() failed: ") + strerror(errno));
    }
}


int     Server::acceptConnection(void) {
    struct sockaddr_in clientAddr;
    socklen_t addrLen = sizeof(clientAddr);
    int clientFd = -1;
    
    while (1) {
        clientFd = accept(_socketFd, (struct sockaddr*)&clientAddr, &addrLen);
        if (clientFd >= 0) {
            return clientFd;
        } else {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                return -1; //no more pending connections
            } else if (errno == EINTR) {
                continue; //interepted by signal, try again
            } else {
                throw std::runtime_error(std::string("accept() failed: ") + strerror(errno));
            }
        }
    }
}


void    Server::closeSocket(void) {
    if (_socketFd != -1)
        close(_socketFd);
    _socketFd = -1;
}

//handler
void Server::handleRead(time_t now) {
    _mgr->newConnection(this, now);
}

//never happens
void Server::handleWrite(time_t now) {
    (void) now;
}

bool Server::handleError(uint32_t events) {
    (void) events;
    return (handleError());
}

bool Server::handleError() {
    Logger::error("[listening fd " + Logger::toString(_socketFd) + "] triggered EPOLLERR/HUP");
    return false;
}



//Getters
int             Server::getFd(void) const {
    return (_socketFd);
}

const std::string&     Server::getPort(void) const {
    return (_serverConf.port);
}

const std::string&     Server::getHost(void) const {
    return (_serverConf.host);
}

const ServerConf&     Server::getServerConf() const {
    return (_serverConf);
}
