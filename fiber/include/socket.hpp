

#ifndef FIBER_SOCKET_HPP
#define FIBER_SOCKET_HPP

#include <string>
#include <iostream>

#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>

namespace fiber {

class socketaddr {
public:
    enum class family {
        ipv4 = 0,
        ipv6 = 1,
    };
    typedef unsigned short port_type;
    typedef sockaddr native_sockaddr_type;
    typedef socklen_t native_socklen_type; 

private:
    family __family;

    union {
        sockaddr __s;
        sockaddr_in __sin;
        sockaddr_in6 __sin6; 
    };

    void __build_ipv4(const char *ip, port_type port) {
        __sin.sin_family = AF_INET;
        __sin.sin_port = htons(port);
        if (::inet_aton(ip, &__sin.sin_addr) != 0) {
            //throw
        }
    }

    void __build_ipv6(const char *ip, port_type port) {
        __sin6.sin6_family = AF_INET6;
        __sin6.sin6_port = htons(port);
        if (::inet_pton(AF_INET6, ip, &__sin6) != 0) {
            //throw
        }
    }

public:
    native_sockaddr_type *native_sockaddr() const {
        return const_cast<sockaddr *>(reinterpret_cast<const sockaddr*>(&__s)); 
    }

    native_socklen_type native_socklen() const {
        switch (__family) {
        case family::ipv4:
            return sizeof(__sin);
        case family::ipv6:
            return sizeof(__sin6);
        }
        return 0;
    }

public:
    explicit socketaddr(const std::string &ip, port_type p, family f = family::ipv4): socketaddr(ip.c_str(), p, f) { }

    explicit socketaddr(port_type p, family f = family::ipv4): socketaddr("0.0.0.0", p, f) { }

    explicit socketaddr(family f = family::ipv4): __family(f), __s() { }

    explicit socketaddr(const char *ip, port_type p, family f = family::ipv4): __family(f), __s() {
        switch (__family) {
        case family::ipv4:
            __build_ipv4(ip, p);
            break;
        case family::ipv6:
            __build_ipv6(ip, p);
            break;
        }
    }

    port_type port() const { 
        switch (__family) {
        case family::ipv4:
            return ntohs(__sin.sin_port);
        case family::ipv6:
            return ntohs(__sin6.sin6_port);
        }
        return 0;
    }

    family get_family() const { return __family; }

};


class socket_base {
public:
    typedef socketaddr::family family;
    typedef int native_handle_type;
    enum class type {
        tcp = 0,
        udp = 1,
    };

protected:
    static int __native_family(family f) {
        static const int native_family_table[] = { AF_INET, AF_INET6 };
        return native_family_table[static_cast<int>(f)];
    }

    static int __native_type(type t) {
        static const int native_type_table[] = { SOCK_STREAM, SOCK_DGRAM };
        return native_type_table[static_cast<int>(t)];
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

    native_handle_type native_handle() const noexcept {
        return __socket;
    }

    void swap(socket_base& s) noexcept {
        std::swap(s.__socket, __socket);
    }

    bool non_blocking(bool nb = true) const noexcept {
        int flags = fcntl(__socket, F_GETFL, 0); 
        if (flags == -1) {
            return false;
        }
        if (nb) {
            flags = fcntl(__socket, F_SETFL, flags | O_NONBLOCK);
        } else {
            flags = fcntl(__socket, F_SETFL, flags & ~O_NONBLOCK);
        }
        return flags != -1;
    }

};


template<socket_base::type Type, socket_base::family Family, class CharT, class Traits = std::char_traits<CharT>>
class basic_socket: public socket_base {
public:
    typedef CharT char_type;
    typedef Traits traits_type;

public:
    basic_socket() noexcept: socket_base(-1) { }

    basic_socket(native_handle_type s) noexcept: socket_base(s) { }

    type get_type() const noexcept { return Type; }

    family get_family() const noexcept { return Family; }

    bool open() noexcept {
        __socket = ::socket(__native_family(Family), __native_type(Type), 0); 
        return is_open();
    }

    bool bind(const socketaddr& addr) noexcept {
        return this->__open() && this->__bind();
    }

protected:
    inline bool __open() noexcept {
        return !is_open() && open();
    }

    inline bool __bind(const socketaddr& addr) noexcept {
        if (::bind(__socket, addr.native_sockaddr(), addr.native_socklen()) != 0) {
            return false;
        }
        return true;
    }
};

template<socket_base::family Family, class CharT, class Traits = std::char_traits<CharT>>
class basic_tcp_socket: public basic_socket<socket_base::type::tcp, Family, CharT, Traits>  { 
    typedef basic_socket<socket_base::type::tcp, Family, CharT, Traits> basic_socket_type;
    typedef typename socket_base::native_handle_type native_handle_type;
    
    static const int __default_backlog = 512;

public:
    //default
    basic_tcp_socket() noexcept: basic_socket_type() { }

    //from native handle
    basic_tcp_socket(native_handle_type s) noexcept: basic_socket_type(s) { }

    //copy = delete 
    basic_tcp_socket(const basic_tcp_socket&) = delete;

    //move
    basic_tcp_socket(basic_tcp_socket&& ts) noexcept { this->swap(ts); }
    
    ~basic_tcp_socket() = default;

    basic_tcp_socket& operator=(const basic_tcp_socket&) = delete;

    bool listen(int backlog = __default_backlog) noexcept {
        return this->__open() && this->__listen();
    }

    bool bind_and_listen(const socketaddr& addr, int backlog = __default_backlog) noexcept {
        return this->__open() && this->__bind(addr) && this->__listen(backlog);
    }

    bool connect(const socketaddr& addr) noexcept { 
        return this->__open() && this->__connect(addr);
    }

    basic_tcp_socket accept() noexcept {
        return this->__accept();
    }

protected:
    inline bool __listen(int backlog = 5) noexcept { 
        return ::listen(this->__socket, backlog) == 0;
    }

    inline bool __connect(const socketaddr& addr) noexcept {
        return ::connect(this->__socket, addr.native_sockaddr(), addr.native_socklen()) == 0;
    }

    inline basic_tcp_socket __accept() noexcept {
        socketaddr addr(Family);
        socketaddr::native_socklen_type len = addr.native_socklen();
        return basic_tcp_socket(::accept(this->__socket, addr.native_sockaddr(), &len));
    }
};


template<class SocketT, class CharT, class Traits> 
class basic_socketstream;

template<class SocketT, class CharT, class Traits = std::char_traits<CharT>>
class basic_tcpacceptor {
    typedef basic_socketstream<SocketT, CharT, Traits> basic_socketstream_type;

public:
    typedef SocketT socket_type;

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

    socket_type& socket() const {
        return const_cast<socket_type&>(__socket); 
    }

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

};


typedef basic_tcp_socket<socket_base::family::ipv4, char> tcp_socket; 
typedef basic_tcp_socket<socket_base::family::ipv6, char> tcp6_socket;

/*
typedef basic_socket<socket_base::family::ipv4, socket_base::type::udp, char> udp_socket;
typedef basic_socket<socket_base::family::ipv6, socket_base::type::udp, char> udp6_socket;
*/

typedef basic_tcpacceptor<tcp_socket, char> tcpacceptor; 



}


#endif //FIBER_SOCKET_HPP

