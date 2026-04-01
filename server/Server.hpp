/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: ilazar <ilazar@student.42.de>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/07 15:29:12 by ilazar            #+#    #+#             */
/*   Updated: 2026/01/06 12:20:19 by ilazar           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */


#pragma once

#include <string>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include "ConfigWrapper.hpp"
#include "IEventHandler.hpp"
#include "ServerManager.hpp"

class Server : public IEventHandler {

    private:
        int                 _socketFd;  // listening socket
        const ServerConf    _serverConf;
        ServerManager*      _mgr;

        // Prevent copies. Not to copy a socket fd
        Server(const Server &);
        Server& operator=(const Server &);

    public:
        Server(const ServerConf &serverConf, ServerManager* mgr);
        ~Server();

        //setup
        void    init(); //create socket, bind, listen
        void    closeSocket();
        int     acceptConnection(); //returns new client fd

        //handler
        void handleRead(time_t now);
        void handleWrite(time_t now);
        bool handleError();
        bool handleError(uint32_t events);
        
        // Getters
        int                 getFd() const;
        const std::string&  getPort() const;
        const std::string&  getHost() const;
        const ServerConf&   getServerConf() const;
        
};
