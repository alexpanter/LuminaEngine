#pragma once
#include "Physics/Physics.h"
#include <Jolt/Jolt.h>
#include <Jolt/Physics/PhysicsSystem.h>
#if JPH_DEBUG_RENDERER
#include <Jolt/Renderer/DebugRendererSimple.h>
#endif
#include "Containers/String.h"
#include "Jolt/Core/JobSystemThreadPool.h"

namespace Lumina::Physics
{
    #if JPH_DEBUG_RENDERER
    class FJoltDebugRenderer : public JPH::DebugRendererSimple
    {
    public:
        
        void DrawLine(JPH::RVec3Arg inFrom, JPH::RVec3Arg inTo, JPH::ColorArg inColor) override;
        void DrawText3D(JPH::RVec3Arg inPosition, const std::string_view& inString, JPH::ColorArg inColor, float inHeight) override {}

        void DrawBodies(JPH::PhysicsSystem* System, CWorld* InWorld);

        FORCEINLINE void SetWorld(CWorld* InWorld) { World = InWorld; }
        FORCEINLINE void SetDrawDuration(float InDuration) { Duration = InDuration; }
        
    private:
        
        double Duration = 0.0f;

        CWorld* World = nullptr;
    };
    #endif
    
    struct FJoltData
    {
        TUniquePtr<JPH::JobSystemThreadPool> JobThreadPool;
        #if JPH_DEBUG_RENDERER
        TUniquePtr<FJoltDebugRenderer> DebugRenderer;
        #endif
        FString LastErrorMessage;
    };

    
    class FJoltPhysicsContext : public IPhysicsContext
    {
    public:

        void Initialize() override;
        void Shutdown() override;
        TUniquePtr<IPhysicsScene> CreatePhysicsScene(CWorld* World) override;

        static JPH::JobSystemThreadPool* GetThreadPool();
        #if JPH_DEBUG_RENDERER
		static FJoltDebugRenderer* GetDebugRenderer();
        #endif
    };
}
