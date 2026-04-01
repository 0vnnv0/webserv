# **************************************************************************** #
#                                                                              #
#                                                         :::      ::::::::    #
#    Makefile                                           :+:      :+:    :+:    #
#                                                     +:+ +:+         +:+      #
#    By: ilazar <ilazar@student.42.fr>              +#+  +:+       +#+         #
#                                                 +#+#+#+#+#+   +#+            #
#    Created: 2025/09/24 13:53:19 by ilazar            #+#    #+#              #
#    Updated: 2025/12/12 19:20:46 by ilazar           ###   ########.fr        #
#                                                                              #
# **************************************************************************** #

CXX = c++
CXXFLAGS = -Wall -Wextra -Werror -std=c++98 -I. -Iserver -ICgi -Ilogger -Ihttp -IconfigParser

GREEN = \033[0;32m
YELLOW = \033[0;33m
RESET = \033[0m

NAME = webserv

OBJDIR = obj

SRCS 	= \
		main.cpp \
		configParser/ConfigWrapper.cpp \
		configParser/Lexer.cpp \
		configParser/Parser.cpp \
		configParser/Token.cpp \
		configParser/Utils.cpp \
		server/Server.cpp \
		server/Client.cpp \
		server/Epoll.cpp \
		server/RequestBuffer.cpp \
		server/ServerManager.cpp \
		http/httpRequest.cpp \
		http/httpResponse.cpp \
		http/RequestHandler.cpp \
		http/MultipartParser.cpp \
		Cgi/Cgi.cpp \
		Cgi/CgiStdinHandler.cpp \
		Cgi/CgiStdoutHandler.cpp \
		logger/Logger.cpp
		
OBJS		= $(addprefix $(OBJDIR)/,$(SRCS:.cpp=.o))
DEPS     = $(OBJS:.o=.d)

all: $(NAME)

$(NAME): $(OBJS)
	@$(CXX) $(CXXFLAGS) -o $@ $(OBJS)
	@echo "$(GREEN) Executable $(NAME) created.$(RESET)"

$(OBJDIR)/%.o: %.cpp
	@mkdir -p $(dir $@)
	@echo "$(YELLOW) Compiling $< ... $(RESET)"
	@$(CXX) $(CXXFLAGS) -MMD -MP -c $< -o $@

-include $(DEPS)

clean:
	@echo "$(GREEN)Cleaning object files...$(RESET)"
	@rm -rf $(OBJDIR)

fclean: clean
	@echo "$(GREEN)Removing executable ..."
	@rm -f $(NAME)

re: fclean all

kill:
	lsof -i :8080 || true
	pkill -f webserv || true

.PHONY: all clean fclean re kill
