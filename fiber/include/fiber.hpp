
#ifndef FIBER_FIBER_H
#define FIBER_FIBER_H

#include <memory>
#include <functional>
#include <thread>
#include <iostream>

#include <cstdint>
#include <cassert>

#include <boost/coroutine/coroutine.hpp>

namespace fiber {

enum class fiber_status {
    running,
    suspended,
    normal,
    dead,
};

std::ostream& operator<<(std::ostream &o, fiber_status status) {
    switch (status) {
    case fiber_status::running:
        o << "running";
        break;
    case fiber_status::suspended:
        o << "suspended";
        break;
    case fiber_status::normal:
        o << "normal";
        break;
    case fiber_status::dead:
        o << "dead";
        break;
    }
    return o;
}

class __fiber_block_base;

namespace this_fiber { 

__fiber_block_base* __this_fiber_block() noexcept;

void yield();

}

class __fiber_block_base {
    friend class fiber;
    friend __fiber_block_base* this_fiber::__this_fiber_block() noexcept;
    friend void this_fiber::yield();

    typedef boost::coroutines::coroutine<void> coroutine_type;
    typedef coroutine_type::push_type yield_type;
    typedef coroutine_type::pull_type fiber_type;
    typedef const void *native_handle_type;

    __fiber_block_base *__parent_block;
    fiber_status __status;
    yield_type *__fiber_yield;
    fiber_type __fiber;

    struct __unwind {
        __unwind(__fiber_block_base& block) noexcept: __block(block) {
            __set_thread_block(&__block);
        }
        ~__unwind() noexcept {
            __block.__on_unwind();
        }
        __fiber_block_base& __block;
    };

    static inline void __set_thread_block(__fiber_block_base *block) noexcept {
        __thread_block() = block;
    }

    static inline __fiber_block_base*& __thread_block() noexcept {
        static __thread __fiber_block_base* _thread_block;
        return _thread_block;
    }

    void __set_status(fiber_status status) noexcept { __status = status; }

    virtual void _fiber_block_entry() = 0;

    __fiber_block_base():
        __parent_block(__thread_block()),
        __status(fiber_status::running),
        __fiber_yield(nullptr),
        __fiber(std::bind(&__fiber_block_base::__routine, this, std::placeholders::_1)) {}

    __fiber_block_base(const __fiber_block_base&) = delete;

    __fiber_block_base& operator=(const __fiber_block_base&) = delete;

    void __routine(yield_type &yield) {
        __fiber_yield = &yield;
        __unwind _unwind(*this);

        //yield from here for complete construct of fiber
        __yield();

        _fiber_block_entry();
    }

    void __on_unwind() noexcept {
        __set_thread_block(__parent_block);
        __set_status(fiber_status::dead);
    }

    void __resume() {
        assert(__status == fiber_status::suspended);
        __set_thread_block(this);
        __set_status(fiber_status::running);
        __fiber();
    }

    void __yield() {
        assert(__fiber_yield && __thread_block() == this && __status == fiber_status::running);
        __set_thread_block(__parent_block);
        __set_status(fiber_status::suspended);
        (*__fiber_yield)();
    }

    inline fiber_status __get_status() const noexcept { return __status; }

public:
    virtual ~__fiber_block_base() {};

    inline native_handle_type native_handle() const noexcept { return this; }
};

class fiber {
    //typedef std::allocator<char> allocator_type;
public:
    typedef __fiber_block_base::native_handle_type id;

private:
    template <class Fn>
    class __fiber_block: public __fiber_block_base {
        Fn __fn;
    public:
        __fiber_block(Fn&& fn): __fiber_block_base(), __fn(std::forward<Fn>(fn)) {}

        void _fiber_block_entry() override {
            //try {
            __fn();
        }
    };

public:
    fiber() noexcept = default;

    fiber(const fiber&) = delete;

    fiber(fiber&& f) noexcept { swap(f); }

    template <class Fn, class... Args>
    explicit fiber(Fn&& fn, Args&&... args): __block(__make_shared_block( std::bind(std::forward<Fn>(fn), std::forward<Args>(args)...) )) { 
        //now, construct done and resume this fiber
        resume();
    }

    ~fiber() noexcept { } 

    fiber& operator=(const fiber&) = delete;

    fiber& operator=(fiber&& f) noexcept { swap(f); return *this; }

    void swap(fiber &f) noexcept { __block.swap(f.__block); } 

    void resume() { __block->__resume(); }

    void suspend() { }

    fiber_status get_status() const noexcept { return __block->__get_status(); }

    id get_id() const noexcept { return id(__block->native_handle()); }

    static unsigned int hardware_concurrency() noexcept {
        return std::thread::hardware_concurrency();
    }

private:
    std::shared_ptr<__fiber_block_base> __block;

    template <class Fn>
    std::shared_ptr<__fiber_block<Fn>> __make_shared_block(Fn&& f) {
        return std::make_shared<__fiber_block<Fn>>(std::forward<Fn>(f));
    }

};


namespace this_fiber {

inline __fiber_block_base* __this_fiber_block() noexcept {
    return __fiber_block_base::__thread_block();
}

void yield() {
    __fiber_block_base* block = __this_fiber_block();
    assert(block);
    block->__yield();
}

fiber::id get_id() noexcept {
    __fiber_block_base* block = __this_fiber_block();
    assert(block);
    return fiber::id(block->native_handle());
}

bool is_fiber() noexcept {
    return static_cast<bool>(__this_fiber_block());
}

} //this_fiber
 
} //fiber 

#endif //FIBER_FIBER_H


