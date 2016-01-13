
#ifndef FIBER_FIBER_H
#define FIBER_FIBER_H

#include <memory>
#include <functional>
#include <thread>
#include <iostream>
#include <ostream>
#include <string>
#include <stdexcept>

#include <cstdint>
#include <cassert>

#ifdef USE_BOOST_COROTUINE
#include <boost/coroutine/coroutine.hpp>
#else
#include <ucontext.h>
#endif

namespace fiber {

class fiber_error: public std::runtime_error {
public:
    explicit fiber_error(const std::string& what): std::runtime_error(what) { }
};


enum class fiber_status {
    running,
    suspended,
    normal,
    dead,
};

template<class CharT, class Traits>
std::basic_ostream<CharT, Traits>& operator<<(std::basic_ostream<CharT, Traits>& out, fiber_status status) {
    switch (status) {
    case fiber_status::running:
        return out << "running";
    case fiber_status::suspended:
        return out << "suspended";
    case fiber_status::normal:
        return out << "normal";
    case fiber_status::dead:
        return out << "dead";
    }
    return out;
}


class __fiber_base {
protected:
    class __impl_base;

    class __basic_impl;

    template<class Fn>
    class __fiber_impl;

public:
    class __this_fiber_helper;
};


class __fiber_base::__impl_base {
    friend class __fiber_base::__this_fiber_helper;

protected:
    __impl_base *__parent_impl;
    fiber_status __status;

    struct __unwind {
        __unwind(__impl_base& impl) noexcept: __impl(impl) { __set_thread_impl(&__impl); }
        ~__unwind() noexcept {
            __set_thread_impl(__impl.__parent_impl);
            __impl.__set_status(fiber_status::dead);
        }
        __impl_base& __impl;
    };

    __impl_base() noexcept: __parent_impl(__thread_impl()), __status(fiber_status::running) {
        if (__parent_impl) { __parent_impl->__set_status(fiber_status::normal); }
    }

    __impl_base(const __impl_base&) = delete;

    virtual ~__impl_base() {};

    __impl_base& operator=(const __impl_base&) = delete;

    virtual void __fiber_routine() = 0;

    inline fiber_status __get_status() const noexcept { return __status; }

    inline void __set_status(fiber_status status) noexcept { __status = status; }

    void __set_resume() { 
        assert(__status == fiber_status::suspended);
        __set_thread_impl(this); 
        __set_status(fiber_status::running); 
    }

    void __set_yield() { 
        assert(__thread_impl() == this && __status == fiber_status::running);
        __set_thread_impl(__parent_impl); 
        __set_status(fiber_status::suspended); 
    }

private:
    static inline void __set_thread_impl(__impl_base *impl) noexcept { __thread_impl() = impl; }

    static __impl_base*& __thread_impl() noexcept {
        static __thread __impl_base* _thread_impl;
        return _thread_impl;
    }
};


class __fiber_base::__basic_impl: public __fiber_base::__impl_base { 
    friend class fiber;
    friend class __fiber_base::__this_fiber_helper;

protected:
    typedef const void* __native_handle_type;

    inline __native_handle_type __native_handle() const noexcept { return __native_handle_type(this); }

#ifdef USE_BOOST_COROTUINE
private:
    typedef boost::coroutines::coroutine<void> coroutine_type;
    typedef coroutine_type::push_type yield_type;
    typedef coroutine_type::pull_type fiber_type;

    yield_type *__fiber_yield;
    fiber_type __fiber;

    void __routine(yield_type &yield) {
        __impl_base::__unwind _unwind(*this);

        __fiber_yield = &yield;

        //yield from here for complete construct of fiber
        __yield();

        __fiber_routine();
    }

protected:
    __basic_impl(): __impl_base(), __fiber_yield(nullptr), __fiber(std::bind(&__basic_impl::__routine, this, std::placeholders::_1)) { }

    void __resume() { __set_resume(); __fiber(); }

    void __yield() { assert(__fiber_yield); __set_yield(); (*__fiber_yield)(); }

#else
private:
    ucontext_t __context; 
    ucontext_t &__parent_context;
    char __stack[102400];

    static void __ucontext_entry(unsigned int hthis, unsigned int lthis) {
        __basic_impl *that = reinterpret_cast<__basic_impl *>((static_cast<unsigned long>(hthis) << 32) | lthis); 

        __impl_base::__unwind _unwind(*that);

        //yield from here for complete construct of fiber
        that->__yield();

        that->__fiber_routine();
    }

    void __routine() {
        if (getcontext(&__context) != 0) {
            throw fiber_error("getcontext error"); 
        }
        
        __context.uc_stack.ss_sp = __stack;
        __context.uc_stack.ss_size = sizeof(__stack);
        __context.uc_link = &__parent_context;

        unsigned int hthis = static_cast<unsigned int>(reinterpret_cast<unsigned long>(this) >> 32);
        unsigned int lthis = static_cast<unsigned int>(reinterpret_cast<unsigned long>(this) & 0xffffffff);
        makecontext(&__context, reinterpret_cast<void (*)()>(__ucontext_entry), 2, hthis, lthis);

        if (swapcontext(&__parent_context, &__context) != 0) {
            throw fiber_error("swapcontext error");
        }
    }

    ucontext_t &__get_parent_context() {
        static __thread ucontext_t _thread_context;
        return __parent_impl ? static_cast<__basic_impl *>(__parent_impl)->__context : _thread_context;
    }

protected:
    __basic_impl(): __impl_base(), __parent_context(__get_parent_context()) { __routine(); }

    void __yield() {
        __set_yield();
        if (swapcontext(&__context, &__parent_context) != 0) {
            throw fiber_error("swapcontext error");
        }
    }

    void __resume() {
        __set_resume();
        if (swapcontext(&__parent_context, &__context) != 0) {
            throw fiber_error("swapcontext error");
        }
    }

#endif
};


template <class Fn>
class __fiber_base::__fiber_impl: public __fiber_base::__basic_impl {
    Fn __fn;

public:
    __fiber_impl(Fn&& fn): __fiber_base::__basic_impl(), __fn(std::forward<Fn>(fn)) {
        //now, construct done and resume this fiber
        __resume();
    }

    void __fiber_routine() override { __fn(); }
};


class __fiber_base::__this_fiber_helper {
    static __fiber_base::__basic_impl* __this_fiber_impl() {
        return reinterpret_cast<__fiber_base::__basic_impl *>(__fiber_base::__impl_base::__thread_impl());
    }
public:
    static void __yield() { 
        __fiber_base::__basic_impl* impl = __this_fiber_impl();
        assert(impl);
        impl->__yield();
    };
    static __fiber_base::__basic_impl::__native_handle_type __native_handle() noexcept {
        __fiber_base::__basic_impl* impl = __this_fiber_impl(); 
        assert(impl);
        return impl->__native_handle();
    }
    static bool __is_fiber() noexcept {
        return static_cast<bool>(__fiber_base::__impl_base::__thread_impl());
    }
};


class fiber: public __fiber_base {
public:
    typedef typename __fiber_base::__basic_impl::__native_handle_type native_handle_type;

    class id {
        friend class fiber;

        native_handle_type __fiber;

    public:
        id() noexcept : __fiber() {}

        explicit id(native_handle_type f) noexcept : __fiber(f) {} 

        native_handle_type native_handle() const noexcept { return __fiber; }

        template<class CharT, class Traits>
        friend std::basic_ostream<CharT, Traits>& operator<<(std::basic_ostream<CharT, Traits>& out, fiber::id id);
    };

public:
    fiber() noexcept = default;

    fiber(const fiber&) = delete;

    fiber(fiber&& f) noexcept { swap(f); }

    template <class Fn, class... Args>
    explicit fiber(Fn&& fn, Args&&... args): 
        __impl(__make_shared_impl(std::bind(std::forward<Fn>(fn), std::forward<Args>(args)...))) { }

    ~fiber() noexcept { } 

    fiber& operator=(const fiber&) = delete;

    fiber& operator=(fiber&& f) noexcept { swap(f); return *this; }

    void swap(fiber& f) noexcept { __impl.swap(f.__impl); } 

    void resume() { __impl->__resume(); }

    fiber_status get_status() const noexcept { return __impl->__get_status(); }

    id get_id() const noexcept { return id(__impl->__native_handle()); }

    static unsigned int hardware_concurrency() noexcept {
        return std::thread::hardware_concurrency();
    }

private:
    std::shared_ptr<__fiber_base::__basic_impl> __impl;

    template <class Fn>
    static std::shared_ptr<__fiber_base::__fiber_impl<Fn>> __make_shared_impl(Fn&& f) {
        return std::make_shared<__fiber_base::__fiber_impl<Fn>>(std::forward<Fn>(f));
    }
};


template<class CharT, class Traits>
std::basic_ostream<CharT, Traits>& operator<<(std::basic_ostream<CharT, Traits>& out, fiber::id id) {
    return out << id.native_handle();
}


namespace this_fiber {

inline void yield() {
    __fiber_base::__this_fiber_helper::__yield();
}

inline fiber::id get_id() noexcept {
    return fiber::id(__fiber_base::__this_fiber_helper::__native_handle());
}

inline bool is_fiber() noexcept {
    return __fiber_base::__this_fiber_helper::__is_fiber();
}

} //this_fiber
 
} //fiber 

#endif //FIBER_FIBER_H


