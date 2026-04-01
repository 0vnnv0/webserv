/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Client.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: ilazar <ilazar@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/07 15:28:27 by ilazar            #+#    #+#             */
/*   Updated: 2026/01/08 14:42:24 by ilazar           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once
#include <map>
#include <string>
#include "Epoll.hpp"
#include "Server.hpp"
#include "RequestBuffer.hpp"
#include "RequestHandler.hpp"
#include "Cgi.hpp"
#include "IEventHandler.hpp"
#include "CgiStdinHandler.hpp"
#include "CgiStdoutHandler.hpp"

#define READ_BUFF_SIZE  4096 //4kb - a convetion to handle most small requests in one read.
#define CLIENT_TIMEOUT 15


class ServerManager;

class Client : public IEventHandler {
    private:
        int                          _clientFd;
        Epoll                        &_ep;
        const ConfigWrapper          &_config;
        const ServerConf             &_serverConf;
        ServerManager                &_mgr;
        
        Cgi                          *_cgi;
        bool                         _cgiPending;
        CgiStdinHandler              *_cgiIn;
        CgiStdoutHandler             *_cgiOut;
        
        std::string                  _reqBuf;          //stores read bytes from recv
        std::string                  _respBuf;         //stores ready responses
        bool                         _closedFlag;      //has the fd closed it's reading side
        bool                         _keepAlive;       //should the fd be kept alive
        time_t                       _expireAt;        //absolute expiration time
        bool                         _timedout;        //true if already timed out 
        
        //to avoid a duplication of an fd
        Client(const Client &oth);
        Client& operator=(const Client &oth);
        
        bool            checkHugeContent();
        void            cgiError(int page);
        

        void    handleRead(time_t now);      // handle before reading
        bool    readEvent();                 // read data until EAGAIN()
        void    handleWrite(time_t now);     // send as much data as possible
        bool    writeEvent();
        bool    handleError();
        bool    handleError(uint32_t events);
        
        void    updateExpireTime(time_t time);
        void    onTimeout();
        bool    isTimedout(); 
        void    setTimedout(bool val);
    
        void    closeClient(void);
        bool    controlClose(void);
        void    setToDelete(IEventHandler *handler);
        
        bool    ProcessRequest(time_t now);     //try to parse a complete HTTP requests
        
        void    startCgi(const RequestResult &res, time_t now);
        void    stdinDelete(int fd);            // Cgi handler has finished 
        void    stdoutDelete(int fd);         

        

    public:
        Client(int clientFd, Epoll &ep,
            const ConfigWrapper &config,
            const ServerConf &serverConf,
            ServerManager &mgr);
        ~Client();

        void    addActivityToQueue(IEventHandler *h, time_t now);
        void    interimResp(const std::string &resp);
    
        
        void    cleanCgi(void);
        void    deleteCgiStd(IEventHandler *handler, int id);
        void    cgiTimeout();
        void    handleCgiOutput(const std::string& output);

        
        
        
        //getters setters
        bool            isRespBufEmpty(void) const;
        bool            closedFlag(void) const;
        void            setClosed(bool val);
        bool            isKeepAlive(void) const;
        bool            setKeepAlive(HttpRequest  req);
        std::string&    reqBuf();
        std::string&    respBuf();
        int             getFd(void) const;
        time_t          getExpireAt(void) const;
        bool            isCgiPending();
    };