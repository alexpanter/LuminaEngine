#pragma once

#include "Traits.h"
#include "Containers/Tuple.h"
#include "Core/Assertions/Assert.h"

namespace Lumina::Lua
{
    template<typename T, typename TFuncs = TTuple<>, typename TMethods = TTuple<>>
    requires(!eastl::is_pointer_v<T>)
    class TClass
    {
        using TypeT = eastl::remove_cvref_t<T>;
        
    public:
        
        TClass(lua_State* State, FStringView InName);
        
        void Finalize();
        
        
    private:
        
        lua_State* L = nullptr;
        FStringView Name;
    };

    
    
    template <typename T, typename TFuncs, typename TMethods> requires (!eastl::is_pointer_v<T>)
    TClass<T, TFuncs, TMethods>::TClass(lua_State* State, FStringView InName)
    {
    }

    template <typename T, typename TFuncs, typename TMethods> requires (!eastl::is_pointer_v<T>)
    void TClass<T, TFuncs, TMethods>::Finalize()
    {
    }
}
