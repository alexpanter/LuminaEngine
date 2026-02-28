#include "pch.h"
#include "EntitySystem.h"
#include "World/Entity/EntityUtils.h"

namespace Lumina
{
    using namespace entt::literals;

    const FUpdatePriorityList& FEntitySystemWrapper::GetUpdatePriorityList() const
    {
        return Underlying.data("PriorityList"_hs).get(Instance).cast<const FUpdatePriorityList&>();
    }

    void FEntitySystemWrapper::Startup(const FSystemContext& SystemContext) const noexcept
    {
        ECS::Utils::InvokeMetaFunc(Underlying, "Startup"_hs, entt::forward_as_meta(SystemContext));
    }

    void FEntitySystemWrapper::Update(const FSystemContext& SystemContext) const noexcept
    {
        ECS::Utils::InvokeMetaFunc(Underlying, "Update"_hs, entt::forward_as_meta(SystemContext));
    }

    void FEntitySystemWrapper::Teardown(const FSystemContext& SystemContext) const noexcept
    {
        ECS::Utils::InvokeMetaFunc(Underlying, "Teardown"_hs, entt::forward_as_meta(SystemContext));
    }

    uint64 FEntitySystemWrapper::GetHash() const noexcept
    {
        return Underlying.id();
    }

    uint64 FEntityScriptSystem::GetHash() const noexcept
    {
        if (WeakScript.expired())
        {
            return 0;
        }
        
        return (uint64)WeakScript.lock().get();
    }

    FUpdatePriorityList FEntityScriptSystem::GetUpdatePriorityList() const
    {
        if (const TSharedPtr<Lua::FScript>& Script = WeakScript.lock())
        {
            FUpdatePriorityList PriorityList;
            return PriorityList;
        }
        
        return {};
    }

    void FEntityScriptSystem::Startup(const FSystemContext& SystemContext) const noexcept
    {
        if (const TSharedPtr<Lua::FScript>& Script = WeakScript.lock())
        {
        }
    }

    void FEntityScriptSystem::Update(const FSystemContext& SystemContext) const noexcept
    {
        LUMINA_PROFILE_SCOPE();
        
        if (const TSharedPtr<Lua::FScript>& Script = WeakScript.lock())
        {
        }
    }

    void FEntityScriptSystem::Teardown(const FSystemContext& SystemContext) const noexcept
    {
        if (const TSharedPtr<Lua::FScript>& Script = WeakScript.lock())
        {
        }
    }
}
