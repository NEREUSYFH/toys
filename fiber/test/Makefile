
cc=g++
ccflags=-std=c++11 -DDEBUG -O0 -I../include -Wall -Wextra
ldflags=-lboost_coroutine -lboost_system 

all: test test_fiber

test: test.cpp ../include/fiber.hpp
	$(cc) $(ccflags) $(ldflags) $< -o $@

test_fiber: test_fiber.cpp ../include/fiber.hpp
	$(cc) $(ccflags) $(ldflags) $< -o $@

clean:
	rm test
	rm test_fiber
