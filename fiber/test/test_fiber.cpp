
#include "fiber.hpp"
#include <iostream>
#include <string>

void test_fiber() {

    fiber::fiber([]() { 

            std::cout << "lamdba fiber 0" << std::endl; 

        });

    fiber::fiber([](std::string &&str) { 

            std::cout << str << std::endl; 

        }, "lamdba fiber 1");

    fiber::fiber f2([]() { 

            std::cout << "lamdba fiber 2, before yield" << std::endl; 

            fiber::this_fiber::yield(); 

            std::cout << "lamdba fiber 2, resume back" << std::endl; 

        });

    std::cout << "test fiber" << std::endl;

    f2.resume();
}


int main(int argc, char *argv[]) {
    test_fiber();

    return 0;
}
