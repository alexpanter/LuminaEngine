#pragma once
#include "Traits.h"
#include "Stack.h"

namespace Lumina::Lua
{
    template<auto TFunc>
    auto Invoker(lua_State* L)
    {
        using TraitsT = TFunctionTraits<decltype(TFunc)>;
        using ReturnT = TraitsT::ReturnType;
        using ArgsT   = TraitsT::ArgsTuple;
    
        auto Dispatch = [&]<size_t... Is>(eastl::index_sequence<Is...>)
        {
            if constexpr (eastl::is_member_function_pointer_v<decltype(TFunc)>)
            {
                using ClassT = TraitsT::ClassType;
                ClassT* Self = static_cast<ClassT*>(lua_touserdata(L, 1));
            
                if constexpr (eastl::is_void_v<ReturnT>)
                {
                    (Self->*TFunc)(TStack<eastl::tuple_element_t<Is, ArgsT>>::Get(L, Is + 2)...);
                    return 0;
                }
                else
                {
                    auto Ret = (Self->*TFunc)(TStack<eastl::tuple_element_t<Is, ArgsT>>::Get(L, Is + 2)...);
                    TStack<decltype(Ret)>::Push(L, Ret);
                    return 1;
                }
            }
            else
            {
                if constexpr (eastl::is_void_v<ReturnT>)
                {
                    TFunc(TStack<eastl::tuple_element_t<Is, ArgsT>>::Get(L, Is + 1)...);
                    return 0;
                }
                else
                {
                    auto Ret = TFunc(TStack<eastl::tuple_element_t<Is, ArgsT>>::Get(L, Is + 1)...);
                    TStack<decltype(Ret)>::Push(L, Ret);
                    return 1;
                }
            }
        };
    
        return Dispatch(eastl::make_index_sequence<TraitsT::ArgCount>{});
    }
}
