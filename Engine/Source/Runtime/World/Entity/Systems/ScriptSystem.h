#pragma once
#include "EntitySystem.h"
#include "ScriptSystem.generated.h"

namespace Lumina
{
    REFLECT(System)
    struct SScriptSystem
    {
        GENERATED_BODY()
        ENTITY_SYSTEM(RequiresUpdate(EUpdateStage::PrePhysics))

        static void Update(const FSystemContext& Context) noexcept;
    };
}
