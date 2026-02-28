#pragma once
#include "Namespace.h"


namespace Lumina::Lua
{
    class FTable : public TNamespace<FTable>
    {
        friend class TNamespace;
    public:
        
        FTable(lua_State* State, FStringView InName, int32 InRef)
            : L(State)
            , Name(InName)
            , Ref(InRef)
        {}
        
        FTable CreateNamedTable(FStringView TableName) const
        {
            lua_newtable(L);
            lua_pushvalue(L, -1);
            int TableRef = lua_ref(L, -1);
    
            lua_getref(L, Ref);
            lua_insert(L, -2);
            lua_setfield(L, -2, TableName.data()); 
            lua_pop(L, 1);
    
            return FTable(L, TableName, TableRef);
        }
        
    private:
        
        void RegisterValue(FStringView FieldName)
        {
            lua_getref(L, Ref);
            lua_insert(L, -2);
            lua_setfield(L, -2, FieldName.data());
            lua_pop(L, 1);
        }
        
    private:
        
        lua_State* L = nullptr;
        FStringView Name;
        int32 Ref;
    };
}
