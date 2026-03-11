#pragma once

#include "ImGuiDrawUtils.h"
#include "Core/Reflection/PropertyCustomization/PropertyCustomization.h"
#include "Renderer/CustomPrimitiveData.h"


namespace Lumina
{
    class FCustomPrimDataPropertyCustomization : public IPropertyTypeCustomization
    {
    public:

        static TSharedPtr<FCustomPrimDataPropertyCustomization> MakeInstance();
        
        EPropertyChangeOp DrawProperty(TSharedPtr<FPropertyHandle> Property) override;
        
        void UpdatePropertyValue(TSharedPtr<FPropertyHandle> Property) override;

        void HandleExternalUpdate(TSharedPtr<FPropertyHandle> Property) override;

        SCustomPrimitiveData Value;
        ImGuiTextFilter SearchFilter;
    };
}
