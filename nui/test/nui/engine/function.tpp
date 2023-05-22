namespace Nui::Tests::Engine
{
    template <typename T>
    requires Callable<T>
    Function::Function(T&& function)
        : callable_{Detail::FunctionSignature_t<FunctionArgumentTypes_t<T>>{
              [function = std::forward<T>(function)](auto&&... args) mutable {
                  if constexpr (std::is_same_v<FunctionReturnType_t<T>, void>)
                      return function(std::forward<decltype(args)>(args)...), emscripten::val{};
                  else
                      return emscripten::val{function(std::forward<decltype(args)>(args)...)};
              }}}
        , signature_{[]() -> std::string {
            using argumentTuple = FunctionArgumentTypes_t<T>;
            std::string signature = "emscripten::val(";
            if constexpr (std::tuple_size_v<argumentTuple> > 0)
            {
                signature += Detail::TupleTypePrint<argumentTuple>::toString();
            }
            signature += ")";
            return signature;
        }()}
    {}

    template <typename... Args>
    Function::Function(std::function<emscripten::val(Args const&...)> handler)
        : callable_{handler}
    {}

    template <typename... Args>
    emscripten::val Function::operator()(Args&&... args) const
    {
        return std::any_cast<std::function<emscripten::val(std::decay_t<Args>...)>>(callable_)(
            std::forward<Args>(args)...);
    }
}