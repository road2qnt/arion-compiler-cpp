CXX := g++
CXXFLAGS := -std=c++17 -Wall
DEPFLAGS := -MMD -MP

SRC_DIR := src
BIN_DIR := bin

TARGET := $(BIN_DIR)/arion

SRC_DIRS := \
	$(SRC_DIR) \
	$(SRC_DIR)/lexical-analysis \
	$(SRC_DIR)/syntax-analysis

SRCS := $(foreach dir,$(SRC_DIRS),$(wildcard $(dir)/*.cpp))
OBJS := $(patsubst $(SRC_DIR)/%.cpp,$(BIN_DIR)/%.o,$(SRCS))
DEPS := $(OBJS:.o=.d)

all: $(TARGET)

$(TARGET): $(OBJS)
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -o $@ $^

$(BIN_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(DEPFLAGS) -c $< -o $@

run: $(TARGET)
	./$(TARGET) $(if $(FILE),$(FILE),test.txt)

clean:
	rm -rf $(BIN_DIR)

-include $(DEPS)

.PHONY: all run clean
