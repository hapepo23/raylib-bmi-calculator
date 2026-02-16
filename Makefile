CC      ?= gcc
CFLAGS  ?= -std=c18 -Wall -Wextra -O2 -g 
LDFLAGS ?= /usr/lib/libraylib.a -lm -lpthread -ldl -lrt -lX11

TARGET_EXEC ?= bmicalc
BUILD_DIR   ?= ./build
SRC_DIRS    ?= ./src

MKDIR_P ?= mkdir -p

# Find all source files recursively
SRCS := $(shell find $(SRC_DIRS) -name "*.c")

# Flatten object files into BUILD_DIR
OBJS := $(patsubst $(SRC_DIRS)/%.c,$(BUILD_DIR)/%.o,$(SRCS))
DEPS := $(OBJS:.o=.d)

# Include directories
INC_DIRS := $(shell find $(SRC_DIRS) -type d)
INC_FLAGS := $(addprefix -I,$(INC_DIRS))
CPPFLAGS ?= $(INC_FLAGS) -MMD -MP

.PHONY: all clean run reformat
all: $(TARGET_EXEC)

# Link target → output in project root
$(TARGET_EXEC): $(OBJS)
	$(CC) $(OBJS) -o $@ $(LDFLAGS)

# Compile C files into flattened object files
$(BUILD_DIR)/%.o: $(SRC_DIRS)/%.c
	@$(MKDIR_P) $(dir $@)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

# Clean build directory + executable
clean:
	rm -rf "$(BUILD_DIR)" "$(TARGET_EXEC)"

run:
	./$(TARGET_EXEC)

reformat:
	clang-format --style=Chromium -i $(SRC_DIRS)/*.c $(SRC_DIRS)/*.h

# Include generated dependency files
-include $(DEPS)

