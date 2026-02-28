#pragma once
#include "Containers/String.h"
#include "lua.h"
#include "Traits.h"

namespace Lumina::Lua
{
    template<typename TDerived>
    class TNamespace
    {
    public:
        
        template<auto TFunc>
        requires (!eastl::is_member_function_pointer_v<decltype(TFunc)>)
        TDerived& SetFunction(FStringView Name)
        {
            lua_pushcfunction(GetState(), +[](lua_State* State) -> int
            {
                return TFreeFunctionHelper<TFunc>::Invoke(State);
            }, Name.data());

            Self().RegisterValue(Name);
            return Self();
        }

        template<auto TFunc, typename TInstance>
        requires (eastl::is_member_function_pointer_v<decltype(TFunc)>)
        TDerived& SetFunction(FStringView Name, TInstance* Instance)
        {
            using Helper   = TMemberFunctionHelper<TFunc>;
            using FStorage = Helper::FStorage;

            lua_pushlightuserdata(GetState(), static_cast<void*>(Instance));

            FStorage* Storage = static_cast<FStorage*>(lua_newuserdata(GetState(), sizeof(FStorage)));
            Storage->Func = TFunc;

            lua_pushcclosure(GetState(), +[](lua_State* State) -> int
            {
                return Helper::Invoke(State);
            }, Name.data(), 2);

            Self().RegisterValue(Name);
            return Self();
        }
        
        lua_State* GetState() const { return Self().L; }

    private:

        TDerived& Self()             { return static_cast<TDerived&>(*this); }
        const TDerived& Self() const { return static_cast<const TDerived&>(*this); }
        
        
    private:
    };
}
