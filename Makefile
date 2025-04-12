## DOCUMENTATION
# make setup: create .gitignore, .clangd, and if missing BIN_DIR and OBJ_DIR
# make clean: rm BIN_DIR, OBJ_DIR
# to change between C and C++ edit CXX, CX, BASE_FLAGS variables
# to add libraries edit EXT_LIBS variable - can also be empty

## BASE VARS
SRC_NAMES :=
SRC_DIR := src
OBJ_DIR := obj
BIN_DIR := bin

## COMPILER AND FILETYPE
CXX := clang++
CX := cc

## GENERATE VARS FROM SRC_NAMES
EXE := $(BIN_DIR)/a.out
SRC_FILES := $(addprefix $(SRC_DIR)/, $(addsuffix .$(CX), $(SRC_NAMES)))
OBJ_FILES := $(addprefix $(OBJ_DIR)/, $(addsuffix .o, $(SRC_NAMES)))
ASM_FILES := $(addprefix $(OBJ_DIR)/, $(addsuffix .s, $(SRC_NAMES)))

## FLAGS
BASE_FLAGS := -std=c++23 -I$(SRC_DIR)
# Create dependency files 
# If header changes, trigger recompilation of dependent source files
DEP_FLAGS := -MMD -MP
-include $(OBJ_FILES:.o=.d)

# Warning and Debug flags, debug flags need to be included when linking
M ?= debug
ifeq ($(M), debug)
	DEBUG_FLAGS := -g -fsanitize=address,undefined
	WARNING_FLAGS := -Wall -Wextra -pedantic
else ifeq ($(M), thread)
	DEBUG_FLAGS := -g -fsanitize=thread
	WARNING_FLAGS := -Wall -Wextra -pedantic
else ifeq ($(M), simple)
	DEBUG_FLAGS :=
	WARNING_FLAGS := -Wall
endif

## Debug Flag presets

CXXFLAGS := $(BASE_FLAGS) $(DEBUG_FLAGS) $(WARNING_FLAGS) $(DEP_FLAGS) 
LDFLAGS := -lpthread -lm $(DEBUG_FLAGS) 

## External libraries
EXT_LIBS := sdl3

# Only call pkg-config if at least one external library is specified
ifeq ($(EXT_LIBS),)
	EXT_CFLAGS :=
	EXT_LDFLAGS :=
else
	EXT_CFLAGS := $(shell pkg-config --cflags $(EXT_LIBS))
	EXT_LDFLAGS := $(shell pkg-config --libs $(EXT_LIBS))
endif

CXXFLAGS += $(EXT_CFLAGS)
LDFLAGS += $(EXT_LDFLAGS)

## TARGETS
# Phony targets aren't treated as files
.PHONY: all run asm clean

# Default target, executed with 'make' command
all: $(EXE)

# Execute immediatelly after building
run: $(EXE)
	./$(EXE)

asm: $(ASM_FILES)
	@echo "Assembly files generated in $(OBJ_DIR): $(ASM_FILES)"

# Link all the objectfiles into an exe
$(EXE): $(OBJ_FILES) | $(BIN_DIR)
	$(CXX) $(LDFLAGS) $^ -o $@
	@dsymutil $@ 2>/dev/null || true  # macOS only, fails silently on other OS

# Pattern rule for .s files
$(OBJ_DIR)/%.s: $(SRC_DIR)/%.$(CX) | $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) -S $< -o $@

# Pattern rule for .o files
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.$(CX) | $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Make sure directories exist
$(OBJ_DIR) $(BIN_DIR):
	mkdir -p $@

# Clean for rebuilt - Using implicit variable RM (rm -f)
clean:
	$(RM) -r $(OBJ_DIR) $(BIN_DIR)

setup: $(OBJ_DIR) $(BIN_DIR)
	@echo "Generating .clangd"
	@echo "CompileFlags:" > .clangd
	@echo "  Add: [" >> .clangd
	@for flag in $(CXXFLAGS); do \
		echo "    \"$$flag\"," >> .clangd; \
	done
	@echo "  ]" >> .clangd

	@echo "Generating .gitignore"
	@echo "/bin" > .gitignore
	@echo "/obj" >> .gitignore
	@echo ".gitignore" >> .gitignore
	@echo ".clangd" >> .gitignore

## Additional Information
# normal-prerequisites | order-only-prerequisites (no check for changes)

# $^: all prerequisites
# $<: first prerequisite
# $@: target

# -MDD, -MP: create .d files for header deps
# dsymutil: extract debug info (Mac only)
# Make always uses /bin/sh as shell

## Helpful Commands
# as foo.s -o foo.o && ld -no pie foo.o -o foo
# gcc hello.s -o hello -no pie

# ar r libfoo.a foo.o bar.o
# ranlib libfoo.a
# -L. -lfoo
