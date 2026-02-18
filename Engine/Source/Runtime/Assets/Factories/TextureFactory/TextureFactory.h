#pragma once
#include "Assets/Factories/Factory.h"
#include "Containers/String.h"
#include "Assets/AssetTypes/Textures/Texture.h"
#include "TextureFactory.generated.h"


namespace Lumina
{
    REFLECT()
    class CTextureFactory : public CFactory
    {
        GENERATED_BODY()
    public:

        CObject* CreateNew(const FName& Name, CPackage* Package) override;
        CClass* GetAssetClass() const override { return CTexture::StaticClass(); }
        FString GetAssetName() const override { return "Texture"; }
        FStringView GetDefaultAssetCreationName() override { return "NewTexture"; }

        bool IsExtensionSupported(FStringView Ext) override { return Ext == ".png" || Ext == ".jpg" || Ext == ".jpeg"; }
        bool CanImport() override { return true; }
        
        void TryImport(const FFixedString& RawPath, const FFixedString& DestinationPath, const Import::FImportSettings* Settings) override;

    private:

    };
}
