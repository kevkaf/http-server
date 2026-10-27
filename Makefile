TARGET	:= bin/http
TEST_TARGET	:= bin/tests
SRC_DIR	:= src
INC_DIR	:= include
TEST_DIR	:= tests
BUILD_DIR	:= build
OBJ_DIR	:= $(BUILD_DIR)/obj	


CC	:=gcc
CFLAGS	:= -Wall -Wextra -Werror -g -I$(INC_DIR)


SRCS	:= $(wildcard $(SRC_DIR)/*.c)
OBJS	:= $(SRCS:$(SRC_DIR)/*.c=$(OBJ_DIR)/%.o)

TESTS_SRCS	:= $(wildcard $(TEST_DIR)/*.c)
TESTS_OBJS	:= $(OBJ_DIR)/parser.o $(TESTS_SRCS:$(TEST_DIR)/%.c=$(OBJ_DIR)/%.o)


.PHONY: all
all: $(TARGET)


$(TARGET): $(OBJS)
	@mkdir -p $(dir $@)
	$(CC) $^ -o $@


.PHONY: test
test: $(TEST_TARGET)
	@echo "========================================"
	@echo " Running HTTP Parser Tests... "
	@echo "========================================"
	@./$(TEST_TARGET)

# Link the test runner binary
$(TEST_TARGET): $(TEST_OBJS)
	@mkdir -p $(dir $@)
	$(CC) $(LDFLAGS) $^ $(LDLIBS) -o $@


.PHONY: clean
clean: 
	rm -rf $(BUILD_DIR) bin/
