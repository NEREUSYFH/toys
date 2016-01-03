

#ifndef FIBER_SOCKET_HPP
#define FIBER_SOCKET_HPP

#include <string>
//#include <iostream>
#include <ios>
#include <cstddef>

#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>

namespace fiber {

class socketaddr_base {
    static constexpr const char* __inaddr_any_table[] = { "0.0.0.0", "::" };
    static constexpr const int __native_family_table[] = { AF_INET, AF_INET6 };

public:
    enum class family {
        ipv4 = 0,
        ipv6 = 1,
    };
    typedef unsigned short port_type;
    typedef sockaddr native_sockaddr_type;
    typedef socklen_t native_socklen_type; 

protected:
    static constexpr int __native_family(family f) {
        return __native_family_table[static_cast<int>(f)];
    }
    static constexpr const char* __inaddr_any(family f) {
        return __inaddr_any_table[static_cast<int>(f)];
    }
};

template<socketaddr_base::family Family>
class basic_socketaddr: public socketaddr_base {
    static constexpr const native_socklen_type __native_socklen_table[] = { sizeof(sockaddr_in), sizeof(sockaddr_in6) }; 
    static constexpr native_socklen_type __native_socklen(family f) {
        return __native_socklen_table[static_cast<int>(f)]; 
    }

public:
    static constexpr const int native_family = __native_family(Family);
    static constexpr const char* inaddr_any = __inaddr_any(Family);
    static constexpr native_socklen_type native_socklen = __native_socklen(Family); 

private:
    union {
        native_sockaddr_type s;
        sockaddr_in in;
        sockaddr_in6 in6; 
    } __sockaddr;

    void __build_ipv4(const char *ip, port_type port) {
        __sockaddr.in.sin_family = AF_INET;
        __sockaddr.in.sin_port = htons(port);
        if (::inet_pton(AF_INET, ip, &__sockaddr.in.sin_addr) != 1) {
            std::cout << "ip: " << ip << std::endl;
            //throw
            assert(false);
        }
    }

    void __build_ipv6(const char *ip, port_type port) {
        __sockaddr.in6.sin6_family = AF_INET6;
        __sockaddr.in6.sin6_port = htons(port);
        if (::inet_pton(AF_INET6, ip, &__sockaddr.in6.sin6_addr) != 1) {
            //throw
            assert(false);
        }
    }

public:
    basic_socketaddr() = default;

    //from ip port family
    explicit basic_socketaddr(const std::string &ip, port_type p): basic_socketaddr(ip.c_str(), p) { }

    //from port family
    explicit basic_socketaddr(port_type p): basic_socketaddr(inaddr_any, p) { }

    //from ip port
    explicit basic_socketaddr(const char *ip, port_type p): __sockaddr() {
        switch (Family) {
        case family::ipv4:
            __build_ipv4(ip, p);
            break;
        case family::ipv6:
            __build_ipv6(ip, p);
            break;
        }
    }

    //copy
    basic_socketaddr(const basic_socketaddr& addr): __sockaddr(addr.__sockaddr) {}

    ~basic_socketaddr() = default;

    basic_socketaddr& operator=(const basic_socketaddr& addr) {
        __sockaddr = addr.__sockaddr;
        return *this;
    }

    inline native_sockaddr_type* native_sockaddr() { return &__sockaddr.s; }

    inline const native_sockaddr_type* native_sockaddr() const { return &__sockaddr.s; }

    port_type port() const { 
        switch (Family) {
        case family::ipv4:
            return ntohs(__sockaddr.in.sin_port);
        case family::ipv6:
            return ntohs(__sockaddr.in6.sin6_port);
        }
        return 0;
    }

public:
    inline static family get_family() { return Family; }
};


class socket_base {
    static constexpr const int __native_type_table[] = { SOCK_STREAM, SOCK_DGRAM };

public:
    typedef socketaddr_base::family family;
    typedef int native_handle_type;
    enum class type {
        tcp = 0,
        udp = 1,
    };

protected:
    static constexpr int __native_type(type t) {
        return __native_type_table[static_cast<int>(t)];
    }

protected:
    native_handle_type __socket;

    socket_base() noexcept: __socket(-1) { }

    socket_base(native_handle_type s) noexcept: __socket(s) { }

    virtual ~socket_base() noexcept { if (this->is_open()) { this->close(); } } 

public:
    bool close() noexcept {
        bool ret = ::close(__socket) == 0;
        __socket = -1;
        return ret;
    }

    inline bool is_open() const noexcept { return __socket >= 0; }

    native_handle_type native_handle() const noexcept { return __socket; }

    void swap(socket_base& s) noexcept { std::swap(s.__socket, __socket); }

    bool non_blocking(bool nb = true) noexcept {
        int flags = fcntl(__socket, F_GETFL, 0); 
        if (flags == -1) {
            return false;
        }
        return fcntl(__socket, F_SETFL, nb ? (flags | O_NONBLOCK) : (flags & ~O_NONBLOCK)) != -1;
    }

};


template<socket_base::type Type, socket_base::family Family>
class basic_socket: public socket_base {
    static const int __default_backlog = 512;

public:
    typedef basic_socketaddr<Family> socketaddr_type;
    static constexpr const int native_family = socketaddr_type::native_family;
    static constexpr int native_type = __native_type(Type);

public:
    //default
    basic_socket() noexcept: socket_base(-1) { }

    //from native handle
    basic_socket(native_handle_type s) noexcept: socket_base(s) { }

    //move
    basic_socket(basic_socket&& s) { this->swap(s); }

    //copy = delete
    basic_socket(const basic_socket&) = delete;

    virtual ~basic_socket() = default;

    basic_socket& operator=(const basic_socket&) = delete;

    inline type get_type() const noexcept { return Type; }

    inline family get_family() const noexcept { return Family; }

    bool open() noexcept {
        __socket = ::socket(native_family, native_type, 0); 
        return is_open();
    }

    bool bind(const socketaddr_type& addr) noexcept {
        return ::bind(__socket, addr.native_sockaddr(), addr.native_socklen) == 0;
    }

    template<class... Args>
    bool bind(Args&&... args) noexcept {
        return this->bind(socketaddr_type(std::forward<Args>(args)...));
    }

    bool connect(const socketaddr_type& addr) noexcept { 
        return ::connect(this->__socket, addr.native_sockaddr(), addr.native_socklen) == 0;
    }

    template<class... Args>
    bool connect(Args&&... args) noexcept {
        return this->connect(socketaddr_type(std::forward<Args>(args)...));
    }

    template<class buf_type>
    ssize_t send(buf_type* buf, size_t len) noexcept {
        return ::send(this->__socket, buf, len, 0);
    }

    template<class buf_type>
    ssize_t recv(buf_type* buf, size_t len) noexcept {
        return ::recv(this->__socket, buf, len, 0);
    }


    /** udp **/

    template<class buf_type>
    ssize_t sendto(buf_type *buf, size_t len, const socketaddr_type& addr) noexcept {
        return ::sendto(buf, len, 0, addr.native_sockaddr(), addr.native_socklen()); 
    }

    template<class buf_type, class... Args>
    ssize_t sendto(buf_type *buf, size_t len, Args&&... args) noexcept {
        return this->sendto(buf, len, socketaddr_type(std::forward<Args>(args)...));
    }

    template<class buf_type>
    ssize_t recvfrom(buf_type *buf, size_t len, const socketaddr_type& addr) noexcept {
        return ::recvfrom(buf, len, 0, addr.native_sockaddr(), addr.native_socklen());
    }

    template<class buf_type, class... Args>
    ssize_t recvfrom(buf_type *buf, size_t len, Args&&... args) noexcept {
        return this->recvfrom(buf, len, socketaddr_type(std::forward<Args>(args)...));
    }


    /** tcp **/

    bool listen(int backlog = __default_backlog) noexcept {
        return ::listen(this->__socket, backlog) == 0;
    }

    bool bind_and_listen(const socketaddr_type& addr, int backlog = __default_backlog) noexcept {
        return this->bind(addr) && this->listen(backlog);
    }

    template<class... Args>
    bool bind_and_listen(Args&&... args, int backlog = __default_backlog) noexcept {
        return this->bind_and_listen(socketaddr_type(std::forward<Args>(args)...), backlog); 
    }

    basic_socket accept() noexcept {
        socketaddr_type addr;
        typename socketaddr_type::native_socklen_type len = addr.native_socklen;
        native_handle_type s = ::accept(this->__socket, addr.native_sockaddr(), &len);
        return basic_socket(s);
    }

};


//template<class SocketT, class CharT, class Traits> 
//class basic_socketstream;

/*
template<class SocketT, class CharT, class Traits = std::char_traits<CharT>>
class basic_tcpacceptor {
    //typedef basic_socketstream<SocketT, CharT, Traits> basic_socketstream_type;

public:
    typedef SocketT socket_type;
    typedef CharT char_type;
    typedef Traits traits_type;

private:
    socket_type __socket;

public:
    //default
    basic_tcpacceptor() noexcept: __socket() { }

    //from socketaddr
    explicit basic_tcpacceptor(const socketaddr& addr): __socket() {
        this->bind_and_listen(addr);
    }

    //copy delete
    basic_tcpacceptor(const basic_tcpacceptor&) = delete;

    //move
    basic_tcpacceptor(basic_tcpacceptor&& sa) { this->swap(sa); }

    virtual ~basic_tcpacceptor() = default;

    basic_tcpacceptor& operator=(const basic_tcpacceptor&) = delete;

    void swap(basic_tcpacceptor& sa) { __socket.swap(sa.__socket); }

    socket_type* socket() { return &__socket; }

    const socket_type* socket() const { return &__socket; }

    void bind_and_listen(const socketaddr& addr) {
        if (!__socket.bind_and_listen(addr)) {
            //throw
            assert(false);
        }
    }

    basic_socketstream_type accept() {
        socket_type socket = __socket.accept();
        if (!socket.is_open()) {
            //throw
            assert(false);
        }
        return basic_socketstream_type(std::move(socket));
    }

};*/

typedef basic_socketaddr<socketaddr_base::family::ipv4> socketaddr;
typedef basic_socketaddr<socketaddr_base::family::ipv6> socketaddr6;

typedef basic_socket<socket_base::type::tcp, socket_base::family::ipv4> tcp_socket; 
typedef basic_socket<socket_base::type::tcp, socket_base::family::ipv6> tcp6_socket; 

typedef basic_socket<socket_base::type::udp, socket_base::family::ipv4> udp_socket; 
typedef basic_socket<socket_base::type::udp, socket_base::family::ipv6> udp6_socket; 

//typedef basic_tcpacceptor<tcp_socket, char> tcpacceptor; 
//typedef basic_tcpacceptor<tcp6_socket, char> tcp6acceptor; 



}


#endif //FIBER_SOCKET_HPP

