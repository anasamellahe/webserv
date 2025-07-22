CXX = c++
CXXFLAGS = -Wall -Wextra -Werror -fPIE

NAME = webserv

SRC_DIR = src
OBJ_DIR = obj

# Get all source files
ALL_SRCS = $(wildcard $(SRC_DIR)/*.cpp $(SRC_DIR)/*/*.cpp)
# Exclude HTTP and Server directories
EXCLUDE_DIRS = $(wildcard $(SRC_DIR)/HTTP/*.cpp $(SRC_DIR)/Server/*.cpp)
# Filter out excluded directories
SRCS = $(filter-out $(EXCLUDE_DIRS), $(ALL_SRCS))
OBJS = $(patsubst $(SRC_DIR)/%.cpp,$(OBJ_DIR)/%.o,$(SRCS))

INCLUDES = -I./src

all: $(NAME)

$(NAME): $(OBJS)
		$(CXX) $(CXXFLAGS) $(OBJS) -o $(NAME)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
		@mkdir -p $(dir $@)
		$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

clean:
	rm -rf $(OBJ_DIR)

fclean: clean
	rm -f $(NAME)

re: fclean all
.PHONY: all clean fclean re