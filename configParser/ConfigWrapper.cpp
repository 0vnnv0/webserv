/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ConfigWrapper.cpp                                  :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: nrauh <nrauh@student.42berlin.de>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/10/10 11:54:50 by nrauh             #+#    #+#             */
/*   Updated: 2026/01/09 14:38:31 by nrauh            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ConfigWrapper.hpp"

// CANONICAL ORTHODOX FUNCTIONS

// ConfigWrapper::ConfigWrapper(const Block &configTree): configTree(configTree) {
// 	try {
// 		parseConfigTree();
// 		parseServerBlocks();
// 	} catch (const ConfigWrapper::CWException &e) {
// 		std::cerr << e.what() <<std::endl;
// 	}
// };

/* --------------------------------------------------------------------------------------
								INIT STRUCTS
-------------------------------------------------------------------------------------- */

Location::Location() : prefix(""),
      root(""),
      internal(false),
      index(""),
      statusCode(-1),
      returnTarget(""),
	  autoindex(false),
	  isValid(false) {};

Error::Error() : uri("") {};

ServerConf::ServerConf() :
		host("80"),
		port("0.0.0.0"),
		clientMaxBodySize(-1) {};


// default listen 8080
Connection::Connection() {};

/* --------------------------------------------------------------------------------------
								CLASS CONSTRUCTORS
-------------------------------------------------------------------------------------- */

ConfigWrapper::ConfigWrapper(std::string filename): nrOfServers(0), isValid(false) {
	try {
		Lexer	lexer(filename);
		lexer.validateFilename();
		lexer.tokenize();
		lexer.checkValidity();

		Parser	parser(lexer.getTokens());
		parser.createConfigTree();

		this->configTree = parser.getConfigTree();
		this->parseConfigTree();
		this->parseServerBlocks();
		this->isValid = true;
	} catch (const Lexer::LexerException &e) {
		std::cerr << e.what() << std::endl;
	} catch (const ConfigWrapper::CWException &e) {
		std::cerr << e.what() <<std::endl;
	}
};

ConfigWrapper::ConfigWrapper(const ConfigWrapper &other): configTree(other.configTree), nrOfServers(0) {};

// ConfigWrapper &ConfigWrapper::opconnectionserator=(const ConfigWrapper &other) {
// 	if (this != &other) {
// 		this->serverBlocks = other.serverBlocks;
// 		this->serverConfigs = other.serverConfigs;
// 	}
// 	return *this;
// };

ConfigWrapper::~ConfigWrapper() {};


/* --------------------------------------------------------------------------------------
								MEMBER FUNCTIONS
-------------------------------------------------------------------------------------- */

// parses configTree into SERVER BLOCKS
void	ConfigWrapper::parseConfigTree() {
	const Block	*httpBlock = NULL;
	this->serverBlocks.clear();

	if (this->configTree.name == "http")
		httpBlock = &this->configTree;
	else {
		for (constBlockIt it = this->configTree.blockDir.begin(); it != this->configTree.blockDir.end(); ++it) {
			if (it->name == "http") {
				httpBlock = &(*it);
				break;
			}
		}
	}
	if (!httpBlock)
		throw CWException("No HTTP block found!");
	for (constBlockIt it = httpBlock->blockDir.begin(); it != httpBlock->blockDir.end(); ++it) {
		if (it->name == "server")
			this->serverBlocks.push_back(*it);
	}
};

void	ConfigWrapper::parseConnection(const std::vector<std::string> &args, ServerConf &server) {
		Connection	connection;
		size_t found = args[0].find(":");
		if (found != std::string::npos) {
			connection.host = args[0].substr(0, found);
			connection.port = args[0].substr(found + 1);
		} else
			connection.port = args[0];
		server.connections.push_back(connection);
}

void	ConfigWrapper::parseError(const std::vector<std::string> &args, ServerConf &server) {
	Error	error;
	for (size_t i = 0; i < args.size(); i++) {
		if (Utils::isNumber(args[i]))
			error.statusCodes.push_back(Utils::stringToInt(args[i]));
		else if (args[i].find(".html") != std::string::npos)
			error.uri = args[i];
		else
			throw CWException("Invalid configuration of error_page directive!");
	}
	server.errors.push_back(error);
};

void	ConfigWrapper::handleServer(const std::string &key, const std::vector<std::string> &args, ServerConf &server) {
	if (args.empty() || args[0].empty())
		throw CWException("No args for " + key + "found");
	if (key == "root")
		server.root = args[0];
	else if (key == "index")
		server.index = args[0];
	else if (key == "listen")
		this->parseConnection(args, server);
	else if (key == "error_page")
		this->parseError(args, server);
	else if (key == "client_max_body_size")
		server.clientMaxBodySize = Utils::stringToInt(args[0]);
}

void	ConfigWrapper::parseRedirect(const std::vector<std::string> &args, Location &location) {
	if (Utils::isNumber(args[0]) && args.size() > 1) {
		location.statusCode = Utils::stringToInt(args[0]);
		location.returnTarget = args[1];
	} else
		location.returnTarget = args[0];
}

void	ConfigWrapper::parseCgiExtension(const std::vector<std::string> &args, Location &location) {
		if (args.size() < 2)
			throw CWException("Wrong configuration of cgi_extension!");
		location.cgiExtension = args[0];
		location.cgiInterpreter = args[1];
}

void	ConfigWrapper::handleLocation(const std::string &key, const std::vector<std::string> &args, Location &location) {
	if (args.empty() || args[0].empty())
		throw CWException("No args for " + key + "found");
	if (key == "root")
		location.root = args[0];
	else if (key == "index")
		location.index = args[0];
	else if (key == "methods")
		location.methods.insert(location.methods.end(), args.begin(), args.end());
	else if (key == "return")
		this->parseRedirect(args, location);
	else if (key == "autoindex")
		location.autoindex = (args[0] == "on");
	else if (key == "upload_dir")
			location.uploadDir = args[0];
	else if (key == "cgi_extension")
		this->parseCgiExtension(args, location);
};

// returns a list of SERVER CONFIGS
void 	ConfigWrapper::parseServerBlocks() {
	for (constBlockIt it = this->serverBlocks.begin(); it != this->serverBlocks.end(); ++it) {
		ServerConf	server;
		nrOfServers++;
		if (!it->simpleDir.empty()) {
			for (constSimpleIt dir = it->simpleDir.begin();
				dir != it->simpleDir.end();
				++dir)
				this->handleServer(dir->key, dir->args, server);
		}
		if (server.connections.empty()) {
			Connection	connection;
			server.connections.push_back(connection);
		}
		for (constBlockIt loc = it->blockDir.begin(); loc != it->blockDir.end(); ++loc) {
			if (loc->name == "location") {
				if (loc->parameter.empty())
					throw CWException("Location prefix is missing.");
				if (!loc->simpleDir.empty()) {
					Location	location;
					location.prefix = loc->parameter;
					for (constSimpleIt dir = loc->simpleDir.begin(); dir != loc->simpleDir.end(); ++dir)
						this->handleLocation(dir->key, dir->args, location);
					server.locations.push_back(location);
				}
			}
		}
		this->serverConfigs.push_back(server);
	}
};

// returns one SERVER CONF that matches the correct host & port
ServerConf	ConfigWrapper::findServer(const std::string &hostname) {
	const ServerConf *server = NULL;
	std::string	host = "0.0.0.0";
	std::string	port = "8080";
	std::vector<const ServerConf*> candidates;

	size_t pos = hostname.find(':');
	if (pos != std::string::npos) {
		host = hostname.substr(0, pos);
		port = hostname.substr(pos + 1);
	} else
		host = hostname;
	// need to extend functionality to specified default server block
	for (constServIt it = this->serverConfigs.begin(); it != this->serverConfigs.end(); ++it) {
		for (constConIt con = it->connections.begin(); con != it->connections.end(); ++con) {
				if (port == con->port)
					candidates.push_back(&(*it));
			}
	}
	if (candidates.empty())
		throw CWException("No server config found for port " + port);
	for (std::vector<const ServerConf*>::const_iterator it = candidates.begin(); it != candidates.end(); ++it) {
		const ServerConf* serverConf = *it;
		for (constConIt con = serverConf->connections.begin(); con != serverConf->connections.end(); ++con) {
			if (host == con->host) {
				server = serverConf;
				break ;
			}
		}
	}
	if (!server)
		server = candidates[0];
	return *server;
}


/* --------------------------------------------------------------------------------------
						HELPER FUNCTIONS FOR BUILDING REQUEST PATH
-------------------------------------------------------------------------------------- */

std::string ConfigWrapper::findErrorPage(const ServerConf &server, const int statusCode) const {
	for (constErrIt err = server.errors.begin(); err != server.errors.end(); ++err) {
		std::vector<int>::const_iterator it = std::find(err->statusCodes.begin(), err->statusCodes.end(), statusCode);
		if (it != err->statusCodes.end()) {
			return Utils::stripFirstSlash(this->getServerRoot(server) + err->uri);
		}
	}
	return "";
};

//find the location in the server object that matches the longest request PREFIX
Location	ConfigWrapper::findLocation(const ServerConf &server, std::string request) const {
	Location				location;
	size_t					max = 0;

	for (constLocIt it = server.locations.begin(); it != server.locations.end(); ++it)
	{
		size_t pos = request.find(it->prefix);
		if (pos != std::string::npos && pos == 0) {
			if (it->prefix.size() > max) {
				max = it->prefix.size();
				location = *it;
				location.isValid = true;
			}
		}
	}
	return location;
};

std::string	ConfigWrapper::cutPrefix(const Location &location, const std::string &request) const {
	std::string strippedRequest = request.substr(location.prefix.size());

	if (!strippedRequest.empty() && strippedRequest[0] != '/')
		strippedRequest = "/" + strippedRequest;

	return strippedRequest;
}

// returns the full file path to the requested resource
std::string	ConfigWrapper::buildFilePath(const ServerConf &server, const Location &location, const std::string &request) const {
	// gets root of location and if not specified gets server root
	std::string	rootPath = this->getRoot(server, location);
	std::string fullPath;

	if (request.empty())
		throw CWException("Request path is empty!");

	fullPath = rootPath + cutPrefix(location, request);

	if (Utils::isDirectory(fullPath)) {
		if (fullPath[fullPath.size() - 1] != '/')
			fullPath = fullPath + "/";
		return fullPath + location.index;
	}
	return fullPath;
};


/* --------------------------------------------------------------------------------------
									GETTER FUNCTIONS
-------------------------------------------------------------------------------------- */

std::vector<Block>	ConfigWrapper::getServerBlocks() const {
	if (this->serverBlocks.empty())
		throw CWException("CRITICAL: No server blocks found!");
	return this->serverBlocks;
};

const std::vector<ServerConf>&	ConfigWrapper::getServerConfigs() const {
	if (this->serverConfigs.empty())
		throw CWException("CRITICAL: No server configs found!");
	return this->serverConfigs;
};

std::string	ConfigWrapper::getServerRoot(const ServerConf &server) const {
	if (server.root.empty()) {
		// std::cerr << "No server root found!" << std::endl;
		return "";
	}
	if (server.root[0] == '/')
		return Utils::stripFirstSlash(server.root);
	return server.root;
};

std::string	ConfigWrapper::getServerIndex(const ServerConf &server) const {
	if (server.index.empty()) {
		// std::cerr << "No server index found!" << std::endl;
		return "";
	}
	if (server.index[0] == '/')
		return Utils::stripFirstSlash(server.index);
	return server.index;
};

std::string ConfigWrapper::getRoot(const ServerConf &server, const Location &location) const {
	std::string root = location.root;
	if (root.empty())
		root = this->getServerRoot(server);
	if (root.empty())
		throw CWException("CRITICAL: No location or server root found!");
	if (root[0] == '/')
		return Utils::stripFirstSlash(root);
	return root;
}

std::string ConfigWrapper::getIndex(const ServerConf &server, const Location &location) const {
	std::string index = location.index;
	if (location.index.empty())
		index = this->getServerIndex(server);
	if (index.empty())
		throw CWException("CRITICAL: No location or server index found!");
	if (index[0] == '/')
		return Utils::stripFirstSlash(index);
	return index;
}

int ConfigWrapper::getClientMaxBodySize(const ServerConf &server) const {
	if (server.clientMaxBodySize == -1) {
		// std::cerr << "No max body size found" << std::endl;
		return -1;
	}
	return server.clientMaxBodySize;
};

size_t	ConfigWrapper::getNrOfServers() const {
	if (!this->nrOfServers)
		throw CWException("CRITICAL: Nr of Servers not found!");
	return this->nrOfServers;
};

std::vector<std::string> ConfigWrapper::getHTTPMethods(const Location &location) const {
	// if (location.methods.empty())
		// std::cerr << "No HTTP Methods found!" << std::endl;
	return location.methods;
};

std::string ConfigWrapper::getReturnTarget(const Location &location) const {
	// if (location.returnTarget.empty())
		// std::cerr << "No Return Target found!" << std::endl;
	return location.returnTarget;
};

int ConfigWrapper::getLocationStatusCode(const Location &location) const {
	if (location.statusCode == -1) {
		// std::cerr << "No location status Code found!" << std::endl;
		return -1;
	}
	return location.statusCode;
};

std::string ConfigWrapper::getLocationPrefix(const Location &location) const {
	if (location.prefix.empty())
		throw CWException("CRITICAL: No location prefix found");
	return location.prefix;
};

bool	ConfigWrapper::getAutoindex(const Location &location) const {
	return location.autoindex;
};

std::string	ConfigWrapper::getCGIExtension(const Location &location) const {
	// if (location.cgiExtension.empty())
	// 	std::cerr << "No CGI extension defined!";
	return location.cgiExtension;
};

std::string	ConfigWrapper::getCGIInterpreter(const Location &location) const {
	// if (location.cgiInterpreter.empty())
	// 	std::cerr << "No CGI interpreter defined!";
	return location.cgiInterpreter;
};

// gives back full path to the upload dir
std::string	ConfigWrapper::getUploadPath(const Location &location) const {
	if (location.uploadDir.empty()) {
		// std::cerr << "No upload directory specified";
		return "";
	}
	if (location.uploadDir[0] != '/')
		return Utils::stripFirstSlash(location.uploadDir);
	return location.uploadDir;
};

bool	ConfigWrapper::getValidity() const {
	return this->isValid;
};

/* --------------------------------------------------------------------------------------
									EXCEPTIONS
-------------------------------------------------------------------------------------- */

ConfigWrapper::CWException::CWException(const std::string &msg): msg(msg) {};

ConfigWrapper::CWException::~CWException() throw() {};

const char *ConfigWrapper::CWException::what() const throw() {
	return this->msg.c_str();
}

/* --------------------------------------------------------------------------------------
							PRINT FUNCTIONS FOR DEBUGGING
-------------------------------------------------------------------------------------- */

void	ConfigWrapper::printServerBlocks() const {
	std::cout << "[ ";
	for (constBlockIt it = this->serverBlocks.begin();
		it != serverBlocks.end();
		++it) {
		std::cout << it->name << " { ... }";
		if (it + 1 != serverBlocks.end())
			std::cout << ", ";
	}
	std::cout << " ]" << std::endl;
};

void	ConfigWrapper::printServers() const {
	std::cout << "[ " << std::endl;
	for (constServIt server = this->serverConfigs.begin();
		server != this->serverConfigs.end();
		++server) {
		std::cout << "    {" << std::endl;
		for (constConIt con = server->connections.begin(); con != server->connections.end(); ++con) {
			if (!con->host.empty())
				std::cout << "        host: " << con->host << std::endl;
			if (!con->port.empty())
				std::cout << "        port: " << con->port <<std::endl;
		}
		std::cout << "        locations: { ";
		if (!server->locations.empty()) {
			for (constLocIt loc = server->locations.begin(); loc != server->locations.end(); ++loc) {
				std::cout << loc->root << ": " << loc->prefix;
				if (loc + 1 != server->locations.end())
					std::cout << ", ";
			}
		}
		if (server + 1 != this->serverConfigs.end())
			std::cout << " }, " << std::endl;
		else
			std::cout << " }" << std::endl;
		std::cout << "    }," << std::endl;
	}
	std::cout << "]" << std::endl;
}

void	ConfigWrapper::printServer(const ServerConf &server) const {
	std::cout << "----- SERVER CONFIG FOUND -----" << std::endl;
	for (constConIt con = server.connections.begin(); con != server.connections.end(); ++con) {
		if (!con->host.empty())
			std::cout << "Host: " << con->host << " | ";
		if (!con->port.empty())
			std::cout << "Port: " << con->port <<std::endl;
	}
	std::cout << "Client max body size: " << this->getClientMaxBodySize(server) << std::endl;
	std::cout << "Server root: " << this->getServerRoot(server) << std::endl;
	std::cout << "Server index: " << this->getServerIndex(server) << std::endl;
	std::cout << "-------------------------------" << std::endl;
};

void	ConfigWrapper::printLocation(const ServerConf &server, const Location &location) const {
	if (!location.isValid)
		throw CWException("Location is invalid!");
	std::cout << "-------- LOCATION FOUND --------" << std::endl;
	std::cout << "Prefix: " << this->getLocationPrefix(location) << std::endl;
	std::cout << "Root: " << this->getRoot(server, location) << std::endl;
	std::cout << "Index: " << this->getIndex(server, location) << std::endl;
	std::cout << "Status Code: " << this->getLocationStatusCode(location) << std::endl;
	std::cout << "Return Target: " << this->getReturnTarget(location) << std::endl;
	std::cout << "HTTP Methods: ";
	std::vector<std::string> methods = this->getHTTPMethods(location);
	if (!methods.empty()) {
		for (std::vector<std::string>::const_iterator it = methods.begin(); it != methods.end(); ++it)
			std::cout << *it << " ";
	}
	std::cout << std::endl;
	std::cout << "Autoindex: " << this->getAutoindex(location) << std::endl;
	std::cout << "Upload Directory: " << this->getUploadPath(location) << std::endl;
	std::cout << "CGI Extension: " << this->getCGIExtension(location) << std::endl;
	std::cout << "CGI Interpreter: " << this->getCGIInterpreter(location) << std::endl;
	std::cout << "Valid location: " << location.isValid << std::endl;
	std::cout << "-------------------------------" << std::endl;
};
