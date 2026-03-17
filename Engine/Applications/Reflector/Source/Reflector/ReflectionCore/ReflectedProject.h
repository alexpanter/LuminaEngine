#pragma once
#include "ReflectedHeader.h"
#include "StringHash.h"

#include "EASTL/hash_map.h"
#include "EASTL/unique_ptr.h"
#include "eastl/vector.h"

namespace Lumina::Reflection
{
    class FReflectedWorkspace;

    class FReflectedProject
    {
    public:
        
        FReflectedProject& operator =(const FReflectedProject&) = delete;
        FReflectedProject(const FReflectedProject&) = delete;

        FReflectedProject(FReflectedWorkspace* InWorkspace);
        
        eastl::string                                                       Name;
        eastl::string                                                       Path;
        FReflectedWorkspace*                                                Workspace;
        eastl::hash_map<FStringHash, eastl::unique_ptr<FReflectedHeader>>   Headers;
        eastl::vector<eastl::string>                                        IncludeDirs;
    };
}
