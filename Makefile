CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -g
#CFLAGS = -Wall -Wextra -std=c99 -g -fsanitize=address
SRC_DIR = src
BUILD_DIR = build
TEST_DIR = tests
TARGET = shade

SOURCES = $(wildcard $(SRC_DIR)/*.c) \
          $(wildcard $(SRC_DIR)/types/*.c) \
          $(wildcard $(SRC_DIR)/storage/*.c) \
          $(wildcard $(SRC_DIR)/query/*.c) \
          $(wildcard $(SRC_DIR)/ghost/*.c) \
          $(wildcard $(SRC_DIR)/api/*.c) \
          $(wildcard $(SRC_DIR)/util/*.c)

OBJECTS = $(SOURCES:%.c=$(BUILD_DIR)/%.o)

TEST_SOURCES = $(wildcard $(TEST_DIR)/*.c)
TEST_OBJECTS = $(TEST_SOURCES:%.c=$(BUILD_DIR)/%.o)
TEST_BINARIES = $(TEST_SOURCES:$(TEST_DIR)/%.c=$(BUILD_DIR)/tests/%)

all: $(BUILD_DIR)/$(TARGET)

$(BUILD_DIR)/$(TARGET): $(OBJECTS)
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -static $(OBJECTS) -o $@

$(BUILD_DIR)/%.o: %.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -c $< -o $@

SOURCES_NO_MAIN = $(filter-out $(SRC_DIR)/main.c, $(SOURCES))

test:
ifdef TEST
	@mkdir -p $(BUILD_DIR)/tests
	@echo "Building and running test: $(TEST)"
	$(CC) $(CFLAGS) -I$(SRC_DIR) $(TEST_DIR)/$(TEST).c $(SOURCES_NO_MAIN) -o $(BUILD_DIR)/tests/$(TEST)
	@echo "Running $(BUILD_DIR)/tests/$(TEST)..."
	@$(BUILD_DIR)/tests/$(TEST)
else
	@for t in $(TEST_BINARIES); do echo "Running $$t..."; $$t || exit 1; done
endif

$(BUILD_DIR)/tests/%: $(BUILD_DIR)/tests/%.o $(OBJECTS)
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) $^ -o $@

clean:
	rm -rf $(BUILD_DIR)

.PHONY: all clean test
