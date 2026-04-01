/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Utils.cpp                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: nrauh <nrauh@student.42berlin.de>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/10/30 16:02:22 by nrauh             #+#    #+#             */
/*   Updated: 2025/11/27 14:17:18 by nrauh            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Utils.hpp"

// Define the private constructor and copy/assignment operators (no body needed)
Utils::Utils() {}
Utils::Utils(const Utils&) {}
Utils& Utils::operator=(const Utils&) { return *this; }

// Define the static helper function
int Utils::stringToInt(const std::string& str) {
    int value;
    
    // Use stringstream for robust C++98 conversion
    std::stringstream ss(str);
    ss >> value;

    // Check for failure (no characters were extracted) or remaining unread characters
    if (ss.fail() || !ss.eof()) {
        throw std::runtime_error("Invalid conversion: \"" + str + "\" is not a valid integer.");
    }
    return value;
}

bool Utils::isNumber(const std::string &str) {
    return !str.empty() && str.find_first_not_of("0123456789") == std::string::npos;
}

bool Utils::isDirectory(const std::string &path)
{
    struct stat info;
    if (stat(path.c_str(), &info) != 0)
        return false; // path doesn't exist or can't be accessed
    return (info.st_mode & S_IFDIR) != 0;
}

bool Utils::isFile(const std::string &path)
{
    struct stat info;
    if (stat(path.c_str(), &info) != 0)
        return false;
    return (info.st_mode & S_IFREG) != 0;
}

std::vector<std::string> Utils::split(const std::string &str, char delim) {
	std::vector<std::string>	split;
	size_t						start = 0;
	
	for (size_t end = 0; end < str.size(); end++) {
		std::cout << "Curr " << str[end] << std::endl;
		if (str[end] == delim) {
			split.push_back(str.substr(start, end - start));
			start = end + 1;
			std::cout << split.back() << std::endl;
		}
	}
	return split;
};

std::string Utils::stripFirstSlash(const std::string& str) {
	if (!str.empty() && str[0] == '/')
		return str.substr(1);
	return str;
};
