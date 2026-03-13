#pragma once

#include "ImGuiDrawUtils.h"
#include "Core/Reflection/PropertyCustomization/PropertyCustomization.h"
#include "World/Entity/Components/ScriptComponent.h"


namespace Lumina
{
    class FScriptComponentPropertyCustomization : public IPropertyTypeCustomization
    {
    public:

        static TSharedPtr<FScriptComponentPropertyCustomization> MakeInstance();
        
        EPropertyChangeOp DrawProperty(TSharedPtr<FPropertyHandle> Property) override;
        
        void UpdatePropertyValue(TSharedPtr<FPropertyHandle> Property) override;

        void HandleExternalUpdate(TSharedPtr<FPropertyHandle> Property) override;

        ImGuiTextFilter SearchFilter;
    };
}
