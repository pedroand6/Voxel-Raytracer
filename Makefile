# Use CXX for C++ compiler AND linker
CXX = g++
# Use CC for C compiler
CC = gcc

SRC_DIR = src
OBJ_DIR = build
INC_DIR = include
LIB_DIR = lib

# --- Source and Object Files ---
C_SRC_FILES = $(wildcard $(SRC_DIR)/*.c)
CPP_SRC_FILES = $(wildcard $(SRC_DIR)/*.cpp)
C_OBJ_FILES = $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(C_SRC_FILES))
CPP_OBJ_FILES = $(patsubst $(SRC_DIR)/%.cpp, $(OBJ_DIR)/%.o, $(CPP_SRC_FILES))
OBJ_FILES = $(C_OBJ_FILES) $(CPP_OBJ_FILES)

FINAL = main

# --- Flags ---
# C++ specific flags
CXXFLAGS = -g -std=c++17 -Wall -I$(INC_DIR) -I$(INC_DIR)/vmm
# C specific flags
CFLAGS = -g -Wall -I$(INC_DIR) -I$(INC_DIR)/vmm

# Linker flags
LDFLAGS = -L$(LIB_DIR)

# --- Library Definitions ---
ifeq ($(OS), Windows_NT)
	LIBS = -lws2_32 -lpthread -lglfw3dll -lgdi32 -lvmm -lm
	REMOVE = rmdir /s /q
	TARGET_EXT = .exe
else
	LIBS = -lpthread -lglfw3dll -lvmm -lm
	REMOVE = rm -rf
	TARGET_EXT =
endif

# --- Build Rules ---

# Default target
all: $(FINAL)

# Link rule: Depends on object files
# Uses CXX (g++) to link
$(FINAL): $(OBJ_FILES)
	$(CXX) $^ $(LDFLAGS) -Wl,--start-group $(LIBS) -Wl,--end-group -o $@$(TARGET_EXT)

# Compile rule for .cpp files
# Uses CXX (g++) and CXXFLAGS
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp | $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Compile rule for .c files
# *** THIS IS THE FIX ***
# Uses CC (gcc) and CFLAGS
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Rule to create the build directory
$(OBJ_DIR):
	mkdir -p $@

# --- Clean Rules ---
clean_all: clean

clean:
	$(REMOVE) -p
	$(REMOVE) $(OBJ_DIR) $(FINAL)$(TARGET_EXT)

# Phony targets aren't real files
.PHONY: all clean clean_all