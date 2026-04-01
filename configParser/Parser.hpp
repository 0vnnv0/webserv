/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Parser.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: nrauh <nrauh@student.42berlin.de>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/10/02 15:08:27 by natalierauh       #+#    #+#             */
/*   Updated: 2025/10/24 14:07:53 by nrauh            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once
#include <iostream>
#include <map>
#include "Lexer.hpp"

struct SimpleDirective {
	std::string					key;
	std::vector<std::string>	args;
};


struct Block {
	std::string							name; // like server or location
	std::string							parameter; // like / or /static
	// std::map<std::string, std::string>	simpleDir; // like listen 8080;
	std::vector<SimpleDirective>		simpleDir;
	std::vector<Block>					blockDir; // like location
};

class Parser {
	private:
		Block							configTree;
		std::vector<Token>				tokens; // not sure if needed...

	public:
		Parser(const std::vector<Token> &tokens);
		Parser(const Parser &other);
		Parser &operator=(const Parser &other);
		~Parser();

		// basic versin of reading tokens into block struct
		// needs to be extended to work with more complicated config
		Block	createBlockDirective(std::vector<Token>::iterator it);
		void	createConfigTree();

		const Block&	getConfigTree() const;

		void	printConfig() const;
		void	printBlock(const Block &block, int depth = 0) const;

		// need to include exceptions
};
