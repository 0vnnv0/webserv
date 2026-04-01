/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Cgi.cpp                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: ilazar <ilazar@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/10/29 16:40:30 by ilazar            #+#    #+#             */
/*   Updated: 2026/01/08 17:15:09 by ilazar           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <sstream>
#include <unistd.h>
#include <string.h>
#include <limits.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <sys/epoll.h>
#include "Cgi.hpp"
#include "Logger.hpp"
#include "Epoll.hpp"


Cgi::Cgi(const RequestResult &res, Epoll &ep, const ConfigWrapper &config, const ServerConf &ServerConf)
 : _ep(ep), _epollFd(-1), _pid(-1), _stdinWrite(-1), _stdoutRead(-1), _stdinBytesWritten(0) {
    
    HttpRequest req = res.request;
    std::string fullPath = req.getPath();
    Location location = config.findLocation(ServerConf, fullPath);
    std::string cgiExt = config.getCGIExtension(location);
	
    extractScriptAndPathInfo(fullPath, cgiExt);
    
    _headers = req.getHeaders();
    _version = req.getVersion();
    _method = req.getMethod();
    _body = req.getBody();
    _queryString = req.getQueryString();
    _serverName = ServerConf.serverName;
    _serverPort = ServerConf.port;
    _requestUri = req.getPath();
    if (!_queryString.empty()) {
        _requestUri += "?" + _queryString;
	}    
    
    _interpreter = config.getCGIInterpreter(location);
    if (_interpreter.empty())
        Logger::warn("No CGI interpeter configured this location");
    
    std::string rootPath = location.root.empty() ? ServerConf.root : location.root;
    _scriptFilename = getAbsolutePath(_scriptName) + rootPath + _scriptName;
    Logger::debug("CGI will run: " + _interpreter + "  " + _scriptFilename);

    _workingDir = _scriptFilename.substr(0, _scriptFilename.find_last_of("/"));
    
}   

Cgi::~Cgi() {
    if (_stdinWrite >= 0) {
        _ep.delFd(_stdinWrite);
        close(_stdinWrite);
    }
    if (_stdoutRead >= 0) {
        _ep.delFd(_stdoutRead);
        close(_stdoutRead);
    }
    if (_pid > 0) { //kill child if still running
        int status;
        if (waitpid(_pid, &status, WNOHANG) == 0) {
            kill(_pid, SIGKILL);
            waitpid(_pid, &status, 0);
        }
    }
}

//extract scriptName, and path info in case it exists
void Cgi::extractScriptAndPathInfo(const std::string &urlPath, const std::string &cgiExt)
{
    _scriptName.clear();
    _pathInfo.clear();

    if (!cgiExt.empty()) {
        size_t pos = urlPath.find(cgiExt);
        if (pos != std::string::npos) {
            pos += cgiExt.length();
            _scriptName = urlPath.substr(0, pos);
            _pathInfo = urlPath.substr(pos);
            return;
        }
    }
    _scriptName = urlPath;
    _pathInfo = "";
}

std::string Cgi::getAbsolutePath(const std::string& relativePath) {
    char cwd[PATH_MAX];

    if (getcwd(cwd, sizeof(cwd)) != NULL) {
        std::string absolute = cwd;
        return absolute;
    }
    return relativePath;
}


std::vector<char*>    Cgi::buildArgv() {
    std::vector<char*> argv;
    
    argv.push_back(const_cast<char*>(_interpreter.c_str())); // /usr/bin/python3
    argv.push_back(const_cast<char*>(_scriptFilename.c_str()));  // www/cgi-bin/test.py
    argv.push_back(NULL);
    return argv;
}


void   Cgi::buildEnv() {
    _envStorage.clear();
    _envp.clear();

    _envStorage.push_back("GATEWAY_INTERFACE=CGI/1.1");
    _envStorage.push_back("SERVER_PROTOCOL=" + _version);
    _envStorage.push_back("REQUEST_METHOD=" + _method);
    _envStorage.push_back("SCRIPT_FILENAME=" + _scriptFilename);
    _envStorage.push_back("SCRIPT_NAME=" + _scriptName);
    _envStorage.push_back("PATH_INFO=" + _pathInfo);
    _envStorage.push_back("SERVER_NAME=" + _serverName);
    _envStorage.push_back("SERVER_PORT=" + _serverPort);
    _envStorage.push_back("QUERY_STRING=" + _queryString);
    _envStorage.push_back("REDIRECT_STATUS=200");
    _envStorage.push_back("REQUEST_URI=" + _requestUri);
    _envStorage.push_back("REMOTE_ADDR=127.0.0.1");

    std::string contentLength = "0";
    if (_headers.count("content-length"))
        contentLength = _headers["content-length"];
    _envStorage.push_back("CONTENT_LENGTH=" + contentLength);

    if (_headers.count("content-type"))
        _envStorage.push_back("CONTENT_TYPE=" + _headers["content-type"]);
    
    for (std::map<std::string, std::string>::const_iterator it = _headers.begin();
        it != _headers.end(); ++it)
    {
        std::string name = it->first;
        std::string value = it->second;
        for (size_t i = 0; i < name.size(); ++i)
        {
            name[i] = std::toupper(name[i]);
            if (name[i] == '-')
                name[i] = '_';
        }
        if (name == "CONTENT_TYPE" || name == "CONTENT_LENGTH")
            continue;
        _envStorage.push_back("HTTP_" + name + "=" + value);
    }

    for (size_t i = 0; i < _envStorage.size(); i++)
        _envp.push_back(const_cast<char*>(_envStorage[i].c_str()));
    _envp.push_back(NULL);
}


void    Cgi::closeAllPipes(int inPipe[2], int outPipe[2]) {
    close(inPipe[0]);
    close(inPipe[1]);
    close(outPipe[0]);
    close(outPipe[1]);
}

int    Cgi::checkCgiFile() {
    if (access(_scriptFilename.c_str(), F_OK) != 0) {
        Logger::warn("Cgi file is missing at: " + _scriptFilename);
        return 404;
    }

    if (access(_scriptFilename.c_str(), X_OK) != 0) {
        Logger::warn("Cgi file has no execute permissions at: " + _scriptFilename);
        return 403;
    }
    return 0;
}

//start cgi process. return true if succssed. false otherwise
//by default if returns false - will send 500 error
bool    Cgi::start() {
    int inPipe[2];
    int outPipe[2];
    
    if (pipe(inPipe) < 0 || pipe(outPipe) < 0) {
        Logger::error("Pipe() failed");
        return false;
    }
    
    _stdinWrite  = inPipe[1];
    _stdoutRead  = outPipe[0];
    
    Epoll::setNonBlocking(inPipe[1]); //pipes in parent non-blocking
    Epoll::setNonBlocking(outPipe[0]);
    
    _pid = fork();
    if (_pid < 0) {
        closeAllPipes(inPipe, outPipe);
        Logger::error("Fork() failed");
        return false;
    }

    if (_pid == 0)
    {
        // --- CHILD ---
        dup2(inPipe[0], STDIN_FILENO);   // child reads request body
        dup2(outPipe[1], STDOUT_FILENO); // child writes CGI output

        closeAllPipes(inPipe, outPipe);

        std::vector<char*> argv = buildArgv();
        buildEnv();
        chdir(_workingDir.c_str());

        execve(argv[0], &argv[0], &_envp[0]);
        Logger::error("Execve() failed");
        _exit(1); // if execve fails
    }

    // --- PARENT ---
    close(inPipe[0]);   // parent doesn't read body
    close(outPipe[1]);  // parent doesn't write CGI output

    _stdinWrite  = inPipe[1];
    _stdoutRead  = outPipe[0];
    return true;
}

cgiResponse Cgi::parseRespone(cgiResponse res, std::string output)
{
    if (output.empty())
        return res;

    size_t splitPos;
    std::string delimiter;
    std::string rawHeaders;

    if ((splitPos = output.find("\r\n\r\n")) != std::string::npos)
        delimiter = "\r\n\r\n";
    else if ((splitPos = output.find("\n\n")) != std::string::npos)
        delimiter = "\n\n";
    else
        splitPos = std::string::npos;

    if (splitPos != std::string::npos)
    {
        rawHeaders = output.substr(0, splitPos);
        res.body = output.substr(splitPos + delimiter.size());
    }
    else
    {
        // No headers → entire output is body
        res.body = output;
        return res;
    }

    std::istringstream stream(rawHeaders);
    std::string line;

    while (std::getline(stream, line))
    {
        if (line.empty())
            continue;

        size_t colon = line.find(':');
        if (colon == std::string::npos) {
            res.body = "";
            res.headers.clear();
            return res;
        }

        std::string key = line.substr(0, colon);
        std::string value = line.substr(colon + 1);

        key.erase(0, key.find_first_not_of(" \t"));
        key.erase(key.find_last_not_of(" \t") + 1);

        value.erase(0, value.find_first_not_of(" \t"));
        value.erase(value.find_last_not_of(" \t\r\n") + 1);

        res.headers[key] = value;
    }

    return res;
}

//returns true for 200, false for 502
bool    Cgi::checkChildStatus() {
    int status;
    pid_t ret = waitpid(_pid, &status, 0); // block until child exits

    if (ret == -1) {
        Logger::error("waitpid failed");
        return false;
    }

    // Child exited normally
    if (WIFEXITED(status)) {
        int exitCode = WEXITSTATUS(status);
        if (exitCode != 0) {
            Logger::error("CGI script exited with code " + Logger::toString(exitCode));
            return false;
        }
    }
    // Child killed by signal
    else if (WIFSIGNALED(status)) {
        int sig = WTERMSIG(status);
        Logger::error("CGI script killed by signal " + Logger::toString(sig));
        return false;
    }

    // Child exited normally with code 0 → proceed to parse output
    return true; // empty result signals “no error”
}



int     Cgi::getStdoutReadFd() const {
    return _stdoutRead;
}

int     Cgi::getStdinWriteFd() const{
    return _stdinWrite;
}

void     Cgi::setStdoutReadFd(int val) {
    _stdoutRead = val;
}

void     Cgi::setStdinWriteFd(int val) {
    _stdinWrite = val;
}

size_t   Cgi::getStdinBytesWritten() const {
     return _stdinBytesWritten; 
}

void     Cgi::setStdinBytesWritten(size_t val) {
    _stdinBytesWritten = val;
}

const std::string& Cgi::getRequestBody() const {
    return _body;
}

const std::string& Cgi::getOutput() const {
    return _output;
}

void      Cgi::appendOutput(const char* data, size_t size) {
    _output.append(data, size);
}

//Kill child, close child write and parent read pipes
void      Cgi::onTimeout() {
    if (_pid > 0) {
        kill(_pid, SIGKILL);
        _pid = -1;
    }
}
