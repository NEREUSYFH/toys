

#include "fiber.hpp"
#include "kernel.hpp"
#include "socketstream.hpp"

void http_server_handle(fiber::tcpstream& stream, fiber::socketaddr& addr) {
    std::string line;
    std::cout << addr << "(begin)" << std::endl;
    while (std::getline(stream, line)) {
        line.pop_back();
        std::cout << addr << "> " << line << '(' << line.length() << ')' << std::endl;
        stream << line << std::endl; 
        //stream << line << std::endl; 
    }
    std::cout << addr << "(end)" << std::endl;
}

void http_server_main(int port) {
    fiber::tcpacceptor acceptor(port);
    std::cout << "tcpacceptor listen on: " << port << std::endl;

    acceptor(echo_server_handle);
}

int main(int argc, char* argv[]) {

    if (argc < 2) {
        std::cout << "usage: " << argv[0] << " <port>" << std::endl;
        return 1; 
    }

    fiber::fiber(http_server_main, std::atoi(argv[1]));

    fiber::kernel::run();

    std::cout << "server shutdown" << std::endl;

    return 0;
}
