/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Token.cpp                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: nrauh <nrauh@student.42berlin.de>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/21 14:44:51 by nrauh             #+#    #+#             */
/*   Updated: 2026/01/09 14:37:25 by nrauh            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Token.hpp"

static const TokenEntry tokenTable[] = {
	{ "{", LBRACE },
	{ "}", RBRACE },
	{ ";", SEMICOLON }
};

const int tableSize = sizeof(tokenTable) / sizeof(tokenTable[0]);

Token::Token(std::string value): value(value) {
	this->type = classifyToken();
};

Token::Token(const Token &other) {
	*this = other;
};

Token& Token::operator=(const Token &other) {
	if (this != &other)
	{
		this->type = other.type;
		this->value = other.value;
	}
	return (*this);
};

Token::~Token() {};

std::string Token::getValue() const {
	return (this->value);
};

TokenType Token::getType() const {
	return (this->type);
};

TokenType Token::classifyToken() const {
	for (int i = 0; i < tableSize; i++) {
		if (this->value == tokenTable[i].value)
			return tokenTable[i].token;
	}
	return WORD;
}

std::string	Token::tokenToString() const {
	switch (this->type) {
		// case COMMENT:
		// 	return "COMMENT";
		case LBRACE:
			return "LBRACE";
		case RBRACE:
			return "RBRACE";
		case SEMICOLON:
			return "SEMICOLON";
		case WORD:
			return "WORD";
		case ENDOFFILE:
			return "ENDOFFILE";
		default:
			return "UNKNOWN";
	}
}

std::ostream &operator<<(std::ostream &out, const Token &token) {
	out << token.tokenToString() << "(" << token.getValue() << ")";
	return (out);
}
