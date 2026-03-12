#pragma once
#include "Containers/String.h"
#include "Core/Delegates/Delegate.h"
#include "Core/Object/Class.h"
#include "Core/Reflection/Type/LuminaTypes.h"
#include "Memory/SmartPtr.h"
#include "Tools/Actions/DeferredActions.h"
#include "World/Entity/Components/Component.h"


struct GCMetrics;
struct lua_State;

namespace Lumina
{
    class CStruct;
}

namespace Lumina::Lua
{
    struct FScript;
    DECLARE_MULTICAST_DELEGATE(FScriptTransactionDelegate, FStringView);
    
    
    struct FLuaScriptMetadata;
    void Initialize();
    void Shutdown();
    
    class FScriptingContext
    {
        struct FScriptLoad
        {
            FScriptLoad(FStringView Str)
                : Path(Str)
            {}
            
            FString Path;
        };
        
        struct FScriptDelete
        {
            FScriptDelete(FStringView Str)
                : Path(Str)
            {}
            
            FString Path;
        };
        
        struct FScriptRename
        {
            FScriptRename(FStringView A, FStringView B)
                : NewName(A), OldName(B)
            {}
            
            FString NewName;
            FString OldName;
        };
        
    public:

        RUNTIME_API static FScriptingContext& Get();
        
        void Initialize();
        void SandboxGlobals();
        void Shutdown();
        
        void ProcessDeferredActions();

        RUNTIME_API int GetScriptMemoryUsageBytes() const;
        RUNTIME_API void ScriptReloaded(FStringView ScriptPath);
        RUNTIME_API void ScriptCreated(FStringView ScriptPath);
        RUNTIME_API void ScriptRenamed(FStringView NewPath, FStringView OldPath);
        RUNTIME_API void ScriptDeleted(FStringView ScriptPath);
        RUNTIME_API TSharedPtr<FScript> LoadUniqueScriptPath(FStringView Path);
        RUNTIME_API TSharedPtr<FScript> LoadUniqueScript(FStringView Code, FStringView Name = "");
        RUNTIME_API TVector<TSharedPtr<FScript>> GetAllRegisteredScripts();
        RUNTIME_API void RunGC();
        
        #if LUAI_GCMETRICS
        RUNTIME_API const GCMetrics* GetGCMetrics() const;  
        #endif
        
        FScriptTransactionDelegate OnScriptLoaded;
        FScriptTransactionDelegate OnScriptDeleted;

    public:
        
        RUNTIME_API lua_State* GetVM() const { return L; }
        
    private:
        
        void ReloadScripts(FStringView Path);
    
    private:
        
        lua_State* L = nullptr;
        
        FSharedMutex SharedMutex;
        FDeferredActionRegistry DeferredActions;
        
        THashMap<FName, TVector<TWeakPtr<FScript>>> RegisteredScripts;
    };
    
}

#include "Scripting.inl"