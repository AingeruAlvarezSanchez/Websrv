#SHELL = /bin/bash

NAME = webserv

CXXFLAGS = -Wall -Werror -Wextra -std=c++98 -pedantic -Wshadow -g3 -fsanitize=address

SRCS = webserv.cpp

OBJS = $(SRCS:.cpp=.o)

all: $(NAME)

$(NAME): $(OBJS)
	$(CXX) $(CXXFLAGS) $< -o $@

%.o:%.cpp
	$(CXX) $(CXXFLAGS) -c $^ -o $@

clean:
	$(RM) $(OBJS)

fclean: clean
	$(RM) $(NAME)

re: fclean all

.PHONY: all clean fclean re