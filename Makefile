CXX = g++
CXXFLAGS = -std=c++17 -Wall

SRC_DIR = src
BIN_DIR = bin

TARGET = $(BIN_DIR)/arion

SRCS = $(shell find $(SRC_DIR) -name '*.cpp')
OBJS = $(SRCS:$(SRC_DIR)/%.cpp=$(BIN_DIR)/%.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	@mkdir -p $(BIN_DIR)
	$(CXX) $(CXXFLAGS) -o $@ $^

$(BIN_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -rf $(BIN_DIR)

run: $(TARGET)
	./$(TARGET) $(if $(FILE),$(FILE),test.txt)

.PHONY: all clean run
