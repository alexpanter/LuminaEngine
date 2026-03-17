#pragma once
#include "Reference.h"
#include "Containers/Array.h"


namespace Lumina::Lua
{
    class FVariadicArgs
    {
    public:
        
        friend struct TStack<FVariadicArgs>;
        
        FVariadicArgs() = default;        
        int Count() const { return ArgCount; }

        template<typename T>
        T Get(int Index) const
        {
            return TStack<T>::Get(State, StartIndex + Index);
        }
        
        template<typename T>
        NODISCARD bool Is(int Index)
        {
            return TStack<T>::Check(State, StartIndex + Index);
        }
        
    private:
        
        lua_State*  State = nullptr;
        int         StartIndex = 0;
        int         ArgCount = 0;
    };
    
    
    template<>
    struct TStack<FVariadicArgs>
    {
        static FStringView TypeName(lua_State* State) { return lua_typename(State, LUA_TUSERDATA); }

        [[noreturn]] static void Push(lua_State* State, FVariadicArgs)
        {
            luaL_error(State, "Cannot push FVariadicArgs to Lua stack");
        }

        static FVariadicArgs Get(lua_State* State, int Index)
        {
            FVariadicArgs Result;
            Result.State      = State;
            Result.StartIndex = Index;
            Result.ArgCount   = lua_gettop(State) - Index + 1;
            return Result;
        }

        static bool Check(lua_State* State, int Index)
        {
            return true;
        }
    };
}
