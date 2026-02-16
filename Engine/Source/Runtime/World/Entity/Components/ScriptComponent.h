#pragma once
#include "Core/Object/ObjectMacros.h"
#include "Scripting/ScriptPath.h"
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
            if (!Script)
            {
                return;
            }
            
            sol::table ScriptTable = Script->ScriptTable;
            if (!ScriptTable.valid())
            {
                return;
            }
            
            sol::protected_function ScriptFunction = Script->ScriptTable[Name.data()];
            if (!ScriptFunction.valid())
            {
                return;
            }
            
            sol::protected_function_result Result = ScriptFunction(Script->ScriptTable, Forward<TArgs>(Args)...);
            if (!Result.valid())
            {
                sol::error Error = Result;
                LOG_ERROR("Script Error: {} - {}", Script->Path, Error.what());
            }
        }
        
        PROPERTY(Editable)
        FScriptPath ScriptPath;
        
        FScriptCustomData CustomData;
        TSharedPtr<Scripting::FLuaScript> Script;
    };
}
