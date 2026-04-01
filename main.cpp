/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: ilazar <ilazar@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/23 18:26:11 by ilazar            #+#    #+#             */
/*   Updated: 2026/01/09 15:18:33 by ilazar           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Server.hpp"
#include "ServerManager.hpp"
#include "Logger.hpp"
#include <iostream>
#include <csignal>
#include "Lexer.hpp"
#include "Parser.hpp"
#include "ConfigWrapper.hpp"

int main(int argc, char * argv[]) {
    //ctrl+c
    std::signal(SIGPIPE, SIG_IGN);
    Logger::setEnabled(true);
    // Logger::setLevel(LOG_DEBUG);
    Logger::setLevel(LOG_INFO);
	if (argc == 2) {		
		std::string filename = argv[1];
		try {
			ConfigWrapper	config(filename);
			if (config.getValidity()) {				
				ServerManager manager(config);
				manager.run();
			}
		} catch (const Lexer::LexerException &e) {
			Logger::error(std::string("Config: ") + e.what());
		}  catch (const ConfigWrapper::CWException &e) {
			Logger::error(std::string("Config: ") + e.what());
		} catch (const std::exception &e) {
			Logger::error(std::string("Server: ") + e.what());
		}
		Logger::info("Shutting down Webserver...");
	}
	else
		Logger::error(std::string("Invalid amount of arguments given!"));
	return 0;
}
