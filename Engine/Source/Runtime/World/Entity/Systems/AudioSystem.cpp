#include "pch.h"
#include "AudioSystem.h"
#include "Audio/AudioGlobals.h"
#include "World/Entity/Components/AudioSourceComponent.h"

namespace Lumina
{
    void SAudioSystem::Startup(const FSystemContext& Context) noexcept
    {
    }

    void SAudioSystem::Teardown(const FSystemContext& Context) noexcept
    {
        
    }

    void SAudioSystem::Update(const FSystemContext& SystemContext) noexcept
    {
        LUMINA_PROFILE_SCOPE();

        auto View = SystemContext.CreateView<SAudioListenerComponent, STransformComponent>();
        View.each([](SAudioListenerComponent&, const STransformComponent& TransformComponent)
        {
            GAudioContext->UpdateListenerPosition(TransformComponent.GetLocation(), TransformComponent.GetRotation());
        });
    }
}
