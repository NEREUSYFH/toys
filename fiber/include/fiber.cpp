

#include <stdexcept>
#include "fiber.hpp"

namespace fiber {

}

#include <iostream>
#include <string>
int main(int argc, char *argv[]) {
    std::string hello = "hello";
    fiber::fiber f([&](std::string &&world) { 
        std::cout << hello << " " << world << std::endl;
        fiber::this_fiber::yield();
        std::cout << "back" << std::endl;
    }, "world"); 
    f.resume();
    f.resume();

    return 0;
}

