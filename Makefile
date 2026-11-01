TARGET	:= bin/http

SRC_DIR	:= src
INCLUDE_DIR	:= include
BUILD_DIR	:= build
OBJS_DIR	:= $(BUILD_DIR)/obj


CC	:= gcc
CFLAGS	:= -Wall -Wextra -Werror -pedantic -g -I$(INCLUDE_DIR)

SRCS	:= $(wildcard $(SRC_DIR)/*.c)
OBJS	:= $(patsubst $(SRC_DIR)/%.c, $(OBJS_DIR)/%.o, $(SRCS))

.PHONY: all clean fclean re

all: $(TARGET)


$(TARGET): $(OBJS)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $^ -o $@
	@echo "Built successfully: $@"


$(OBJS_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@
	@echo "Built successfully: $@"


clean: 
	@rm -rf $(BUILD_DIR)
	@echo "Clean object and dependency files"
	
fclean:
	@rm -rf bin
	@echo "Removed compiled binaries"


re: fclean all
