#pragma once

#include "Reference.h"
#include "Containers/Name.h"
#include "Containers/String.h"

namespace Lumina::Lua
{
    struct FScript
    {
        FName               Name;
        FString             Path;
        FRef                Reference;
        FRef                Environment;
        FRef                Thread;
        
        bool                bDirty = false;
    };
}
