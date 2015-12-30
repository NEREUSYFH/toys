
#include <stdexcept>

#include "fiber.hpp"

#include <iostream>
#include <string>

int main(int argc, char *argv[]) {

    std::string hello = "hello";

    fiber::fiber f(
        [&](std::string &&world) { 

            std::cout << hello << " " << world << " id:" << fiber::this_fiber::get_id() << std::endl;

            fiber::this_fiber::yield();

            //test
            std::cout << "back from outside, id:" << fiber::this_fiber::get_id() << std::endl;

            fiber::fiber f(
                [&]() {
                    
                    std::cout << "inner fiber, id:" << fiber::this_fiber::get_id() << std::endl;
                    
                    fiber::this_fiber::yield();

                    std::cout << "inner fiber back" << std::endl;
                });

            //std::cout << "back from inner fiber" << std::endl;

            std::cout << "id:" << fiber::this_fiber::get_id() << " fid:" << f.get_id() << " " << f.get_status() << std::endl;
            f.resume();
            std::cout << "id:" << fiber::this_fiber::get_id() << " fid:" << f.get_id() << " " << f.get_status() << std::endl;

        }, "world"); 

    std::cout << "fid:" << f.get_id() << " " << f.get_status() << std::endl;
    f.resume();
    std::cout << "fid:" << f.get_id() << " " << f.get_status() << std::endl;

    return 0;
}
