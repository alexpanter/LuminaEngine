#pragma once


namespace Lumina::Lua
{
    template<typename T>
    struct TUserdataHeader
    {
        template<typename... TArgs>
        void Emplace(TArgs&&... args)
        {
            new (Storage) T(eastl::forward<TArgs>(args)...);
        }

        T* Underlying()
        {
            return reinterpret_cast<T*>(Storage);
        }

        const T* Underlying() const
        {
            return reinterpret_cast<const T*>(Storage);
        }

        alignas(T) unsigned char Storage[sizeof(T)];
    };
}