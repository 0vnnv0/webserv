/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Epoll.cpp                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: ilazar <ilazar@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/23 19:30:49 by ilazar            #+#    #+#             */
/*   Updated: 2026/01/08 12:14:27 by ilazar           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Epoll.hpp"
#include "Logger.hpp"
#include <sys/epoll.h>
#include <stdexcept>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <iostream>


//Constructor Destructor
Epoll::Epoll(int maxEvents) : _epfd(-1) {
    if (maxEvents <= 0)
        maxEvents = 1;

    _epfd = epoll_create(maxEvents);
    if (_epfd < 0) {
        throw std::runtime_error(std::string("epoll_create failed: ") + strerror(errno));
    }
}

Epoll::~Epoll() {
    if (_epfd != -1)
        close(_epfd);
    _epfd = -1;
}


//Functions
//Add fd to interest list 
//@events like EPOLLIN, EPOLLOUT, EPOLLERR 
bool    Epoll::addFd(int fd, unsigned int events, void* ptr) {
    struct epoll_event ev;
    ev.events = events;
    ev.data.ptr = ptr;

    if (epoll_ctl(_epfd, EPOLL_CTL_ADD, fd, &ev) < 0) {
        Logger::error("epoll ADD failed: " + std::string(strerror(errno)));
        return false;
    }
    // Logger::debug("[fd " + Logger::toString(fd) + "] added to epoll list");
    return true;
}

//Modify events for existing fd
bool Epoll::modFd(int fd, unsigned int events, void* ptr) {
    struct epoll_event ev;
    ev.events = events;
    ev.data.ptr = ptr;

    if (epoll_ctl(_epfd, EPOLL_CTL_MOD, fd, &ev) < 0) {
        Logger::error("epoll MOD(ptr) failed: " + std::string(strerror(errno)));
        return false;
    }
    return true;
}


void    Epoll::setModWritable(int fd, void* ptr) {
    modFd(fd, EPOLLIN | EPOLLOUT | EPOLLRDHUP | EPOLLERR, ptr);
}

void    Epoll::setModNonWritable(int fd, void* ptr) {
    modFd(fd, EPOLLIN | EPOLLRDHUP | EPOLLERR, ptr);
}

//Deletes an fd from interest list
void    Epoll::delFd(int fd) {
    if (epoll_ctl(_epfd, EPOLL_CTL_DEL, fd, 0) < 0)
        Logger::error("epoll DEL failed: " + std::string(strerror(errno)));
    // Logger::debug("[fd " + Logger::toString(fd) + "] deleted from epoll");
}

//Wait for events. Blocks until some fds are ready
//@events an array where epoll stores ready events
int     Epoll::wait(struct epoll_event *events, int maxEvents, int timeout) {
    int n;
    
    do {
        n = epoll_wait(_epfd, events, maxEvents, timeout);
    } while (n < 0 && errno == EINTR); //if interrupted by a signal, continue 

    if (n < 0)
        throw std::runtime_error(std::string("epoll_wait failed: ") + strerror(errno));

    return n;
}

int     Epoll::getFd(void) const {
    return _epfd;
}

//Set a socket fd to non-blocking.
//prevents recv()/send() from blocking in epoll loops
void    Epoll::setNonBlocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1)
        throw std::runtime_error(std::string("fcntl F_GETFL non-blobk failed: ") + strerror(errno));

    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1)
        throw std::runtime_error(std::string("fcntl F_SETFL non-blobk failed: ") + strerror(errno));
}