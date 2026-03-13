#include "pch.h"
#include "Traits.h"
#include "Core/Assertions/Assert.h"

namespace Lumina::Lua
{
    uint16 FTypeIndex::Next()
    {
        static uint16_t GNextID = 1;
        DEBUG_ASSERT(GNextID <= eastl::numeric_limits<uint16>::max(), "Hit the maximum number of allowed userdata!");
        return static_cast<uint16>(GNextID++);
    }
}
