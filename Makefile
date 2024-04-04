# Compiler settings
CXX = g++
CXXFLAGS = -std=c++11 -Wall -Wextra -pedantic -I$(LLAMA_DIR)

# Paths
LLAMA_DIR = C:/Users/pixlo/Desktop/Lucys_Labyrinth/llama_cpp
LLAMA_OBJS = $(addprefix $(LLAMA_DIR)/, llama.o ggml.o ggml-backend.o ggml-alloc.o ggml-quants.o common.o sampling.o grammar-parser.o unicode.o build-info.o)

# Your project file
SOURCE = main.cpp
OBJECT = $(SOURCE:.cpp=.o)
EXECUTABLE = game

# Build rules
all: $(EXECUTABLE)

$(EXECUTABLE): $(OBJECT)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LLAMA_OBJS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJECT) $(EXECUTABLE)

.PHONY: all clean