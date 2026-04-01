/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   httpResponse.hpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: ilazar <ilazar@student.42.de>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/10/24 13:59:39 by anschmit          #+#    #+#             */
/*   Updated: 2025/12/24 19:52:48 by ilazar           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once

#include <string>
#include <map>
#include <sstream>

// enum CGIstate {
//     NO_CGI,
//     CGI_OK,
//     CGI_404,
//     CGI_403
// };

class HttpResponse {
    private:
        std::string _version;
        int         _code;
        std::string _reason;
        std::map<std::string, std::string> _headers;
        std::string _body;
        
        bool        _cgiState;
		
    public:
        HttpResponse();
        HttpResponse(const HttpResponse &other);
        HttpResponse &operator=(const HttpResponse &other);
        ~HttpResponse();

        void setVersion(const std::string &version);
        void setStatus(int code);
        void setReason(const std::string &reason);
        void setHeader(const std::string &name, const std::string &value);
        void setBody(const std::string &body);
        
        std::string getVersion() const;
        int getCode() const;
        std::string getReason() const;
        std::string getHeader(const std::string &name) const;
        std::string getBody() const;

        std::string toString() const;

        void        setCgiState(bool val);
        bool        getCgiState() const;
        // void        setCgiError(bool val);
        // bool        getCgiError() const;
};

