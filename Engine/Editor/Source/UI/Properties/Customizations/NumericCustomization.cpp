#include "CoreTypeCustomization.h"
#include "imgui.h"
#include "Core/Object/ObjectCore.h"
#include "Core/Reflection/Type/LuminaTypes.h"
#include <limits>
#include <glm/gtc/type_ptr.hpp>

#include "Core/Math/Transform.h"
#include "Core/Reflection/Type/Properties/StructProperty.h"
#include "Tools/UI/ImGui/ImGuiDesignIcons.h"
#include "Tools/UI/ImGui/ImGuiX.h"

namespace Lumina
{

    EPropertyChangeOp FVec2PropertyCustomization::DrawProperty(TSharedPtr<FPropertyHandle> Property)
    {
        ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x);
        FStructProperty* Prop = static_cast<FStructProperty*>(Property->Property);

        TOptional<float> MinOpt;
        TOptional<float> MaxOpt;

        if (Prop->HasMetadata("ClampMin"))
        {
            MinOpt = std::stof(Prop->GetMetadata("ClampMin").c_str());
        }
        if (Prop->HasMetadata("ClampMax"))
        {
            MaxOpt = std::stof(Prop->GetMetadata("ClampMax").c_str());
        }

        float Min = MinOpt ? MinOpt.value() : 0.0f;
        float Max = MaxOpt ? MaxOpt.value() : 0.0f;
        
        ImGui::DragFloat2("##", glm::value_ptr(DisplayValue), 0.01f, Min, Max);

        ImGui::PopItemWidth();
        
        return ImGui::IsItemEdited() ? EPropertyChangeOp::Updated : EPropertyChangeOp::None;
    }

    void FVec2PropertyCustomization::UpdatePropertyValue(TSharedPtr<FPropertyHandle> Property)
    {
        CachedValue = DisplayValue;
        Property->Property->SetValue(Property->ContainerPtr, CachedValue, Property->Index);
    }

    void FVec2PropertyCustomization::HandleExternalUpdate(TSharedPtr<FPropertyHandle> Property)
    {
        glm::vec2 ActualValue;
        Property->Property->GetValue(Property->ContainerPtr, &ActualValue, Property->Index);
        
        if (CachedValue != ActualValue)
        {
            CachedValue = DisplayValue = ActualValue;
        }
    }

    EPropertyChangeOp FVec3PropertyCustomization::DrawProperty(TSharedPtr<FPropertyHandle> Property)
    {
        FStructProperty* Prop = static_cast<FStructProperty*>(Property->Property);

        ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x);

        if (Prop->Metadata.HasMetadata("Color"))
        {
            ImGui::ColorEdit3("##", glm::value_ptr(DisplayValue));
        }
        else
        {
            TOptional<float> MinOpt;
            TOptional<float> MaxOpt;

            if (Prop->HasMetadata("ClampMin"))
            {
                MinOpt = std::stof(Prop->GetMetadata("ClampMin").c_str());
            }
            if (Prop->HasMetadata("ClampMax"))
            {
                MaxOpt = std::stof(Prop->GetMetadata("ClampMax").c_str());
            }

            float Min = MinOpt ? MinOpt.value() : 0.0f;
            float Max = MaxOpt ? MaxOpt.value() : 0.0f;
        
            
            ImGui::DragFloat3("##", glm::value_ptr(DisplayValue), 0.01f, Min, Max);
        }
        
        ImGui::PopItemWidth();

        return ImGui::IsItemEdited() ? EPropertyChangeOp::Updated : EPropertyChangeOp::None;
    }
    
    void FVec3PropertyCustomization::UpdatePropertyValue(TSharedPtr<FPropertyHandle> Property)
    {
        CachedValue = DisplayValue;
        Property->Property->SetValue(Property->ContainerPtr, CachedValue, Property->Index);
    }

    void FVec3PropertyCustomization::HandleExternalUpdate(TSharedPtr<FPropertyHandle> Property)
    {
        glm::vec3 ActualValue;
        Property->Property->GetValue(Property->ContainerPtr, &ActualValue, Property->Index);
        
        if (CachedValue != ActualValue)
        {
            CachedValue = DisplayValue = ActualValue;
        }
    }

    EPropertyChangeOp FVec4PropertyCustomization::DrawProperty(TSharedPtr<FPropertyHandle> Property)
    {
        FStructProperty* Prop = static_cast<FStructProperty*>(Property->Property);

        ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x);

        if (Prop->Metadata.HasMetadata("Color"))
        {
            ImGui::ColorEdit4("##", glm::value_ptr(DisplayValue));
        }
        else
        {
            TOptional<float> MinOpt;
            TOptional<float> MaxOpt;

            if (Prop->HasMetadata("ClampMin"))
            {
                MinOpt = std::stof(Prop->GetMetadata("ClampMin").c_str());
            }
            if (Prop->HasMetadata("ClampMax"))
            {
                MaxOpt = std::stof(Prop->GetMetadata("ClampMax").c_str());
            }

            float Min = MinOpt ? MinOpt.value() : 0.0f;
            float Max = MaxOpt ? MaxOpt.value() : 0.0f;
                    
            ImGui::DragFloat4("##", glm::value_ptr(DisplayValue), 0.01f, Min, Max);
        }

        ImGui::PopItemWidth();
        
        return ImGui::IsItemEdited() ? EPropertyChangeOp::Updated : EPropertyChangeOp::None;
    }

    void FVec4PropertyCustomization::UpdatePropertyValue(TSharedPtr<FPropertyHandle> Property)
    {
        CachedValue = DisplayValue;
        Property->Property->SetValue(Property->ContainerPtr, CachedValue, Property->Index);
    }

    void FVec4PropertyCustomization::HandleExternalUpdate(TSharedPtr<FPropertyHandle> Property)
    {
        glm::vec4 ActualValue;
        Property->Property->GetValue(Property->ContainerPtr, &ActualValue, Property->Index);
        
        if (CachedValue != ActualValue)
        {
            CachedValue = DisplayValue = ActualValue;
        }
    }

    EPropertyChangeOp FQuatPropertyCustomization::DrawProperty(TSharedPtr<FPropertyHandle> Property)
    {
        ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x);

        ImGui::DragFloat4("##", glm::value_ptr(DisplayValue), 0.01f);

        ImGui::PopItemWidth();
        
        return ImGui::IsItemEdited() ? EPropertyChangeOp::Updated : EPropertyChangeOp::None;
    }

    void FQuatPropertyCustomization::UpdatePropertyValue(TSharedPtr<FPropertyHandle> Property)
    {
        CachedValue = DisplayValue;
        Property->Property->SetValue(Property->ContainerPtr, CachedValue, Property->Index);
    }

    void FQuatPropertyCustomization::HandleExternalUpdate(TSharedPtr<FPropertyHandle> Property)
    {
        glm::quat ActualValue;
        Property->Property->GetValue(Property->ContainerPtr, &ActualValue, Property->Index);
        
        if (CachedValue != ActualValue)
        {
            CachedValue = DisplayValue = ActualValue;
        }
    }

    EPropertyChangeOp FTransformPropertyCustomization::DrawProperty(TSharedPtr<FPropertyHandle> Property)
    {
        bool bWasChanged = false;
        ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x);
        
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.4f, 0.7f, 1.0f, 1.0f));
        ImGui::TextUnformatted(LE_ICON_AXIS_ARROW);
        ImGui::PopStyleColor();
        
        ImGui::SameLine();
        
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort))
        {
            ImGui::SetTooltip("Translation (Location)");
        }
                
        if (ImGui::DragFloat3("T", glm::value_ptr(DisplayValue.Location), 0.01f))
        {
            bWasChanged = true;
        }
        
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.4f, 1.0f, 0.7f, 1.0f));
        ImGui::TextUnformatted(LE_ICON_ROTATE_360);
        ImGui::PopStyleColor();
        
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort))
        {
            ImGui::SetTooltip("Rotation (Euler Angles)");
        }
        
        ImGui::SameLine();
        
        glm::vec3 EulerRotation = glm::degrees(glm::eulerAngles(DisplayValue.Rotation));
        if (ImGui::DragFloat3("R", glm::value_ptr(EulerRotation), 0.01f))
        {
            DisplayValue.SetRotationFromEuler(EulerRotation);
            bWasChanged = true;
        }
        
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.7f, 0.4f, 1.0f));
        ImGui::TextUnformatted(LE_ICON_ARROW_TOP_RIGHT_BOTTOM_LEFT);
        ImGui::PopStyleColor();
        
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort))
        {
            ImGui::SetTooltip("Scale");
        }
                
        ImGui::SameLine();
        
        if (ImGui::DragFloat3("S", glm::value_ptr(DisplayValue.Scale), 0.01f))
        {
            bWasChanged = true;
        }
        
        ImGui::PopItemWidth();

        return bWasChanged ? EPropertyChangeOp::Updated : EPropertyChangeOp::None;
    }

    void FTransformPropertyCustomization::UpdatePropertyValue(TSharedPtr<FPropertyHandle> Property)
    {
        CachedValue = DisplayValue;
        Property->Property->SetValue(Property->ContainerPtr, CachedValue, Property->Index);
    }

    void FTransformPropertyCustomization::HandleExternalUpdate(TSharedPtr<FPropertyHandle> Property)
    {
        FTransform ActualValue;
        Property->Property->GetValue(Property->ContainerPtr, &ActualValue, Property->Index);
        
        if (CachedValue != ActualValue)
        {
            CachedValue = DisplayValue = ActualValue;
        }
    }
}
