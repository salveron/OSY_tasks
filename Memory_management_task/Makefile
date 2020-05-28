GCC = gcc
FLAGS = -Wall -pedantic -g
LIBS = -lm

all: memory_manager
run:
	valgrind ./memory_manager
memory_manager: main.cpp
	$(GCC) $(FLAGS) $< -o $@ $(LIBS)
clean:
	rm memory_manager 2>/dev/null