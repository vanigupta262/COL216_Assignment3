CC = g++
CFLAGS = -Wall -g -std=c++11
TARGET = L1simulate
SOURCES = main.cpp cache.cpp bus.cpp
OBJECTS = $(SOURCES:.cpp=.o)

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) -o $(TARGET)

%.o: %.cpp cache.hpp bus.hpp
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJECTS) $(TARGET)

.PHONY: all clean