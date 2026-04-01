/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Token.hpp                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: nrauh <nrauh@student.42berlin.de>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/21 14:40:50 by nrauh             #+#    #+#             */
/*   Updated: 2026/01/09 14:37:20 by nrauh            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once
#include <iostream>
#include <vector>
#include <string>

enum TokenType {
	LBRACE,
	RBRACE,
	SEMICOLON,
	WORD,
	UNKNOWN,
	ENDOFFILE,
};

struct TokenEntry {
	std::string 	value;
	TokenType		token;
};

class Token {
	private:
		TokenType		type;
		std::string		value;

	public:
		Token(std::string value);
		Token(const Token &other);
		Token &operator=(const Token &other);
		~Token();

		std::string 	getValue() const;
		TokenType 		getType() const;
		TokenType		classifyToken() const;
		std::string		tokenToString() const;
};

std::ostream &operator<<(std::ostream &out, const Token &token);
