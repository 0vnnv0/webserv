/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Utils.hpp                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: ilazar <ilazar@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/10/30 16:01:19 by nrauh             #+#    #+#             */
/*   Updated: 2025/11/27 14:13:42 by nrauh            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <string>
#include <sstream>
#include <stdexcept>
#include <vector>
#include <sys/stat.h>
#include <iostream>

// This class acts as a container for static, reusable functions.
// It effectively serves as a "namespace" when namespaces are forbidden.
class Utils {
private:
    // Private constructor prevents creation of Utils objects.
    Utils();
    // Private copy constructor and assignment operator prevent copying.
    Utils(const Utils&);
    Utils& operator=(const Utils&);

public:
    // Static functions can be called directly using the class name:
    // Example: Utils::stringToInt("123");

    /**
     * @brief Converts a string to an integer, throws std::runtime_error on failure.
     * @param str The input string.
     * @return The resulting integer.
     */
    static int stringToInt(const std::string& str);
	static bool isNumber(const std::string& str);
	static bool isDirectory(const std::string &path);
	static bool isFile(const std::string &path);
	static std::vector<std::string> split(const std::string &str, char delim);
	static std::string stripFirstSlash(const std::string& str);
};
