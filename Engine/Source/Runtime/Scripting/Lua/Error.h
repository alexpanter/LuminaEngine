#pragma once

#include "Containers/String.h"
#include "Core/Utils/Expected.h"

namespace Lumina::Lua
{
    struct FLuaError
    {
        FString Message;
    };
    
    template<typename T>
    using TResult = TExpected<T, FLuaError>;
}
