#include "pch.h"
#include "Traits.h"

namespace Lumina::Lua
{
    uint16 FTypeIndex::Next()
    {
        static uint16_t GNextID = 1;
        return GNextID++;
    }
}
