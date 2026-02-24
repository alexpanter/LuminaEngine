#pragma once
#include <clang-c/Index.h>
#include "EASTL/string.h"


namespace Lumina
{
    struct FFieldInfo
    {
        CXType                              Type;
        EPropertyTypeFlags                  Flags;
        EPropertyFlags                      PropertyFlags = EPropertyFlags::None;
        eastl::string                       Name;
        eastl::string                       TypeName;
        eastl::string                       RawFieldType;
    };
}
