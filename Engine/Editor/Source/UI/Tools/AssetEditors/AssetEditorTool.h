#pragma once

#include "Core/Object/Cast.h"
#include "Core/Object/Class.h"
#include "Core/Object/Object.h"
#include "Core/Object/ObjectHandleTyped.h"
#include "Core/Object/Package/Package.h"
#include "UI/Properties/PropertyTable.h"
#include "UI/Tools/EditorTool.h"

namespace Lumina
{
    class FAssetEditorTool : public FEditorTool
    {
    public:
        
        FAssetEditorTool(IEditorToolContext* Context, const FString& AssetName, CObject* InAsset, CWorld* InWorld = nullptr)
            : FEditorTool(Context, AssetName, InWorld)
            , PropertyTable(FPropertyTable(InAsset, InAsset->GetClass()))
            , bAssetLoadBroadcasted(false)
        {
            Asset = InAsset;
            PropertyTable.MarkDirty();
            PropertyTable.SetPostEditCallback([&](const FPropertyChangedEvent&)
            {
               Asset->GetPackage()->MarkDirty();
            });
        }
        
        
        void OnInitialize() override;
        void Deinitialize(const FUpdateContext& UpdateContext) override;
        void Update(const FUpdateContext& UpdateContext) override;
        
        FName GetToolName() const override;
        virtual void OnAssetLoadFinished() { }
        void OnSave() override;

        bool IsAssetEditorTool() const override;
        FPropertyTable* GetPropertyTable() { return &PropertyTable; }
        
    protected:

        template<Concept::IsACObject T>
        T* GetAsset()
        {
            return Cast<T>(Asset.Get());
        }
    
    protected:

        TObjectPtr<CObject>         Asset;
        FPropertyTable              PropertyTable;
        uint8                       bAssetLoadBroadcasted:1;
    };
}
