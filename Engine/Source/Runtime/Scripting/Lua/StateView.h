#pragma once
#include "Class.h"
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
        
        FStateView(lua_State* State)
            : L(State)
        {}
        
        template<typename T>
        TClass<T> NewClass(FStringView Name)
        {
            return TClass<T>(L, Name);
        }
        
        void RegisterValue(FStringView Name)
        {
            lua_setglobal(L, Name.data());
        }
        
    private:
        lua_State* L = nullptr;
    };
}
