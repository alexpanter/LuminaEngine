#include "pch.h"
#include "WorldManager.h"
#include "Core/Profiler/Profile.h"


namespace Lumina
{
    RUNTIME_API FWorldManager* GWorldManager = nullptr;

    FWorldManager::~FWorldManager()
    {
        for (FManagedWorld& World : Worlds)
        {
            World.World->TeardownWorld();
        }
    }

    void FWorldManager::UpdateWorlds(const FUpdateContext& UpdateContext) 
    { 
        LUMINA_PROFILE_SCOPE(); 
        
        for (FManagedWorld& World : Worlds) 
        { 
            if (World.World->IsSuspended()) 
            { 
                continue;
            }
            
            World.World->Update(UpdateContext);
        } 
    }

    void FWorldManager::RenderWorlds(FRenderGraph& RenderGraph)
    {
        LUMINA_PROFILE_SCOPE();
        
        for (FManagedWorld& World : Worlds)
        {
            if (World.World->IsSuspended())
            {
                continue;
            }
            
            World.World->Render(RenderGraph);
        }
    }

    void FWorldManager::RemoveWorld(CWorld* World)
    {
        if (Worlds.empty())
        {
            return;
        }

        size_t idx  = World->WorldIndex;
        size_t last = Worlds.size() - 1;

        if (idx != last)
        {
            eastl::swap(Worlds[idx], Worlds[last]);
            Worlds[idx].World->WorldIndex = idx;
        }

        Worlds.pop_back();
    }

    
    void FWorldManager::AddWorld(CWorld* World)
    {
        FManagedWorld MWorld;
        MWorld.World = World;
        World->WorldIndex = Worlds.size(); 
        Worlds.push_back(MWorld);
    }
}
