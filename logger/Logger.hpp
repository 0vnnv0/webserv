/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Logger.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: ilazar <ilazar@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/10/01 15:38:46 by ilazar            #+#    #+#             */
/*   Updated: 2025/11/07 15:29:52 by ilazar           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <string>

enum LogLevel {
    
    LOG_ERROR,
    LOG_WARN,
    LOG_INFO,
    LOG_DEBUG
};

class Logger {
    private:
        static bool     _enabled;
        static LogLevel _level;
    
        Logger();
        Logger(const Logger &);
        Logger& operator=(const Logger&);
        ~Logger();
        
        static void log(LogLevel level, const std::string &msg);      
    
    public:
        static void setEnabled(bool value);
        static void setLevel(LogLevel level);

        static void error(const std::string &msg);
        static void warn(const std::string &msg);
        static void info(const std::string &msg);
        static void debug(const std::string &msg);

        static std::string toString(int n);
};