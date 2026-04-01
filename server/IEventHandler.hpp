/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   IEventHandler.hpp                                  :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: ilazar <ilazar@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/28 13:27:40 by ilazar            #+#    #+#             */
/*   Updated: 2026/01/08 14:44:20 by ilazar           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once
#include <stdint.h>
#include <ctime>
#include <iostream>

class IEventHandler {
public:
    
    virtual void handleRead(time_t now) = 0;
    virtual void handleWrite(time_t now) = 0;
    virtual int  getFd() const = 0;
    virtual bool handleError(uint32_t events) = 0;
    
    virtual void updateExpireTime(time_t now) { (void) now; }
    virtual time_t getExpireAt() const { return 0; }
    virtual void onTimeout() { std::cout << "timeout! fd " << getFd() << std::endl; }
    virtual bool isTimedout() { return false; }
    virtual void setTimedout(bool val) { (void) val; }
    virtual ~IEventHandler() {};
};