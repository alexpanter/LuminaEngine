#pragma once

#include "lua.h"

namespace Lumina::Lua
{
    template<typename TDerived>
    class TNamespace
    {
    public:
        
        
        lua_State* GetState() const { return Self().L; }

    private:

        TDerived& Self()             { return static_cast<TDerived&>(*this); }
        const TDerived& Self() const { return static_cast<const TDerived&>(*this); }
        
        
    private:
    };
}
