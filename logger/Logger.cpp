/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Logger.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: ilazar <ilazar@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/07 15:29:44 by ilazar            #+#    #+#             */
/*   Updated: 2025/11/07 15:29:45 by ilazar           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */


#include "Logger.hpp"
#include <iostream>
#include <string>
#include <sstream>


//Define default static members
bool        Logger::_enabled = true;
LogLevel    Logger::_level = LOG_INFO;


void    Logger::setEnabled(bool value) {
    _enabled = value;
}

void    Logger::setLevel(LogLevel level) {
    _level = level;
}

void    Logger::log(LogLevel level, const std::string &msg) {
    if (_enabled == false || level > _level)
        return;
    
    std::string str;
    switch (level)
    {
        case LOG_ERROR:
            str = "\033[1;31mError\033[0m";
            break;
        case LOG_WARN:
            str = "\033[1;33mWarn\033[0m";
            break;
        case LOG_INFO:
            str = "\033[1;36mInfo\033[0m";
            break;
        case LOG_DEBUG:
            str = "\033[1;35mDebug\033[0m";
            break;
    }

    std::cout << str << ": " << msg <<std::endl; 
}


void Logger::error(const std::string &msg) {
    log(LOG_ERROR, msg);
}

void Logger::warn(const std::string &msg) {
    log(LOG_WARN, msg);
}

void Logger::info(const std::string &msg) {
    log(LOG_INFO, msg);
}

void Logger::debug(const std::string &msg) {
    log(LOG_DEBUG, msg);
}

std::string Logger::toString(int n) {
    std::stringstream ss;
    ss << n;
    return ss.str();
}