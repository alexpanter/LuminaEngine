#pragma once
#include "lua.h"
#include "Traits.h"
#include <entt/entt.hpp>
#include "UserDataHeader.h"
#include "Containers/Array.h"
#include "Containers/Name.h"
#include "Containers/String.h"
#include "Memory/Memcpy.h"


namespace Lumina::Lua
{
    template<typename T>
    struct TLuaNativeType : eastl::false_type {};
    
    struct FNil {};

    // Specialize for types that have native Lua representations
    template<> struct TLuaNativeType<FNil>              : eastl::true_type {};
    template<> struct TLuaNativeType<int>               : eastl::true_type {};
    template<> struct TLuaNativeType<float>             : eastl::true_type {};
    template<> struct TLuaNativeType<double>            : eastl::true_type {};
    template<> struct TLuaNativeType<int8>              : eastl::true_type {};
    template<> struct TLuaNativeType<uint8>             : eastl::true_type {};
    template<> struct TLuaNativeType<uint32>            : eastl::true_type {};
    template<> struct TLuaNativeType<int64>             : eastl::true_type {};
    template<> struct TLuaNativeType<uint64>            : eastl::true_type {};
    template<> struct TLuaNativeType<bool>              : eastl::true_type {};
    template<> struct TLuaNativeType<FString>           : eastl::true_type {};
    template<> struct TLuaNativeType<FStringView>       : eastl::true_type {};
    template<> struct TLuaNativeType<FName>             : eastl::true_type {};
    template<> struct TLuaNativeType<const char*>       : eastl::true_type {};
    template<> struct TLuaNativeType<glm::vec2>         : eastl::true_type {};
    template<> struct TLuaNativeType<glm::vec3>         : eastl::true_type {};
    template<> struct TLuaNativeType<glm::vec4>         : eastl::true_type {};

    template<typename T>
    struct TStack {};
    
    template<>
    struct TStack<FNil>
    {
        static FStringView TypeName(lua_State* State)       { return lua_typename(State, LUA_TNIL); }
        static void Push(lua_State* State, FNil)            { lua_pushnil(State); }
        static FNil  Get(lua_State* State, int Index)       { return {}; }
        static bool Check(lua_State* State, int Index)      { return lua_isnil(State, Index); }
    };
    
    template<>
    struct TStack<int>
    {
        static FStringView TypeName(lua_State* State)       { return lua_typename(State, LUA_TNUMBER); }
        static void Push(lua_State* State, int Value)       { lua_pushinteger(State, Value); }
        static int  Get(lua_State* State, int Index)        { return luaL_checkinteger(State, Index); }
        static bool Check(lua_State* State, int Index)      { return lua_isnumber(State, Index); }
    };
    
    template<>
    struct TStack<unsigned>
    {
        static FStringView TypeName(lua_State* State)           { return lua_typename(State, LUA_TNUMBER); }
        static void Push(lua_State* State, unsigned Value)      { lua_pushunsigned(State, Value); }
        static unsigned  Get(lua_State* State, int Index)       { return luaL_checkunsigned(State, Index); }
        static bool Check(lua_State* State, int Index)          { return lua_isnumber(State, Index); }
    };

    template<>
    struct TStack<float>
    {
        static FStringView TypeName(lua_State* State)       { return lua_typename(State, LUA_TNUMBER); }
        static void Push(lua_State* State, float Value)     { lua_pushnumber(State, Value); }
        static float Get(lua_State* State, int Index)       { return (float)luaL_checknumber(State, Index); }
        static bool Check(lua_State* State, int Index)      { return lua_isnumber(State, Index); }
    };
    
    template<>
    struct TStack<double>
    {
        static FStringView TypeName(lua_State* State)        { return lua_typename(State, LUA_TNUMBER); }
        static void Push(lua_State* State, double Value)     { lua_pushnumber(State, Value); }
        static double Get(lua_State* State, int Index)       { return (float)luaL_checknumber(State, Index); }
        static bool Check(lua_State* State, int Index)       { return lua_isnumber(State, Index); }
    };
    
    template<>
    struct TStack<bool>
    {
        static FStringView TypeName(lua_State* State)   { return lua_typename(State, LUA_TBOOLEAN); }
        static void Push(lua_State* State, bool Value)  { lua_pushboolean(State, Value); }
        static bool Get(lua_State* State, int Index)    { return luaL_checkboolean(State, Index); }
        static bool Check(lua_State* State, int Index)  { return lua_isboolean(State, Index); }
    };

    template<>
    struct TStack<const char*>
    {
        static FStringView TypeName(lua_State* State)         { return lua_typename(State, LUA_TSTRING); }
        static void Push(lua_State* State, const char* Value) { lua_pushstring(State, Value); }
        static const char* Get(lua_State* State, int Index)   { return luaL_checkstring(State, Index); }
        static bool Check(lua_State* State, int Index)        { return lua_isstring(State, Index); }
    };
    
    template<>
    struct TStack<FStringView>
    {
        static FStringView TypeName(lua_State* State)         { return lua_typename(State, LUA_TSTRING); }
        static void Push(lua_State* State, FStringView Value) { lua_pushstring(State, Value.data()); }
        static FStringView Get(lua_State* State, int Index)   { return luaL_checkstring(State, Index); }
        static bool Check(lua_State* State, int Index)        { return lua_isstring(State, Index); }
    };
    
    template<>
    struct TStack<FString>
    {
        static FStringView TypeName(lua_State* State)               { return lua_typename(State, LUA_TSTRING); }
        static void Push(lua_State* State, const FString& Value)    { lua_pushstring(State, Value.c_str()); }
        static FString Get(lua_State* State, int Index)             { return luaL_checkstring(State, Index); }
        static bool Check(lua_State* State, int Index)              { return lua_isstring(State, Index); }
    };
    
    template<>
    struct TStack<FFixedString>
    {
        static FStringView TypeName(lua_State* State)                   { return lua_typename(State, LUA_TSTRING); }
        static void Push(lua_State* State, const FFixedString& Value)   { lua_pushstring(State, Value.c_str()); }
        static FFixedString Get(lua_State* State, int Index)            { return luaL_checkstring(State, Index); }
        static bool Check(lua_State* State, int Index)                  { return lua_isstring(State, Index); }
    };
    
    template<>
    struct TStack<FName>
    {
        static FStringView TypeName(lua_State* State)               { return lua_typename(State, LUA_TSTRING); }
        static void Push(lua_State* State, const FName& Value)      { lua_pushstring(State, Value.c_str()); }
        static FName Get(lua_State* State, int Index)               { return luaL_checkstring(State, Index); }
        static bool Check(lua_State* State, int Index)              { return lua_isstring(State, Index); }
    };
    
    template<>
    struct TStack<entt::entity>
    {
        static FStringView TypeName(lua_State* State)                   { return lua_typename(State, LUA_TNUMBER); }
        static void Push(lua_State* State, const entt::entity& Value)   { lua_pushinteger(State, (int)entt::to_integral(Value)); }
        static entt::entity Get(lua_State* State, int Index)            { return static_cast<entt::entity>(luaL_checkinteger(State, Index)); }
        static bool Check(lua_State* State, int Index)                  { return lua_isnumber(State, Index); }
    };
    
    template<>
    struct TStack<glm::vec2>
    {
        static FStringView TypeName(lua_State* State) { return lua_typename(State, LUA_TVECTOR); }

        static void Push(lua_State* State, const glm::vec2& Value)
        {
            lua_pushvector(State, Value.x, Value.y, 0.0f, 0.0f);
        }
    
        static glm::vec2 Get(lua_State* State, int Index)
        {
            const float* V = luaL_checkvector(State, Index);
            return { V[0], V[1] };
        }
    
        static bool Check(lua_State* State, int Index)
        {
            return lua_isvector(State, Index);
        }
    };
    
    template<>
    struct TStack<glm::vec3>
    {
        static FStringView TypeName(lua_State* State) { return lua_typename(State, LUA_TVECTOR); }
        static void Push(lua_State* State, const glm::vec3& Value)
        {
            lua_pushvector(State, Value.x, Value.y, Value.z, 0.0f);
        }
    
        static glm::vec3 Get(lua_State* State, int Index)
        {
            const float* V = luaL_checkvector(State, Index);
            return { V[0], V[1], V[2] };
        }
    
        static bool Check(lua_State* State, int Index)
        {
            return lua_isvector(State, Index);
        }
    };
    
    template<>
    struct TStack<glm::vec4>
    {
        static FStringView TypeName(lua_State* State) { return lua_typename(State, LUA_TVECTOR); }
        static void Push(lua_State* State, const glm::vec4& Value)
        {
            lua_pushvector(State, Value.x, Value.y, Value.z, Value.w);
        }
    
        static glm::vec4 Get(lua_State* State, int Index)
        {
            const float* V = luaL_checkvector(State, Index);
            return { V[0], V[1], V[2], V[3]};
        }
    
        static bool Check(lua_State* State, int Index)
        {
            return lua_isvector(State, Index);
        }
    };
    
    template<typename T>
    requires(eastl::is_arithmetic_v<T>)
    struct TStack<TVector<T>>
    {
        static FStringView TypeName(lua_State* State) { return lua_typename(State, LUA_TBUFFER); }
        
        static void Push(lua_State* State, const TVector<T>& Value)
        {
            LUMINA_PROFILE_SCOPE();
            void* Buffer = lua_newbuffer(State, Value.size());
            Memory::Memcpy(Buffer, Value.data(), Value.size() * sizeof(T));
        }
        
        static TVector<T> Get(lua_State* State, int Index)
        {
            size_t Size = 0;
            void* Buffer = luaL_checkbuffer(State, Index, &Size);
        
            TVector<T> Result(Size / sizeof(T));
            Memory::Memcpy(Result.data(), Buffer, Size * sizeof(T));
            return Result;
        }
        
        static bool Check(lua_State* State, int Index)
        {
            return lua_isbuffer(State, Index);
        }
    };
    
    template<typename T>
    requires(!eastl::is_enum_v<T> && !eastl::is_pointer_v<T> && !eastl::is_reference_v<T>)
    struct TStack<T>
    {
        static FStringView TypeName(lua_State* State) { return lua_typename(State, LUA_TUSERDATA); }

        using RawT = eastl::remove_cvref_t<T>;

        template<typename... TArgs>
        requires(eastl::is_constructible_v<RawT, TArgs...>)
        static void Push(lua_State* State, TArgs&&... Args)
        {
            LUMINA_PROFILE_SCOPE();
            void* Block = lua_newuserdatataggedwithmetatable(State, sizeof(TUserdataHeader<RawT>), TClassTraits<RawT>::Tag());
            TUserdataHeader<RawT>* Header = new (Block) TUserdataHeader<RawT>{};
            Header->Emplace(eastl::forward<TArgs>(Args)...);
        }

        static RawT& Get(lua_State* State, int Index)
        {
            LUMINA_PROFILE_SCOPE();
            auto* Userdata = static_cast<TUserdataHeader<RawT>*>(lua_touserdatatagged(State, Index, TClassTraits<RawT>::Tag()));
            DEBUG_ASSERT(Userdata, "Tagged userdata not found!");
            return *Userdata->Underlying();
        }

        static bool Check(lua_State* State, int Index)
        {
            return lua_userdatatag(State, Index) == TClassTraits<RawT>::Tag();
        }
    };
    
    template<typename T>
    struct TStack<T*>
    {
        static FStringView TypeName(lua_State* State) { return lua_typename(State, LUA_TUSERDATA); }

        using RawT = eastl::remove_pointer_t<eastl::remove_cvref_t<T>>;

        static void Push(lua_State* State, T* Ptr)
        {
            LUMINA_PROFILE_SCOPE();
            void* Block = lua_newuserdatataggedwithmetatable(State, sizeof(TUserdataHeader<RawT*>), TClassTraits<RawT>::Tag());
            auto* Header = new (Block) TUserdataHeader<RawT*>{};
            Header->Emplace(Ptr);
        }

        static RawT* Get(lua_State* State, int Index)
        {
            LUMINA_PROFILE_SCOPE();
            auto* Header = static_cast<TUserdataHeader<RawT*>*>(lua_touserdatatagged(State, Index, TClassTraits<RawT>::Tag()));
            if (!ALERT_IF_NOT(Header, "Type is not registered as a userdata for Luau"))
            {
                return nullptr;
            }
            
            return *Header->Underlying();
        }

        static bool Check(lua_State* State, int Index)
        {
            return lua_userdatatag(State, Index) == TClassTraits<RawT>::Tag();
        }
    };
    
    template<>
    struct TStack<void*>
    {
        static FStringView TypeName(lua_State* State) { return lua_typename(State, LUA_TLIGHTUSERDATA); }

        static void Push(lua_State* State, void* Ptr)
        {
            lua_pushlightuserdata(State, Ptr);
        }
    
        static void* Get(lua_State* State, int Index)
        {
            return lua_touserdata(State, Index);
        }
    
        static bool Check(lua_State* State, int Index)
        {
            return lua_islightuserdata(State, Index);
        }
    };
    
    template<typename T>
    struct TStack<T&>
    {
        static FStringView TypeName(lua_State* State) { return lua_typename(State, LUA_TUSERDATA); }

        static void Push(lua_State* State, T& Ref)
        {
            TStack<T*>::Push(State, &Ref);
        }

        static T& Get(lua_State* State, int Index)
        {
            return *TStack<T*>::Get(State, Index);
        }

        static bool Check(lua_State* State, int Index)
        {
            return TStack<T*>::Check(State, Index);
        }
    };
    
    template<typename T>
    requires(eastl::is_enum_v<T>)
    struct TStack<T>
    {
        static FStringView TypeName(lua_State* State)   { return lua_typename(State, LUA_TNUMBER); }
        static void Push(lua_State* State, T Value)     { lua_pushinteger(State, (int)Value); }
        static T  Get(lua_State* State, int Index)      { return (T)luaL_checkinteger(State, Index); }
        static bool Check(lua_State* State, int Index)  { return lua_isnumber(State, Index); }
    };
}
