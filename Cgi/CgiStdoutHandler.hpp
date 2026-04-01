/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   CgiStdoutHandler.hpp                               :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: ilazar <ilazar@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/02 13:23:00 by ilazar            #+#    #+#             */
/*   Updated: 2026/01/08 14:42:40 by ilazar           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once
#include "IEventHandler.hpp"
#include "Cgi.hpp"

class Client;

class CgiStdoutHandler : public IEventHandler {
    private:
        Cgi     *_cgi;
        Client  *_client;
        Epoll   &_ep;
        bool    _closedFlag;
        int     _expireAt;
        bool    _timedout;

        CgiStdoutHandler(const CgiStdoutHandler &oth);
        CgiStdoutHandler& operator=(const CgiStdoutHandler &oth);

    public:
        CgiStdoutHandler(Cgi *_cgi, Client *client, Epoll &_ep);
        ~CgiStdoutHandler();
        void    handleRead(time_t now);
        void    handleWrite(time_t now);
        void    handleError();
        bool    handleError(uint32_t events);
        
        void    updateExpireTime(time_t time);
        void    onTimeout();
        time_t  getExpireAt() const;
        bool    isTimedout();
        void    setTimedout(bool val);
        
        void    receiveEOF();
        int     getFd() const;
        void    eraseHandler();
};