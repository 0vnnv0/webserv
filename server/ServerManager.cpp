/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ServerManager.cpp                                  :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: ilazar <ilazar@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/07 15:29:24 by ilazar            #+#    #+#             */
/*   Updated: 2026/01/08 15:05:15 by ilazar           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */


#include "ServerManager.hpp"
#include "Server.hpp"
#include "Client.hpp"
#include "Logger.hpp"
#include "RequestHandler.hpp"
#include "httpRequest.hpp"
#include <sstream>
#include <sys/epoll.h>
#include <iostream>
#include <cerrno>
#include <cstring>
#include <stdlib.h>
#include <csignal>

//Constructor Destructor
ServerManager::ServerManager(const ConfigWrapper &config)
    : _ep(MAX_EVENTS), _config(config), _running(true) {
        s_instance = this;
    }

ServerManager::~ServerManager() {
    cleanExit();
}

//initialize all the servers according to the config file
void    ServerManager::initServers() {
for (size_t i = 0; i < _config.getNrOfServers(); i++) {
    std::vector<ServerConf> configs = _config.getServerConfigs();
    ServerConf serverConf = configs[i];
    
    for (size_t j = 0; j < serverConf.connections.size(); j++) {
        serverConf.host = serverConf.connections[j].host;
        serverConf.port = serverConf.connections[j].port;
        Server* serv = new Server(serverConf, this);
        serv->init();
        _servers.push_back(serv);
    }
    }
}

void    ServerManager::run(void) {
    initServers();
    std::signal(SIGINT, ServerManager::handleSigInt);
    Logger::info("Webserver starting...");
    mainEvent();
}

/* Accept new clients and add to epoll.
EPOLLIN - Wake up on new data.
EPOLLET + non-blocking - Edge trigger + Never block
EPOLLRDHUP - Get notified immediately if a client half-closes.
EPOLLERR - Get notified on socket errors.
*/
void    ServerManager::newConnection(Server *serv, time_t now) { 
    int clientFd;
    while ((clientFd = serv->acceptConnection()) >= 0) {
        Epoll::setNonBlocking(clientFd);
        
        IEventHandler *client = new Client(clientFd, _ep, _config, serv->getServerConf(), *this);
        _ep.addFd(clientFd, EPOLLIN | EPOLLRDHUP | EPOLLERR, client); //level triggered
        _handlersIds[clientFd] = client; 
        const std::string &host = serv->getServerConf().host;
        const std::string &port = serv->getServerConf().port;
        Logger::info("[fd " + Logger::toString(clientFd) + "] new client on " + host + ":" + port);
        addActivityToQueue(client, now);
    }
}

//register all listening sockets on epoll
void    ServerManager::registerListeningFds(void) {
    for (size_t i = 0; i < _servers.size(); i++) {
        int listenFd = _servers[i]->getFd();
        Epoll::setNonBlocking(listenFd);
        _ep.addFd(listenFd, EPOLLIN, _servers[i]); //level trigger
        const std::string &host = _servers[i]->getHost();
        const std::string &port = _servers[i]->getPort();
        Logger::info("[fd " + Logger::toString(listenFd) + "] listening on " + host + ":" + port);
    }
}


//main loop
void ServerManager::mainEvent(void) {
    registerListeningFds();
    struct epoll_event events[MAX_EVENTS];

    while (_running) {
        time_t currentTime = time(NULL);
        int numEvents = _ep.wait(events, MAX_EVENTS, MAINEVENT_TIMEOUT);

        for (int i = 0; i < numEvents; ++i) {
            IEventHandler* handler = reinterpret_cast<IEventHandler*>(events[i].data.ptr);
            uint32_t ev = events[i].events;
 
            if (ev & (EPOLLERR | EPOLLHUP)) {
                if (handler->handleError(ev))
                    continue;
            }
            if (ev & EPOLLIN)
                handler->handleRead(currentTime);
            if (ev & EPOLLOUT)
                handler->handleWrite(currentTime);
            controlDeletion();
            }
            checkTimeout(currentTime);
    }
}

void    ServerManager::controlDeletion() {
    std::vector<IEventHandler*>::iterator it = _deleteHandler.begin();
    while (it != _deleteHandler.end())
    {
        IEventHandler* handler = *it;
        Client* client = dynamic_cast<Client*>(handler);
        if (client) {
            client->cleanCgi();
        }
        delete *it;
        *it = NULL;
        ++it;
    }
    _deleteHandler.clear();
}

// Adding handler to deletion list where handlers are deleted
//avoiding inserting duplicates
void    ServerManager::setToDelete(IEventHandler *handler) {
    if (!handler)
        return;
    for (std::vector<IEventHandler*>::iterator it = _deleteHandler.begin();
        it != _deleteHandler.end(); ++it) {
        if (*it == handler)
            return;
    }
    _deleteHandler.push_back(handler);
}

void    ServerManager::addHandlersId(IEventHandler *h, int fd) {
    _handlersIds[fd] = h;    
}

//removes only from id list in favour of timeouts
void    ServerManager::removeHandlersId(int fd) {
    std::map<int, IEventHandler*>::iterator it = _handlersIds.find(fd);
    if (it != _handlersIds.end()) {
        _handlersIds.erase(it);
    }
}

// finds the corresponding handler in the map according to its pointer and fd
// Returns its pointer, or NULL if not found.
IEventHandler* ServerManager::findHandlerId(IEventHandler* expectedHandler, int fd) {
    std::map<int, IEventHandler*>::iterator it = _handlersIds.find(fd);
    if (it == _handlersIds.end())
        return NULL;
    if (it->second != expectedHandler)
        return NULL;
    return it->second;
}

//update time of last activity
void    ServerManager::addActivityToQueue(IEventHandler *h, time_t now) {
    // Logger::debug("[fd " + Logger::toString(h->getFd()) +"] updated activity");
    h->updateExpireTime(now);
    int id = h->getFd();
    _timeoutQueue.push(TimeoutEntry(h, id, h->getExpireAt()));
}

//Trigger timeouts on expired handlers. avoids stale entries.
void ServerManager::checkTimeout(time_t now)
{
    while (!_timeoutQueue.empty())
    {
        TimeoutEntry TimeoutEntry = _timeoutQueue.top();
        if (TimeoutEntry.expireAt > now)
            break;
        _timeoutQueue.pop();
        std::map<int, IEventHandler*>::iterator it = _handlersIds.find(TimeoutEntry.id);
        if (it == _handlersIds.end())
            continue;
        IEventHandler *handler = it->second;
        if (handler != TimeoutEntry.handler)
            continue;
        if (handler->getExpireAt() != TimeoutEntry.expireAt)
            continue;
        if (!handler->isTimedout()) {
            handler->setTimedout(true);
            handler->onTimeout();
        }
    }
}

//Clean exit with SIGINT

void    ServerManager::handleSigInt(int) {
    if (s_instance) {
        Logger::info("\033[31mSIGINT received. Stopping server.\n\033[0m");
        s_instance->stop();
    }
}

void    ServerManager::stop(void) {
    _running = false;
}

ServerManager* ServerManager::s_instance = 0;

void    ServerManager::cleanExit(void) {
    Logger::debug("Performs a clean exit");
    // close all listening sockets
    for (size_t i = 0; i < _servers.size(); ++i) {
        int listenFd = _servers[i]->getFd();
        close(listenFd);
    }
    // delete all server objects
    for (size_t i = 0; i < _servers.size(); i++) {
        delete _servers[i];
    }
    _servers.clear();
    controlDeletion();
}
