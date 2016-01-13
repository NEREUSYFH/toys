
#include <stdexcept>

#include "fiber.hpp"

#include <iostream>
#include <string>

void test_fiber() {
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
}

#include <socketstream.hpp>

#include <sys/socket.h>
#include <netinet/in.h>

int tcp_listen() {
    struct sockaddr_in hostaddr; 

    hostaddr.sin_family = AF_INET;  
    hostaddr.sin_port = htons(8888); 
    hostaddr.sin_addr.s_addr = htonl(INADDR_ANY);  

    int fd = socket(AF_INET, SOCK_STREAM, 0);  

    int ret = bind(fd, (struct sockaddr *)&hostaddr, sizeof(hostaddr));
    assert(ret == 0);

    ret = listen(fd, 10);
    assert(ret == 0);

    return fd;
}

void test_socketstream() {
    int fd = tcp_listen();
    std::cout << "listen on: " << fd << std::endl;

    struct sockaddr_in clientaddr;
    socklen_t socklen = sizeof(clientaddr); 

    while (true) {
        int cfd = accept(fd, (struct sockaddr *)&clientaddr, &socklen); 
        assert(cfd >= 0);

        fiber::socketstream ss(cfd);
        while (ss.good()) {
            ss.write("hello\n", 6);
            //ss << "hello\n" << std::flush;
            //std::string msg;
            //ss >> msg;
            char msg[1024];
            ss.read(msg, sizeof(msg));
            std::cout << "msg: " << msg << std::endl;
            ss.clear();
        }
    }


}

int main(int /*argc*/, char* /*argv*/[]) {
    test_socketstream();

    return 0;
}
