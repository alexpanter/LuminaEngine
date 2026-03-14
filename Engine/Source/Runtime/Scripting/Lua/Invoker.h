#pragma once

#include "lualib.h"
#include "Stack.h"
#include "Traits.h"

namespace Lumina::Lua
{
    template<typename TParam>
    static decltype(auto) GetArg(lua_State* L, int Index)
    {
        using BaseT = eastl::remove_cvref_t<TParam>;
    
        if constexpr (TLuaNativeType<BaseT>::value)
        {
            return TStack<BaseT>::Get(L, Index);
        }
        else if constexpr (eastl::is_pointer_v<TParam>)
        {
            return TStack<TParam>::Get(L, Index);
        }
        else if constexpr (eastl::is_enum_v<BaseT>)
        {
            return TStack<BaseT>::Get(L, Index);
        }
        else
        {
            return TStack<BaseT&>::Get(L, Index);
        }
    }
    
    template<typename TParam>
    static bool CheckArg(lua_State* L, int Index)
    {
        using BaseT = eastl::remove_cvref_t<TParam>;
    
        if constexpr (TLuaNativeType<BaseT>::value)
        {
            return TStack<BaseT>::Check(L, Index);
        }
        else if constexpr (eastl::is_pointer_v<TParam>)
        {
            return TStack<TParam>::Check(L, Index);
        }
        else if constexpr (eastl::is_enum_v<BaseT>)
        {
            return TStack<BaseT>::Check(L, Index);
        }
        else
        {
            return TStack<BaseT&>::Check(L, Index);
        }
    }
    
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
                using ClassT  = TraitsT::ClassType;
                
                ClassT* Self = nullptr;
                if (lua_islightuserdata(L, 1))
                {
                    Self = static_cast<ClassT*>(lua_tolightuserdata(L, 1));
                }
                else
                {
                    Self = TStack<ClassT*>::Get(L, 1);
                }
                
                if (Self == nullptr)
                {
                    luaL_errorL(L, "[%s]", "Cannot invoke lua function with incorrect usertype.");
                }
                
                if constexpr (eastl::is_void_v<ReturnT>)
                {
                    (Self->*TFunc)(GetArg<eastl::tuple_element_t<Is, ArgsT>>(L, Is + 2)...);
                    return 0;
                }
                else
                {
                    auto Ret = (Self->*TFunc)(GetArg<eastl::tuple_element_t<Is, ArgsT>>(L, Is + 2)...);
                    TStack<decltype(Ret)>::Push(L, Ret);
                    return 1;
                }
            }
            else
            {
                if constexpr (eastl::is_void_v<ReturnT>)
                {
                    TFunc(GetArg<eastl::tuple_element_t<Is, ArgsT>>(L, Is + 1)...);
                    return 0;
                }
                else
                {
                    auto Ret = TFunc(GetArg<eastl::tuple_element_t<Is, ArgsT>>(L, Is + 1)...);
                    TStack<decltype(Ret)>::Push(L, Ret);
                    return 1;
                }
            }
        };
        
        LUMINA_PROFILE_SCOPE();
        return Dispatch(eastl::make_index_sequence<TraitsT::ArgCount>{});
    }
    
    template<auto TFunc>
    auto InvokerWithInstance(lua_State* L)
    {
        using TraitsT = TFunctionTraits<decltype(TFunc)>;
        using ReturnT = TraitsT::ReturnType;
        using ArgsT   = TraitsT::ArgsTuple;
        
    
        auto Dispatch = [&]<size_t... Is>(eastl::index_sequence<Is...>)
        {
            if constexpr (eastl::is_member_function_pointer_v<decltype(TFunc)>)
            {
                using ClassT = TraitsT::ClassType;
                
                ClassT* Self = static_cast<ClassT*>(lua_tolightuserdatatagged(L, lua_upvalueindex(1), TClassTraits<ClassT>::Tag()));
                if (Self == nullptr)
                {
                    luaL_errorL(L, "[%s]", "Cannot invoke lua function with incorrect usertype.");
                }
                
                if constexpr (eastl::is_void_v<ReturnT>)
                {
                    (Self->*TFunc)(GetArg<eastl::tuple_element_t<Is, ArgsT>>(L, Is + 1)...);
                    return 0;
                }
                else
                {
                    auto Ret = (Self->*TFunc)(GetArg<eastl::tuple_element_t<Is, ArgsT>>(L, Is + 1)...);
                    TStack<decltype(Ret)>::Push(L, Ret);
                    return 1;
                }
            }
            else
            {
                if constexpr (eastl::is_void_v<ReturnT>)
                {
                    TFunc(GetArg<eastl::tuple_element_t<Is, ArgsT>>(L, Is + 1)...);
                    return 0;
                }
                else
                {
                    auto Ret = TFunc(GetArg<eastl::tuple_element_t<Is, ArgsT>>(L, Is + 1)...);
                    TStack<decltype(Ret)>::Push(L, Ret);
                    return 1;
                }
            }
        };
    
        LUMINA_PROFILE_SCOPE();
        return Dispatch(eastl::make_index_sequence<TraitsT::ArgCount>{});
    }
}
