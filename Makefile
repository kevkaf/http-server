TARGET	:= bin/http
TEST_TARGET	:= bin/http_tests

SRC_DIR	:= src
INCLUDE_DIR	:= include
TEST_DIR	:= tests
BUILD_DIR	:= build
OBJS_DIR	:= $(BUILD_DIR)/obj


CC	:= gcc
CFLAGS	:= -Wall -Wextra -Werror -pedantic -g -I$(INCLUDE_DIR) -MMD -MP

SRCS	:= $(wildcard $(SRC_DIR)/*.c)
OBJS	:= $(patsubst $(SRC_DIR)/%.c, $(OBJS_DIR)/%.o, $(SRCS))


TEST_SRC	:= $(wildcard $(TEST_DIR)/*.c)
TEST_OBJ	:= $(patsubst $(TEST_DIR)/%.c, $(OBJS_DIR)/test_%.o, $(TEST_SRC))

CORE_TEST_OBJS	:= $(filter-out $(OBJS_DIR)/main.o, $(OBJS))

DEPS	:= $(OBJS:.o=.d) $(TEST_OBJ:.o=.d)

.PHONY: all clean fclean re

all: $(TARGET)


$(TARGET): $(OBJS)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $^ -o $@
	@echo "Built successfully: $@"


test: $(TEST_TARGET)
	@echo "Running test suite..."
	@./$(TEST_TARGET)

$(TEST_TARGET): $(TEST_OBJ) $(CORE_TEST_OBJS)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $^ -o $@
	@echo "Test binary successfully built: $@"



$(OBJS_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@
	@echo "Built successfully: $@"


$(OBJS_DIR)/test_%.o: $(TEST_DIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@
	@echo "compiled test: $< -> $@"


clean: 
	@rm -rf $(BUILD_DIR)
	@echo "Clean object and dependency files"
	
fclean: clean
	@rm -rf bin
	@echo "Removed compiled binaries"


re: fclean all


-include $(DEPS)
