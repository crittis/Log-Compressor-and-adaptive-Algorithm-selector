# ---------- Compiler settings ----------
CXX      := g++
CXXFLAGS := -std=c++17 -O2 -Wall -Wextra -Isrc

# ---------- Folders ----------
SRC_DIR   := src
TEST_DIR  := tests
BUILD_DIR := build
BIN_DIR   := bin

# ---------- Windows vs Linux/Mac ----------
ifeq ($(OS),Windows_NT)
    EXE   := .exe
    MKDIR  = if not exist "$(subst /,\,$(1))" mkdir "$(subst /,\,$(1))"
    RMDIR  = if exist "$(subst /,\,$(1))" rmdir /S /Q "$(subst /,\,$(1))"
else
    EXE   :=
    MKDIR  = mkdir -p $(1)
    RMDIR  = rm -rf $(1)
endif

# ---------- Source files ----------
# Everything in src/ except main.cpp is a "library" file shared with the tests
LIB_SRCS  := $(filter-out $(SRC_DIR)/main.cpp, $(wildcard $(SRC_DIR)/*.cpp))
LIB_OBJS  := $(patsubst $(SRC_DIR)/%.cpp, $(BUILD_DIR)/%.o, $(LIB_SRCS))
MAIN_OBJ  := $(BUILD_DIR)/main.o

# Every tests/test_*.cpp becomes its own executable
TEST_SRCS := $(wildcard $(TEST_DIR)/test_*.cpp)
TEST_BINS := $(patsubst $(TEST_DIR)/%.cpp, $(BIN_DIR)/%$(EXE), $(TEST_SRCS))

TARGET := $(BIN_DIR)/logx$(EXE)

# ---------- Targets ----------
.PHONY: all test clean run

all: $(TARGET)

# Main program
$(TARGET): $(MAIN_OBJ) $(LIB_OBJS) | $(BIN_DIR)
	$(CXX) $(CXXFLAGS) $^ -o $@

# src/*.cpp -> build/*.o  (also rebuilds if any header in src/ changes)
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp $(wildcard $(SRC_DIR)/*.hpp) | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Each test program links with the library objects (not main.o)
$(BIN_DIR)/test_%$(EXE): $(TEST_DIR)/test_%.cpp $(LIB_OBJS) | $(BIN_DIR)
	$(CXX) $(CXXFLAGS) $< $(LIB_OBJS) -o $@

# Build and run all tests
test: $(TEST_BINS)
	@for t in $(TEST_BINS); do echo "== $$t =="; ./$$t || exit 1; done

run: $(TARGET)
	./$(TARGET)

# Create folders
$(BUILD_DIR):
	@$(call MKDIR,$(BUILD_DIR))

$(BIN_DIR):
	@$(call MKDIR,$(BIN_DIR))

clean:
	@$(call RMDIR,$(BUILD_DIR))
	@$(call RMDIR,$(BIN_DIR))