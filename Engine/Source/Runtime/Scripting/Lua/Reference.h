#pragma once

#include "Class.h"
#include "Error.h"
#include "Invoker.h"
#include "lua.h"
#include "Stack.h"
#include "Log/Log.h"


namespace Lumina::Lua
{
    class RUNTIME_API FRef
    {
    public:
        
        FRef() = default;
        FRef(lua_State* L, int Index);
        ~FRef();
        
        FRef(const FRef& Other);
        FRef& operator=(const FRef& Other);
        
        FRef(FRef&& Other) noexcept;
        FRef& operator=(FRef&& Other) noexcept;
        
        template<typename T>
        requires(!eastl::is_function_v<T> && !eastl::is_member_function_pointer_v<T>)
        void Set(FStringView Key, const T& Value);
        
        template<auto TFunc, typename TClass = void>
        void SetFunction(FStringView Key, TClass* Instance = nullptr);
        
        template<auto TFunc, typename TClass = void>
        void SetFunction(EMetaMethod Meta, TClass* Instance = nullptr);
        
        template<typename T>
        NODISCARD bool Is() const;
        
        template<typename T>
        NODISCARD T Get() const;
        
        template<typename T>
        NODISCARD T GetOr(const T& Default);
        
        template<typename... TArgs>
        NODISCARD FRef Invoke(TArgs&& ... Args);
        
        template<typename T, typename... TArgs>
        NODISCARD T InvokeGet(TArgs&& ... Args);
        
        template<typename T>
        NODISCARD TClass<T> NewClass(FStringView Name);
        
        void Reset();
        bool Push() const;
        
        
        NODISCARD FRef NewTable(FStringView Key) const;
        NODISCARD bool IsValid() const;
        NODISCARD FRef GetField(FStringView Key) const;
        NODISCARD bool IsInvokable() const;
        NODISCARD bool IsTable() const;
        NODISCARD lua_State* GetState() const { return State; }
    
        
    public:
        
        template<typename... TArgs>
        FRef operator()(TArgs&&... Args)
        {
            return Invoke(eastl::forward<TArgs>(Args)...);
        }
        
        FRef operator [](FStringView Key) const
        {
            return GetField(Key);
        }
        
        explicit operator bool() const { return IsValid(); }

        bool operator==(const FRef& Other) const
        {
            if (State != Other.State)
            {
                return false;
            }
            
            (void)Push();
            (void)Other.Push();
            bool Result = lua_rawequal(State, -1, -2);
            lua_pop(State, 2);
            return Result;
        }    
        
    private:
        
        lua_State* State = nullptr;
        int Ref = LUA_NOREF;
    };

    template<>
    struct TStack<FRef>
    {
        static void Push(lua_State* State, const FRef& Value)
        {
            if (!Value.IsValid())
            {
                lua_pushnil(State);
                return;
            }
        
            (void)Value.Push();
        }

        static bool Check(lua_State* State, int Index)
        {
            return !lua_isnone(State, Index);
        }

        static FRef Get(lua_State* State, int Index)
        {
            if (lua_isnoneornil(State, Index))
            {
                return {};
            }
        
            return FRef(State, Index);
        }
    };

    template <typename T>
    requires(!eastl::is_function_v<T> && !eastl::is_member_function_pointer_v<T>)
    void FRef::Set(FStringView Key, const T& Value)
    {
        if (!Push())
        {
            return;
        }
        
        TStack<T>::Push(State, Value);
        lua_setfield(State, -2, Key.data());
        lua_pop(State, 1);
    }

    template <auto TFunc, typename TClass = void>
    void FRef::SetFunction(FStringView Key, TClass* Instance)
    {
        if (!Push())
        {
            return;
        }
        
        if constexpr (eastl::is_void_v<TClass>)
        {
            lua_pushcfunction(State, [](lua_State* L)
            {
                return Invoker<TFunc>(L);
            }, Key.data());
        }
        else
        {
            lua_pushlightuserdata(State, Instance);
            lua_pushcclosure(State, [](lua_State* L)
            {
                return InvokerWithInstance<TFunc>(L);
            }, Key.data(), 1);
        }
        lua_setfield(State, -2, Key.data());
        lua_pop(State, 1);
    }

    template <auto TFunc, typename TClass>
    void FRef::SetFunction(EMetaMethod Meta, TClass* Instance)
    {
        SetFunction<TFunc>(MetaMethodName(Meta), Instance);
    }

    template <typename T>
    bool FRef::Is() const
    {
        if (!Push())
        {
            return false;
        }
        
        bool Result = TStack<T>::Check(State, Ref);
        lua_pop(State, 1);
        return Result;
    }

    template <typename T>
    T FRef::Get() const
    {
        (void)Push();
        T Result = TStack<T>::Get(State, -1);
        lua_pop(State, 1);
        return Result;
    }

    template <typename T>
    T FRef::GetOr(const T& Default)
    {
        if(!Push())
        {
            return Default;
        }
        
        T Result = TStack<T>::Get(State, -1);
        lua_pop(State, 1);
        return Result;
    }

    template <typename ... TArgs>
    FRef FRef::Invoke(TArgs&&... Args)
    {
        if (!IsValid())
        {
            return {};
        }
        
        lua_checkstack(State, sizeof...(Args) + 2);
        
        if (!Push())
        {
            return {};
        }
        
        (TStack<eastl::decay_t<TArgs>>::Push(State, eastl::forward<TArgs>(Args)), ...);
    
        int Status = lua_pcall(State, sizeof...(Args), 1, 0);
        
        if (Status != LUA_OK)
        {
            FString ErrMsg = lua_tostring(State, -1);
            LOG_ERROR("[Lua] - Invoke Failed {}", ErrMsg);
            lua_pop(State, 1);
            return {};
        }
        
        FRef Result(State, -1);
        lua_pop(State, 1);
        return Result;
    }

    template <typename T, typename ... TArgs>
    T FRef::InvokeGet(TArgs&&... Args)
    {
        lua_checkstack(State, sizeof...(Args) + 2);
        
        if (!Push())
        {
            return T{};
        }
        
        (TStack<eastl::decay_t<TArgs>>::Push(State, eastl::forward<TArgs>(Args)), ...);

        if (lua_pcall(State, sizeof...(Args), 1, 0) != LUA_OK)
        {
            LOG_ERROR("[Lua] - Invoke Failed {}", lua_tostring(State, -1));
            lua_pop(State, 1);
            return T{};
        }

        T Result = TStack<T>::Get(State, -1);
        lua_pop(State, 1);
        return Result;
    }

    template <typename T>
    TClass<T> FRef::NewClass(FStringView Name)
    {
        int Top = lua_gettop(State);
        if (!Push())
        {
            return TClass<T>(State, Name);
        }
        
        ASSERT(lua_istable(State, -1));
        TClass<T> Class(State, Name);
        
        lua_pop(State, 1);
        DEBUG_ASSERT(lua_gettop(State) == Top);
        return Class;
    }
}
