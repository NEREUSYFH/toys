
cc=g++
ccflags_o=-std=c++11 -DDEBUG -g -O0 -I../include -Wall -Wextra
#ccflags=$(ccflags_o) -DUSE_BOOST_COROTUINE
ccflags=$(ccflags_o)
ldflags=-lboost_coroutine -lboost_system 

all: test_client test_server

test: test.cpp ../include/fiber.hpp ../include/socketstream.hpp
	$(cc) $(ccflags) $(ldflags) $< -o $@

test_client: test_client.cpp ../include/fiber.hpp ../include/socketstream.hpp
	$(cc) $(ccflags) $(ldflags) $< -o $@

test_server: test_server.cpp ../include/fiber.hpp ../include/socketstream.hpp
	$(cc) $(ccflags) $(ldflags) $< -o $@



test_fiber: test_fiber.cpp ../include/fiber.hpp
	$(cc) $(ccflags) $(ldflags) $< -o $@

clean:
	rm test_client
	rm test_server
