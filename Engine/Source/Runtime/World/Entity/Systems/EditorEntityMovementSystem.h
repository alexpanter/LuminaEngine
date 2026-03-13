#pragma once
#include "EntitySystem.h"
#include "EditorEntityMovementSystem.generated.h"

namespace Lumina
{
    REFLECT(System)
    struct SEditorEntityMovementSystem
    {
        GENERATED_BODY()
        ENTITY_SYSTEM(RequiresUpdate(EUpdateStage::PrePhysics), RequiresUpdate(EUpdateStage::Paused))

        static void Update(const FSystemContext& SystemContext) noexcept;
    };
}
