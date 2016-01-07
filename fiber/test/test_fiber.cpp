
#include "fiber.hpp"
#include <iostream>
#include <string>

class A {
public:
    A() {
        std::cout << "A()" << std::endl;
    }
    A(const A&) = delete;
    A(A&&) {
        std::cout << "A(A&&)" << std::endl;
    }
    ~A() {
        std::cout << "~A()" << std::endl;
    }
};


void fun(A& a) {
    std::cout << "fun" << std::endl;
};

void test_fiber() {

    /*
    A a = A();
    fiber::fiber f([](A& a) { 
            std::cout << "in fiber" << std::endl;
            fiber::this_fiber::yield();
            std::cout << "back fiber" << std::endl;
        }, std::move(A())); 

    std::cout << "out fiber" << std::endl;
    f.resume();
    */
    

    A a;
    auto fn = std::bind(fun, A()); 

    fn();



    /*
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

    */


}


int main(int argc, char *argv[]) {
    test_fiber();

    return 0;
}
