#pragma once

#include "Invoker.h"
#include "lualib.h"
#include "Traits.h"
#include "Log/Log.h"
#include "Containers/String.h"

namespace Lumina::Lua
{
    template<typename T, typename TMethods = TTuple<>, typename TProperties = TTuple<>>
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
        }
        
        void Register()
        {
            using TMethodStorage   = TMethods;
            using TPropStorage     = TProperties;

            {
                auto* Stored = static_cast<TMethodStorage*>(lua_newuserdata(L, sizeof(TMethodStorage)));
                new (Stored) TMethodStorage(Methods);
            
                lua_pushcclosure(L, [](lua_State* State)
                {
                    int RawAtom = 0;
                    const char* AtomName = lua_namecallatom(State, &RawAtom);
                    int16 Atom = static_cast<int16>(RawAtom);
                    auto* BoundMethods = static_cast<TMethodStorage*>(lua_touserdata(State, lua_upvalueindex(1)));
                
                    int Result = 0;
                    bool bFound = false;
                    
                    eastl::apply([&](auto&&... Entry)
                    {
                        bFound = ((Entry.Atom == Atom ? (Result = Entry.Invoke(State), true) : false) || ...);
                    }, *BoundMethods);
                    
                    if (!bFound)
                    {
                        LOG_ERROR("Failed to find function: {} ({})", AtomName, Atom);
                    }
                
                    return Result;
                }, "__namecall", 1);
            
                lua_setfield(L, MetaTableIdx, "__namecall");
            }
            
            {
                auto* Stored = static_cast<TPropStorage*>(lua_newuserdata(L, sizeof(TPropStorage)));
                new (Stored) TPropStorage(Props);

                lua_pushvalue(L, MetaTableIdx);
                lua_pushcclosure(L, [](lua_State* State) -> int
                {
                    const char* Key = lua_tostring(State, 2);
                    uint16 Hash = Hash::FNV1a::GetHash16(Key);

                    auto* BoundProps = static_cast<TPropStorage*>(lua_touserdata(State, lua_upvalueindex(1)));

                    int Result = 0;
                    bool bFound = false;

                    eastl::apply([&](const auto&... Entry)
                    {
                        bFound = ((Entry.Atom == Hash ? (Result = Entry.Get(State), true) : false) || ...);
                    }, *BoundProps);

                    if (bFound)
                    {
                        return Result;
                    }

                    lua_pushvalue(State, 2);
                    lua_rawget(State, lua_upvalueindex(2));
                    return 1;

                }, "__index", 2);

                lua_setfield(L, MetaTableIdx, "__index");
            }
            
            {
                auto* Stored = static_cast<TPropStorage*>(lua_newuserdata(L, sizeof(TPropStorage)));
                new (Stored) TPropStorage(Props);

                lua_pushcclosure(L, [](lua_State* State) -> int
                {
                    const char* Key = lua_tostring(State, 2);
                    uint16 Hash = Hash::FNV1a::GetHash16(Key);

                    auto* BoundProps = static_cast<TPropStorage*>(lua_touserdata(State, lua_upvalueindex(1)));

                    bool bFound = false;

                    eastl::apply([&](const auto&... Entry)
                    {
                        bFound = ((Entry.Atom == Hash ? (Entry.Set(State), true) : false) || ...);
                    }, *BoundProps);

                    if (!bFound)
                    {
                        Key = lua_tostring(State, 2);
                        luaL_error(State, "Unknown property '%s'", Key ? Key : "?");
                    }

                    return 0;
                }, "__newindex", 1);
                

                lua_setfield(L, MetaTableIdx, "__newindex");
            }
            
            if constexpr (eastl::is_destructible_v<ClassT>)
            {
                lua_setuserdatadtor(L, MetaTableIdx, +[](lua_State* L, void* Userdata)
                {
                    auto* Header = static_cast<TUserdataHeader<ClassT>*>(Userdata);
                    Header->InvokeDtor();
                });
            }
            
            lua_setuserdatametatable(L, TClassTraits<T>::Tag());
        }
        
        template<auto TFunc>
        auto AddFunction(FStringView FnName);
        
        
        template<auto TGetter, auto TSetter = TGetter>
        auto AddProperty(FStringView PropName);
    
    private:
        
        template<typename U, typename UMethods, typename UProperties>
        friend class TClass;
        
        TClass(lua_State* VM, FStringView InName, int InMTIdx, TMethods InMethods, TProperties InProperties)
            : MetaTableIdx(InMTIdx)
            , Methods(InMethods)
            , Props(InProperties)
            , Name(InName)
            , L(VM)
        {}
        
    private:
        
        int32           MetaTableIdx;
        TMethods        Methods;
        TProperties     Props;
        FStringView     Name;
        lua_State*      L = nullptr;
    };

    template <typename T, typename TMethods, typename TProperties>
    template <auto TFunc>
    auto TClass<T, TMethods, TProperties>::AddFunction(FStringView FnName)
    {
        struct FMethodEntry
        {
            int Atom;

            int Invoke(lua_State* State)
            {
                return Invoker<TFunc>(State);
            }
        };
            
        int16 Atom = static_cast<int16>(Hash::FNV1a::GetHash16(FnName.data()));
        auto MethodTuple = eastl::tuple_cat(Methods, eastl::make_tuple(FMethodEntry{Atom}));
        using NewMethodsT = decltype(MethodTuple);
            
        return TClass<ClassT, NewMethodsT, TProperties>(L, Name, MetaTableIdx, MethodTuple, Props);
    }

    template <typename T, typename TMethods, typename TProperties>
    template <auto TGetter, auto TSetter>
    auto TClass<T, TMethods, TProperties>::AddProperty(FStringView PropName)
    {
        struct FPropertyEntry
        {
            uint16 Atom;

            int Get(lua_State* State) const
            {
                return Invoker<TGetter>(State);
            }
                
            int Set(lua_State* State) const
            {
                return Invoker<TSetter>(State);
            }
        };

        uint16 Atom = Hash::FNV1a::GetHash16(PropName.data());
        auto NewProps = eastl::tuple_cat(Props, eastl::make_tuple(FPropertyEntry{ Atom }));
        using NewPropsT = decltype(NewProps);

        return TClass<ClassT, TMethods, NewPropsT>(L, Name, MetaTableIdx, Methods, NewProps);
    }
}
