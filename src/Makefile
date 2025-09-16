# Simple Makefile for webserv (placed in src/)
CXX = g++
CXXFLAGS = -std=c++98 -Wall -Wextra -I./ -pthread
SRCDIR = .
BUILDDIR = ../build
TARGET = $(BUILDDIR)/webserv

SOURCES := $(shell find . -name "*.cpp")
OBJECTS := $(patsubst ./%.cpp, $(BUILDDIR)/%.o, $(SOURCES))

all: $(TARGET)

$(TARGET): $(OBJECTS)
	@mkdir -p $(BUILDDIR)
	$(CXX) $(CXXFLAGS) -o $@ $^

$(BUILDDIR)/%.o: ./%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

run: all
	$(TARGET)

clean:
	rm -rf $(BUILDDIR)

.PHONY: all clean run
