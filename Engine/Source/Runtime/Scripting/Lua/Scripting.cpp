#include "pch.h"
#include "Scripting.h"
#include <glm/gtx/string_cast.hpp>

#include "luacode.h"
#include "Containers/Name.h"
#include "Core/Math/Color.h"
#include "Events/KeyCodes.h"
#include "FileSystem/FileSystem.h"
#include "Input/InputProcessor.h"
#include "Luau/include/lua.h"
#include "Memory/SmartPtr.h"
#include "Paths/Paths.h"
#include "Scripting/DeferredScriptRegistry.h"
#include "Scripting/ScriptTypes.h"
#include "Scripting/EnttGlue/EnttGlue.h"
#include "World/Entity/Systems/SystemContext.h"

namespace Lumina::Scripting
{
    static TUniquePtr<FScriptingContext> GScriptingContext;
    
    static int SolExceptionHandler(lua_State* L, sol::optional<const std::exception&> MaybeException, sol::string_view Desc) 
    {
        // L is the lua state, which you can wrap in a state_view if necessary
        // maybe_exception will contain exception, if it exists
        // description will either be the what() of the exception or a description saying that we hit the general-case catch(...)
        FFixedString ErrorString;
        if (MaybeException) 
        {
            const std::exception& ex = *MaybeException;
            ErrorString = ex.what();
        }
        else 
        {
            ErrorString = FFixedString(Desc.data(), Desc.length());
        }
        
        LOG_ERROR("An exception occured in a script function {0}", ErrorString.c_str());


        // you must push 1 element onto the stack to be
        // transported through as the error object in Lua
        // note that Lua -- and 99.5% of all Lua users and libraries -- expects a string
        // so we push a single string (in our case, the description of the error)
        return sol::stack::push(L, Desc);
    }
    
    static void SolPanicHandler(sol::optional<std::string> MaybeMsg) 
    {
        LOG_ERROR("Lua is in a panic state and will now abort() the application");
        if (MaybeMsg) 
        {
            LOG_ERROR("Error Message: {0}", MaybeMsg.value());
        }
        // When this function exits, Lua will exhibit default behavior and abort()
    }
    
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
        State.set_exception_handler(&SolExceptionHandler);
        State.set_panic(sol::c_call<decltype(&SolPanicHandler), &SolPanicHandler>);
        State.open_libraries(
            sol::lib::base,
            sol::lib::package,
            sol::lib::coroutine,
            sol::lib::string,
            sol::lib::math,
            sol::lib::table,
            sol::lib::io);

        FDeferredScriptRegistry::Get().ProcessRegistrations(sol::state_view(State));
        RegisterCoreTypes();
        SetupInput();
    }

    void FScriptingContext::Shutdown()
    {
        State.collect_gc();
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
        return State.memory_used();
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
        State.collect_gc();

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

        sol::environment Environment(State, sol::create, State.globals());
        
        sol::protected_function_result Result = State.safe_script(ScriptData.c_str(), Environment);
        
        if (!Result.valid())
        {
            sol::error Error = Result;
            LOG_ERROR("Lua - Failed to execute script '{}': {}", Path, Error.what());
            return {};
        }

        if (Result.get_type() == sol::type::none || Result.get_type() == sol::type::lua_nil)
        {
            LOG_ERROR("Lua - Script '{}' did not return a value", Path);
            return {};
        }

        sol::object ReturnedObject = Result;
        
        if (!ReturnedObject.is<sol::table>())
        {
            LOG_ERROR("Lua - Script '{}' must return a table, got: {}", Path, sol::type_name(ReturnedObject.lua_state(), ReturnedObject.get_type()));
            return {};
        }

        sol::table ScriptTable = ReturnedObject.as<sol::table>();
        
        if (ScriptTable.empty())
        {
            LOG_WARN("Lua - Script '{}' returned an empty table", Path);
        }
        
        auto NewScript = MakeShared<FLuaScript>();
        NewScript->Environment  = std::move(Environment);
        NewScript->ScriptTable  = std::move(ScriptTable);
        NewScript->Name         = VFS::FileName(Path, true);
        NewScript->Path         = Path;
        
        RegisteredScripts[Path].emplace_back(NewScript);
        
        LOG_INFO("Lua - Successfully loaded script: {}", Path);
        
        return NewScript;
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
        State.collect_garbage();
    }

    void FScriptingContext::RegisterCoreTypes()
    {
        State.set_function("print", [&](sol::this_state s, const sol::variadic_args& args)
        {
            sol::state_view lua(s);
            sol::protected_function LuaStringFunc = lua["tostring"];
    
            FFixedString Output;
    
            for (size_t i = 0; i < args.size(); ++i)
            {
                sol::object Obj = args[i];
                
        
                sol::protected_function_result Result = LuaStringFunc(Obj);
        
                if (Result.valid())
                {
                    if (sol::optional<const char*> str = Result)
                    {
                        Output += *str;
                    }
                    else
                    {
                        Output += "[tostring error]";
                    }
                }
                else
                {
                    sol::error err = Result;
                    Output += "[error: ";
                    Output += err.what();
                    Output += "]";
                }
        
                if (i < args.size() - 1)
                {
                    Output += "\t";
                }
            }
    
            LOG_INFO("[Lua] {}", Output);
        });        
        State.create_named_table("Logger")
            .set_function("Info",       &FScriptingContext::Lua_Info)
            .set_function("Warning",    &FScriptingContext::Lua_Warning)
            .set_function("Error",      &FScriptingContext::Lua_Error);
        
        State.set_function("LoadObject", [](const sol::object& Name)
        {
            const char* Char = Name.as<const char*>();
            CObject* Object = LoadObject<CObject>(Char);
            return Object ? Object->AsLua(Name.lua_state()) : sol::nil;
        });
        
        State.new_usertype<FString>("FString",
	        sol::constructors<sol::types<>, sol::types<const char*>>(),
		    "size", [](const FString& Self) { return Self.size(); },
		    "trim", [](FString& Self) { return Self.trim(); },
	        sol::meta_function::to_string, [](const FString &s) { return s.c_str(); },
	        sol::meta_function::length, &FString::size,
	        sol::meta_function::concatenation, [](const FString& LHS, const FString& RHS) { return LHS + RHS; },
	        sol::meta_function::index, [](const FString &s, size_t i) { return s.at(i - 1); },
	        "append", sol::overload(
			    [](FString &self, const FString &other) { self.append(other); },
	            [](FString &self, const char *suffix) { self.append(suffix); }
		    )
	    );
        
        State.new_usertype<FName>("FName",
	        sol::constructors<sol::types<>, sol::types<const char*>>(),
		    "Length",   [](const FName& Self) { return Self.Length(); },
		    "IsNone",   [](const FName& Self) { return Self.IsNone(); },
	        sol::meta_function::to_string, [](const FName &s) { return s.c_str(); },
	        sol::meta_function::length, &FName::Length,
	        sol::meta_function::index, [](const FName &s, size_t i) { return s.At(i - 1); }
	    );
        
        static entt::entity NullEntity = entt::null;
        auto EnttModule = State["entt"].get_or_create<sol::table>();
        EnttModule["null"] = NullEntity;
        
        Glue::RegisterRegistry(EnttModule);
        Glue::RegisterRuntimeView(EnttModule);
        
        State.new_usertype<FGuid>("FGuid",
            sol::constructors<FGuid()>(),
            "IsValid", &FGuid::IsValid,
            "Hash", &FGuid::Hash,
            "New", &FGuid::New,
            "Empty", &FGuid::Empty,
            sol::meta_function::equal_to, &FGuid::operator==,
            sol::meta_function::to_string, [](const FGuid& Guid) { return Guid.ToString(); }
            );

        // vec2
        State.new_usertype<glm::vec2>("vec2",
            sol::constructors<glm::vec2(), glm::vec2(float), glm::vec2(float, float)>(),
            "x", &glm::vec2::x,
            "y", &glm::vec2::y,
            
            sol::meta_function::addition, sol::overload(
                [](const glm::vec2& a, const glm::vec2& b){ return a + b; },
                [](const glm::vec2& a, float s){ return a + s; }
            ),
            sol::meta_function::subtraction, sol::overload(
                [](const glm::vec2& a, const glm::vec2& b){ return a - b; },
                [](const glm::vec2& a, float s){ return a - s; }
            ),
            sol::meta_function::multiplication, sol::overload(
                [](const glm::vec2& a, const glm::vec2& b){ return a * b; },
                [](const glm::vec2& a, float s){ return a * s; },
                [](float s, const glm::vec2& a){ return s * a; }
            ),
            sol::meta_function::division, sol::overload(
                [](const glm::vec2& a, const glm::vec2& b){ return a / b; },
                [](const glm::vec2& a, float s){ return a / s; }
            ),
            sol::meta_function::unary_minus, [](const glm::vec2& a){ return -a; },
            sol::meta_function::equal_to, [](const glm::vec2& a, const glm::vec2& b){ return a == b; },
            sol::meta_function::to_string, [](const glm::vec2& v)
            { 
                return glm::to_string(v);
            },
            
            "Length", [](const glm::vec2& v){ return glm::length(v); },
            "Normalize", [](const glm::vec2& v){ return glm::normalize(v); },
            "Dot", [](const glm::vec2& a, const glm::vec2& b){ return glm::dot(a, b); },
            "Distance", [](const glm::vec2& a, const glm::vec2& b){ return glm::distance(a, b); }
        );
        
        // vec3
        State.new_usertype<glm::vec3>("vec3",
            sol::constructors<glm::vec3(), glm::vec3(float), glm::vec3(float, float, float)>(),
            "x", &glm::vec3::x,
            "y", &glm::vec3::y,
            "z", &glm::vec3::z,
            
            sol::meta_function::addition, sol::overload(
                [](const glm::vec3& a, const glm::vec3& b){ return a + b; },
                [](const glm::vec3& a, float s){ return a + s; }
            ),
            sol::meta_function::subtraction, sol::overload(
                [](const glm::vec3& a, const glm::vec3& b){ return a - b; },
                [](const glm::vec3& a, float s){ return a - s; }
            ),
            sol::meta_function::multiplication, sol::overload(
                [](const glm::vec3& a, const glm::vec3& b){ return a * b; },
                [](const glm::vec3& a, float s){ return a * s; },
                [](float s, const glm::vec3& a){ return s * a; }
            ),
            sol::meta_function::division, sol::overload(
                [](const glm::vec3& a, const glm::vec3& b){ return a / b; },
                [](const glm::vec3& a, float s){ return a / s; }
            ),
            sol::meta_function::unary_minus, [](const glm::vec3& a){ return -a; },
            sol::meta_function::equal_to, [](const glm::vec3& a, const glm::vec3& b){ return a == b; },
            sol::meta_function::to_string, [](const glm::vec3& v)
            { 
                return glm::to_string(v);
            },
            
            "Length", [](const glm::vec3& v){ return glm::length(v); },
            "Normalize", [](const glm::vec3& v){ return glm::normalize(v); },
            "Dot", [](const glm::vec3& a, const glm::vec3& b){ return glm::dot(a, b); },
            "Cross", [](const glm::vec3& a, const glm::vec3& b){ return glm::cross(a, b); },
            "Distance", [](const glm::vec3& a, const glm::vec3& b){ return glm::distance(a, b); },
            "Reflect", [](const glm::vec3& v, const glm::vec3& n){ return glm::reflect(v, n); },
            "Lerp", [](const glm::vec3& a, const glm::vec3& b, float t){ return glm::mix(a, b, t); }
        );
        
        // vec4
        State.new_usertype<glm::vec4>("vec4",
            sol::constructors<glm::vec4(), glm::vec4(float), glm::vec4(float, float, float, float), glm::vec4(const glm::vec3&, float)>(),
            "x", &glm::vec4::x,
            "y", &glm::vec4::y,
            "z", &glm::vec4::z,
            "w", &glm::vec4::w,
            
            sol::meta_function::addition, sol::overload(
                [](const glm::vec4& a, const glm::vec4& b){ return a + b; },
                [](const glm::vec4& a, float s){ return a + s; }
            ),
            sol::meta_function::subtraction, sol::overload(
                [](const glm::vec4& a, const glm::vec4& b){ return a - b; },
                [](const glm::vec4& a, float s){ return a - s; }
            ),
            sol::meta_function::multiplication, sol::overload(
                [](const glm::vec4& a, const glm::vec4& b){ return a * b; },
                [](const glm::vec4& a, float s){ return a * s; },
                [](float s, const glm::vec4& a){ return s * a; }
            ),
            sol::meta_function::division, sol::overload(
                [](const glm::vec4& a, const glm::vec4& b){ return a / b; },
                [](const glm::vec4& a, float s){ return a / s; }
            ),
            sol::meta_function::unary_minus, [](const glm::vec4& a){ return -a; },
            sol::meta_function::equal_to, [](const glm::vec4& a, const glm::vec4& b){ return a == b; },
            sol::meta_function::to_string, [](const glm::vec4& v)
            {
                return glm::to_string(v);
            },
            
            "Length", [](const glm::vec4& v){ return glm::length(v); },
            "Normalize", [](const glm::vec4& v){ return glm::normalize(v); },
            "Dot", [](const glm::vec4& a, const glm::vec4& b){ return glm::dot(a, b); },
            "Distance", [](const glm::vec4& a, const glm::vec4& b){ return glm::distance(a, b); }
        );
        
        // quat
        State.new_usertype<glm::quat>("quat",
            sol::constructors<
                glm::quat(), 
                glm::quat(float, float, float, float),
                glm::quat(const glm::vec3&)
            >(),
            "x", &glm::quat::x,
            "y", &glm::quat::y,
            "z", &glm::quat::z,
            "w", &glm::quat::w,
            
            sol::meta_function::multiplication, sol::overload(
                [](const glm::quat& a, const glm::quat& b){ return a * b; },
                [](const glm::quat& a, const glm::vec3& v){ return a * v; },
                [](const glm::quat& a, const glm::vec4& v){ return a * v; },
                [](const glm::quat& a, float s){ return a * s; }
            ),
            sol::meta_function::equal_to, [](const glm::quat& a, const glm::quat& b){ return a == b; },
            sol::meta_function::to_string, [](const glm::quat& q)
            {
                return glm::to_string(q);
            },
            
            "Length", [](const glm::quat& q){ return glm::length(q); },
            "Normalize", [](const glm::quat& q){ return glm::normalize(q); },
            "Conjugate", [](const glm::quat& q){ return glm::conjugate(q); },
            "Inverse", [](const glm::quat& q){ return glm::inverse(q); },
            "Dot", [](const glm::quat& a, const glm::quat& b){ return glm::dot(a, b); },
            "Slerp", [](const glm::quat& a, const glm::quat& b, float t){ return glm::slerp(a, b, t); },
            "Lerp", [](const glm::quat& a, const glm::quat& b, float t){ return glm::mix(a, b, t); },
            "EulerAngles", [](const glm::quat& q){ return glm::eulerAngles(q); },
            "AngleAxis", [](float angle, const glm::vec3& axis){ return glm::angleAxis(angle, axis); },
            "FromEuler", [](const glm::vec3& euler){ return glm::quat(euler); }
        );
        
        State.new_enum("ScriptType",
            "WorldSystem",       EScriptType::WorldSystem,
            "EntitySystem",      EScriptType::EntitySystem);
        
        State.new_enum("UpdateStage",
            "FrameStart",       EUpdateStage::FrameStart,
            "PrePhysics",       EUpdateStage::PrePhysics,
            "DuringPhysics",    EUpdateStage::DuringPhysics,
            "PostPhysics",      EUpdateStage::PostPhysics,
            "FrameEnd",         EUpdateStage::FrameEnd,
            "Paused",           EUpdateStage::Paused);
        
        sol::table ColorTable = State.create_named_table("Color");
        ColorTable.set_function("RandomColor4", []() -> glm::vec4 { return FColor::MakeRandom(1); });
        ColorTable.set_function("RandomColor3", []() -> glm::vec3 { return FColor::MakeRandom(1); });
        
        ColorTable["Red"]     = glm::vec4(1, 0, 0, 1);
        ColorTable["Green"]   = glm::vec4(0, 1, 0, 1);
        ColorTable["Blue"]    = glm::vec4(0, 0, 1, 1);
        ColorTable["White"]   = glm::vec4(1, 1, 1, 1);
        ColorTable["Black"]   = glm::vec4(0, 0, 0, 1);
        ColorTable["Yellow"]  = glm::vec4(1, 1, 0, 1);
        ColorTable["Cyan"]    = glm::vec4(0, 1, 1, 1);
        ColorTable["Magenta"] = glm::vec4(1, 0, 1, 1);
        ColorTable["Orange"]  = glm::vec4(1.0f,  0.65f, 0.0f,  1);
        ColorTable["Purple"]  = glm::vec4(0.5f,  0.0f,  0.5f,  1);
        ColorTable["Pink"]    = glm::vec4(1.0f,  0.75f, 0.8f,  1);
        ColorTable["Gray"]    = glm::vec4(0.5f,  0.5f,  0.5f,  1);
        ColorTable["Brown"]   = glm::vec4(0.65f, 0.16f, 0.16f, 1);

        sol::table GLMTable = State.create_named_table("glm");
        
        // Vector operations
        GLMTable.set_function("Normalize", [](glm::vec3 Vec) { return glm::normalize(Vec); });
        GLMTable.set_function("Length", [](glm::vec3 Vec) { return glm::length(Vec); });
        GLMTable.set_function("Dot", [](glm::vec3 A, glm::vec3 B) { return glm::dot(A, B); });
        GLMTable.set_function("Cross", [](glm::vec3 A, glm::vec3 B) { return glm::cross(A, B); });
        GLMTable.set_function("Distance", [](glm::vec3 A, glm::vec3 B) { return glm::distance(A, B); });
        GLMTable.set_function("Mix", [](glm::vec3 A, glm::vec3 B, float t) { return glm::mix(A, B, t); });
        GLMTable.set_function("ClampVec3", [](glm::vec3 V, float Min, float Max) { return glm::clamp(V, Min, Max); });
        GLMTable.set_function("LerpVec3", [](glm::vec3 A, glm::vec3 B, float t) { return glm::mix(A, B, t); });
        
        // Quaternion operations
        GLMTable.set_function("QuatLookAt", [](glm::vec3 Forward, glm::vec3 Up) { return glm::quatLookAt(Forward, Up); });
        GLMTable.set_function("QuatRotate", [](glm::quat Q, float AngleRad, glm::vec3 Axis) { return glm::rotate(Q, AngleRad, Axis); });
        GLMTable.set_function("QuatFromEuler", [](glm::vec3 Euler) { return glm::quat(Euler); });
        GLMTable.set_function("QuatToEuler", [](glm::quat Q) { return glm::eulerAngles(Q); });
        GLMTable.set_function("QuatMul", [](glm::quat A, glm::quat B) { return A * B; });
        GLMTable.set_function("QuatInverse", [](glm::quat Q) { return glm::inverse(Q); });
        
        // Matrix operations
        GLMTable.set_function("LookAt", [](glm::vec3 Eye, glm::vec3 Center, glm::vec3 Up) { return glm::lookAt(Eye, Center, Up); });
        GLMTable.set_function("QuatLookAt", [](const glm::vec3& Direction, const glm::vec3& Up) { return glm::quatLookAt(Direction, Up); });
        GLMTable.set_function("Perspective", [](float FOV, float Aspect, float Near, float Far) { return glm::perspective(FOV, Aspect, Near, Far); });
        GLMTable.set_function("Translate", [](glm::mat4 M, glm::vec3 V) { return glm::translate(M, V); });
        GLMTable.set_function("Rotate", [](glm::mat4 M, float AngleRad, glm::vec3 Axis) { return glm::rotate(M, AngleRad, Axis); });
        GLMTable.set_function("Scale", [](glm::mat4 M, glm::vec3 S) { return glm::scale(M, S); });
        
        // Math utilities
        GLMTable.set_function("Radians", [](float Deg) { return glm::radians(Deg); });
        GLMTable.set_function("Degrees", [](float Rad) { return glm::degrees(Rad); });
        GLMTable.set_function("ClampFloat", [](float Value, float Min, float Max) { return glm::clamp(Value, Min, Max); });
        GLMTable.set_function("MixFloat", [](float A, float B, float t) { return glm::mix(A, B, t); });
        GLMTable.set_function("Lerp", [](float A, float B, float t) { return glm::mix(A, B, t); });
        GLMTable.set_function("Min", [](float A, float B) { return glm::min(A, B); });
        GLMTable.set_function("Max", [](float A, float B) { return glm::max(A, B); });
        GLMTable.set_function("Sign", [](float Value) { return (Value > 0.0f) ? 1.0f : (Value < 0.0f ? -1.0f : 0.0f); });

        // Quaternion constructors
        GLMTable.set_function("QuatAngleAxis", [](float AngleRad, glm::vec3 Axis) { return glm::angleAxis(AngleRad, Axis); });
        GLMTable.set_function("QuatAxis", [](glm::quat Q) { return glm::axis(Q); });
        GLMTable.set_function("QuatAngle", [](glm::quat Q) { return glm::angle(Q); });

        // Quaternion interpolation
        GLMTable.set_function("QuatSlerp", [](glm::quat A, glm::quat B, float t) { return glm::slerp(A, B, t); });
        
        
        
        FSystemContext::RegisterWithLua(State);
        
    }

    void FScriptingContext::SetupInput()
    {
        sol::table InputTable = State.create_named_table("Input");
        InputTable.set_function("GetMouseX",            [] () { return FInputProcessor::Get().GetMouseX(); });
        InputTable.set_function("GetMouseY",            [] () { return FInputProcessor::Get().GetMouseY(); });
        InputTable.set_function("GetMouseZ",            [] () { return FInputProcessor::Get().GetMouseZ(); });
        InputTable.set_function("GetMouseDeltaX",       [] () { return FInputProcessor::Get().GetMouseDeltaX(); });
        InputTable.set_function("GetMouseDeltaY",       [] () { return FInputProcessor::Get().GetMouseDeltaY(); });
                                                        
        InputTable.set_function("SetMouseMode",         [] (EMouseMode Mode) { FInputProcessor::Get().SetMouseMode(Mode); });
                                                        
        InputTable.set_function("IsKeyDown",            [] (EKey Key) { return FInputProcessor::Get().IsKeyDown(Key); });
        InputTable.set_function("IsKeyUp",              [] (EKey Key) { return FInputProcessor::Get().IsKeyUp(Key); });
        InputTable.set_function("IsKeyRepeated",        [] (EKey Key) { return FInputProcessor::Get().IsKeyRepeated(Key); });
        
        InputTable.set_function("IsMouseButtonDown",    [] (EMouseKey Key) { return FInputProcessor::Get().IsMouseButtonDown(Key); });
        InputTable.set_function("IsMouseButtonUp",      [] (EMouseKey Key) { return FInputProcessor::Get().IsMouseButtonUp(Key); });
        
        State.new_enum("EMouseMode",
            "Hidden",   EMouseMode::Hidden,
            "Normal",   EMouseMode::Normal,
            "Captured", EMouseMode::Captured
        );
        
        State.new_enum("EMouseKey",
            "Button0",      EMouseKey::Button0,
            "Button1",      EMouseKey::Button1,
            "Button2",      EMouseKey::Button2,
            "Button3",      EMouseKey::Button3,
            "Button4",      EMouseKey::Button4,
            "Button5",      EMouseKey::Button5,
            "Button6",      EMouseKey::Button6,
            "Button7",      EMouseKey::Button7,
            "Scroll",       EMouseKey::Scroll,
    
            "ButtonLast",   EMouseKey::ButtonLast,
            "ButtonLeft",   EMouseKey::ButtonLeft,
            "ButtonRight",  EMouseKey::ButtonRight,
            "ButtonMiddle", EMouseKey::ButtonMiddle
        );
        
        State.new_enum("EKey",
            "Space",        EKey::Space,
            "Apostrophe",   EKey::Apostrophe,
            "Comma",        EKey::Comma,
            "Minus",        EKey::Minus,
            "Period",       EKey::Period,
            "Slash",        EKey::Slash,
        
            "D0",           EKey::D0,
            "D1",           EKey::D1,
            "D2",           EKey::D2,
            "D3",           EKey::D3,
            "D4",           EKey::D4,
            "D5",           EKey::D5,
            "D6",           EKey::D6,
            "D7",           EKey::D7,
            "D8",           EKey::D8,
            "D9",           EKey::D9,
        
            "Semicolon",    EKey::Semicolon,
            "Equal",        EKey::Equal,
        
            "A",            EKey::A,
            "B",            EKey::B,
            "C",            EKey::C,
            "D",            EKey::D,
            "E",            EKey::E,
            "F",            EKey::F,
            "G",            EKey::G,
            "H",            EKey::H,
            "I",            EKey::I,
            "J",            EKey::J,
            "K",            EKey::K,
            "L",            EKey::L,
            "M",            EKey::M,
            "N",            EKey::N,
            "O",            EKey::O,
            "P",            EKey::P,
            "Q",            EKey::Q,
            "R",            EKey::R,
            "S",            EKey::S,
            "T",            EKey::T,
            "U",            EKey::U,
            "V",            EKey::V,
            "W",            EKey::W,
            "X",            EKey::X,
            "Y",            EKey::Y,
            "Z",            EKey::Z,
        
            "LeftBracket",  EKey::LeftBracket,
            "Backslash",    EKey::Backslash,
            "RightBracket", EKey::RightBracket,
            "GraveAccent",  EKey::GraveAccent,
        
            "World1",       EKey::World1,
            "World2",       EKey::World2,
        
            "Escape",       EKey::Escape,
            "Enter",        EKey::Enter,
            "Tab",          EKey::Tab,
            "Backspace",    EKey::Backspace,
            "Insert",       EKey::Insert,
            "Delete",       EKey::Delete,
            "Right",        EKey::Right,
            "Left",         EKey::Left,
            "Down",         EKey::Down,
            "Up",           EKey::Up,
            "PageUp",       EKey::PageUp,
            "PageDown",     EKey::PageDown,
            "Home",         EKey::Home,
            "End",          EKey::End,
            "CapsLock",     EKey::CapsLock,
            "ScrollLock",   EKey::ScrollLock,
            "NumLock",      EKey::NumLock,
            "PrintScreen",  EKey::PrintScreen,
            "Pause",        EKey::Pause,
        
            "F1",           EKey::F1,
            "F2",           EKey::F2,
            "F3",           EKey::F3,
            "F4",           EKey::F4,
            "F5",           EKey::F5,
            "F6",           EKey::F6,
            "F7",           EKey::F7,
            "F8",           EKey::F8,
            "F9",           EKey::F9,
            "F10",          EKey::F10,
            "F11",          EKey::F11,
            "F12",          EKey::F12,
            "F13",          EKey::F13,
            "F14",          EKey::F14,
            "F15",          EKey::F15,
            "F16",          EKey::F16,
            "F17",          EKey::F17,
            "F18",          EKey::F18,
            "F19",          EKey::F19,
            "F20",          EKey::F20,
            "F21",          EKey::F21,
            "F22",          EKey::F22,
            "F23",          EKey::F23,
            "F24",          EKey::F24,
            "F25",          EKey::F25,
        
            "KP0",          EKey::KP0,
            "KP1",          EKey::KP1,
            "KP2",          EKey::KP2,
            "KP3",          EKey::KP3,
            "KP4",          EKey::KP4,
            "KP5",          EKey::KP5,
            "KP6",          EKey::KP6,
            "KP7",          EKey::KP7,
            "KP8",          EKey::KP8,
            "KP9",          EKey::KP9,
            "KPDecimal",    EKey::KPDecimal,
            "KPDivide",     EKey::KPDivide,
            "KPMultiply",   EKey::KPMultiply,
            "KPSubtract",   EKey::KPSubtract,
            "KPAdd",        EKey::KPAdd,
            "KPEnter",      EKey::KPEnter,
            "KPEqual",      EKey::KPEqual,
        
            "LeftShift",    EKey::LeftShift,
            "LeftControl",  EKey::LeftControl,
            "LeftAlt",      EKey::LeftAlt,
            "LeftSuper",    EKey::LeftSuper,
            "RightShift",   EKey::RightShift,
            "RightControl", EKey::RightControl,
            "RightAlt",     EKey::RightAlt,
            "RightSuper",   EKey::RightSuper,
            "Menu",         EKey::Menu
        );
    }

    void FScriptingContext::ReloadScripts(FStringView Path)
    {
        TVector<TWeakPtr<FLuaScript>>& ScriptVector = RegisteredScripts[Path];
        for (const TWeakPtr<FLuaScript>& WeakScript : ScriptVector)
        {
            if (TSharedPtr<FLuaScript> Script = WeakScript.lock())
            {
                State.collect_gc();

                FString ScriptData;
                if (!VFS::ReadFile(ScriptData, Path))
                {
                    LOG_ERROR("Lua - Failed to read script file: {}", Path);
                    return;
                }
        
                if (ScriptData.empty())
                {
                    LOG_WARN("Lua - Script file is empty: {}", Path);
                    return;
                }

                sol::environment Environment(State, sol::create, State.globals());
        
                sol::protected_function_result Result = State.safe_script(ScriptData.c_str(), Environment);
        
                if (!Result.valid())
                {
                    sol::error Error = Result;
                    LOG_ERROR("Lua - Failed to execute script '{}': {}", Path, Error.what());
                    return;
                }

                if (Result.get_type() == sol::type::none || Result.get_type() == sol::type::lua_nil)
                {
                    LOG_ERROR("Lua - Script '{}' did not return a value", Path);
                    return;
                }

                sol::object ReturnedObject = Result;
        
                if (!ReturnedObject.is<sol::table>())
                {
                    LOG_ERROR("Lua - Script '{}' must return a table, got: {}", Path, sol::type_name(ReturnedObject.lua_state(), ReturnedObject.get_type()));
                    return;
                }

                sol::table ScriptTable = ReturnedObject.as<sol::table>();
        
                if (ScriptTable.empty())
                {
                    LOG_WARN("Lua - Script '{}' returned an empty table", Path);
                    return;
                }
        
                Script->Environment  = std::move(Environment);
                Script->ScriptTable  = std::move(ScriptTable);
                Script->Name         = VFS::FileName(Path, true);
                Script->Path         = Path;
            }
        }
        
        LOG_INFO("Reloaded Scripts: {}", Path);
    }

    void FScriptingContext::Lua_Info(const sol::variadic_args& Args)
    {
        sol::protected_function ToString = State[sol::meta_function::to_string];
        FFixedString Output;
        for (size_t i = 0; i < Args.size(); ++i)
        {
            sol::object Obj = Args[i];
                
            sol::protected_function_result Result = ToString(Obj);
                
            if (Result.valid())
            {
                if (sol::optional<const char*> str = Result)
                {
                    Output += *str;
                }
                else
                {
                    Output += "[tostring error]";
                }
            }
            else
            {
                sol::error err = Result;
                Output += "[error: ";
                Output += err.what();
                Output += "]";
            }
                
            if (i < Args.size() - 1)
            {
                Output += "\t";
            }
        }
            
        LOG_INFO("[Lua] {}", Output);
    }
    
    void FScriptingContext::Lua_Warning(const sol::variadic_args& Args)
    {
        sol::protected_function ToString = State[sol::meta_function::to_string];
        FFixedString Output;
        for (size_t i = 0; i < Args.size(); ++i)
        {
            sol::object Obj = Args[i];
                
            sol::protected_function_result Result = ToString(Obj);
                
            if (Result.valid())
            {
                if (sol::optional<const char*> str = Result)
                {
                    Output += *str;
                }
                else
                {
                    Output += "[tostring error]";
                }
            }
            else
            {
                sol::error err = Result;
                Output += "[error: ";
                Output += err.what();
                Output += "]";
            }
                
            if (i < Args.size() - 1)
            {
                Output += "\t";
            }
        }
            
        LOG_WARN("[Lua] {}", Output);
    }

    void FScriptingContext::Lua_Error(const sol::variadic_args& Args)
    {
        sol::protected_function ToString = State[sol::meta_function::to_string];
        FFixedString Output;
        for (size_t i = 0; i < Args.size(); ++i)
        {
            sol::object Obj = Args[i];
                
            sol::protected_function_result Result = ToString(Obj);
                
            if (Result.valid())
            {
                if (sol::optional<const char*> str = Result)
                {
                    Output += *str;
                }
                else
                {
                    Output += "[tostring error]";
                }
            }
            else
            {
                sol::error err = Result;
                Output += "[error: ";
                Output += err.what();
                Output += "]";
            }
                
            if (i < Args.size() - 1)
            {
                Output += "\t";
            }
        }
            
        LOG_ERROR("[Lua] {}", Output);
    }
}
