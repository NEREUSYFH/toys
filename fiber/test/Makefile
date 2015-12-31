
cc=g++
ccflags_o=-std=c++11 -DDEBUG -g -O0 -I../include -Wall -Wextra
ccflags=$(ccflags_o) -DUSE_BOOST_COROTUINE
#ccflags=$(ccflags_o)
ldflags=-lboost_coroutine -lboost_system 

all: test

test: test.cpp ../include/fiber.hpp
	$(cc) $(ccflags) $(ldflags) $< -o $@

test_fiber: test_fiber.cpp ../include/fiber.hpp
	$(cc) $(ccflags) $(ldflags) $< -o $@

clean:
	rm test
