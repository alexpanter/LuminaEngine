#include "CoreTypeCustomization.h"
#include "imgui.h"
#include "Core/Reflection/Type/Properties/EnumProperty.h"

namespace Lumina
{
    EPropertyChangeOp FEnumPropertyCustomization::DrawProperty(TSharedPtr<FPropertyHandle> Property)
    {
        FEnumProperty* EnumProperty = static_cast<FEnumProperty*>(Property->Property);
        bool bWasChanged = false;
        bool bIsBitmask = EnumProperty->GetEnum()->IsBitmaskEnum();
    
        ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x);
    
        if (bIsBitmask)
        {
            int64 EnumCount = (int64)EnumProperty->GetEnum()->Names.size();
            FFixedString PreviewString = EnumProperty->GetEnum()->GetValueOrBitFieldAsString(CachedValue);
    
            if (PreviewString.empty())
            {
                PreviewString = "None";
            }

            if (ImGui::BeginCombo("##", PreviewString.c_str(), ImGuiComboFlags_HeightLarge))
            {
                bool bNoneSelected = (CachedValue == 0);
                if (ImGui::Selectable("None", bNoneSelected))
                {
                    CachedValue = 0;
                    bWasChanged = true;
                }
                if (bNoneSelected)
                {
                    ImGui::SetItemDefaultFocus();
                }

                ImGui::Separator();
    
                for (int64 i = 0; i < EnumCount; ++i)
                {
                    int64 BitValue = static_cast<int64>(EnumProperty->GetEnum()->GetValueAtIndex(i));
                    if (BitValue == 0)
                    {
                        continue;
                    }

                    const char* Label = EnumProperty->GetEnum()->GetNameAtIndex(i).c_str();
                    bool bIsSet = (CachedValue & BitValue) != 0;
    
                    if (ImGui::Checkbox(Label, &bIsSet))
                    {
                        if (bIsSet)
                        {
                            CachedValue |= BitValue;
                        }
                        else
                        {
                            CachedValue &= ~BitValue;
                        }

                        bWasChanged = true;
                    }
                }
                ImGui::EndCombo();
            }
            
            
        }
        else
        {
            int64 EnumCount = (int64)EnumProperty->GetEnum()->Names.size();
            const char* PreviewValue = EnumProperty->GetEnum()->GetNameAtValue(CachedValue).c_str();
    
            if (ImGui::BeginCombo("##", PreviewValue))
            {
                for (int64 i = 0; i < EnumCount; ++i)
                {
                    const char* Label = EnumProperty->GetEnum()->GetNameAtIndex(i).c_str();
                    bool bIsSelected = (i == CachedValue);
    
                    if (ImGui::Selectable(Label, bIsSelected))
                    {
                        CachedValue = i;
                        bWasChanged = true;
                    }
    
                    if (bIsSelected)
                    {
                        ImGui::SetItemDefaultFocus();
                    }
                }
    
                ImGui::EndCombo();
            }
        }
    
        ImGui::PopItemWidth();
    
        return bWasChanged ? EPropertyChangeOp::Updated : EPropertyChangeOp::None;
    }

    void FEnumPropertyCustomization::UpdatePropertyValue(TSharedPtr<FPropertyHandle> Property)
    {
        FEnumProperty* EnumProperty = static_cast<FEnumProperty*>(Property->Property);
        EnumProperty->GetInnerProperty()->SetIntPropertyValue(Property->Property->GetValuePtr<void>(Property->ContainerPtr), CachedValue);
    }

    void FEnumPropertyCustomization::HandleExternalUpdate(TSharedPtr<FPropertyHandle> Property)
    {
        FEnumProperty* EnumProperty = static_cast<FEnumProperty*>(Property->Property);
        CachedValue = EnumProperty->GetInnerProperty()->GetSignedIntPropertyValue(Property->Property->GetValuePtr<void>(Property->ContainerPtr));
    }
}
