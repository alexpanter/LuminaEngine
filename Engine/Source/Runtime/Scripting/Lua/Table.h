#pragma once
#include "Namespace.h"


namespace Lumina::Lua
{
    class FTable : public TNamespace<FTable>
    {
        friend class TNamespace;
    public:
        
        lua_State* L = nullptr;
        FStringView Name;
        int32 Ref;
    };
}
