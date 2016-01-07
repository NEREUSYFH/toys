
#ifndef FIBER_FUTURE_H
#define FIBER_FUTURE_H

#include "fiber.hpp"

namespace fiber {

enum class future_errc {
    future_already_retrieved = 1,
    promise_already_satisfied,
    no_state,
    broken_promise
};

enum class launch {
    async = 1,
    deferred = 2
};

enum class future_status {
    ready,
    timeout,
    deferred
};


class future_error : public logic_error {
    error_code          __code;

public:
    explicit future_error(error_code ec): logic_error("std::future_error"), __code(ec) {}

    virtual ~future_error() noexcept {};

    //virtual const char* what() const noexcept;

    const error_code& code() const noexcept { return __code; }
};

struct __future_base {

    struct _result_base {
    };

    struct _result: public _result_base {

    };

    template<typename _Res_ptr, typename _BoundFn>
    static _Task_setter<_Res_ptr>_S_task_setter(_Res_ptr& __ptr, _BoundFn&& __call) {
        return _Task_setter<_Res_ptr>{ __ptr, std::ref(__call) };
    }

    template<typename _Ptr_type, typename Res>
    struct _task_setter {
        _Ptr_type operator()() {
            try {
                _M_result->_M_set(_M_fn());
            } catch(...) {
                _M_result->_M_error = current_exception();
            }
            return std::move(_M_result);
        }
        _Ptr_type&                _M_result;
        std::function<_Res()>     _M_fn;
    };

    class _state_base {
        typedef _Ptr<_Result_base> _Ptr_type;

        _Ptr_type         __result;
        mutex                 _M_mutex;

        condition_variable    __cond;
        atomic_flag           __retrieved;

        std::once_flag         __once;

        virtual void _run_deferred() { }

        bool _ready() const noexcept { return static_cast<bool>(__result); }

    public:
        _state_base() noexcept : _M_result(), __retrieved(ATOMIC_FLAG_INIT) { }
        _state_base(const _state_base&) = delete;
        _state_base& operator=(const _state_base&) = delete;

        virtual ~_state_base() {}

        template<typename Tp>
        static void _check(const shared_ptr<Tp>& p) {
            if (!static_cast<bool>(p)) {
                throw future_error(int(future_errc::no_state));
            }
        }

        void _set_retrieved_flag() {
            if (__retrieved.test_and_set()) {
                throw future_error(std::make_error_code(future_errc::future_already_retrieved));
            }
        }

        _result_base& wait() {
            _run_deferred();
            unique_lock<mutex> lock(__mutex);
            _cond.wait(lock, [&] { return _ready(); });
            return *__result;
        }

        void __set_result(function<_Ptr_type()> res, bool ignore_failure = false) {
            bool set = __ignore_failure;
            // all calls to this function are serialized,
            // side-effects of invoking __res only happen once
            std::call_once(__once, &_State_base::_M_do_set, this, ref(res), ref(set));
            if (!set) {
                throw future_error(std::make_error_code(future_errc::promise_already_satisfied));
            }
        }

    };

    class _deferred_state: public _state_base {

    };

    class _async_state_common: public _state_base {
    protected:
        
        virtual void _run_deferred() { _join(); }

        void _join() { std::call_once(__once, &fiber::join, std::ref(__fiber)); }
            
        fiber __fiber; 
        std::once_flag __once;
    };

    template<typename BoundFn, typename Res>
    class _async_state_impl: public _async_state_common {
    public:
        explicit _async_state_impl(BoundFn&& fn): __fn(std::forward(fn)) {
            __fiber = fiber([this] { __set_result(_S_task_setter(__result, __fn)); } );
        }

    private:
        //__result;
        BoundFn __fn;
    };

    template<typename BoundFn>
    static std::shared_ptr<_state_base> _make_deferred_state(BoundFn&& fn) {
        typedef typename remove_reference<BoundFn>::type fn_type;
        typedef _deferred_state<fn_type> state_type;
        return std::make_shared<state_type>(std::move(fn));
    }

    template<typename BoundFn>
    static std::shared_ptr<_state_base> _make_async_state(BoundFn&& fn) {
        typedef typename remove_reference<BoundFn>::type fn_type;
        typedef _async_state_impl<fn_type> state_type;
        return std::make_shared<state_type>(std::forward(fn));
    }

};

template <class Res>
class __basic_future: public __futrue_base {

protected:
    typedef shared_ptr<_state_base> state_type;
    typedef __futrue_base::_result<Res> result_type;

private:
    state_type __state;

protected:
    typedef shared_ptr<_state_base> state_type;

    explicit __basic_future(const state_type& state) : __state(state) {
        _state_base::_check(__state);
        __state->_set_retrieved_flag();
    }

    void _swap(__basic_future& f) noexcept { __state.swap(f.__state); } 

    result_type __get_result() const {
        _state_base::_check(__state);
        _result_base& res = __state->wait();
        //if (!(res.__error == 0))
        //    rethrow_exception(__res._M_error);
        return static_cast<result_type>(res);
    }

    struct _reset {
        explicit _reset(__basic_future& f) noexcept : __fut(f) { }
        ~_reset() { __fut.__state.reset(); }
        __basic_future& __fut;
    };

};

template <class Res>
class future: public __basic_future<Res> {

    template<class Fn, class... Args>
    friend future<typename result_of<Fn(Args...)>::type> async(launch policy, Fn&& fn, Args&&... args);

    typedef Res result_type;
    typedef __basic_future<result_type> base_type;
    typedef typename base_type::state_type state_type;

    explicit future(const state_type& state) : base_type(state) {}

public:
    constexpr future() noexcept: base_type() {}

    future(future&& f) noexcept: base_type(std::move(f)) {}

    future(const future&) = delete;
    future& operator=(const future&) = delete;

    future& operator=(future&& f) noexcept {
        _swap(f);
        return *this;
    }

    result_type get() {
        typename base_type::_reset __reset(*this);
        return std::move(__get_result()._value());
    }

};

template<class Fn, class... Args>
future<typename result_of<Fn(Args...)>::type> async(launch policy, Fn&& fn, Args&&... args) {    
    typedef typename result_of<Fn(Args...)>::type result_type;
    std::shared_ptr<__future_base::_state_base> state;
    if (policy & launch::async) { 
        state = __future_base::_make_async_state( std::bind(std::forward<Fn>(fn), std::forward<Args>(args)...) );
    } else {    
        state = __future_base::_make_deferred_state( std::bind(std::forward<Fn>(fn), std::forward<Args>(args)...) );
    }    
    return future<result_type>(state);
}   

template<class Fn, class... Args>
inline future<class result_of<Fn(Args...)>::type> async(Fn&& fn, Args&&... args) {
    return async(launch::async|launch::deferred, std::forward<Fn>(fn), std::forward<Args>(args)...);
}


}

#endif //FIBER_FUTURE_H
