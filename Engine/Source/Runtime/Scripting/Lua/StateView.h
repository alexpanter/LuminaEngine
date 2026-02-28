#pragma once
#include "Traits.h"
#include "Stack.h"
#include "Table.h"


struct lua_State;

namespace Lumina::Lua
{
    class FStateView : public TNamespace<FStateView>
    {
        friend class TNamespace;

    public:
        
        FStateView(lua_State* InLua)
            : L(InLua)
        {}
        
        FTable CreateNamedTable(FStringView Name) const
        {
            lua_newtable(L);
            int Ref = lua_ref(L, -1);
            lua_setglobal(L, Name.data());
            return FTable(L, Name, Ref);
        }
        
    private:
        
        void RegisterValue(FStringView Name)
        {
            lua_setglobal(L, Name.data());
        }
        
    private:
        lua_State* L = nullptr;
    };
}
