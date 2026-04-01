/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   CgiStdinHandler.hpp                                :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: ilazar <ilazar@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/02 13:23:00 by ilazar            #+#    #+#             */
/*   Updated: 2026/01/08 14:42:34 by ilazar           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once
#include "IEventHandler.hpp"
#include "Cgi.hpp"

class Client;

class CgiStdinHandler : public IEventHandler {
    private:
        Cgi     *_cgi;
        Client  *_client;
        Epoll   &_ep;
        bool    _closedFlag;
        time_t  _expireAt;
        bool    _timedout;

        CgiStdinHandler(const CgiStdinHandler &oth);
        CgiStdinHandler& operator=(const CgiStdinHandler &oth);

    public:
        CgiStdinHandler(Cgi *_cgi, Client *client, Epoll &_ep);
        ~CgiStdinHandler();
        void    handleWrite(time_t now);
        void    handleRead(time_t now);
        bool    handleError();
        bool    handleError(uint32_t events);
        
        void    updateExpireTime(time_t time);
        void    onTimeout();
        time_t  getExpireAt() const;
        bool    isTimedout();
        void    setTimedout(bool val);
        
        int     getFd() const;
        void    eraseHandler();
};