#include "ThumbnailManager.h"
#include "Assets/AssetTypes/Mesh/StaticMesh/StaticMesh.h"
#include "Core/Object/Package/Package.h"
#include "Core/Object/Package/Thumbnail/PackageThumbnail.h"
#include "Paths/Paths.h"
#include "Renderer/RenderContext.h"
#include "Renderer/RHIGlobals.h"

#include "TaskSystem/TaskSystem.h"
#include "World/Scene/RenderScene/SceneMeshes.h"


namespace Lumina
{

    static CThumbnailManager* ThumbnailManagerSingleton = nullptr;

    CThumbnailManager::CThumbnailManager()
    {
    }

    void CThumbnailManager::Initialize()
    {
        (void)CPackage::OnPackageDestroyed.AddMember(this, &ThisClass::OnPackageDestroyed);
        
        {
            TUniquePtr<FMeshResource> Resource = MakeUnique<FMeshResource>();
            PrimitiveMeshes::GenerateCube(Resource->Vertices.emplace<TVector<FVertex>>(), Resource->Indices);
            
            FGeometrySurface Surface;
            Surface.ID = "CubeMesh";
            Surface.IndexCount = (uint32)Resource->Indices.size();
            Surface.StartIndex = 0;
            Surface.MaterialIndex = 0;
            Resource->GeometrySurfaces.push_back(Surface);

            CubeMesh = NewObject<CStaticMesh>(nullptr, "ThumbnailCubeMesh", FGuid::New(), OF_Transient);
            CubeMesh->Materials.resize(1);
            CubeMesh->SetMeshResource(Move(Resource));
        }

        {
            TUniquePtr<FMeshResource> Resource = MakeUnique<FMeshResource>();
            PrimitiveMeshes::GenerateSphere(Resource->Vertices.emplace<TVector<FVertex>>(), Resource->Indices);
            
            FGeometrySurface Surface;
            Surface.ID = "SphereMesh";
            Surface.IndexCount = (uint32)Resource->Indices.size();
            Surface.StartIndex = 0;
            Surface.MaterialIndex = 0;
            Resource->GeometrySurfaces.push_back(Surface);

            SphereMesh = NewObject<CStaticMesh>(nullptr, "ThumbnailSphereMesh", FGuid::New(), OF_Transient);
            SphereMesh->Materials.resize(1);
            SphereMesh->SetMeshResource(Move(Resource));
        }

        {
            TUniquePtr<FMeshResource> Resource = MakeUnique<FMeshResource>();
            PrimitiveMeshes::GeneratePlane(Resource->Vertices.emplace<TVector<FVertex>>(), Resource->Indices);
            
            FGeometrySurface Surface;
            Surface.ID = "PlaneMesh";
            Surface.IndexCount = (uint32)Resource->Indices.size();
            Surface.StartIndex = 0;
            Surface.MaterialIndex = 0;
            Resource->GeometrySurfaces.push_back(Surface);

            PlaneMesh = NewObject<CStaticMesh>(nullptr, "ThumbnailPlaneMesh", FGuid::New(), OF_Transient);
            PlaneMesh->Materials.resize(1);
            PlaneMesh->SetMeshResource(Move(Resource));
        }
    }

    CThumbnailManager& CThumbnailManager::Get()
    {
        static std::once_flag Flag;
        std::call_once(Flag, []()
        {
            ThumbnailManagerSingleton = NewObject<CThumbnailManager>();
            ThumbnailManagerSingleton->Initialize();
        });

        return *ThumbnailManagerSingleton;
    }

    void CThumbnailManager::AsyncLoadThumbnailsForPackage(const FName& Package)
    {
        Task::AsyncTask(1, 1, [this, Package](uint32, uint32, uint32)
        {
            CPackage* MaybePackage = CPackage::LoadPackage(Package.c_str());
            if (MaybePackage == nullptr)
            {
                return;
            }
            
            FPackageThumbnail* Thumbnail = MaybePackage->GetPackageThumbnail();
            
            FPackageThumbnail::EState Expected = FPackageThumbnail::EState::None;
            if (!Thumbnail->LoadState.compare_exchange_strong(Expected, FPackageThumbnail::EState::Loading, std::memory_order_acquire))
            {
                return;
            }
            
            if (Thumbnail->ImageData.empty())
            {
                FWriteScopeLock Lock(ThumbnailLock);
                Thumbnails.insert_or_assign(Package, Thumbnail);
                return;
            }
            
            FRHIImageDesc ImageDesc;
            ImageDesc.Dimension = EImageDimension::Texture2D;
            ImageDesc.Extent = {256, 256};
            ImageDesc.Format = EFormat::RGBA8_UNORM;
            ImageDesc.Flags.SetFlag(EImageCreateFlags::ShaderResource);
            FRHIImageRef Image = GRenderContext->CreateImage(ImageDesc);
            
            FRHICommandListRef CommandList = GRenderContext->CreateCommandList(FCommandListInfo::Transfer());
            CommandList->Open();
            
            const uint8 BytesPerPixel = RHI::Format::BytesPerBlock(ImageDesc.Format);
            const uint32 RowBytes = ImageDesc.Extent.x * BytesPerPixel;
            
            TVector<uint8> FlippedData(Thumbnail->ImageData.size());
            uint8* Destination = FlippedData.data();
            const uint8* Source = Thumbnail->ImageData.data();

            for (uint32 y = 0; y < ImageDesc.Extent.y; ++y)
            {
                const uint32 FlippedY = ImageDesc.Extent.y - 1 - y;
                Memory::Memcpy(Destination + FlippedY * RowBytes, Source + y * RowBytes, RowBytes);
            }
    
            const uint32 RowPitch = RowBytes;
            constexpr uint32 DepthPitch = 0;
            
            CommandList->BeginTrackingImageState(Image, AllSubresources, EResourceStates::Unknown);
            CommandList->WriteImage(Image, 0, 0, FlippedData.data(), RowPitch, DepthPitch);
            CommandList->SetPermanentImageState(Image, EResourceStates::ShaderResource);
            
            CommandList->Close();
            GRenderContext->ExecuteCommandList(CommandList, ECommandQueue::Transfer);
            
            Thumbnail->LoadedImage = Image;
            Thumbnail->LoadState.store(FPackageThumbnail::EState::Loaded, std::memory_order_release);

            FWriteScopeLock Lock(ThumbnailLock);
            Thumbnails.insert_or_assign(Package, Thumbnail);
        });
    }

    FPackageThumbnail* CThumbnailManager::GetThumbnailForPackage(const FName& Package)
    {
        {
            FReadScopeLock Lock(ThumbnailLock);
            auto It = Thumbnails.find(Package);
            if (It != Thumbnails.end())
            {
                FPackageThumbnail* CachedThumbnail = It->second;
                if (CachedThumbnail && CachedThumbnail->IsReadyForRender())
                {
                    return CachedThumbnail;
                }
                
                if (CachedThumbnail)
                {
                    auto LoadState = CachedThumbnail->LoadState.load(std::memory_order_acquire);
                    if (LoadState == FPackageThumbnail::EState::None) // The package thumbnail may be dirty.
                    {
                        AsyncLoadThumbnailsForPackage(Package);
                    }
                }
                
                return nullptr;
            }
        }
    
        AsyncLoadThumbnailsForPackage(Package);
        return nullptr;
    }

    void CThumbnailManager::OnPackageDestroyed(FName Package)
    {
        FWriteScopeLock Lock(ThumbnailLock);

        auto It = Thumbnails.find(Package);
        if (It != Thumbnails.end())
        {
            Thumbnails.erase(It);
        }
    }
}
