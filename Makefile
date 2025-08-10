CXX = c++
CXXFLAGS = -Wall -Wextra -Werror 

NAME = webserv

SRC_DIR = src
OBJ_DIR = obj

SRCS = $(wildcard $(SRC_DIR)/*.cpp $(SRC_DIR)/*/*.cpp)
OBJS = $(patsubst $(SRC_DIR)/%.cpp,$(OBJ_DIR)/%.o,$(SRCS))

INCLUDES = -I./src

# Add DirectoryListing.cpp to HTTP objects
HTTP_SOURCES = src/HTTP/Request.cpp src/HTTP/Response.cpp src/HTTP/Utils.cpp src/HTTP/DirectoryListing.cpp
HTTP_OBJECTS = $(patsubst src/%.cpp,obj/%.o,$(HTTP_SOURCES))

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