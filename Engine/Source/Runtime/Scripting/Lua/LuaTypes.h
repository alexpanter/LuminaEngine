#pragma once
#include "Platform/GenericPlatform.h"

namespace Lumina::Lua
{
    enum lua_Type
    {
        LUA_TNIL = 0,     // must be 0 due to lua_isnoneornil
        LUA_TBOOLEAN = 1, // must be 1 due to l_isfalse

        LUA_TLIGHTUSERDATA,
        LUA_TNUMBER,
        LUA_TVECTOR,

        LUA_TSTRING, // all types above this must be value types, all types below this must be GC types - see iscollectable

        LUA_TTABLE,
        LUA_TFUNCTION,
        LUA_TUSERDATA,
        LUA_TTHREAD,
        LUA_TBUFFER,

        // values below this line are used in GCObject tags but may never show up in TValue type tags
        LUA_TPROTO,
        LUA_TUPVAL,
        LUA_TDEADKEY,

        // the count of TValue type tags
        LUA_T_COUNT = LUA_TPROTO
    };
    
    enum class EType : uint8
    {
        Nil = 0,
        Boolean = 1,
        LightUserData = 2,
        Number = 3,
        Vector = 4,
        String = 5,
        Table = 6,
        Function = 7,
        Userdata = 8,
        Thread = 9,
        Buffer = 10,
        
        Count = 11,
    };
}
