

#ifndef FIBER_EPOLL_HPP
#define FIBER_EPOLL_HPP

#include <string>
#include <iostream>
#include <type_traits>

#include <sys/epoll.h>

namespace fiber {

class __reactor_base {

};

class epoll_base: public __reactor_base {
public:
    typedef int native_handle_type;
    typedef epoll_event native_event_type; 
    typedef int native_events_type; 

    static const int max_size = 1000;
    
protected:
    native_handle_type __epoll;

protected:
    basic_epoll() noexcept: __epoll(-1) {}

    basic_epoll(native_handle_type ep) noexcept: __epoll(ep) { }

    ~basic_epoll() noexcept { if (is_open()) { close(); } }

    bool is_open() const noexcept { return __epoll >= 0; }

    bool close() noexcept {
        bool ret = ::close(__epoll) != -1; 
        __epoll = -1;
        return ret;
    }

    bool open() noexcept {
        __epoll = ::epoll_create(max_size); 
        return is_open();
    }

    native_handle_type native_handle() const noexcept { return __epoll; }

};

template<EventT>
class basic_epoll: public epoll_base {

public:
    typedef EventT event_type;
    typedef std::queue<event_type*> event_queue_type;

    static constexpr const int max_wait_event = 256;

    static int default_wait_timeout = 0; 

private:
    native_event_type __events[max_wait_event];

public:
    event_queue_type wait(int timeout = default_wait_timeout) noexcept {
        event_queue_type event_queue;
        int event_cnt = ::epoll_wait(this->__epoll, __events, max_wait_event, timeout);
        if (event_cnt == -1) {
            //error
            return event_queue;
        }
        for (int i = 0; i < event_cnt; ++i) {
            event_type* event = reinterpret_cast<event_type*>(__events[i].data.ptr);
            event_queue.push(event);
        }
        return event_queue;
    }

    bool push(event_type *event) {
        native_event_type ev = { 0, { 0 } };
        ev.events = EPOLLIN | EPOLLERR | EPOLLET;
        ev.data.ptr = event;
        return ::epoll_ctl(__epoll, EPOLL_CTL_ADD, event->socket()->native_handle(), &ev) != -1;
    }

    bool remove(event_type *event) {
        native_event_type ev; //linux 2.6.9
        return ::epoll_ctl(__epoll, EPOLL_CTL_DEL, event->socket()->native_handle(), &ev) != -1;
    }
};


class __event_base {
    typedef const void* native_handle_type;

protected:
    class __impl_base; 

    template<class Fn>
    class __basic_impl;
};

class __event_base::__impl_base {
    virtual void __complete() = 0;

    native_handle_type __native_handle() { return native_handle_type(this); }
};

template<class Fn>
class __event_base::__basic_impl {
    socket_base __socket;
    Fn __fn;

public:
    void __complete() override {
        __fn();
    }
};

class event: public __event_base {
    std::shared_ptr<__impl_base> __impl;

public:
    class id {
        native_handle_type __event;

    public:
        id(): __event(0) { }

        explicit id(native_handle_type e): __event(e) { }

        native_handle_type native_handle() const { return __event; }
    };

public:
    template<class Fn, class... Args>
    event(Fn&& fn, Args&&... args): 
        __impl(__make_shared_impl(std::bind(std::forward<Fn>(fn), std::forward<Args>(args)...))) {}

    void complete() { __impl->complete(); }

    id get_id() const { return id(__impl->__native_handle()); }

    template<class Fn>
    st::shared_ptr<basic_impl<Fn>> __make_shared_impl(Fn&& fn) {
        return std::make_shared(std::forward<Fn>(fn));
    }

};


typedef basic_epoll<event_base> epoll;


}


#endif //FIBER_EPOLL_HPP

