# **************************************************************************** #
#                                                                              #
#                                                         :::      ::::::::    #
#    Makefile                                           :+:      :+:    :+:    #
#                                                     +:+ +:+         +:+      #
#    By: tle-moel <tle-moel@student.42.fr>          +#+  +:+       +#+         #
#                                                 +#+#+#+#+#+   +#+            #
#    Created: 2025/04/16 14:43:25 by rpandipe          #+#    #+#              #
#    Updated: 2025/05/26 11:36:21 by tle-moel         ###   ########.fr        #
#                                                                              #
# **************************************************************************** #

CXX = c++
CFLAGS = -Wall -Wextra -Werror -Iincludes -std=c++98 -g
NAME = webserv
SRCS = $(shell find src -name '*.cpp')
HEADERS = $(shell find includes -name '*.hpp')
OBJSDIR = objs
OBJS = $(patsubst src/%.cpp, $(OBJSDIR)/%.o, $(SRCS))
RM = rm -rf

all: $(NAME)

$(OBJSDIR)/%.o: src/%.cpp $(HEADERS)
	@mkdir -p $(dir $@)
	@$(CXX) $(CFLAGS) -c $< -o $@

$(NAME) : $(OBJS)
	@$(CXX) $(CFLAGS) -o $@ $(OBJS)

clean:
	@$(RM) $(OBJSDIR)
	
fclean: clean
	@$(RM) $(NAME)

re: fclean all

.PHONY: all re clean fclean