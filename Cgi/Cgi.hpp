/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Cgi.hpp                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: ilazar <ilazar@student.42.de>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/10/27 12:03:43 by ilazar            #+#    #+#             */
/*   Updated: 2026/01/07 17:14:03 by ilazar           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once
#include "httpRequest.hpp"
#include "httpResponse.hpp"
#include "ConfigWrapper.hpp"
#include "Epoll.hpp"
#include "RequestHandler.hpp"
#include <string>
#include <map>

struct RequestResult;

#define CGI_BUFF_SIZE 4096 //4kb
#define CGI_TIMEOUT 5

struct cgiResponse {
    int status;
    std::string reason;
    std::map<std::string, std::string> headers;
    std::string body;
};

class Cgi {

    private:
        Epoll &_ep;
    
        std::string _scriptName;                        //path to the script file (ex "test.py")
        std::string _pathInfo;                          //additional info
        std::string _scriptFilename;                    //absulote path
        std::string _interpreter;                       //path to the interpeter executable (ex /usr/bin/python3)
        std::map<std::string, std::string> _headers;    //all the HTTP request headers from the client
        std::string _version;                           //HTTP version
        std::string _method;                            //HTTP method
        std::string _body;                              //HTTP request body
        std::string _queryString;                       //the part of the URL after the "?"
        std::string _serverName;                        //server's hostname
        std::string _serverPort;                        //server's port
        std::string _requestUri;                        //URL path + query
        std::string _workingDir;                        //Current child directory

        std::vector<std::string> _envStorage;           //stores real strings
        std::vector<char*>       _envp;                 //stores pointers to envStorage[i].c_str()
        
        int         _epollFd;
        pid_t       _pid;
        int         _stdinWrite; //parent writes request body to CGI
        int         _stdoutRead; //parent reads CGI output
        size_t      _stdinBytesWritten;
        std::string _output;

        //disable copiers
        Cgi(const Cgi &other);
        Cgi& operator=(const Cgi &other);
        
        //functions
        void                  buildEnv();
        std::vector<char*>    buildArgv();

        
        //helpers
        void                  closeAllPipes(int inPipe[2], int outPipe[2]);
        std::string           getAbsolutePath(const std::string& relativePath);
        void                  extractScriptAndPathInfo(const std::string &urlPath, const std::string &cgiExt);
        
        public:
        Cgi(const RequestResult &res, Epoll &ep, const ConfigWrapper &config, const ServerConf &ServerConf);
        ~Cgi();        
        
        bool start();               // fork + pipes + register with epoll
        static cgiResponse           parseRespone(cgiResponse res, std::string output);
        
        int                 getStdoutReadFd() const;
        int                 getStdinWriteFd() const;
        void                setStdoutReadFd(int val);
        void                setStdinWriteFd(int val);
        size_t              getStdinBytesWritten() const;
        void                setStdinBytesWritten(size_t val);
        const std::string&  getRequestBody() const;
        bool                isFinished();
        void                setFinished(bool val);
        const std::string&  getOutput() const;
        void                appendOutput(const char* data, size_t size);
        void                onTimeout();
        bool                checkChildStatus();
        int                checkCgiFile();
    
};