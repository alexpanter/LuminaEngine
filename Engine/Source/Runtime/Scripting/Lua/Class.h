#pragma once

#include "Invoker.h"
#include "lualib.h"
#include "Traits.h"
#include "Containers/String.h"

namespace Lumina::Lua
{
    template<typename T, typename TMethods = TTuple<>>
    class TClass
    {
        using ClassTraitsT = TClassTraits<T>;
        using ClassT = T;
    public:
        
        TClass(lua_State* VM, FStringView InName)
            : Name(InName)
            , L(VM)
        {
            luaL_newmetatable(L, InName.data());
            MetaTableIdx = lua_gettop(L);
            
            lua_pushvalue(L, MetaTableIdx);
            lua_rawsetp(L, LUA_REGISTRYINDEX, ClassTraitsT::MetaTableKey());
        }
        
        void Register()
        {
            using TStorage = TMethods;
            TStorage* Stored = static_cast<TStorage*>(lua_newuserdata(L, sizeof(TStorage)));
            new (Stored) TStorage(Methods);
            
            lua_pushcclosure(L, [](lua_State* State)
            {
                int Atom = 0;
                lua_namecallatom(State, &Atom);
                
                auto* BoundMethods = static_cast<TStorage*>(lua_touserdata(State, lua_upvalueindex(1)));
                
                int Result = 0;
                bool bFound = false;
        
                eastl::apply([&](auto&&... Entry)
                {
                    bFound = ((Entry.Atom == Atom ? (Result = Entry.Invoke(State), true) : false) || ...);
                }, *BoundMethods);
                
                return Result;
            }, "__namecall", 1);
            
            lua_setfield(L, MetaTableIdx, "__namecall");
            lua_pushvalue(L, MetaTableIdx);
            lua_setfield(L, MetaTableIdx, "__index");
            lua_pop(L, 1);

            lua_newtable(L);
            lua_pushstring(L, Name.data());
            lua_pushcclosure(L, [](lua_State* State) -> int
            {
                const char* Name = lua_tostring(State, lua_upvalueindex(1));
                
                void* Block = lua_newuserdata(State, sizeof(T));
                new (Block) T();
            
                lua_rawgetp(State, LUA_REGISTRYINDEX, ClassTraitsT::MetaTableKey());
                lua_setmetatable(State, -2);
            
                return 1;
            }, "new", 1);
            
            lua_setfield(L, -2, "new");
            lua_setglobal(L, Name.data());
        }
        
        template<auto TFunc>
        auto Function(FStringView FnName)
        {
            struct FMethodEntry
            {
                FName   Name;
                int     Atom;

                int Invoke(lua_State* State)
                {
                    return Invoker<TFunc>(State);
                }
            };
            
            
            lua_pushstring(L, FnName.data());
            int Atom;
            lua_tostringatom(L, -1, &Atom);
            auto MethodTuple = eastl::tuple_cat(Methods, eastl::make_tuple(FMethodEntry{FnName, Atom}));
            using NewMethodsT = decltype(MethodTuple);
            
            return TClass<ClassT, NewMethodsT>(L, Name, MetaTableIdx, MethodTuple);
        }
    
    private:
        
        template<typename U, typename UMethods>
        friend class TClass;
        
        TClass(lua_State* VM, FStringView InName, int InMTIdx, TMethods InMethods)
            : MetaTableIdx(InMTIdx)
            , Methods(InMethods)
            , Name(InName)
            , L(VM)
        {}
        
    private:
        
        int32           MetaTableIdx;
        TMethods        Methods;
        FStringView     Name;
        lua_State*      L = nullptr;
    };
}
