/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ConfigWrapper.hpp                                  :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: nrauh <nrauh@student.42berlin.de>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/10/10 11:54:50 by nrauh             #+#    #+#             */
/*   Updated: 2026/01/08 17:12:29 by nrauh            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once
#include "Parser.hpp"
#include "Utils.hpp"
#include "Lexer.hpp"
#include <stdexcept>
#include <algorithm>

struct	ServerConf;

struct	Location {
	std::string						prefix;
	std::string						root;
	bool							internal;
	std::vector<std::string>		methods;
	std::string						index;
	int								statusCode;
	std::string						returnTarget;
	bool							autoindex;
	std::string						uploadDir;
	std::string						cgiExtension;
	std::string						cgiInterpreter;
	// ServerConf						*server;
	bool							isValid;

	Location();
};

struct Error {
	std::vector<int>				statusCodes;
	std::string						uri;

	Error();
};

struct Connection {
	std::string						host;
	std::string						port;

	Connection();
};

struct	ServerConf {
	std::string						root;
	std::string						index;
	std::string						host;
	std::string						port;
	std::vector<Connection>			connections;
	std::string						serverName;
	int								clientMaxBodySize;
	std::vector<Error>				errors;
	std::vector<Location>			locations;

	ServerConf();
};

// We will include the Parser and Lexer in creating the ConfigWrapper!
class ConfigWrapper {
	private:
		typedef std::vector<SimpleDirective>::const_iterator constSimpleIt;
		typedef std::vector<Block>::const_iterator constBlockIt;
		typedef std::vector<Error>::const_iterator constErrIt;
		typedef std::vector<ServerConf>::const_iterator constServIt;
		typedef std::vector<Connection>::const_iterator constConIt;
		typedef std::vector<Location>::const_iterator constLocIt;

		Block						configTree;
		std::vector<Block>			serverBlocks;
		std::vector<ServerConf>		serverConfigs;
		size_t						nrOfServers;
		bool						isValid;

		// to prevent copies of the ConfigWrapper
		ConfigWrapper &operator=(const ConfigWrapper &other);
		ConfigWrapper(const ConfigWrapper &other);

	public:
		// ConfigWrapper(const Block &configTree);
		ConfigWrapper(std::string filename);
		~ConfigWrapper();

		void						parseConfigTree(); // converts configTree to SERVER BLOCK format
		void					 	parseServerBlocks(); // returns list of SERVER CONFIGS?
		void						parseRedirect(const std::vector<std::string> &args, Location &location);
		void						parseCgiExtension(const std::vector<std::string> &args, Location &location);
		void						parseConnection(const std::vector<std::string> &args, ServerConf &server);
		void						parseError(const std::vector<std::string> &args, ServerConf &server);
		void						handleServer(const std::string &key, const std::vector<std::string> &args, ServerConf &server);
		void						handleLocation(const std::string &key, const std::vector<std::string> &args, Location &location);

		// use on server
		ServerConf					findServer(const std::string &hostname = "0.0.0.0:80");
		Location					findLocation(const ServerConf &server, std::string request) const;
		std::string					findErrorPage(const ServerConf &server, const int statusCode) const;
		std::vector<Block>			getServerBlocks() const;
		const std::vector<ServerConf>&	getServerConfigs() const; //INBAR changed it back to const - works now.
		std::string					getServerRoot(const ServerConf &server) const;
		std::string					getServerIndex(const ServerConf &server) const;
		size_t						getNrOfServers() const;

		// use on location
		std::string					buildFilePath(const ServerConf &server, const Location &location, const std::string &request) const;
		std::string					cutPrefix(const Location &location, const std::string &request) const;
		std::string					getRoot(const ServerConf &server, const Location &location) const;
		std::string					getIndex(const ServerConf &server, const Location &location) const;
		int							getClientMaxBodySize(const ServerConf &server) const;
		std::vector<std::string>	getHTTPMethods(const Location &location) const;
		std::string					getReturnTarget(const Location &location) const;
		int							getLocationStatusCode(const Location &location) const;
		std::string					getLocationPrefix(const Location &location) const;
		bool						getAutoindex(const Location &location) const;
		std::string					getUploadPath(const Location &location) const;
		std::string					getCGIExtension(const Location &location) const;
		std::string					getCGIInterpreter(const Location &location) const;
		bool						getValidity() const;

		// print & debug
		void						printServerBlocks() const;
		void						printServers() const;
		void						printServer(const ServerConf &server) const;
		void						printLocation(const ServerConf &server, const Location &location) const;

		class CWException: private std::exception {
			private:
				std::string	msg;

			public:
				explicit CWException(const std::string &msg);
				virtual ~CWException() throw();
				virtual char const *what(void) const throw();
		};
};
