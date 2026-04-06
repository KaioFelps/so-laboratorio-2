CXX = g++
CXXFLAGS = --std=c++23

BUILD_DIR = build
BIN_NAME = exercicio_4

SOURCES = $(wildcard *.cpp)

OBJS = $(patsubst %.cpp, $(BUILD_DIR)/%.o, $(SOURCES))

build: $(OBJS)
	$(CXX) $(OBJS) -o $(BIN_NAME)

$(BUILD_DIR)/%.o: %.cpp
	mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -rf $(BUILD_DIR) $(BIN_NAME)

.PHONY: build clean