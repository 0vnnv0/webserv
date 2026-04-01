/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   MultipartParser.hpp                                :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: nrauh <nrauh@student.42berlin.de>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/26 16:32:32 by anschmit          #+#    #+#             */
/*   Updated: 2025/12/19 11:10:28 by nrauh            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once
#include <string>
#include <iostream>

class MultipartParser {

	private:
		std::string _boundary;
		std::string _body;
		MultipartParser(const MultipartParser &other);
        MultipartParser &operator=(const MultipartParser &other);
		bool extractBoundary(const std::string &contentType);
		bool extractFilename(size_t start, std::string &filename) const;
		bool extractFileContent(size_t start, std::string &fileContent) const;
	public:
		MultipartParser();
		~MultipartParser();
		MultipartParser(const std::string &contentType, const std::string &body);
		bool parse(std::string &filename, std::string &fileContent) const;
};
