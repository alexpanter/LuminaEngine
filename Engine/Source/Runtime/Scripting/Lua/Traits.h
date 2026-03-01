#pragma once
#include "lua.h"
#include "Containers/String.h"
#include "Containers/Tuple.h"
#include "Platform/GenericPlatform.h"


namespace Lumina::Lua
{
    template<typename T>
    struct TFunctionTraits;
    
    template<typename TReturn, typename ... TArgs>
    struct TFunctionTraits<TReturn(*)(TArgs...)>
    {
        using ReturnType                    = TReturn;
        using ArgsTuple                     = TTuple<TArgs...>;
        static constexpr size_t ArgCount    = sizeof...(TArgs);
    };
    
    template<typename TReturn, typename TClass, typename... TArgs>
    struct TFunctionTraits<TReturn(TClass::*)(TArgs...)>
    {
        using ReturnType                 = TReturn;
        using ClassType                  = TClass;
        using ArgsTuple                  = TTuple<TArgs...>;
        static constexpr size_t ArgCount = sizeof...(TArgs);
    };

    template<typename TReturn, typename TClass, typename... TArgs>
    struct TFunctionTraits<TReturn(TClass::*)(TArgs...) const>
    {
        using ReturnType                 = TReturn;
        using ClassType                  = TClass;
        using ArgsTuple                  = TTuple<TArgs...>;
        static constexpr size_t ArgCount = sizeof...(TArgs);
    };
    
    
    
    
    template<typename T>
    struct TClassTraits
    {
        static void* MetaTableKey()
        {
            static int Unique = 0;
            return &Unique;
        }
    };
    
    
    
    template<typename T>
    T GetArg(lua_State* L, int Index)
    {
        if constexpr (eastl::is_same_v<T, int>)         return lua_tointeger(L, Index);
        if constexpr (eastl::is_same_v<T, float>)       return (float)lua_tonumber(L, Index);
        if constexpr (eastl::is_same_v<T, double>)      return lua_tonumber(L, Index);
        if constexpr (eastl::is_same_v<T, bool>)        return lua_toboolean(L, Index);
        if constexpr (eastl::is_same_v<T, const char*>) return lua_tostring(L, Index);
    }
    
    
    
    enum class EMetaMethod : uint8
    {
        Index,          // __index
        NewIndex,       // __newindex
        ToString,       // __tostring
        Add,            // __add
        Sub,            // __sub
        Mul,            // __mul
        Div,            // __div
        Mod,            // __mod
        UnaryMinus,     // __unm
        Eq,             // __eq
        Lt,             // __lt
        Le,             // __le
        Len,            // __len
        Call,           // __call
        GC,             // __gc
    };
    
    constexpr FStringView MetaMethodName(EMetaMethod M) noexcept
    {
        switch (M)
        {
            case EMetaMethod::Index:      return "__index";
            case EMetaMethod::NewIndex:   return "__newindex";
            case EMetaMethod::ToString:   return "__tostring";
            case EMetaMethod::Add:        return "__add";
            case EMetaMethod::Sub:        return "__sub";
            case EMetaMethod::Mul:        return "__mul";
            case EMetaMethod::Div:        return "__div";
            case EMetaMethod::Mod:        return "__mod";
            case EMetaMethod::UnaryMinus: return "__unm";
            case EMetaMethod::Eq:         return "__eq";
            case EMetaMethod::Lt:         return "__lt";
            case EMetaMethod::Le:         return "__le";
            case EMetaMethod::Len:        return "__len";
            case EMetaMethod::Call:       return "__call";
            case EMetaMethod::GC:         return "__gc";
            default:                      return "__unknown";
        }
    }
    
}
