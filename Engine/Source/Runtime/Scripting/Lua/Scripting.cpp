#include "pch.h"
#include "Scripting.h"
#include "luacode.h"
#include "lualib.h"
#include "ScriptTypes.h"
#include "Core/Utils/TimedEvent.h"
#include "Events/KeyCodes.h"
#include "FileSystem/FileSystem.h"
#include "Input/InputProcessor.h"
#include "Luau/include/lua.h"
#include "Memory/SmartPtr.h"
#include "World/Entity/Systems/SystemContext.h"

namespace Lumina::Lua
{
    static void* ScriptingMemoryReallocFn([[maybe_unused]] void* Caller, void* Memory, [[maybe_unused]] size_t OldSize, size_t NewSize)
    {
        if (NewSize == 0)
        {
            Memory::Free(Memory);
            return nullptr;
        }
        
        return Memory::Realloc(Memory, NewSize);
    }
    
    static void LuaPanicHandler(lua_State* L, int ErrorCode)
    {
        PANIC("Lua Panic {}", ErrorCode);
    }
    
    static int16 AtomString(lua_State* L, const char* Str, size_t Length)
    {
        return static_cast<int16>(Hash::FNV1a::GetHash16(Str)); 
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
        L = lua_newstate(ScriptingMemoryReallocFn, this);
        
        lua_Callbacks* Callbacks    = lua_callbacks(L);
        Callbacks->useratom         = AtomString;
        Callbacks->panic            = LuaPanicHandler;

        luaL_openlibs(L);
        
        lua_pushcfunction(L, LuminaLuaPrint, "LuminaLuaPrint");
        lua_setglobal(L, "print");
        
        lua_pushvalue(L, LUA_GLOBALSINDEX);
        FRef GlobalsRef(L, -1);
        
        FRef InputTable = GlobalsRef.NewTable("Input");
        InputTable.Set("IsKeyDown", +[](EKey Key){ return FInputProcessor::Get().IsKeyDown(Key); });
        InputTable.Set("IsKeyUp", +[](EKey Key){ return FInputProcessor::Get().IsKeyUp(Key); });
        InputTable.Set("IsKeyPressed", +[](EKey Key){ return FInputProcessor::Get().IsKeyPressed(Key); });
        InputTable.Set("IsKeyRepeated", +[](EKey Key){ return FInputProcessor::Get().IsKeyRepeated(Key); });
        
        InputTable.Set("GetMouseDeltaX", +[](){ return FInputProcessor::Get().GetMouseDeltaX(); });
        InputTable.Set("GetMouseDeltaY", +[](){ return FInputProcessor::Get().GetMouseDeltaY(); });
        InputTable.Set("GetMouseX", +[](){ return FInputProcessor::Get().GetMouseX(); });
        InputTable.Set("GetMouseY", +[](){ return FInputProcessor::Get().GetMouseY(); });
        InputTable.Set("GetMouseZ", +[](){ return FInputProcessor::Get().GetMouseZ(); });

    }

    void FScriptingContext::SandboxGlobals()
    {
        luaL_sandbox(L);
    }

    void FScriptingContext::Shutdown()
    {
        lua_close(L);
        L = nullptr;
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
    
    int FScriptingContext::GetScriptMemoryUsageBytes() const
    {
        int KB        = lua_gc(L, LUA_GCCOUNT, 0);
        int Remainder = lua_gc(L, LUA_GCCOUNTB, 0);
        return (KB * 1024) + Remainder;
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

    TSharedPtr<FScript> FScriptingContext::LoadUniqueScriptPath(FStringView Path)
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
        
        FStringView FileName = VFS::FileName(Path);
        return LoadUniqueScript(ScriptData, FileName);
    }

    TSharedPtr<FScript> FScriptingContext::LoadUniqueScript(FStringView Code, FStringView Name)
    {
        LUMINA_PROFILE_SCOPE();
        
        size_t BytecodeSize = 0;
        char* Bytecode = luau_compile(Code.data(), Code.length(), nullptr, &BytecodeSize);

        if (!Bytecode)
        {
            return {};
        }

        int StackBefore = lua_gettop(L);
        lua_State* Thread = lua_newthread(L);
        FRef ThreadRef(L, -1);
        luaL_sandboxthread(Thread);
        lua_pop(L, 1); // We have to pop off the main lua state.
        
        // Push a copy since FRef pops internally.
        lua_pushvalue(Thread, LUA_GLOBALSINDEX);
        lua_pushvalue(Thread, -1);
        FRef Environment(Thread, -1);
        lua_pop(Thread, 1); // Pops the copy.
        
        int LoadResult = luau_load(Thread, Name.data(), Bytecode, BytecodeSize, 0);
        free(Bytecode);

        if (LoadResult != LUA_OK)
        {
            LOG_ERROR("Lua - Failed to load: {}", lua_tostring(Thread, -1));
            lua_pop(Thread, 1);
            return {};
        }

        int CallResult = lua_pcall(Thread, 0, 1, 0);
        if (CallResult != LUA_OK)
        {
            LOG_ERROR("Runtime Error {}", lua_tostring(Thread, -1));
            lua_pop(Thread, 1);
            return {};
        }

        auto NewScript = MakeShared<FScript>();
        NewScript->Name         = Name;
        NewScript->Path         = "";
        NewScript->Reference    = FRef(Thread, -1);
        NewScript->Environment  = Environment;
        NewScript->Thread       = ThreadRef;
        lua_pop(Thread, 1);
        
        
        
        int StackAfter = lua_gettop(L);

        DEBUG_ASSERT(StackAfter == StackBefore);
        
        return NewScript;
    }

    TVector<TSharedPtr<FScript>> FScriptingContext::GetAllRegisteredScripts()
    {
        TVector<TSharedPtr<FScript>> ReturnValue;

        for (auto& [Path, Vector] : RegisteredScripts)
        {
            for (TWeakPtr<FScript>& WeakPtr : Vector)
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
        
    }

   
    void FScriptingContext::ReloadScripts(FStringView Path)
    {
        
        LOG_INFO("Reloaded Scripts: {}", Path);
    }
}
