RED = \033[0;31m
GREEN = \033[0;32m
YELLOW = \033[0;33m
RESET = \033[0m

SRC = srcs/main.cpp srcs/server.cpp srcs/commands.cpp srcs/client.cpp srcs/parser.cpp

NAME = ircserv

OBJ = $(SRC:.cpp=.o)

FLAGS = -std=c++98 -Wall -Werror -Wextra -Iincludes

CXX = c++

%.o: %.cpp
	@$(CXX) $(FLAGS) -c $< -o $@

all	: $(NAME)

$(OBJ): includes/server.hpp includes/client.hpp includes/parser.hpp

$(NAME)	: $(OBJ)
	@echo "$(YELLOW)[$(NAME)] $(GREEN).o created $(RESET)"
	@$(CXX) $(FLAGS) $(OBJ) -o $(NAME)
	@echo "$(YELLOW)[$(NAME)] $(GREEN)executable created successfully$(RESET)"

clean	:
	@rm -rf $(OBJ)
	@echo "$(YELLOW)[$(NAME)] $(RED).o deleted$(RESET)"

fclean	: clean
	@rm -rf $(NAME)
	@echo "$(YELLOW)[$(NAME)] $(RED).a deleted$(RESET)"
	@echo "$(YELLOW)[$(NAME)] $(GREEN)forced clean successfull$(RESET)"

re	: fclean all

.PHONY: all clean fclean re
