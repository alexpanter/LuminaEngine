#pragma once

#include "Core/Variant/Variant.h"

namespace Lumina::Lua
{
    template<typename T>
    struct TUserdataHeader
    {
        using RawT                      = eastl::remove_pointer_t<eastl::decay_t<T>>;
        
        template<typename... TArgs>
        void Emplace(TArgs&&... Args)
        {
            Storage.template emplace<RawT>(eastl::forward<TArgs>(Args)...);
        }

        void SetExternal(RawT* Ptr)
        {
            Storage.template emplace<RawT*>(Ptr);
        }

        RawT* Underlying()
        {
            if (auto* Obj = eastl::get_if<RawT>(&Storage))
            {
                return Obj;
            }
            
            return *eastl::get_if<RawT*>(&Storage);
        }
        
        void InvokeDtor()
        {
            Storage = {};
        }

        TVariant<eastl::monostate, RawT, RawT*> Storage;
    };
}