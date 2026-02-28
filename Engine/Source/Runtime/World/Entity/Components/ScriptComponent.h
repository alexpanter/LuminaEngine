#pragma once
#include "Core/Object/ObjectMacros.h"
#include "Scripting/Lua/ScriptTypes.h"
#include "Scripting/Lua/ScriptPath.h"
#include "Memory/SmartPtr.h"
#include "ScriptComponent.generated.h"

namespace Lumina
{
    REFLECT(Component)
    struct RUNTIME_API SScriptComponent
    {
        GENERATED_BODY()
        
        template<typename... TArgs>
        void InvokeScriptFunction(FStringView Name, TArgs&&... Args)
        {
            
        }
        
        PROPERTY(Editable)
        FScriptPath ScriptPath;
        
        TSharedPtr<Lua::FScript> Script;
    };
}
