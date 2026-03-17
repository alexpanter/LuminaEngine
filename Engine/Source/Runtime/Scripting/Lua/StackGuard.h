#pragma once
#include "lua.h"
#include "Core/LuminaMacros.h"


namespace Lumina::Lua
{
    class FStackGuard
    {
    public:
        
        LE_NO_COPYMOVE(FStackGuard);
        explicit FStackGuard(lua_State* L)
            : VM(L)
            , Top(lua_gettop(L))
        {
        }
        
        ~FStackGuard()
        {
            lua_settop(VM, Top);
        }
        
    private:
        
        lua_State* VM;
        int Top;
    };
}
