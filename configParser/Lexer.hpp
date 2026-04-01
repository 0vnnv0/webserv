/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Lexer.hpp                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: nrauh <nrauh@student.42berlin.de>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/26 21:51:13 by natalierauh       #+#    #+#             */
/*   Updated: 2026/01/08 13:58:48 by nrauh            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once
#include <iostream>
#include <stdexcept>
#include <fstream>
#include <cctype>
#include <string>
#include <istream>
#include "Token.hpp"
#include <vector>

class Lexer {
	private:
		std::string			filename;
		std::vector<Token>	tokens;

	public:
		Lexer();
		Lexer(std::string filename);
		Lexer(const Lexer &other);
		Lexer &operator=(const Lexer &other);
		~Lexer();

		void				setFilename(std::string filename);
		void				tokenize();
		std::vector<Token>	getTokens();

		// Validity Check Functions
		void				validateFilename() const;
		void				checkValidity() const;
		void				checkDelimiters() const;
		void				checkBraceBalance() const; // ✅ Braces balance → { and } must match.
		void				checkIllegalChars() const; // ✅ Illegal characters → reject tokens that contain characters that don’t belong (e.g. listen$8080).

		// Debug Functions
		void				printTokens();

		class LexerException: public std::exception {
			private:
				std::string	msg;
				
			public:
				explicit LexerException(const std::string& msg);
				virtual ~LexerException() throw();
				virtual char const *what(void) const throw();
		};
};
