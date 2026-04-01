/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Parser.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: nrauh <nrauh@student.42berlin.de>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/10/02 15:21:49 by natalierauh       #+#    #+#             */
/*   Updated: 2026/01/09 14:38:40 by nrauh            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Parser.hpp"
#include "Token.hpp"
#include <string>
#include <map>

Parser::Parser(const std::vector<Token> &tokens): tokens(tokens) {};

Parser::Parser(const Parser &other): tokens(other.tokens) {};

Parser &Parser::operator=(const Parser &other) {
	if (this != &other) {
		this->configTree = other.configTree;
		this->tokens = other.tokens;
	}
	return (*this);
};

Parser::~Parser() {};

void	Parser::createConfigTree() {
	this->configTree = createBlockDirective(this->tokens.begin());
};

Block	Parser::createBlockDirective(std::vector<Token>::iterator it) {
	Block		block;
	int			word_count = 0;

	for (std::vector<Token>::iterator curr = it; curr != this->tokens.end(); ++curr) {
		while (curr->getType() == WORD) {
			curr++;
			word_count++;
		}
		if (curr->getType() == LBRACE) {
			if (block.name.empty()) {
				block.name = (curr - word_count)->getValue();
				if (word_count > 1)
					block.parameter = (curr - word_count + 1)->getValue();
			} else {
				Block	newBlock = this->createBlockDirective(curr - word_count);
				block.blockDir.push_back(newBlock);
				int			brace_depth = 1;
				curr++;
                while (curr != this->tokens.end() && brace_depth > 0) {
                    if (curr->getType() == LBRACE)
                        brace_depth++;
                    else if (curr->getType() == RBRACE)
                        brace_depth--;
                    if (brace_depth > 0)
                        curr++;
                }
			}
		} else if (curr->getType() == SEMICOLON) {
			SimpleDirective simpleDir;
			simpleDir.key = (curr - word_count)->getValue();
			// std::cout << simpleDir.key << ": { ";
			// std::cout << "Word count: " << word_count << std::endl;
			for (int i = 1; i < word_count; i++) {
				simpleDir.args.push_back((curr - word_count + i)->getValue());
				// std::cout << (curr - word_count + i)->getValue() << " ";	
			}
			// std::cout << "}" << std::endl;
			// std::map<std::string, std::string> m;
			block.simpleDir.push_back(simpleDir);
			// block.simpleDir.insert(std::pair<std::string, std::string>((curr - word_count)->getValue(), (curr - word_count + 1)->getValue()));
		} else if (curr->getType() == RBRACE)
			return block;
		word_count = 0;
	}
	return block;
};

const Block&	Parser::getConfigTree() const {
	return this->configTree;
}

void	Parser::printConfig() const {
    this->printBlock(this->configTree); // start from top-level block
}

void Parser::printBlock(const Block &block, int depth) const {
    std::string indent(depth * 4, ' ');

    // print block name and parameter
    std::cout << "Block (name=\"" << block.name
              << "\", parameter=\"" << block.parameter << "\")" << std::endl;

    // print simple directives
    if (!block.simpleDir.empty()) {
        std::cout << indent << "    directives:" << std::endl;
        for (std::vector<SimpleDirective>::const_iterator it = block.simpleDir.begin();
             it != block.simpleDir.end(); 
			 ++it) {
            	std::cout << indent << "        \"" << it->key;
				for (std::vector<std::string>::const_iterator arg = it->args.begin();
					arg != it->args.end();
					++arg)
                    std::cout << "\" = \"" << *arg << "\"" << std::endl;
        }
    }

    // print child blocks recursively
    for (size_t i = 0; i < block.blockDir.size(); ++i) {
        std::cout << indent << "    [" << i << "] ";
        this->printBlock(block.blockDir[i], depth + 1); // recursive call
    }
};
