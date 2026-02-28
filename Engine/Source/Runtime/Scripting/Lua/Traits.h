#pragma once
#include "lua.h"
#include "Containers/String.h"
#include "Stack.h"
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
    
    template<typename T>
    struct TMemberTraits;
    
    template<typename TClass, typename TReturn, typename... TArgs>
    struct TMemberTraits<TReturn(TClass::*)(TArgs...)>
    {
        using ClassType                     = TClass;
        using ReturnType                    = TReturn;
        using ArgsTuple                     = TTuple<TArgs...>;
        using IsConst                       = eastl::false_type;
        static constexpr size_t ArgCount    = sizeof...(TArgs);
    };
    
    template<typename TClass, typename TReturn, typename... TArgs>
    struct TMemberTraits<TReturn(TClass::*)(TArgs...) const>
    {
        using ClassType                     = TClass;
        using ReturnType                    = TReturn;
        using ArgsTuple                     = TTuple<TArgs...>;
        using IsConst                       = eastl::true_type;
        static constexpr size_t ArgCount    = sizeof...(TArgs);
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
    
    
    
    template<auto TFunc>
    struct TFreeFunctionHelper
    {
        using Traits        = TFunctionTraits<decltype(TFunc)>;
        using TRet          = Traits::ReturnType;
        using TArgsTuple    = Traits::ArgsTuple;

        static int Invoke(lua_State* State)
        {
            return InvokeImpl(State, TArgsTuple{}, eastl::make_index_sequence<Traits::ArgCount>{});
        }
        
        template<typename... TArgs, size_t... Is>
        static int InvokeImpl(lua_State* State, TTuple<TArgs...>, eastl::index_sequence<Is...>)
        {
            int ProvidedArgs = lua_gettop(State);
            if (ProvidedArgs != Traits::ArgCount)
            {
                LOG_ERROR("Bad argument count!");
                return 0;
            }
            
            if (!(TStack<TArgs>::Check(State, Is + 1) && ...))
            {
                LOG_ERROR("Invalid type mismatch");
                return 0;
            }
            
            int Index = 1;
            TTuple<TArgs...> Args = { GetArg<TArgs>(State, Index++)... };
        
            if constexpr (eastl::is_void_v<TRet>)
            {
                eastl::apply(TFunc, Args);
                return 0;
            }
            else
            {
                TRet Ret = eastl::apply(TFunc, Args);
                TStack<TRet>::Push(State, Ret);
                return 1;
            }
        }
    };
    
    template<auto TFunc>
    struct TMemberFunctionHelper
    {
        using Traits    = TMemberTraits<decltype(TFunc)>;
        using TClass    = Traits::ClassType;
        using TRet      = Traits::ReturnType;

        struct FStorage
        {
            decltype(TFunc) Func;
        };

        static int Invoke(lua_State* State)
        {
            TClass* Instance = static_cast<TClass*>(lua_touserdata(State, lua_upvalueindex(1)));
            FStorage* Storage = static_cast<FStorage*>(lua_touserdata(State, lua_upvalueindex(2)));

            return InvokeImpl(State, Instance, Storage->Func, typename Traits::ArgsTuple{}, eastl::make_index_sequence<Traits::ArgCount>{});
        }

    private:

        template<typename... TArgs, size_t... Is>
        static int InvokeImpl(lua_State* State, TClass* Instance, decltype(TFunc) Func, TTuple<TArgs...>, eastl::index_sequence<Is...>)
        {
            int ProvidedArgs = lua_gettop(State);
            if (ProvidedArgs != sizeof...(TArgs))
            {
                return 0;
            }

            if (!(TStack<TArgs>::Check(State, Is + 1) && ...))
            {
                return 0;
            }

            int Index = 1;
            TTuple<TArgs...> Args = { TStack<TArgs>::Get(State, Index++)... };

            if constexpr (eastl::is_void_v<TRet>)
            {
                eastl::apply([&](auto&&... A){ (Instance->*Func)(A...); }, Args);
                return 0;
            }
            else
            {
                TRet Ret = eastl::apply([&](auto&&... A){ return (Instance->*Func)(A...); }, Args);
                TStack<TRet>::Push(State, Ret);
                return 1;
            }
        }
    };
    
    
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
