/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ServerManager.hpp                                  :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: ilazar <ilazar@student.42.de>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/23 21:26:20 by ilazar            #+#    #+#             */
/*   Updated: 2026/01/06 13:42:18 by ilazar           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once

// #include "Server.hpp"
#include "Epoll.hpp"
#include "IEventHandler.hpp"
#include "ConfigWrapper.hpp"
#include <vector>
#include <queue>
#include <map>
#include <string>

#define MAX_EVENTS            64   //maximum number of events that epoll can return in one call
#define MAINEVENT_TIMEOUT     1000 //timeout for main loop, to check the SIGINT
#define HANDLER_TIMEOUT        20  //timeout for client and cgi fds

class Server;
class Client;

// for clients and cgi timeout
struct TimeoutEntry {
    IEventHandler* handler;
    int id;
    time_t expireAt;
    TimeoutEntry(IEventHandler* h, int i, time_t e) : handler(h), id(i), expireAt(e) {}
};

struct TimeoutCompare { //DO I NEED THIS?
    bool operator()(const TimeoutEntry &a, const TimeoutEntry &b) const {
        return a.expireAt > b.expireAt; // min-heap: earliest expiration on top
    }
};

class ServerManager {

    public:
        static ServerManager*      s_instance;  //static pointer to corrently running instance
        ServerManager(const ConfigWrapper &config);
        ~ServerManager();
        
        //main event
        void    initServers();
        void    run();
        void    mainEvent();                    //runs the main loop
        void    newConnection(Server *serv, time_t now);    //accepts clients, add to epoll
        void    setToDelete(IEventHandler *handler);
        
        //timeout
        void    addHandlersId(IEventHandler *h, int fd);
        void    removeHandlersId(int fd);
        void    addActivityToQueue(IEventHandler *h, time_t now);
        
        //Ctrl+C
        static void handleSigInt(int);         //static signal handler
        void        stop(void);                //ctrl + c will exit cleanly

    private:
        Epoll                           _ep;
        std::vector<Server*>            _servers;
        const ConfigWrapper             &_config;
        // std::map<int, Client*>          _clients;       //do i need this?
        std::map<int, IEventHandler*>   _handlersIds;   //map of handlers according to their fd number
        bool                            _running;       //controls the main loop
        std::vector<IEventHandler*>     _deleteHandler;

        
        std::priority_queue<
        TimeoutEntry,
        std::vector<TimeoutEntry>,
        TimeoutCompare>             _timeoutQueue; //for timing out clients and cgi

        
        //Disable copying
        ServerManager(const ServerManager &);
        ServerManager& operator=(const ServerManager &);
        
        //main event
        void    registerListeningFds(void);
        void    controlDeletion();
        //timeout
        IEventHandler* findHandlerId(IEventHandler* expectedHandler, int fd);
        void    checkTimeout(time_t current_time);
        
        void    cleanExit(void);
    };