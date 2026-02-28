#pragma once

#include "Containers/Name.h"
#include "Containers/String.h"

namespace Lumina::Lua
{
    struct FScript
    {
        FName               Name;
        FString             Path;
    };
}
