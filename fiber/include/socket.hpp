

#ifndef FIBER_SOCKET_HPP
#define FIBER_SOCKET_HPP

#include <string>
//#include <iostream>
#include <sstream>
#include <ios>
#include <cstddef>

#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>

namespace fiber {

template<class T, class U> struct is_same_decay {
    static constexpr bool value = std::is_same<typename std::decay<T>::type, U>::value;
};  


class socketaddr_base {
public:
    enum class family {
        ipv4 = 0,
        ipv6 = 1,
    };
    typedef unsigned short port_type;
    typedef sockaddr native_sockaddr_type;
    typedef socklen_t native_socklen_type; 

private:
    static constexpr const char* __inaddr_any_table[] = { "0.0.0.0", "::" };
    static constexpr const int __native_family_table[] = { AF_INET, AF_INET6 };
    static constexpr const native_socklen_type __native_socklen_table[] = { sizeof(sockaddr_in), sizeof(sockaddr_in6) }; 
    static constexpr const size_t __port_offset_table[] = { offsetof(sockaddr_in, sin_port), offsetof(sockaddr_in6, sin6_port) }; 
    static constexpr const size_t __addr_offset_table[] = { offsetof(sockaddr_in, sin_addr), offsetof(sockaddr_in6, sin6_addr) };

protected:
    static constexpr int __native_family(family f) {
        return __native_family_table[static_cast<int>(f)];
    }
    static constexpr const char* __inaddr_any(family f) {
        return __inaddr_any_table[static_cast<int>(f)];
    }
    static constexpr native_socklen_type __native_socklen(family f) {
        return __native_socklen_table[static_cast<int>(f)]; 
    }
    static constexpr size_t __port_offset(family f) {
        return __port_offset_table[static_cast<int>(f)];
    }
    static constexpr size_t __addr_offset(family f) {
        return __addr_offset_table[static_cast<int>(f)];
    }
};

template<socketaddr_base::family Family>
class basic_socketaddr: public socketaddr_base {
    union {
        native_sockaddr_type s;
        sockaddr_in in;
        sockaddr_in6 in6; 
    } __sockaddr;

public:
    static constexpr const int native_family = __native_family(Family);
    static constexpr const char* inaddr_any = __inaddr_any(Family);
    static constexpr native_socklen_type native_socklen = __native_socklen(Family); 
    static constexpr native_socklen_type native_max_socklen = sizeof(__sockaddr);

private:
    static constexpr size_t port_offset = __port_offset(Family); 
    static constexpr size_t addr_offset = __addr_offset(Family); 

    port_type* __port_ptr() const {
        return const_cast<port_type *>(reinterpret_cast<const port_type *>(reinterpret_cast<const char *>(&__sockaddr) + port_offset)); 
    }
    void *__addr_ptr() const {
        return const_cast<char *>(reinterpret_cast<const char *>(&__sockaddr) + addr_offset);
    }

    void __init(const char *addr, port_type port) {
         __sockaddr.s.sa_family = native_family;
        if (::inet_pton(native_family, addr, __addr_ptr()) != 1) {
            //throw
            assert(false);
        }
        *__port_ptr() = htons(port);
    }

public:
    //default
    basic_socketaddr() { };

    //from port
    basic_socketaddr(port_type port): basic_socketaddr(inaddr_any, port) { }

    //from addr port
    explicit basic_socketaddr(const std::string &addr, port_type port): basic_socketaddr(addr.c_str(), port) { }

    //from addr port
    explicit basic_socketaddr(const char *addr, port_type port): __sockaddr() {
        __init(addr, port);
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

    port_type port() const { return htons(*__port_ptr()); } 

    std::string dotted_addr_only() const {
        char _addr_str[128];
        if (::inet_ntop(native_family, __addr_ptr(), _addr_str, sizeof(_addr_str))) {
            return std::string(_addr_str);
        }
        return std::string("invalid addr");
    }

    std::string dotted_addr() const {
        std::ostringstream oss;
        oss << dotted_addr_only() << ':' << port();
        return oss.str(); 
    }

public:
    inline static family get_family() { return Family; }
};

template<class CharT, class Traits, socketaddr_base::family Family>
std::basic_ostream<CharT, Traits>& operator<<(std::basic_ostream<CharT, Traits>& out, basic_socketaddr<Family> addr) {
    out << addr.dotted_addr();
    return out;
}

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

    inline operator bool() const noexcept { return is_open(); }

    native_handle_type native_handle() const noexcept { return __socket; }

    void swap(socket_base& s) noexcept { std::swap(s.__socket, __socket); }

    bool nonblocking(bool nonblock = true) noexcept {
        int flags = fcntl(__socket, F_GETFL, 0); 
        return flags != -1 && fcntl(__socket, F_SETFL, nonblock ? (flags | O_NONBLOCK) : (flags & ~O_NONBLOCK)) != -1;
    }

};


template<socket_base::type Type, socket_base::family Family>
class basic_socket: public socket_base {
public:
    static const int default_backlog = SOMAXCONN;

    typedef basic_socketaddr<Family> socketaddr_type;

    static constexpr const int native_family = socketaddr_type::native_family;

    static constexpr int native_type = __native_type(Type);

    static const int default_protocal = 0;

public:
    //default
    basic_socket() = default;

    //from native handle
    basic_socket(native_handle_type s) noexcept: socket_base(s) { }

    //move
    basic_socket(basic_socket&& s) { this->swap(s); }

    //copy = delete
    basic_socket(const basic_socket&) = delete;

    ~basic_socket() = default;

    basic_socket& operator=(const basic_socket&) = delete;

    inline type get_type() const noexcept { return Type; }

    inline family get_family() const noexcept { return Family; }

    bool open(bool nonblock = false, int protocal = default_protocal) noexcept {
        __socket = ::socket(native_family, native_type, protocal); 
        if (!is_open()) {
            return false;
        }
        return nonblock ? nonblocking() : true;
    }

    template<class Addr, class = typename std::enable_if<is_same_decay<Addr, socketaddr_type>::value>::type>
    bool bind(Addr&& addr) noexcept {
        return ::bind(__socket, addr.native_sockaddr(), addr.native_socklen) == 0;
    }

    template<class... Args, class = typename std::enable_if<sizeof...(Args)>::type>
    bool bind(Args&&... args) noexcept {
        return this->bind(socketaddr_type(std::forward<Args>(args)...));
    }

    template<class Addr, class = typename std::enable_if<is_same_decay<Addr, socketaddr_type>::value>::type>
    bool connect(Addr&& addr) noexcept { 
        return ::connect(this->__socket, addr.native_sockaddr(), addr.native_socklen) == 0;
    }

    template<class... Args, class = typename std::enable_if<sizeof...(Args)>::type>
    bool connect(Args&&... args) noexcept {
        return this->connect(socketaddr_type(std::forward<Args>(args)...));
    }

    template<class Buf>
    ssize_t send(Buf buf, size_t len) noexcept {
        return ::send(this->__socket, buf, len, 0);
    }

    template<class Buf>
    ssize_t recv(Buf buf, size_t len) noexcept {
        return ::recv(this->__socket, buf, len, 0);
    }

    template<class Buf, class Addr, class = typename std::enable_if<is_same_decay<Addr, socketaddr_type>::value>::type>
    ssize_t send(Buf buf, size_t len, Addr&& addr) noexcept {
        return ::sendto(buf, len, 0, addr.native_sockaddr(), addr.native_socklen); 
    }

    template<class Buf, class... Args, class = typename std::enable_if<sizeof...(Args)>::type>
    ssize_t send(Buf buf, size_t len, Args&&... args) noexcept {
        return this->send(buf, len, socketaddr_type(std::forward<Args>(args)...));
    }

    template<class Buf, class Addr, class = typename std::enable_if<is_same_decay<Addr, socketaddr_type>::value>::type>
    ssize_t recv(Buf buf, size_t len, Addr&& addr) noexcept {
        return ::recv(buf, len, 0, addr.native_sockaddr(), addr.native_socklen);
    }

    template<class Buf, class... Args, class = typename std::enable_if<sizeof...(Args)>::type>
    ssize_t recv(Buf buf, size_t len, Args&&... args) noexcept {
        return this->recv(buf, len, socketaddr_type(std::forward<Args>(args)...));
    }

    // tcp //

    bool listen(int backlog = default_backlog) noexcept {
        return ::listen(this->__socket, backlog) == 0;
    }

    basic_socket accept() noexcept {
        native_handle_type s = ::accept(this->__socket, nullptr, 0);
        return basic_socket(s);
    }

    basic_socket accept(socketaddr_type& addr) noexcept {
        typename socketaddr_type::native_socklen_type len = addr.native_max_socklen;
        native_handle_type s = ::accept(this->__socket, addr.native_sockaddr(), &len);
        return basic_socket(s);
    }

};

typedef basic_socketaddr<socketaddr_base::family::ipv4> socketaddr;
typedef basic_socketaddr<socketaddr_base::family::ipv6> socketaddr6;

typedef basic_socket<socket_base::type::tcp, socket_base::family::ipv4> tcpsocket; 
typedef basic_socket<socket_base::type::tcp, socket_base::family::ipv6> tcp6socket; 

typedef basic_socket<socket_base::type::udp, socket_base::family::ipv4> udpsocket; 
typedef basic_socket<socket_base::type::udp, socket_base::family::ipv6> udp6socket; 


}


#endif //FIBER_SOCKET_HPP

