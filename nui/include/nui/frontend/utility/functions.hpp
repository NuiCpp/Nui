// Copyright (c) 2017 Damien Buhl alias daminetreg (damien.buhl@lecbna.org)
// https://github.com/daminetreg/js-bind

#ifndef JS_BIND_HPP
#define JS_BIND_HPP

#include <utility>
#include <functional>

#include <emscripten/bind.h>
#include <emscripten/val.h>

#include <traits/functions.hpp>

#include <boost/preprocessor/repetition/repeat.hpp>
#include <boost/preprocessor/punctuation/comma_if.hpp>
#include <boost/preprocessor/stringize.hpp>

#ifndef JS_BIND_MAX_ARITY
/**
 * \brief JS_BIND_MAX_ARITY Defines how many arguments the javascript
 *        exported C++  std::function can have.
 *        It defaults to 16, simply override it if you need more.
 */
#    define JS_BIND_MAX_ARITY 16
#endif

#define JS_BIND_DETAIL_ARGS_EACH(z, n, unused) BOOST_PP_COMMA_IF(n) emscripten::val

#define JS_BIND_DETAIL_ARGS(COUNT) BOOST_PP_REPEAT(COUNT, JS_BIND_DETAIL_ARGS_EACH, unused)

#define JS_BIND_DETAIL_EACH(z, n, return_type) \
    emscripten::class_<std::function<return_type(JS_BIND_DETAIL_ARGS(n))>>( \
        BOOST_PP_STRINGIZE(return_type) BOOST_PP_STRINGIZE(n) "ArgsFunctor") \
            .constructor<>() \
            .function("opcall", &std::function<return_type(JS_BIND_DETAIL_ARGS(n))>::operator());

#define JS_BIND_DETAIL_GENERATE_BINDINGS() \
    BOOST_PP_REPEAT(JS_BIND_MAX_ARITY, JS_BIND_DETAIL_EACH, void) \
    BOOST_PP_REPEAT(JS_BIND_MAX_ARITY, JS_BIND_DETAIL_EACH, emscripten::val)

#define JS_BIND_FUNCTION_TYPES_EACH(z, n, return_type) \
    template <> \
    struct functor_t<std::is_same<return_type, void>::value, n> \
    { \
        typedef ::std::function<return_type(JS_BIND_DETAIL_ARGS(n))> type; \
    };

#define JS_BIND_FUNCTION_TYPES() \
    BOOST_PP_REPEAT(JS_BIND_MAX_ARITY, JS_BIND_FUNCTION_TYPES_EACH, void) \
    BOOST_PP_REPEAT(JS_BIND_MAX_ARITY, JS_BIND_FUNCTION_TYPES_EACH, emscripten::val)

#define JS_BIND_DETAIL_FWD(...) ::std::forward<decltype(__VA_ARGS__)>(__VA_ARGS__)

namespace Nui
{

    /**
     * \brief Equivalent of [std::bind](http://en.cppreference.com/w/cpp/utility/functional/bind) returning a javascript
     * functor. \param f The lambda, function, member function to bind \param args \return An emscripten::val
     * representing a javascript functor with it's this-scope bound correctly. It can be called from js or c++ with the
     * call operator ( *i.e.* `operator()` ).
     *
     */
    template <class F, class... Args>
    emscripten::val bind(F&& f, Args&&... args);

    namespace detail
    {
        template <typename... Args>
        struct placeholders_count;

        template <>
        struct placeholders_count<>
        {
            static constexpr size_t value = 0;
        };

        template <class T>
        struct one_if_placeholder
        {
            static constexpr size_t value = (std::is_placeholder<typename std::decay<T>::type>::value) ? 1 : 0;
        };

        template <typename Arg, typename... Args>
        struct placeholders_count<Arg, Args...>
        {
            static constexpr size_t value = one_if_placeholder<Arg>::value + placeholders_count<Args...>::value;
        };
    }

    template <const bool returns_void, const size_t arity>
    struct functor_t;

    JS_BIND_FUNCTION_TYPES();

    template <class F, class... Args>
    emscripten::val bind(F&& f, Args&&... args)
    {
        using emscripten::val;

        using result_type = typename Traits::FunctionTraits<std::decay_t<F>>::ReturnType;
        auto bind_result = std::bind(std::forward<decltype(f)>(f), JS_BIND_DETAIL_FWD(args)...);

        using callback_t =
            typename functor_t<std::is_same<result_type, void>::value, detail::placeholders_count<Args...>::value>::
                type;

        callback_t functor = bind_result;

        // We ensure the correct object will always be bound to the this of the function
        auto functor_adapter =
            emscripten::val(functor)["opcall"].call<emscripten::val>("bind", emscripten::val(functor));
        return functor_adapter;
    }

}

#endif // JS_BIND_HPP
