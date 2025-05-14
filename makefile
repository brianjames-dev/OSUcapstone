# Compiler
CXX = g++

# Compiler flags
CXXFLAGS = -std=c++20 -Wall -Wextra -pedantic

# Check OS
UNAME_S := $(shell uname -s)

# Set SDL2 flags based on OS
SDL_CXXFLAGS =
SDL_LDFLAGS =

ifeq ($(UNAME_S), Linux)
	ECHO_MESSAGE = "Linux"
	SDL_CXXFLAGS = $(shell sdl2-config --cflags)
	SDL_LDFLAGS = $(shell sdl2-config --libs)
endif

ifeq ($(UNAME_S), Windows_NT)
	ECHO_MESSAGE = "MinGW"
	SDL_CXXFLAGS = -I/mingw64/include/SDL2
	SDL_LDFLAGS = -L/mingw64/lib -lmingw32 -lSDL2 -mconsole
endif

# Target executable
TARGET = emulator

# Source files
SRCS = CPU.cpp main.cpp tests.cpp ROM.cpp NES.cpp Bus.cpp APU.cpp PPU.cpp

# Object files
OBJS = $(SRCS:.cpp=.o)

# Default target
all: $(TARGET)

# Link the executable w/ SDL2 (audio)
$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(SDL_LDFLAGS)

# Compile source files into object files with SDL2 includes
%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(SDL_CXXFLAGS) -c $< -o $@

# Clean up build files
clean:
	rm -f $(OBJS) $(TARGET)

# Phony targets
.PHONY: all clean

