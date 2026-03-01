#pragma once
#include "lstate.h"


namespace Lumina::Lua
{
    template<typename T>
    struct TStack;
    
    template<>
    struct TStack<int>
    {
        static void Push(lua_State* State, int Value)   { lua_pushinteger(State, Value); }
        static int  Get(lua_State* State, int Index)    { return lua_tointeger(State, Index); }
        static bool Check(lua_State* State, int Index)  { return lua_type(State, Index) == LUA_TNUMBER; }
    };

    template<>
    struct TStack<float>
    {
        static void Push(lua_State* State, float Value) { lua_pushnumber(State, Value); }
        static float Get(lua_State* State, int Index)   { return (float)lua_tonumber(State, Index); }
        static bool Check(lua_State* State, int Index)  { return lua_type(State, Index) == LUA_TNUMBER; }
    };

    template<>
    struct TStack<bool>
    {
        static void Push(lua_State* State, bool Value)  { lua_pushboolean(State, Value); }
        static bool Get(lua_State* State, int Index)    { return lua_toboolean(State, Index); }
        static bool Check(lua_State* State, int Index)  { return lua_isboolean(State, Index); }
    };

    template<>
    struct TStack<const char*>
    {
        static void Push(lua_State* State, const char* Value) { lua_pushstring(State, Value); }
        static const char* Get(lua_State* State, int Index)   { return lua_tostring(State, Index); }
        static bool Check(lua_State* State, int Index)        { return lua_type(State, Index) == LUA_TSTRING; }
    };
    
    template<>
    struct TStack<FStringView>
    {
        static void Push(lua_State* State, const char* Value) { lua_pushstring(State, Value); }
        static const char* Get(lua_State* State, int Index)   { return lua_tostring(State, Index); }
        static bool Check(lua_State* State, int Index)        { return lua_type(State, Index) == LUA_TSTRING; }
    };
    
    template<>
    struct TStack<FString>
    {
        static void Push(lua_State* State, const FString& Value)    { lua_pushstring(State, Value.c_str()); }
        static FString Get(lua_State* State, int Index)             { return lua_tostring(State, Index); }
        static bool Check(lua_State* State, int Index)              { return lua_type(State, Index) == LUA_TSTRING; }
    };
    
    template<typename T>
    struct TStack
    {
        template<typename... TArgs>
        static void Push(lua_State* State, TArgs&&... Args)
        {
            void* Block = lua_newuserdata(State, sizeof(T));
            new (Block) T(eastl::forward<TArgs>(Args)...);

            lua_rawgetp(State, LUA_REGISTRYINDEX, TClassTraits<T>::MetaTableKey());
            lua_setmetatable(State, -2);
        }

        static T* Get(lua_State* State, int Index)
        {
            return static_cast<T*>(lua_touserdata(State, Index));
        }

        static bool Check(lua_State* State, int Index)
        {
            return lua_type(State, Index) == LUA_TUSERDATA;
        }
    };
    
}
