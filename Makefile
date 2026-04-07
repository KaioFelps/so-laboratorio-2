CXX = g++
CXXFLAGS = --std=c++23 -Iinclude

SRC_DIR = src
BUILD_DIR = build
BIN_NAME = exercicio_4

SOURCES = $(shell find $(SRC_DIR) -name "*.cpp")

OBJS = $(patsubst $(SRC_DIR)/%.cpp, $(BUILD_DIR)/%.o, $(SOURCES))

build: $(OBJS)
	$(CXX) $(OBJS) -o $(BIN_NAME)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -rf $(BUILD_DIR) $(BIN_NAME)

.PHONY: build clean