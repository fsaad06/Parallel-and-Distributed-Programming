CXX = g++
CXXFLAGS = -Wall -Wextra

SRC_DIR = .
SRCS := $(wildcard $(SRC_DIR)/*.cpp)
BINS := $(patsubst $(SRC_DIR)/%.cpp,%,$(SRCS))

.PHONY: all clean

all: $(BINS)

$(BINS): %: $(SRC_DIR)/%.cpp
	$(CXX) $(CXXFLAGS) -o $@ $<

clean:
	rm -f $(BINS)
