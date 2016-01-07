

#ifndef FIBER_SOCKETSTREAM_HPP
#define FIBER_SOCKETSTREAM_HPP

#include <string>
#include <iostream>
#include <type_traits>
#include <thread>

#include "fiber.hpp"
#include "socket.hpp"

namespace fiber {

using std::streamsize;
using std::ios_base;

enum __sios_openmode { 
    __sios_block = 1 << 0,
    __sios_conn  = 1 << 1,
    __sios_bind  = 1 << 2,
}; 

inline constexpr __sios_openmode operator&(__sios_openmode a, __sios_openmode b) { 
    return __sios_openmode(static_cast<int>(a) & static_cast<int>(b)); 
}
inline constexpr __sios_openmode operator|(__sios_openmode a, __sios_openmode b) { 
    return __sios_openmode(static_cast<int>(a) | static_cast<int>(b)); 
}
inline constexpr __sios_openmode operator^(__sios_openmode a, __sios_openmode b) { 
    return __sios_openmode(static_cast<int>(a) ^ static_cast<int>(b)); 
}
inline constexpr __sios_openmode operator~(__sios_openmode a) { 
    return __sios_openmode(~static_cast<int>(a)); 
}
inline const __sios_openmode& operator|=(__sios_openmode& a, __sios_openmode b) { 
    return a = a | b; 
}
inline const __sios_openmode& operator&=(__sios_openmode& a, __sios_openmode b) { 
    return a = a & b; 
}
inline const __sios_openmode& operator^=(__sios_openmode& a, __sios_openmode b) { 
    return a = a ^ b; 
}

class sios_base {
public:
    typedef __sios_openmode openmode;

    static const openmode block = __sios_block;
    static const openmode conn = __sios_conn;
    static const openmode bind = __sios_bind;
};

template<class SocketT, class CharT, class Traits = std::char_traits<CharT>> 
class basic_socketbuf: public std::basic_streambuf<CharT, Traits> {
    typedef std::basic_streambuf<CharT, Traits> streambuf_type;

public:
    typedef SocketT socket_type; 
    typedef typename socket_type::socketaddr_type socketaddr_type;

    typedef typename streambuf_type::char_type char_type;
    typedef typename streambuf_type::int_type int_type;
    typedef typename streambuf_type::traits_type traits_type;

    typedef typename sios_base::openmode openmode;

private:
    socket_type __socket;
    char __buf[1024];

public:
    //default
    basic_socketbuf(): streambuf_type(), __socket() { 
        this->setbuf(__buf, 0);
    }

    //open 
    template<class... Args>
    explicit basic_socketbuf(openmode mode, Args&&... args): streambuf_type(), __socket() { 
        this->setbuf(__buf, 0);
        this->open(mode, std::forward<Args>(args)...);
    } 

    //from socket
    explicit basic_socketbuf(socket_type&& s): streambuf_type(), __socket(std::forward<socket_type>(s)) {
        this->setbuf(__buf, 0);
    }

    //copy = delete
    basic_socketbuf(const basic_socketbuf&) = delete;

    //move
    //basic_socketbuf(basic_socketbuf&& sb) noexcept { 
    //    //this->swap(sb); 
    //    this->pubimbue(sb.getloc());
    //    this->setbuf(__buf, sizeof(__buf));
    //    __socket.swap(sb.__socket); 
    //} 
    basic_socketbuf(basic_socketbuf&&) = delete;

    virtual ~basic_socketbuf() noexcept { close(); }

    basic_socketbuf& operator=(const basic_socketbuf&) = delete;

    //void swap(basic_socketbuf& sb) { 
    //    //streambuf_type::swap(sb); //gcc4.8 not implement
    //    __socket.swap(sb.__socket); 
    //}

    bool close() noexcept { 
        if (__socket.is_open()) {
            this->sync();
            return __socket.close();
        }
        return true;
    }

    template<class... Args>
    bool open(openmode mode, Args&&... args) noexcept { 
        if (!mode || __socket.is_open() || !__socket.open(!(mode & sios_base::block))) {
            return false;
        }
        if (mode & sios_base::conn) {
            return __socket.connect(std::forward<Args>(args)...) || (__socket.close(), false);
        }
        if (mode & sios_base::bind) {
            return __socket.bind(std::forward<Args>(args)...) || (__socket.close(), false);
        }
        __socket.close();
        return false;
    }

    bool is_open() const noexcept { return __socket.is_open(); }

    bool nonblocking(bool nonblock = true) noexcept { return __socket.nonblocking(nonblock); }

    const socket_type* socket() const noexcept { return &__socket; }

    virtual basic_socketbuf* setbuf(char_type* s, streamsize n) noexcept {
        std::cout << "---> basic_socketbuf.setbuf" << std::endl;
        this->setg(s, s, s + n);
        this->setp(0, 0);
        return this;
    }

    virtual streamsize showmanyc() override {
        std::cout << "---> basic_socketbuf.showmanyc" << std::endl;
        return 0;
    }

    virtual streamsize xsgetn(char_type* s, streamsize n) override {
        std::cout << "---> basic_socketbuf.xsgetn, n:" << n << std::endl;
        return __socket.recv(s, n);
    }

    virtual int_type pbackfail(int_type c) override {
        std::cout << "---> basic_socketbuf.pbackfail, c:" << c << std::endl;
        return 0;
    }
    virtual int_type uflow() override {
        std::cout << "---> basic_socketbuf.uflow" << std::endl;
        return 0;
    }

    virtual int_type underflow() override {
        std::cout << "---> basic_socketbuf.underflow" << std::endl;
        streamsize slen = this->xsgetn(this->eback(), sizeof(__buf));
        if (slen == 0) {
            return traits_type::eof();
        }
        this->setg(__buf, __buf, __buf + slen);
        return traits_type::to_int_type(*this->gptr());
    }

    virtual int sync() override {
        std::cout << "---> basic_socketbuf.sync" << std::endl;
        return 0;
    }

    virtual streamsize xsputn(const char_type* s, streamsize n) override {
        std::cout << "---> basic_socketbuf.xsputn, n:" << n << std::endl;
        streamsize slen = __socket.send(s, n);
        std::cout << "xsputn slen:" << slen << std::endl;
        return slen;
    }

    virtual int_type overflow(int_type c) override {
        std::cout << "---> basic_socketbuf.overflow, c:" << c << std::endl;
        const char_type s = traits_type::to_char_type(c);
        if (__socket.send(&s, 1) != 1) {
            return traits_type::eof();
        }
        return c;
    }
};


/*
template<class CharT, class Traits = std::char_traits<CharT>> 
class basic_isocketstream: public std::basic_istream<CharT, Traits> {
};
template<class CharT, class Traits = std::char_traits<CharT>> 
class basic_osocketstream: public std::basic_ostream<CharT, Traits> {
};*/


template<class SocketT, class CharT, class Traits = std::char_traits<CharT>> 
class basic_socketstream: public sios_base, public std::basic_iostream<CharT, Traits> {
    typedef std::basic_iostream<CharT, Traits> iostream_type;
    typedef std::basic_istream<CharT, Traits> istream_type;

public:
    typedef basic_socketbuf<SocketT, CharT, Traits> socketbuf_type; 
    typedef SocketT socket_type;
    typedef typename socket_type::socketaddr_type socketaddr_type;

private:
    socketbuf_type __socketbuf;

public:
    //default
    basic_socketstream(): iostream_type(), __socketbuf() { }
    
    //open
    template<class... Args>
    explicit basic_socketstream(openmode mode, Args&&... args): iostream_type(), __socketbuf() { 
        this->init(&__socketbuf); 
        this->open(mode, std::forward<Args>(args)...);
    }

    //from socket
    explicit basic_socketstream(socket_type&& s): iostream_type(), __socketbuf(std::forward<socket_type>(s)) { 
        this->init(&__socketbuf); 
        if (!__socketbuf.is_open()) {
            this->setstate(ios_base::failbit);
        }
    }

    //copy = delete
    basic_socketstream(const basic_socketstream&) = delete;

    //move
    //basic_socketstream(basic_socketstream&& ss): __socketbuf(std::move(ss.__socketbuf)) { 
    //    //this->swap(ss); 
    //    this->_M_gcount = ss.gcount();
    //}
    basic_socketstream(basic_socketstream&&) = delete;

    virtual ~basic_socketstream() = default;

    basic_socketstream& operator= (const basic_socketstream&) = delete;

    //void swap(basic_socketstream& ss) {
    //    //iostream_type::swap(ss); //gcc4.8 not implement
    //    __socketbuf.swap(ss.__socketbuf);
    //}

    socketbuf_type* rdbuf() const noexcept { return const_cast<socketbuf_type *>(&__socketbuf); }

    void close() noexcept { 
        if (!__socketbuf.close()) { 
            this->setstate(ios_base::failbit); 
        } 
    }

    template<class... Args>
    void open(openmode mode, Args&&... args) noexcept { 
        if (!__socketbuf.open(mode, std::forward<Args>(args)...)) {
            this->setstate(ios_base::failbit);
        }
    } 

    bool is_open() const noexcept { return __socketbuf.is_open(); }

    void nonblocking(bool nonblock = true) noexcept { 
        if (!__socketbuf.nonblocking(nonblock)) {
            this->setstate(ios_base::failbit);
        }
    }

    const socket_type* socket() const noexcept { return __socketbuf.socket(); }

};


template<class SocketStreamT>
class basic_tcpacceptor {
public:
    typedef SocketStreamT socketstream_type;
    typedef typename socketstream_type::socket_type socket_type;
    typedef typename socket_type::socketaddr_type socketaddr_type;

    static const int default_backlog = socket_type::default_backlog;

private:
    socket_type __socket;


public:
    //default
    basic_tcpacceptor() = default;

    //open
    template<class... Args>
    explicit basic_tcpacceptor(Args&&... args): __socket() {
        this->open(std::forward<Args>(args)...);
    }

    //copy = delete
    basic_tcpacceptor(const basic_tcpacceptor&) = delete;

    //move
    basic_tcpacceptor(basic_tcpacceptor&& sa) { this->swap(sa); }

    virtual ~basic_tcpacceptor() = default;

    basic_tcpacceptor& operator=(const basic_tcpacceptor&) = delete;

    void swap(basic_tcpacceptor& sa) noexcept { __socket.swap(sa.__socket); }

    template<class... Args>
    void open(Args&&... args) { 
        if (!__socket.open() || !__socket.bind(std::forward<Args>(args)...) || !__socket.listen(default_backlog)) {
            //throw
            assert(false);
        }
    } 

    bool is_open() const { return __socket.is_open(); }

    template<class... Args>
    socket_type accepts(Args&&... args) {
        return __socket.accept(std::forward<Args>(args)...);
    }

    template<class... Args>
    socketstream_type accept(Args&&... args) {
        return socketstream_type(__socket.accept(std::forward<Args>(args)...));
    }

    const socket_type* socket() const noexcept { return &__socket; }

    template<class Fn>
    void operator()(Fn&& fn) {
        while (this->is_open()) {
            socketaddr_type addr;
            socket_type s = this->accepts(addr);
            std::thread(__operator<Fn&>, std::move(s), addr, std::ref(fn)).detach();
            //fiber(__operator<Fn&>, std::move(s), addr, std::ref(fn));
        }
    }

    template<class Fn>
    static void __operator(socket_type&& s, socketaddr_type addr, Fn&& fn) { 
        socketstream_type ss(std::move(s)); 
        fn(ss, addr); 
    }

};


//tcp
typedef basic_socketbuf<tcpsocket, char> tcpbuf;
typedef basic_socketstream<tcpsocket, char> tcpstream;

//tcp ipv6
typedef basic_socketbuf<tcp6socket, char> tcp6buf;
typedef basic_socketstream<tcp6socket, char> tcp6stream;

//udp
typedef basic_socketbuf<udpsocket, char> udpbuf;
typedef basic_socketstream<udpsocket, char> udpstream;

//udp ipv6
typedef basic_socketbuf<udp6socket, char> udp6buf;
typedef basic_socketstream<udp6socket, char> udp6stream;

//tcp acceptor
typedef basic_tcpacceptor<tcpstream> tcpacceptor; 
typedef basic_tcpacceptor<tcp6stream> tcp6acceptor; 


}


#endif //FIBER_SOCKETSTREAM_HPP

