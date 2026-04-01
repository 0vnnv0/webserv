/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Lexer.cpp                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: nrauh <nrauh@student.42berlin.de>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/26 21:54:50 by natalierauh       #+#    #+#             */
/*   Updated: 2026/01/08 17:08:18 by nrauh            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Lexer.hpp"

Lexer::Lexer(): filename(""), tokens() {};

Lexer::Lexer(std::string filename): filename(filename) {};

Lexer::Lexer(const Lexer &other): filename(other.filename), tokens(other.tokens) {};

Lexer &Lexer::operator=(const Lexer &other) {
	if (this != &other) {
		this->filename = other.filename;
		this->tokens = other.tokens;
	}
	return (*this);
};

Lexer::~Lexer() {};

void	Lexer::setFilename(std::string filename) {
	this->filename = filename;
}

void	Lexer::tokenize() {
	std::string	word;
	std::string line;
	size_t		start;
	std::string	d = "{;}";
	bool		comment;

	std::ifstream	ReadFile(this->filename.c_str());
	if (ReadFile.fail())
		throw LexerException("File cannot be opened!");
	while (std::getline(ReadFile, line)) {
		start = 0;
		comment = false;
		for (size_t i = 0; i < line.size(); i++) {
			if (line[i] == '#') {
				comment = true;
				break;
			}
			if (std::isspace(static_cast<unsigned char>(line[i]))) {
				word = line.substr(start, i - start);
				if (!word.empty())
					this->tokens.push_back(Token(word));
				start = i + 1;
			}
			if (d.find(line[i]) != std::string::npos) {
				if (i > start)
					this->tokens.push_back(Token(line.substr(start, i - start)));
				this->tokens.push_back(Token(line.substr(i, 1)));
				start = i + 1;
			}
		}
		if (start < line.size() && !comment) {
			word = line.substr(start);
			std::cout << "Word: " << word << std::endl;
			if (!word.empty())
				this->tokens.push_back(Token(word));
		}
	}
	if (ReadFile.bad()) {
    	throw LexerException("File cannot be opened!");
	}
	ReadFile.close();
};

std::vector<Token>	Lexer::getTokens() {
	return this->tokens;
};

void	Lexer::validateFilename() const {
	std::string::size_type start = this->filename.find_last_of("/");
	std::string cleaned;
	if (start != std::string::npos)
		cleaned = filename.substr(start + 1);
	else
		cleaned = filename;
		
	std::string::size_type pos = cleaned.find_last_of(".");
	std::string::size_type len = cleaned.size();
	std::string ext;
	std::string name;
	std::string illegalChars = "*?[]&;:|,.$ ()";
	
	if (len < 6)
		throw LexerException("Invalid filename!");
	else if (pos == std::string::npos || pos == 0 || pos == len - 1)
		throw LexerException("File extension is missing!");

	ext = cleaned.substr(pos);
	if (ext != ".conf")
		throw LexerException("Invalid file extension!");

	name = cleaned.substr(0, pos);
	for (std::string::size_type i = 0; i < name.size(); i++) {
		if (illegalChars.find(name[i]) != std::string::npos)
			throw LexerException("Invalid filename: Illegal chars found.");
	}
};

void	Lexer::checkValidity() const {
	this->checkBraceBalance();
	this->checkIllegalChars();
	this->checkDelimiters();
};

void	Lexer::checkDelimiters() const {
	int	brace_count = 0;
	int	semikolon_count = 0;

	for (std::vector<Token>::const_iterator it = this->tokens.begin(); it != this->tokens.end(); ++it) {
		if (it->getType() == LBRACE || it->getType() == RBRACE)
			brace_count++;
		if (it->getType() == SEMICOLON)
			semikolon_count++;
		if (it->getType() == WORD && (it + 1)->getType() == RBRACE)
			throw LexerException("Missing semicolon!");
	}
	if (brace_count < 2 || semikolon_count < 1)
		throw LexerException("Braces or semicolons missing!");
}

void	Lexer::checkBraceBalance() const {
	int	brace_balance = 0;

	for (std::vector<Token>::const_iterator it = this->tokens.begin(); it != this->tokens.end(); ++it) {
		if (it->getType() == LBRACE) {
			brace_balance++;
		} else if (it->getType() == RBRACE) {
			brace_balance--;
			if (brace_balance < 0)
				throw LexerException("Invalid amount of braces!");
		}
	}
	if (brace_balance != 0)
		throw LexerException("Invalid amount of braces!");
};

void	Lexer::checkIllegalChars() const {
	std::string	legalChars = "._/-\\~()|$:";

	for (std::vector<Token>::const_iterator it = this->tokens.begin(); it != this->tokens.end(); ++it) {
		if (it->getType() == WORD) {
			std::string	word = it->getValue();
			for (size_t i = 0; i < word.size(); i++) {
				if (!std::isalnum(word[i]) && legalChars.find(word[i]) == std::string::npos)
					throw LexerException("Illegal Char found!");
			}
		}
	}
};

// Exceptions

Lexer::LexerException::LexerException(const std::string& msg): msg(msg) {};

Lexer::LexerException::~LexerException() throw() {};

const char *Lexer::LexerException::what() const throw() {
	return this->msg.c_str();
}

// Debugging
void	Lexer::printTokens() {
	for (std::vector<Token>::iterator it = this->tokens.begin(); it != this->tokens.end(); ++it) {
		std::cout << *it << std::endl;
	}
};
