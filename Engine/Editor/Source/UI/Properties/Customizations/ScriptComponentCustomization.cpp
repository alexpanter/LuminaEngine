#include "ScriptComponentCustomization.h"
#include "imgui.h"
#include "Core/Reflection/Type/LuminaTypes.h"
#include "Platform/Process/PlatformProcess.h"
#include "Scripting/Lua/Scripting.h"
#include "UI/Tools/ContentBrowserEditorTool.h"

namespace Lumina
{
    static constexpr ImVec2 GButtonSize(42, 0);
    
    TSharedPtr<FScriptComponentPropertyCustomization> FScriptComponentPropertyCustomization::MakeInstance()
    {
        return MakeShared<FScriptComponentPropertyCustomization>();
    }

    EPropertyChangeOp FScriptComponentPropertyCustomization::DrawProperty(TSharedPtr<FPropertyHandle> Property)
    {
        bool bWasChanged = false;

        ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x);
        
        
        auto LoadScript = [&](FStringView Path)
        {
            FScriptCustomData OldCustomData = Value.CustomData;
            
            Value.CustomData = {};
            Value.ScriptPath.Path = Path;
            Value.Script = Scripting::FScriptingContext::Get().LoadUniqueScript(Path);
            if (!Value.Script || !Value.Script->ScriptTable.valid())
            {
                return false;
            }
            
            for (auto&& [K, V] : Value.Script->ScriptTable)
            {
                FName KeyName = K.as<const char*>();
                            
                if (V.is<bool>())
                {
                    bool AsBool = V.as<bool>();
                    eastl::get<0>(Value.CustomData).emplace_back(KeyName, AsBool);
                }
                else if (V.is<float>() || V.is<double>())
                {
                    float AsFloat = V.as<float>();
                    eastl::get<1>(Value.CustomData).emplace_back(KeyName, AsFloat);
                }
                else if (V.is<int>())
                {
                    int AsInt = V.as<int>();
                    eastl::get<2>(Value.CustomData).emplace_back(KeyName, AsInt);
                }
                else if (V.is<const char*>())
                {
                    const char* AsChar = V.as<const char*>();
                    eastl::get<3>(Value.CustomData).emplace_back(KeyName, AsChar);
                }
                else if (V.is<sol::table>())
                {
                }
            }
            
            auto RestoreValues = [&]<typename T>(const TVector<TNamedScriptVar<T>>& OldVector, TVector<TNamedScriptVar<T>>& NewVector)
            {
                for (const auto& OldVar : OldVector)
                {
                    for (auto& NewVar : NewVector)
                    {
                        if (NewVar.Name == OldVar.Name)
                        {
                            sol::object MaybeValue = Value.Script->ScriptTable[NewVar.Name.c_str()];
                            
                            if (MaybeValue.valid() && MaybeValue.is<T>())
                            {
                                Value.Script->ScriptTable[NewVar.Name.c_str()] = OldVar.Value;
                                NewVar.Value = OldVar.Value;
                                break;
                            }
                        }
                    }
                }
            };
            
            RestoreValues(eastl::get<0>(OldCustomData), eastl::get<0>(Value.CustomData));
            RestoreValues(eastl::get<1>(OldCustomData), eastl::get<1>(Value.CustomData));
            RestoreValues(eastl::get<2>(OldCustomData), eastl::get<2>(Value.CustomData));
            RestoreValues(eastl::get<3>(OldCustomData), eastl::get<3>(Value.CustomData));
            
            return true;
            
        };
        

        ImGui::PushID(this);
        if (ImGui::BeginChild("OP", ImVec2(-1, 0), ImGuiChildFlags_AutoResizeY, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse))
        {
            const auto& Style = ImGui::GetStyle();
            
            ImGui::BeginGroup();

            float const ComboArrowWidth = ImGui::GetFrameHeight();
            float const TotalPathWidgetWidth = ImGui::GetContentRegionAvail().x;
            float const TextWidgetWidth = TotalPathWidgetWidth - ComboArrowWidth;

            ImGui::SetNextItemWidth(TextWidgetWidth);
            
            FFixedString PathString(Value.ScriptPath.Path.begin(), Value.ScriptPath.Path.end());
        
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f, 0.6f, 0.6f, 1.0f));
        
            ImGui::InputText("##ScriptPathText", PathString.data(), PathString.max_size(), ImGuiInputTextFlags_AutoSelectAll | ImGuiInputTextFlags_ReadOnly);
        
            ImGuiX::TextTooltip("{}", PathString);
        
            ImGui::PopStyleColor();

            const ImVec2 ComboDropDownSize = ImMax(ImVec2(200, 200), ImVec2(TextWidgetWidth, 300.0f));

            ImGui::SameLine(0, 0);
        
            bool bComboOpen = ImGui::BeginCombo("##ScriptPath", "", ImGuiComboFlags_HeightLarge | ImGuiComboFlags_PopupAlignLeft | ImGuiComboFlags_NoPreview);
            
            if (bComboOpen)
            {
                SearchFilter.Draw("##Search", ComboDropDownSize.x - 30.0f);
                if (!SearchFilter.IsActive())
                {
                    ImDrawList* DrawList = ImGui::GetWindowDrawList();
                    ImVec2 TextPos = ImGui::GetItemRectMin();
                    TextPos.x += Style.FramePadding.x + 2.0f;
                    TextPos.y += Style.FramePadding.y;
                    DrawList->AddText(TextPos, IM_COL32(100, 100, 110, 255), LE_ICON_FILE_SEARCH " Search Scripts...");
                }
                
                ImGui::SameLine();
                ImGui::Button(LE_ICON_FILTER, ImVec2(30.0f, 0.0f));
                ImGui::SetNextWindowSizeConstraints(ImVec2(200, 200), ComboDropDownSize);

                if (ImGui::BeginChild("##OptList", ComboDropDownSize, false, ImGuiChildFlags_NavFlattened))
                {
                    using namespace Scripting;
                    
                    VFS::RecursiveDirectoryIterator("/Game/Scripts", [&](const VFS::FFileInfo& FileInfo)
                    {
                        if (!FileInfo.IsLua())
                        {
                            return;
                        }
                        
                        if (!SearchFilter.PassFilter(FileInfo.VirtualPath.c_str()))
                        {
                            return;
                        }
                        
                        FFixedString SelectableLabel;
                        SelectableLabel.append(LE_ICON_LANGUAGE_LUA).append(" ").append(FileInfo.VirtualPath.c_str());
                        if (ImGui::Selectable(SelectableLabel.c_str()))
                        {
                            if (!LoadScript(FileInfo.VirtualPath))
                            {
                                ImGuiX::Notifications::NotifyError("Failed to load script! {}, Please check console for details.", FileInfo.VirtualPath);
                            }
                            
                            ImGui::CloseCurrentPopup();
                        
                            bWasChanged = true;
                        }
                    });
                }
                
                ImGui::EndChild();
                ImGui::EndCombo();
            }
            
            if (ImGui::Button(LE_ICON_OPEN_IN_NEW "##Open", GButtonSize))
            {
                VFS::PlatformOpen(Value.ScriptPath.Path);
            }
            
            ImGuiX::TextTooltip("Open the script in your native editor");
            
            ImGui::SameLine();
            
            if (ImGui::Button(LE_ICON_CONTENT_COPY "##Copy", GButtonSize))
            {
                ImGui::SetClipboardText(Value.ScriptPath.Path.c_str());
            }
            
            ImGuiX::TextTooltip("Copy the script-path to your native clipboard");

            ImGui::SameLine();
            
            if (ImGui::Button(LE_ICON_REFRESH "##Refresh", GButtonSize))
            {
                if (!LoadScript(Value.ScriptPath.Path))
                {
                    ImGuiX::Notifications::NotifyError("Failed to load script! {}, Please check console for details.", Value.ScriptPath.Path);
                }
                bWasChanged = true;
            }
            
            ImGuiX::TextTooltip("Reload the script, will attempt to keep any matching values");

            ImGui::SameLine();
        
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.6f, 0.2f, 0.2f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.7f, 0.25f, 0.25f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.5f, 0.15f, 0.15f, 1.0f));

            if (ImGui::Button(LE_ICON_CLOSE_CIRCLE "##Clear", GButtonSize))
            {
                Value = {};
                bWasChanged = true;
            }
            
            ImGuiX::TextTooltip("Clears the script from this component");
            
            ImGui::Separator();
            
            if (Value.Script.get() && Value.Script->ScriptTable.valid())
            {
                ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(8.0f, 8.0f));
                ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4.0f, 6.0f));

                if (ImGui::BeginTable("ScriptProperties", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingStretchProp))
                {
                    ImGui::TableSetupColumn("Property", ImGuiTableColumnFlags_WidthFixed, 150.0f);
                    ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);
                    ImGui::TableHeadersRow();
                    
                    auto ScriptVarVisitor = [&]<typename T>(TVector<TNamedScriptVar<T>>& Vector)
                    {
                        for (TNamedScriptVar<T>& Var : Vector)
                        {
                            const char* KeyName = Var.Name.c_str();
                            sol::object ExistingValue = Value.Script->ScriptTable[KeyName];
                    
                            if (!ExistingValue.valid())
                            {
                                continue;
                            }
                            
                            ImGui::TableNextRow();
                            ImGui::TableSetColumnIndex(0);
                            ImGui::TextUnformatted(KeyName);
                            
                            ImGui::TableSetColumnIndex(1);
                            
                            ImGui::PushID(&Var.Value);
                            if constexpr (eastl::is_same_v<T, bool>)
                            {
                                bool AsBool = Var.Value;
                                if (ImGui::Checkbox("##Bool", &AsBool))
                                {
                                    Value.Script->ScriptTable[KeyName] = AsBool;
                                    Var.Value = AsBool;
                                    bWasChanged = true;
                                }
                            }
                            else if constexpr (eastl::is_same_v<T, int>)
                            {
                                int AsInt = Var.Value;
                                if (ImGui::InputInt("##Int", &AsInt))
                                {
                                    Value.Script->ScriptTable[KeyName] = AsInt;
                                    Var.Value = AsInt;
                                    bWasChanged = true;
                                }
                            }
                            else if constexpr (eastl::is_same_v<T, float>)
                            {
                                float AsFloat = Var.Value;
                                if (ImGui::DragFloat("##Float", &AsFloat))
                                {
                                    Value.Script->ScriptTable[KeyName] = AsFloat;
                                    Var.Value = AsFloat;
                                    bWasChanged = true;
                                }
                            }
                            else if constexpr (eastl::is_same_v<T, double>)
                            {
                                float AsFloat = static_cast<float>(Var.Value);
                                if (ImGui::DragFloat("##Double", &AsFloat))
                                {
                                    Value.Script->ScriptTable[KeyName] = static_cast<double>(AsFloat);
                                    Var.Value = static_cast<double>(AsFloat);
                                    bWasChanged = true;
                                }
                            }
                            else if constexpr (eastl::is_same_v<T, FString>)
                            {
                                char buffer[256];
                                strncpy_s(buffer, Var.Value.c_str(), sizeof(buffer) - 1);
                                buffer[sizeof(buffer) - 1] = '\0';
                            
                                if (ImGui::InputText("##String", buffer, sizeof(buffer)))
                                {
                                    Value.Script->ScriptTable[KeyName] = buffer;
                                    Var.Value = FString(buffer);
                                    bWasChanged = true;
                                }
                            }
                            ImGui::PopID();
                        }
                    };
                    
                    eastl::apply([&](auto&... Vector)
                    {
                        (ScriptVarVisitor(Vector), ...);
                    }, Value.CustomData);
                    
                    ImGui::EndTable();
                }
                
                ImGui::PopStyleVar(2);
            }
            
            ImGui::PopStyleColor(3);
            
            ImGui::EndGroup();
        }
        ImGui::EndChild();

        ImGui::PopID();

        ImGui::PopItemWidth();
        
        return bWasChanged ? EPropertyChangeOp::Updated : EPropertyChangeOp::None;
    }

    void FScriptComponentPropertyCustomization::UpdatePropertyValue(TSharedPtr<FPropertyHandle> Property)
    {
        SScriptComponent* Updated = static_cast<SScriptComponent*>(Property->ContainerPtr);
        *Updated = Value;
    }

    void FScriptComponentPropertyCustomization::HandleExternalUpdate(TSharedPtr<FPropertyHandle> Property)
    {
        SScriptComponent* Updated = static_cast<SScriptComponent*>(Property->ContainerPtr);
        Value = *Updated;
    }
}
