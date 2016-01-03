

#ifndef FIBER_SOCKETSTREAM_HPP
#define FIBER_SOCKETSTREAM_HPP

#include <string>
#include <iostream>

#include "socket.hpp"

namespace fiber {

using std::streamsize;
using std::ios_base;


class sios_base {
public:
    enum openmode {
        connect = 1,
        bind = 2,
        listen = 3,
    };
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
    char __buf[1];

public:
    //default
    basic_socketbuf(): streambuf_type(), __socket() { }

    //open
    explicit basic_socketbuf(openmode mode, const socketaddr_type& addr): streambuf_type(), __socket() { 
        this->setbuf(__buf, sizeof(__buf));
        this->open(mode, addr);
    }

    //open
    template<class... Args>
    explicit basic_socketbuf(openmode mode, Args&&... args): basic_socketbuf(mode, static_cast<const socketaddr_type&>(socketaddr_type(std::forward<Args>(args)...))) { } 

    //from socket
    explicit basic_socketbuf(socket_type&& s): streambuf_type(), __socket(std::forward<socket_type>(s)) {
        this->setbuf(__buf, sizeof(__buf));
    }

    //copy = delete
    basic_socketbuf(const basic_socketbuf&) = delete;

    //move
    basic_socketbuf(basic_socketbuf&& sb) noexcept { this->swap(sb); } 

    virtual ~basic_socketbuf() noexcept { close(); }

    basic_socketbuf& operator=(const basic_socketbuf&) = delete;

    void swap(basic_socketbuf& sb) { 
        //streambuf_type::swap(sb);
        __socket.swap(sb.__socket); 
    }

    bool close() noexcept { 
        if (__socket.is_open()) {
            this->sync();
            return __socket.close();
        }
        return true;
    }

    bool open(openmode mode, const socketaddr_type& addr) noexcept {
        if (__socket.is_open() || !__socket.open()) {
            return false;
        }
        switch(mode) {
        case openmode::connect:
            return __socket.connect(addr) || (__socket.close(), false);
        case openmode::bind:
            return __socket.bind(addr) || (__socket.close(), false);
        case openmode::listen:
            return __socket.bind_and_listen(addr) || (__socket.close(), false);
        }
        __socket.close();
        return false;
    }

    template<class... Args>
    bool open(openmode mode, Args&&... args) { 
        return this->open(mode, static_cast<const socketaddr_type&>(socketaddr_type(std::forward<Args>(args)...))); 
    } 

    bool is_open() const noexcept { return __socket.is_open(); }

    socket_type accept() { return __socket.accept(); }

    socket_type* socket() const noexcept { return const_cast<socket_type *>(&__socket); }

    basic_socketbuf& flush() { std::cout << "basic_socketbuf.flush" << std::endl; return *this; }

    virtual basic_socketbuf* setbuf(char_type* s, streamsize n) {
        this->setg(s, s, s + n);
        this->setp(0, 0);
        return this;
    }

    virtual streamsize showmanyc() override {
        std::cout << "basic_socketbuf.showmanyc" << std::endl;
        return 0;
    }

    virtual streamsize xsgetn(char_type* s, streamsize n) override {
        std::cout << "basic_socketbuf.xsgetn, n:" << n << std::endl;
        return ::read(__socket.native_handle(), s, n);
    }

    virtual int_type pbackfail(int_type c) override {
        std::cout << "basic_socketbuf.pbackfail, c:" << c << std::endl;
        return 0;
    }
    virtual int_type uflow() override {
        std::cout << "basic_socketbuf.uflow" << std::endl;
        return 0;
    }

    virtual int_type underflow() override {
        std::cout << "basic_socketbuf.underflow" << std::endl;
        streamsize slen = this->xsgetn(this->eback(), sizeof(__buf));
        if (slen == 0) {
            return traits_type::eof();
        }
        this->setg(__buf, __buf, __buf + slen);
        return traits_type::to_int_type(*this->gptr());
    }

    virtual int sync() override {
        std::cout << "basic_socketbuf.sync" << std::endl;
        return 0;
    }

    virtual streamsize xsputn(const char_type* s, streamsize n) override {
        std::cout << "basic_socketbuf.xsputn(" << s << ", " << n << ")" << std::endl;
        return write(__socket.native_handle(), s, n);
    }

    virtual int_type overflow(int_type c) override {
        std::cout << "basic_socketbuf.overflow(" << c << ", " << c << ")" << std::endl;
        return traits_type::eof();
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
    basic_socketstream(openmode mode, const socketaddr_type& addr): iostream_type(), __socketbuf() { 
        this->init(&__socketbuf); 
        this->open(mode, addr);
    }

    //open
    template<class... Args>
    explicit basic_socketstream(openmode mode, Args&&... args): basic_socketstream(mode, static_cast<const socketaddr_type&>(socketaddr_type(std::forward<Args>(args)...))) { }

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
    basic_socketstream(basic_socketstream&& ss) { this->swap(ss); }

    virtual ~basic_socketstream() = default;

    basic_socketstream& operator= (const basic_socketstream&) = delete;

    void swap(basic_socketstream& ss) {
        //iostream_type::swap(ss);
        __socketbuf.swap(ss.__socketbuf);
    }

    socketbuf_type* rdbuf() const { return const_cast<socketbuf_type *>(&__socketbuf); }

    void close() { 
        if (!__socketbuf.close()) { 
            this->setstate(ios_base::failbit); 
        } 
    }

    void open(openmode mode, const socketaddr_type& addr) {
        if (!__socketbuf.open(mode, addr)) {
            this->setstate(ios_base::failbit);
        }
    }

    template<class... Args>
    bool open(openmode mode, Args&&... args) { return this->open(mode, static_cast<const socketaddr_type&>(socketaddr_type(std::forward<Args>(args)...))); } 

    bool is_open() const { return __socketbuf.is_open(); }

    basic_socketstream accept() { return basic_socketstream(__socketbuf.accept()); }

    socket_type* socket() const noexcept { return __socketbuf.socket(); }
    
};


//tcp
typedef basic_socketbuf<tcp_socket, char> tcpbuf;
typedef basic_socketstream<tcp_socket, char> tcpstream;

//tcp ipv6
typedef basic_socketbuf<tcp6_socket, char> tcp6buf;
typedef basic_socketstream<tcp6_socket, char> tcp6stream;

//udp
typedef basic_socketbuf<udp_socket, char> udpbuf;
typedef basic_socketstream<udp_socket, char> udpstream;

//udp ipv6
typedef basic_socketbuf<udp6_socket, char> udp6buf;
typedef basic_socketstream<udp6_socket, char> udp6stream;


}


#endif //FIBER_SOCKETSTREAM_HPP

