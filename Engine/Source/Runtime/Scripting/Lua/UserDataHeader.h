#pragma once


namespace Lumina::Lua
{
    struct alignas(std::max_align_t) FUserdataHeader
    {
        void* Ptr   = nullptr;
        
        bool bOwned = false;
    };
}