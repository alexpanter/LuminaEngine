#include "pch.h"
#include "Scripting.h"

#include "luacode.h"
#include "lualib.h"
#include "StateView.h"
#include "FileSystem/FileSystem.h"
#include "Luau/include/lua.h"
#include "Memory/SmartPtr.h"
#include "Paths/Paths.h"
#include "World/Entity/Systems/SystemContext.h"

namespace Lumina::Lua
{
    static void* ScriptingMemoryAllocationFn(void* Caller, void* Memory, size_t OldSize, size_t NewSize)
    {
        (void)Caller;
        (void)OldSize;
        
        if (NewSize == 0)
        {
            Memory::Free(Memory);
            return nullptr;
        }
        
        return Memory::Realloc(Memory, NewSize);
    }
    
    static int16 AtomString(lua_State* L, const char* Str, size_t Length)
    {
        return static_cast<int16>(Hash::FNV1a::GetHash32(Str)); 
    }

    static int LuminaLuaPrint(lua_State* L)
    {
        const int32 Count = lua_gettop(L);
        FFixedString Output;
        
        for (int32 Index = 1; Index <= Count; ++Index)
        {
            size_t Length = 0;
            FStringView String = luaL_tolstring(L, Index, &Length);
            
            if (Index > 1)
            {
                Output.append(" ");
            }
            
            Output.append_convert(String.begin(), String.length());
            
            lua_pop(L, 1);
        }
        
        LOG_INFO("[Lua] - {}", Output);
        
        return 0;
    }
    
    static TUniquePtr<FScriptingContext> GScriptingContext;
    
    void Initialize()
    {
        GScriptingContext = MakeUnique<FScriptingContext>();
        GScriptingContext->Initialize();
    }

    void Shutdown()
    {
        GScriptingContext->Shutdown();
        GScriptingContext.reset();
    }

    FScriptingContext& FScriptingContext::Get()
    {
        return *GScriptingContext.get();
    }
    
    void FScriptingContext::Initialize()
    {
        L = lua_newstate(ScriptingMemoryAllocationFn, this);
        
        lua_Callbacks* Callbacks = lua_callbacks(L);
        Callbacks->useratom = AtomString;

        luaL_openlibs(L);
        
        lua_pushcfunction(L, LuminaLuaPrint, "LuminaLuaPrint");
        lua_setglobal(L, "print"); 
        //luaL_sandbox(LuaState);
    }

    void FScriptingContext::Shutdown()
    {
        lua_close(L);
        L = nullptr;
    }

    void FScriptingContext::DoThing()
    {
        const char* source = R"(
        local Test = SHealthComponent:new()
        for i = 1, 100 do
            local X: number = Test.Health
            print(X)
        end
        )";

        size_t bytecodeSize = 0;
        char* bytecode = luau_compile(source, strlen(source), nullptr, &bytecodeSize);

        if (!bytecode)
        {
            return;
        }

        lua_State* T = lua_newthread(L);
        luaL_sandboxthread(T);

        int loadResult = luau_load(T, "test", bytecode, bytecodeSize, 0);
        free(bytecode);

        if (loadResult != LUA_OK)
        {
            lua_pop(T, 1);
            return;
        }

        auto start = std::chrono::steady_clock::now();
        int callResult = lua_pcall(T, 0, 0, 0);
        auto End = std::chrono::steady_clock::now();
        auto Duration = std::chrono::duration_cast<std::chrono::milliseconds>(End - start);
        LOG_INFO("Duration {}", Duration.count());

        if (callResult != LUA_OK)
        {
            LOG_ERROR("Runtime Error {}", lua_tostring(T, -1));
            lua_pop(T, 1);
        }
    }

    void FScriptingContext::ProcessDeferredActions()
    {
        FWriteScopeLock Lock(SharedMutex);
        
        DeferredActions.ProcessAllOf<FScriptDelete>([&](const FScriptDelete& Delete)
        {
            OnScriptDeleted.Broadcast(Delete.Path);
        });
        
        DeferredActions.ProcessAllOf<FScriptRename>([&](const FScriptRename& Reload)
        {
            
        });
        
        DeferredActions.ProcessAllOf<FScriptLoad>([&](const FScriptLoad& Load)
        {
            OnScriptLoaded.Broadcast(Load.Path);
            ReloadScripts(Load.Path);
        });
    }

    size_t FScriptingContext::GetScriptMemoryUsage() const
    {
        return 0;
    }

    void FScriptingContext::ScriptReloaded(FStringView ScriptPath)
    {
        FWriteScopeLock Lock(SharedMutex);
        
        DeferredActions.EnqueueAction<FScriptLoad>(ScriptPath);
    }

    void FScriptingContext::ScriptCreated(FStringView ScriptPath)
    {
        FWriteScopeLock Lock(SharedMutex);
    }

    void FScriptingContext::ScriptRenamed(FStringView NewPath, FStringView OldPath)
    {
        FWriteScopeLock Lock(SharedMutex);
        
        DeferredActions.EnqueueAction<FScriptRename>(NewPath, OldPath);
    }

    void FScriptingContext::ScriptDeleted(FStringView ScriptPath)
    {
        FWriteScopeLock Lock(SharedMutex);
        
        DeferredActions.EnqueueAction<FScriptDelete>(ScriptPath);
    }

    TSharedPtr<FLuaScript> FScriptingContext::LoadUniqueScript(FStringView Path)
    {
        FString ScriptData;
        if (!VFS::ReadFile(ScriptData, Path))
        {
            LOG_ERROR("Lua - Failed to read script file: {}", Path);
            return {};
        }
        
        if (ScriptData.empty())
        {
            LOG_WARN("Lua - Script file is empty: {}", Path);
            return {};
        }
        return {};
    }

    TVector<TSharedPtr<FLuaScript>> FScriptingContext::GetAllRegisteredScripts()
    {
        TVector<TSharedPtr<FLuaScript>> ReturnValue;

        for (auto& [Path, Vector] : RegisteredScripts)
        {
            for (TWeakPtr<FLuaScript>& WeakPtr : Vector)
            {
                if (auto StrongPtr = WeakPtr.lock())
                {
                    ReturnValue.emplace_back(Move(StrongPtr));
                }
            }
        }
        
        return ReturnValue;
    }

    void FScriptingContext::RunGC()
    {
        //State.collect_garbage();
    }

   
    void FScriptingContext::ReloadScripts(FStringView Path)
    {
        
        LOG_INFO("Reloaded Scripts: {}", Path);
    }
}
