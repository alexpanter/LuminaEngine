#include "pch.h"
#include "ForwardRenderScene.h"
#include "Assets/AssetTypes/Material/Material.h"
#include "Core/Console/ConsoleVariable.h"
#include "Core/Templates/AsBytes.h"
#include "Core/Windows/Window.h"
#include "Assets/AssetTypes/Mesh/SkeletalMesh/SkeletalMesh.h"
#include "assets/assettypes/mesh/skeleton/skeleton.h"
#include "Assets/AssetTypes/Textures/Texture.h"
#include "Paths/Paths.h"
#include "Renderer/RendererUtils.h"
#include "Renderer/RHIStaticStates.h"
#include "Renderer/ShaderCompiler.h"
#include "Renderer/RenderGraph/RenderGraphDescriptor.h"
#include "Tools/Import/ImportHelpers.h"
#include "World/World.h"
#include "World/Entity/Components/BillboardComponent.h"
#include "world/entity/components/environmentcomponent.h"
#include "world/entity/components/lightcomponent.h"
#include "World/Entity/Components/LineBatcherComponent.h"
#include "World/Entity/Components/SkeletalMeshComponent.h"
#include "world/entity/components/staticmeshcomponent.h"
#include "World/Scene/RenderScene/MeshDrawCommand.h"

namespace Lumina
{
    static TConsoleVar CVarSelectionThickness("r.SelectionThickness", 5, "Changes thickness of entity selection.");

    FForwardRenderScene::FForwardRenderScene(CWorld* InWorld)
        : World(InWorld)
        , LightData()
        , SceneGlobalData()
        , ShadowAtlas(FShadowAtlasConfig())
        , DepthMeshPass()
        , OpaqueMeshPass()
        , TranslucentMeshPass()
        , ShadowMeshPass()
    {
    }

    void FForwardRenderScene::Init()
    {
        LOG_TRACE("Initializing Forward Render Scene");
        
        SceneViewport = GRenderContext->CreateViewport(Windowing::GetPrimaryWindowHandle()->GetExtent(), "Forward Renderer Viewport");

        InitBuffers();
        InitFrameResources();

        SwapchainResizedHandle = FRenderManager::OnSwapchainResized.AddMember(this, &FForwardRenderScene::SwapchainResized); 
        
        #if USING(WITH_EDITOR)
        NamedImages[(int)ENamedImage::PointLightIcon]       = Import::Textures::CreateTextureFromImport(Paths::GetEngineResourceDirectory() + "/Textures/PointLight.png", true);  
        NamedImages[(int)ENamedImage::DirectionalLightIcon] = Import::Textures::CreateTextureFromImport(Paths::GetEngineResourceDirectory() + "/Textures/SkyLight.png", true);  
        NamedImages[(int)ENamedImage::SpotLightIcon]        = Import::Textures::CreateTextureFromImport(Paths::GetEngineResourceDirectory() + "/Textures/SpotLight.png", true);  
        #endif
        
    }

    void FForwardRenderScene::Shutdown()
    {
        GRenderContext->WaitIdle();
        GRenderContext->ClearCommandListCache();
        GRenderContext->ClearBindingCaches();

        FRenderManager::OnSwapchainResized.Remove(SwapchainResizedHandle);
        
        LOG_TRACE("Shutting down Forward Render Scene");
    }

    void FForwardRenderScene::RenderScene(FRenderGraph& RenderGraph, const FViewVolume& ViewVolume)
    {
        LUMINA_PROFILE_SCOPE();
        
        SetViewVolume(ViewVolume);

        // Wait for shader tasks.
        if(GRenderContext->GetShaderCompiler()->HasPendingRequests())
        {
            return;
        }

        ResetPass(RenderGraph);
        CompileDrawCommands(RenderGraph);
        CullPass(RenderGraph);
        DepthPrePass(RenderGraph);
        DepthPyramidPass(RenderGraph);
        ClusterBuildPass(RenderGraph);
        LightCullPass(RenderGraph);
        PointShadowPass(RenderGraph);
        SpotShadowPass(RenderGraph);
        CascadedShowPass(RenderGraph);
        EnvironmentPass(RenderGraph);
        BasePass(RenderGraph);
        TransparentPass(RenderGraph);
        BatchedLineDraw(RenderGraph);
        BillboardPass(RenderGraph);
        //SelectionPass(RenderGraph);
        ToneMappingPass(RenderGraph);
        DebugDrawPass(RenderGraph);
    }

    void FForwardRenderScene::SetViewVolume(const FViewVolume& ViewVolume)
    {
        SceneViewport->SetViewVolume(ViewVolume);
        
        SceneGlobalData.CameraData.Location             = glm::vec4(SceneViewport->GetViewVolume().GetViewPosition(), 1.0f);
        SceneGlobalData.CameraData.Up                   = glm::vec4(SceneViewport->GetViewVolume().GetUpVector(), 1.0f);
        SceneGlobalData.CameraData.Right                = glm::vec4(SceneViewport->GetViewVolume().GetRightVector(), 1.0f);
        SceneGlobalData.CameraData.Forward              = glm::vec4(SceneViewport->GetViewVolume().GetForwardVector(), 1.0f);
        SceneGlobalData.CameraData.View                 = SceneViewport->GetViewVolume().GetViewMatrix();
        SceneGlobalData.CameraData.InverseView          = SceneViewport->GetViewVolume().GetInverseViewMatrix();
        SceneGlobalData.CameraData.Projection           = SceneViewport->GetViewVolume().GetProjectionMatrix();
        SceneGlobalData.CameraData.InverseProjection    = SceneViewport->GetViewVolume().GetInverseProjectionMatrix();
        SceneGlobalData.ScreenSize                      = glm::vec4(SceneViewport->GetSize().x, SceneViewport->GetSize().y, 0.0f, 0.0f);
        SceneGlobalData.GridSize                        = glm::vec4(ClusterGridSizeX, ClusterGridSizeY, ClusterGridSizeZ, 0.0f);
        SceneGlobalData.Time                            = (float)World->GetTimeSinceWorldCreation();
        SceneGlobalData.DeltaTime                       = (float)World->GetWorldDeltaTime();
        SceneGlobalData.FarPlane                        = SceneViewport->GetViewVolume().GetFar();
        SceneGlobalData.NearPlane                       = SceneViewport->GetViewVolume().GetNear();
        SceneGlobalData.CullData.Frustum                = SceneViewport->GetViewVolume().GetFrustum();
        SceneGlobalData.CullData.ViewMatrix             = SceneViewport->GetViewVolume().GetViewMatrix();
        SceneGlobalData.CullData.P00                    = SceneViewport->GetViewVolume().GetProjectionMatrix()[0][0];
        SceneGlobalData.CullData.P11                    = SceneViewport->GetViewVolume().GetProjectionMatrix()[1][1];
        SceneGlobalData.CullData.zNear                  = SceneViewport->GetViewVolume().GetNear();
        SceneGlobalData.CullData.zFar                   = SceneViewport->GetViewVolume().GetFar();
        SceneGlobalData.CullData.InstanceNum            = (uint32)InstanceData.size();
        SceneGlobalData.CullData.bFrustumCull           = RenderSettings.bFrustumCull;
        SceneGlobalData.CullData.bOcclusionCull         = RenderSettings.bOcclusionCull;
        SceneGlobalData.CullData.PyramidWidth           = (float)GetNamedImage(ENamedImage::DepthPyramid)->GetSizeX();
        SceneGlobalData.CullData.PyramidHeight          = (float)GetNamedImage(ENamedImage::DepthPyramid)->GetSizeY();
    }

    void FForwardRenderScene::SwapchainResized(glm::vec2 NewSize)
    {
        SceneViewport = GRenderContext->CreateViewport(NewSize, "Forward Renderer Viewport");
        InitFrameResources();
        
        BindingCache.ReleaseResources();
    }

    void FForwardRenderScene::CompileDrawCommands(FRenderGraph& RenderGraph)
    {
        LUMINA_PROFILE_SCOPE();
        
        //========================================================================================================================
        {
            LUMINA_PROFILE_SECTION("Compile Draw Commands");
            
            auto StaticView = World->GetEntityRegistry().view<SStaticMeshComponent, STransformComponent>();
            auto SkeletalView = World->GetEntityRegistry().view<SSkeletalMeshComponent, STransformComponent>();

            const size_t EntityCount = StaticView.size_hint() + SkeletalView.size_hint();
            const size_t EstimatedProxies = EntityCount * 2;

            InstanceData.reserve(EstimatedProxies * 2);
            IndirectDrawArguments.reserve(EstimatedProxies * 2);
			DrawCommands.reserve(EstimatedProxies);
            
            TFixedHashMap<CMaterial*, uint64, 4> BatchedDraws;
            
            {
                LUMINA_PROFILE_SECTION("Process Static Mesh Primitives");

                StaticView.each([&](entt::entity Entity, const SStaticMeshComponent& MeshComponent, const STransformComponent& TransformComponent)
                {
                    CMesh* Mesh = MeshComponent.StaticMesh;
                    if (!IsValid(Mesh))
                    {
                        return;
                    }
        
                    const FMeshResource& Resource = Mesh->GetMeshResource();
                    
                    
                    RenderStats.NumVertices += Resource.GetNumVertices();
                    RenderStats.NumTriangles += Resource.GetNumTriangles();
                    
                    
                    glm::mat4 TransformMatrix = TransformComponent.GetMatrix();
                    
                    FAABB BoundingBox       = Mesh->GetAABB().ToWorld(TransformMatrix);
                    glm::vec3 Center        = (BoundingBox.Min + BoundingBox.Max) * 0.5f;
                    glm::vec3 Extents       = BoundingBox.Max - Center;
                    float Radius            = glm::length(Extents);
                    glm::vec4 SphereBounds  = glm::vec4(Center, Radius);
                    
                    EInstanceFlags Flags = EInstanceFlags::None;
                    if (World->IsSelected(Entity))
                    {
                        Flags |= EInstanceFlags::Selected;
                    }
                    if (MeshComponent.bCastShadow)
                    {
                        Flags |= EInstanceFlags::CastShadow;
                    }
                    if (MeshComponent.bReceiveShadow)
                    {
                        Flags |= EInstanceFlags::ReceiveShadow;
                    }
                    
                    for (const FGeometrySurface& Surface : Resource.GeometrySurfaces)
                    {
                        CMaterialInterface* Material = MeshComponent.GetMaterialForSlot(Surface.MaterialIndex);
                    
                        if (!IsValid(Material) || !IsValid(Material->GetMaterial()) || !Material->IsReadyForRender())
                        {
                            Material = CMaterial::GetDefaultMaterial();
                        }
                        
                        auto [BatchIt, bBatchInserted] = BatchedDraws.try_emplace(Material->GetMaterial(), DrawCommands.size());
                        uint64 DrawID = BatchIt->second;
                        
                        if (bBatchInserted)
                        {
                            DrawCommands.emplace_back(FMeshDrawCommand
                            {
                                .VertexShader           = Material->GetVertexShader(EVertexFormat::Static),
                                .PixelShader            = Material->GetPixelShader(),
                                .IndirectDrawOffset     = 0,
                                .DrawArgumentIndexMap   = {},
                                .DrawCount              = 0,
                            });
                        }
                        
                        auto& DrawArguments = DrawCommands[DrawID].DrawArgumentIndexMap;
                        
                        auto [DrawIt, bDrawInserted] = DrawArguments.try_emplace(FDrawKey{Surface.StartIndex, Surface.IndexCount}, IndirectDrawArguments.size());
                        
                        if (bDrawInserted)
                        {
                            IndirectDrawArguments.emplace_back(FDrawIndirectArguments
                            {
                                .VertexCount            = (uint32)Surface.IndexCount,
                                .InstanceCount          = 0,
                                .StartVertexLocation    = Surface.StartIndex,
                                .StartInstanceLocation  = 0,
                            });
                        }

                        IndirectDrawArguments[DrawIt->second].InstanceCount++;
                        
                        InstanceData.emplace_back(FInstanceData
                        {
                            .Transform              = TransformMatrix,
                            .SphereBounds           = SphereBounds,
                            .EntityID               = entt::to_integral(Entity),
                            .BatchedDrawID          = DrawIt->second,
                            .Flags                  = Flags,
                            .BoneOffset             = 0,
                            .VertexBufferAddress    = RenderUtils::SplitAddress(Mesh->GetVertexBuffer()->GetAddress()),
                            .IndexBufferAddress     = RenderUtils::SplitAddress(Mesh->GetIndexBuffer()->GetAddress()),
                        });
                    }
                });
            }
            
            {
                LUMINA_PROFILE_SECTION("Process Skeletal Mesh Primitives");

                SkeletalView.each([&](entt::entity Entity, const SSkeletalMeshComponent& MeshComponent, const STransformComponent& TransformComponent)
                {
                    CMesh* Mesh = MeshComponent.SkeletalMesh;
                    if (!IsValid(Mesh))
                    {
                        return;
                    }
        
                    const FMeshResource& Resource = Mesh->GetMeshResource();
                    
                    uint32 BoneDataOffset = static_cast<uint32>(BonesData.size());
                    BonesData.insert(BonesData.end(), MeshComponent.BoneTransforms.begin(), MeshComponent.BoneTransforms.end());
                    
                    RenderStats.NumVertices += Resource.GetNumVertices();
                    RenderStats.NumTriangles += Resource.GetNumTriangles();

                    glm::mat4 TransformMatrix = TransformComponent.GetMatrix();
                    
                    FAABB BoundingBox       = Mesh->GetAABB().ToWorld(TransformMatrix);
                    glm::vec3 Center        = (BoundingBox.Min + BoundingBox.Max) * 0.5f;
                    glm::vec3 Extents       = BoundingBox.Max - Center;
                    float Radius            = glm::length(Extents);
                    glm::vec4 SphereBounds  = glm::vec4(Center, Radius);
                    
                    EInstanceFlags Flags = EInstanceFlags::Skinned;
                    if (World->IsSelected(Entity))
                    {
                        Flags |= EInstanceFlags::Selected;
                    }
                    if (MeshComponent.bCastShadow)
                    {
                        Flags |= EInstanceFlags::CastShadow;
                    }
                    if (MeshComponent.bReceiveShadow)
                    {
                        Flags |= EInstanceFlags::ReceiveShadow;
                    }
                    
                    for (const FGeometrySurface& Surface : Resource.GeometrySurfaces)
                    {
                        CMaterialInterface* Material = MeshComponent.GetMaterialForSlot(Surface.MaterialIndex);
                    
                        if (!IsValid(Material) || !IsValid(Material->GetMaterial()) || !Material->IsReadyForRender())
                        {
                            Material = CMaterial::GetDefaultMaterial();
                        }
                        
                        auto [BatchIt, bBatchInserted] = BatchedDraws.try_emplace(Material->GetMaterial(), DrawCommands.size());
                        uint64 DrawID = BatchIt->second;
                        
                        if (bBatchInserted)
                        {
                            DrawCommands.emplace_back(FMeshDrawCommand
                            {
                                .VertexShader           = Material->GetVertexShader(EVertexFormat::Skinned),
                                .PixelShader            = Material->GetPixelShader(),
                                .IndirectDrawOffset     = 0,
                                .DrawArgumentIndexMap   = {},
                                .DrawCount              = 0,
                            });
                        }
                        
                        auto& DrawArguments = DrawCommands[DrawID].DrawArgumentIndexMap;
                        
                        auto [DrawIt, bDrawInserted] = DrawArguments.try_emplace(FDrawKey{Surface.StartIndex, Surface.IndexCount}, IndirectDrawArguments.size());
                        
                        if (bDrawInserted)
                        {
                            IndirectDrawArguments.emplace_back(FDrawIndirectArguments
                            {
                                .VertexCount            = (uint32)Surface.IndexCount,
                                .InstanceCount          = 0,
                                .StartVertexLocation    = Surface.StartIndex,
                                .StartInstanceLocation  = 0,
                            });
                        }

                        IndirectDrawArguments[DrawIt->second].InstanceCount++;
                        
                        InstanceData.emplace_back(FInstanceData
                        {
                            .Transform              = TransformMatrix,
                            .SphereBounds           = SphereBounds,
                            .EntityID               = entt::to_integral(Entity),
                            .BatchedDrawID          = DrawIt->second,
                            .Flags                  = Flags,
                            .BoneOffset             = BoneDataOffset,
                            .VertexBufferAddress    = RenderUtils::SplitAddress(Mesh->GetVertexBuffer()->GetAddress()),
                            .IndexBufferAddress     = RenderUtils::SplitAddress(Mesh->GetIndexBuffer()->GetAddress()),
                        });
                    }
                });
            }
            
            
            TVector<uint32> IndexRemap(IndirectDrawArguments.size());
            TVector<FDrawIndirectArguments> ReorderedIndirectDrawArguments;
            ReorderedIndirectDrawArguments.reserve(IndirectDrawArguments.size());

            uint32 Counter = 0;
            uint32 CumulativeInstanceCount = 0;
            for (FMeshDrawCommand& DrawCommand : DrawCommands)
            {
                DrawCommand.DrawCount           = (uint32)DrawCommand.DrawArgumentIndexMap.size();
                DrawCommand.IndirectDrawOffset  = Counter;
                
                RenderStats.NumDraws            += DrawCommand.DrawCount;

                for (auto& [Key, OldIndex] : DrawCommand.DrawArgumentIndexMap)
                {
                    IndexRemap[OldIndex] = Counter;
        
                    FDrawIndirectArguments Args = IndirectDrawArguments[OldIndex];
                    Args.StartInstanceLocation = CumulativeInstanceCount;
                    CumulativeInstanceCount += Args.InstanceCount;
                    
                    RenderStats.NumInstances += Args.InstanceCount;
                    
                    Args.InstanceCount = 0;
        
                    ReorderedIndirectDrawArguments.push_back(Args);
                    Counter++;
                }
            }

            IndirectDrawArguments = std::move(ReorderedIndirectDrawArguments);

            for (FInstanceData& Instance : InstanceData)
            {
                Instance.BatchedDrawID = IndexRemap[Instance.BatchedDrawID];
            }
            
            RenderStats.NumBatches = DrawCommands.size();
        }
        
        //========================================================================================================================
        
        {
            LUMINA_PROFILE_SECTION("Process Billboard Primitives");

            uint32 NumBillboards = 0;
            auto View = World->GetEntityRegistry().view<SBillboardComponent, STransformComponent>();
            View.each([this, NumBillboards](const SBillboardComponent& BillboardComponent, const STransformComponent& TransformComponent)
            {
                if (!BillboardComponent.Texture.IsValid() || !BillboardComponent.Texture->GetRHIRef()->IsValid())
                {
                    return;
                }
                
                FBillboardInstance& Instance = BillboardInstances.emplace_back();
                Instance.Position = TransformComponent.GetLocation();
                Instance.Texture = BillboardComponent.Texture->GetRHIRef();
                Instance.Size = BillboardComponent.Scale;
                
                GRenderContext->WriteDescriptorTable(SceneDescriptorTable, FBindingSetItem::TextureSRV(NumBillboards, Instance.Texture));
            });
        }
        
        {
            LUMINA_PROFILE_SECTION("Directional Light Processing");

            LightData.bHasSun = false;
            auto View = World->GetEntityRegistry().view<SDirectionalLightComponent>();
            View.each([this](const SDirectionalLightComponent& DirectionalLightComponent)
            {
                LightData.bHasSun = true;
                const FViewVolume& ViewVolume = SceneViewport->GetViewVolume();
                
                #if USING(WITH_EDITOR)
                if (!World->IsGameWorld())
                {
                    DrawBillboard(GetNamedImage(ENamedImage::DirectionalLightIcon), glm::vec3(0.0f), 0.35f);
                }
                #endif    
                
                float NearClip          = ViewVolume.GetNear();

                FLight Light            = {};
                Light.Flags             = LIGHT_TYPE_DIRECTIONAL;
                Light.Color             = PackColor(glm::vec4(DirectionalLightComponent.Color, 1.0));
                Light.Intensity         = DirectionalLightComponent.Intensity;
                Light.Direction         = glm::normalize(DirectionalLightComponent.Direction);
                LightData.SunDirection  = Light.Direction;
                
                LightData.CascadeSplits[0] = 15.0f;
                LightData.CascadeSplits[1] = 50.0f;
                LightData.CascadeSplits[2] = 200.0f;

                for (int i = 0; i < NumCascades; ++i)
                {
                    float CascadeFar = LightData.CascadeSplits[i];
                    
                    glm::mat4 ViewProjection        = glm::perspective(glm::radians(ViewVolume.GetFOV()), ViewVolume.GetAspectRatio(), NearClip, CascadeFar);
                    glm::mat4 ViewProjectionMatrix  = ViewProjection * ViewVolume.GetViewMatrix();
                    
                    glm::vec3 FrustumCorners[8];
                    FFrustum::ComputeFrustumCorners(ViewProjectionMatrix, FrustumCorners);
                    
                    glm::vec3 FrustumCenter = std::reduce(std::begin(FrustumCorners), std::end(FrustumCorners)) / 8.0f;
                    glm::mat4 LightView     = glm::lookAt(FrustumCenter + Light.Direction, FrustumCenter, FViewVolume::UpAxis);
                    
                    float MinX = eastl::numeric_limits<float>::max();
                    float MaxX = eastl::numeric_limits<float>::lowest();
                    float MinY = eastl::numeric_limits<float>::max();
                    float MaxY = eastl::numeric_limits<float>::lowest();
                    float MinZ = eastl::numeric_limits<float>::max();
                    float MaxZ = eastl::numeric_limits<float>::lowest();
                    for (const auto& V : FrustumCorners)
                    {
                        const auto TRF = LightView * glm::vec4(V, 1.0f);
                        MinX = eastl::min(MinX, TRF.x);
                        MaxX = eastl::max(MaxX, TRF.x);
                        MinY = eastl::min(MinY, TRF.y);
                        MaxY = eastl::max(MaxY, TRF.y);
                        MinZ = eastl::min(MinZ, TRF.z);
                        MaxZ = eastl::max(MaxZ, TRF.z);
                    }
                    
                    constexpr float zMult = 10.0f;
                    if (MinZ < 0)
                    {
                        MinZ *= zMult;
                    }
                    else
                    {
                        MinZ /= zMult;
                    }
                    if (MaxZ < 0)
                    {
                        MaxZ /= zMult;
                    }
                    else
                    {
                        MaxZ *= zMult;
                    }
                    
                    glm::mat4 LightProjection       = glm::ortho(MinX, MaxX, MinY, MaxY, MinZ, MaxZ);
                    Light.ViewProjection[i]         = LightProjection * LightView;
                }
                
                LightData.Lights[0] = Light;
                LightData.NumLights++;
            });
        }
        
        //========================================================================================================================
        
        {
            LUMINA_PROFILE_SECTION("Point Light Processing");

            auto View = World->GetEntityRegistry().view<SPointLightComponent, STransformComponent>();
            View.each([&] (const SPointLightComponent& PointLightComponent, const STransformComponent& TransformComponent)
            {
                FLight Light;
                Light.Flags                 = LIGHT_TYPE_POINT;
                Light.Falloff               = PointLightComponent.Falloff;
                Light.Color                 = PackColor(glm::vec4(PointLightComponent.LightColor, 1.0));
                Light.Intensity             = PointLightComponent.Intensity;
                Light.Radius                = PointLightComponent.Attenuation;
                Light.Position              = TransformComponent.WorldTransform.Location;
                
                #if USING(WITH_EDITOR)
                if (!World->IsGameWorld())
                {
                    DrawBillboard(GetNamedImage(ENamedImage::PointLightIcon), TransformComponent.GetLocation(), 0.35f);
                }
                #endif
                
                FViewVolume LightView(90.0f, 1.0f, 0.01f, Light.Radius);
                
                auto SetView = [&Light](FViewVolume& View, uint32 Index)
                {
                    switch (Index)
                    {
                    case 0: // +X
                        View.SetView(Light.Position, FViewVolume::RightAxis, FViewVolume::DownAxis);
                        break;
                    case 1: // -X
                        View.SetView(Light.Position, FViewVolume::LeftAxis, FViewVolume::DownAxis);
                        break;
                    case 2: // +Y
                        View.SetView(Light.Position, FViewVolume::UpAxis, FViewVolume::ForwardAxis);
                        break;
                    case 3: // -Y
                        View.SetView(Light.Position, FViewVolume::DownAxis, FViewVolume::BackwardAxis);
                        break;
                    case 4: // +Z
                        View.SetView(Light.Position, FViewVolume::ForwardAxis, FViewVolume::DownAxis);
                        break;
                    case 5: // -Z
                        View.SetView(Light.Position, FViewVolume::BackwardAxis, FViewVolume::DownAxis);
                        break;
                    default:
                        UNREACHABLE();
                    }
                };

                if (PointLightComponent.bCastShadows)
                {
                    int32 TileIndex = ShadowAtlas.AllocateTile();
                    
                    if (TileIndex != INDEX_NONE)
                    {
                        const FShadowTile& Tile = ShadowAtlas.GetTile(TileIndex);

                        for (int Face = 0; Face < 6; ++Face)
                        {
                            SetView(LightView, Face);
                            
                            Light.ViewProjection[Face]              = LightView.ToReverseDepthViewProjectionMatrix();
                            Light.Shadow[Face].ShadowMapIndex       = TileIndex;
                            Light.Shadow[Face].ShadowMapLayer       = Face;
                            Light.Shadow[Face].AtlasUVOffset        = Tile.UVOffset;
                            Light.Shadow[Face].AtlasUVScale         = Tile.UVScale;
                            Light.Shadow[Face].LightIndex           = (int32)LightData.NumLights;
                        }
                        
                        PackedShadows[(uint32)ELightType::Point].push_back(Light.Shadow[0]);
                    }
                }
                else
                {
                    Light.Shadow[0].ShadowMapIndex = INDEX_NONE;
                }
                
                LightData.Lights[LightData.NumLights++] = Light;
        
                //World->DrawDebugSphere(Light.Position, 0.25f, glm::vec4(PointLightComponent.LightColor, 1.0));
            });
        }
        
        //========================================================================================================================
        
        {
            LUMINA_PROFILE_SECTION("Spot Light Processing");

            auto View = World->GetEntityRegistry().view<SSpotLightComponent, STransformComponent>();
            View.each([&] (SSpotLightComponent& SpotLightComponent, STransformComponent& TransformComponent)
            {
                const FTransform& Transform = TransformComponent.WorldTransform;
                
                #if USING(WITH_EDITOR)
                if (!World->IsGameWorld())
                {
                    DrawBillboard(GetNamedImage(ENamedImage::SpotLightIcon), TransformComponent.GetLocation(), 0.35f);
                }
                #endif                
        
                glm::vec3 UpdatedForward    = Transform.Rotation * FViewVolume::ForwardAxis;
                glm::vec3 UpdatedUp         = Transform.Rotation * FViewVolume::UpAxis;
        
                float InnerDegrees = SpotLightComponent.InnerConeAngle;
                float OuterDegrees = SpotLightComponent.OuterConeAngle;
        
                float InnerCos = glm::cos(glm::radians(InnerDegrees));
                float OuterCos = glm::cos(glm::radians(OuterDegrees));
                
                FViewVolume ViewVolume(OuterDegrees * 2.00f, 1.0f, 0.01f, SpotLightComponent.Attenuation);
                ViewVolume.SetView(Transform.Location, -UpdatedForward, UpdatedUp);
                
                FLight Light;
                Light.Flags                 = LIGHT_TYPE_SPOT;
                Light.Position              = Transform.Location;
                Light.Direction             = glm::normalize(UpdatedForward);
                Light.Falloff               = SpotLightComponent.Falloff;
                Light.Color                 = PackColor(glm::vec4(SpotLightComponent.LightColor, 1.0));
                Light.Intensity             = SpotLightComponent.Intensity;
                Light.Radius                = SpotLightComponent.Attenuation;
                Light.Angles                = glm::vec2(InnerCos, OuterCos);
                Light.ViewProjection[0]     = ViewVolume.ToReverseDepthViewProjectionMatrix();
        
                if (SpotLightComponent.bCastShadows)
                {
                    int32 TileIndex = ShadowAtlas.AllocateTile();
                    if (TileIndex != INDEX_NONE)
                    {
                        const FShadowTile& Tile             = ShadowAtlas.GetTile(TileIndex);
                        Light.Shadow[0].ShadowMapIndex      = TileIndex;
                        Light.Shadow[0].ShadowMapLayer      = 6;
                        Light.Shadow[0].AtlasUVOffset       = Tile.UVOffset;
                        Light.Shadow[0].AtlasUVScale        = Tile.UVScale;
                        Light.Shadow[0].LightIndex          = (int32)LightData.NumLights;

                    }
                    
                    PackedShadows[(uint32)ELightType::Spot].push_back(Light.Shadow[0]);
                }
                else
                {
                    Light.Shadow[0].ShadowMapIndex = INDEX_NONE;
                }
        
                LightData.Lights[LightData.NumLights++] = Light;
                
               //World->DrawViewVolume(ViewVolume, FColor::Red);
        
               //World->DrawDebugCone(SpotLight.Position, Forward, glm::radians(OuterDegrees), SpotLightComponent.Attenuation, glm::vec4(SpotLightComponent.LightColor, 1.0f));
               //World->DrawDebugCone(SpotLight.Position, Forward, glm::radians(InnerDegrees), SpotLightComponent.Attenuation, glm::vec4(SpotLightComponent.LightColor, 1.0f));
        
            });
        }
        
        //========================================================================================================================
        {
            LUMINA_PROFILE_SECTION("Batched Line Processing");
        
            auto View = World->GetEntityRegistry().view<FLineBatcherComponent>();
            View.each([&](FLineBatcherComponent& LineBatcherComponent)
            {
                if (LineBatcherComponent.Lines.empty())
                {
                    return;
                }
        
                for (FLineBatcherComponent::FLineInstance& Line : LineBatcherComponent.Lines)
                {
                    if (Line.RemainingLifetime >= 0.0f)
                    {
                        Line.RemainingLifetime -= SceneGlobalData.DeltaTime;
                    }
                }
        
                TVector<FLineBatcherComponent::FLineInstance> NewLines;
                TVector<FSimpleElementVertex> NewVertices;
                
                NewLines.reserve(LineBatcherComponent.Lines.size());
                NewVertices.reserve(LineBatcherComponent.Vertices.size());
        
                struct FLineWithVertices
                {
                    FLineBatcherComponent::FLineInstance Line;
                    FSimpleElementVertex Vertex0;
                    FSimpleElementVertex Vertex1;
                };
                
                TVector<FLineWithVertices> AliveLinesWithVertices;
                AliveLinesWithVertices.reserve(LineBatcherComponent.Lines.size());
                
                for (const FLineBatcherComponent::FLineInstance& Line : LineBatcherComponent.Lines)
                {
                    if (Line.RemainingLifetime > 0.0f)
                    {
                        FLineWithVertices LineData;
                        LineData.Line = Line;
                        LineData.Vertex0 = LineBatcherComponent.Vertices[Line.StartVertexIndex];
                        LineData.Vertex1 = LineBatcherComponent.Vertices[Line.StartVertexIndex + 1];
                        AliveLinesWithVertices.emplace_back(LineData);
                    }
                }
                
                eastl::sort(AliveLinesWithVertices.begin(), AliveLinesWithVertices.end(), [](const FLineWithVertices& A, const FLineWithVertices& B)
                {
                    if (A.Line.bDepthTest != B.Line.bDepthTest)
                    {
                        return A.Line.bDepthTest < B.Line.bDepthTest;
                    }
                    return A.Line.Thickness < B.Line.Thickness;
                });
                
                uint32 CurrentVertexIndex = 0;
                for (const FLineWithVertices& LineData : AliveLinesWithVertices)
                {
                    FLineBatcherComponent::FLineInstance NewLine = LineData.Line;
                    NewLine.StartVertexIndex = CurrentVertexIndex;
                    NewLines.emplace_back(NewLine);
        
                    NewVertices.emplace_back(LineData.Vertex0);
                    NewVertices.emplace_back(LineData.Vertex1);
                    
                    CurrentVertexIndex += 2;
                }
        
                LineBatcherComponent.Lines      = std::move(NewLines);
                LineBatcherComponent.Vertices   = std::move(NewVertices);
                
                if (!LineBatcherComponent.Vertices.empty())
                {
                    SimpleVertices = LineBatcherComponent.Vertices;
            
                    LineBatches.clear();
            
                    if (!LineBatcherComponent.Lines.empty())
                    {
                        FLineBatch CurrentBatch;
                        CurrentBatch.StartVertex = 0;
                        CurrentBatch.VertexCount = 2;
                        CurrentBatch.Thickness = LineBatcherComponent.Lines[0].Thickness;
                        CurrentBatch.bDepthTest = LineBatcherComponent.Lines[0].bDepthTest;
                
                        for (size_t i = 1; i < LineBatcherComponent.Lines.size(); ++i)
                        {
                            const auto& Line = LineBatcherComponent.Lines[i];
                    
                            if (Line.Thickness == CurrentBatch.Thickness && Line.bDepthTest == CurrentBatch.bDepthTest)
                            {
                                CurrentBatch.VertexCount += 2;
                            }
                            else
                            {
                                LineBatches.emplace_back(CurrentBatch);
                        
                                CurrentBatch.StartVertex = Line.StartVertexIndex;
                                CurrentBatch.VertexCount = 2;
                                CurrentBatch.Thickness = Line.Thickness;
                                CurrentBatch.bDepthTest = Line.bDepthTest;
                            }
                        }
                
                        LineBatches.emplace_back(CurrentBatch);
                    }
                }
            });
        }
        
        
        //========================================================================================================================
        
        
        {
            LUMINA_PROFILE_SECTION("Environment Processing");

            RenderSettings.bHasEnvironment = false;
            LightData.AmbientLight = glm::vec4(0.0f);
            RenderSettings.bSSAO = false;
            auto View = World->GetEntityRegistry().view<SEnvironmentComponent>();
            View.each([this] (const SEnvironmentComponent& EnvironmentComponent)
            {
                LightData.AmbientLight          = glm::vec4(EnvironmentComponent.AmbientColor, EnvironmentComponent.Intensity);
                RenderSettings.bHasEnvironment  = true;
                RenderSettings.bSSAO            = false;
            });
        }
        
        {
            FRGPassDescriptor* Descriptor = RenderGraph.AllocDescriptor();
            RenderGraph.AddPass(RG_Raster, FRGEvent("Write Scene Buffers"), Descriptor, [this](ICommandList& CmdList)
            {
                LUMINA_PROFILE_SECTION_COLORED("Write Scene Buffers", tracy::Color::OrangeRed3);
                
                SceneGlobalData.CullData.InstanceNum = (uint32)InstanceData.size();
                
                const SIZE_T SimpleVertexSize   = SimpleVertices.size() * sizeof(FSimpleElementVertex);
                const SIZE_T InstanceDataSize   = InstanceData.size() * sizeof(FInstanceData);
                const SIZE_T BoneDataSize       = BonesData.size() * sizeof(glm::mat4);
                const SIZE_T IndirectArgsSize   = IndirectDrawArguments.size() * sizeof(FDrawIndirectArguments);
                const SIZE_T ActiveLightsSize   = LightData.NumLights * sizeof(FLight);
                const SIZE_T LightUploadSize    = offsetof(FSceneLightData, Lights) + ActiveLightsSize;
                
                bool bAnyBufferResized = false;
                
                if (RenderUtils::ResizeBufferIfNeeded(NamedBuffers[(int)ENamedBuffer::Instance], (uint32)InstanceDataSize, 2))
                {
                    bAnyBufferResized = true;
                }
                
                if (RenderUtils::ResizeBufferIfNeeded(NamedBuffers[(int)ENamedBuffer::InstanceMapping], sizeof(uint32) * InstanceData.size(), 2))
                {
                    bAnyBufferResized = true;
                }
                
                if (RenderUtils::ResizeBufferIfNeeded(NamedBuffers[(int)ENamedBuffer::SimpleVertex], (uint32)SimpleVertexSize, 2))
                {
                    bAnyBufferResized = true;
                }
                
                if (RenderUtils::ResizeBufferIfNeeded(NamedBuffers[(int)ENamedBuffer::Bone], (uint32)BoneDataSize, 2))
                {
                    bAnyBufferResized = true;
                }
                
                if (RenderUtils::ResizeBufferIfNeeded(NamedBuffers[(int)ENamedBuffer::Indirect], (uint32)IndirectArgsSize, 2))
                {
                    bAnyBufferResized = true;
                }
                
                if (RenderUtils::ResizeBufferIfNeeded(NamedBuffers[(int)ENamedBuffer::Light], (uint32)LightUploadSize, 2))
                {
                    bAnyBufferResized = true;
                }
                
                if (bAnyBufferResized)
                {
                    CreateLayouts();
                }
                
                CmdList.SetBufferState(GetNamedBuffer(ENamedBuffer::Scene), EResourceStates::CopyDest);
                CmdList.SetBufferState(GetNamedBuffer(ENamedBuffer::Instance), EResourceStates::CopyDest);
                CmdList.SetBufferState(GetNamedBuffer(ENamedBuffer::Bone), EResourceStates::CopyDest);
                CmdList.SetBufferState(GetNamedBuffer(ENamedBuffer::Indirect), EResourceStates::CopyDest);
                CmdList.SetBufferState(GetNamedBuffer(ENamedBuffer::SimpleVertex), EResourceStates::CopyDest);
                CmdList.SetBufferState(GetNamedBuffer(ENamedBuffer::Light), EResourceStates::CopyDest);
                CmdList.CommitBarriers();
                
                CmdList.DisableAutomaticBarriers();
                CmdList.WriteBuffer(GetNamedBuffer(ENamedBuffer::Scene), &SceneGlobalData, sizeof(FSceneGlobalData));
                CmdList.WriteBuffer(GetNamedBuffer(ENamedBuffer::Instance), InstanceData.data(), InstanceDataSize);
                CmdList.WriteBuffer(GetNamedBuffer(ENamedBuffer::Bone), BonesData.data(),  BoneDataSize);
                CmdList.WriteBuffer(GetNamedBuffer(ENamedBuffer::Indirect), IndirectDrawArguments.data(), IndirectArgsSize);
                CmdList.WriteBuffer(GetNamedBuffer(ENamedBuffer::SimpleVertex), SimpleVertices.data(), SimpleVertexSize);
                CmdList.WriteBuffer(GetNamedBuffer(ENamedBuffer::Light), &LightData, LightUploadSize);
                CmdList.EnableAutomaticBarriers();
            });
        }
    }

    void FForwardRenderScene::DrawBillboard(FRHIImage* Image, const glm::vec3& Location, float Scale)
    {
        FBillboardInstance& Billboard = BillboardInstances.emplace_back();
        Billboard.Texture = Image;
        Billboard.Position = Location;
        Billboard.Size = Scale;
    }

    void FForwardRenderScene::ResetPass(FRenderGraph& RenderGraph)
    {
        SimpleVertices.clear();
        DrawCommands.clear();
        IndirectDrawArguments.clear();
        InstanceData.clear();
        LightData.NumLights = 0;
        ShadowAtlas.FreeTiles();
        BonesData.clear();
        BillboardInstances.clear();
        RenderStats = {};

        for (int i = 0; i < (int)ELightType::Num; ++i)
        {
            PackedShadows[i].clear();
        }
    }

    void FForwardRenderScene::CullPass(FRenderGraph& RenderGraph)
    {
        if (DrawCommands.empty())
        {
            return;
        }
        
        FRGPassDescriptor* Descriptor = RenderGraph.AllocDescriptor();
        RenderGraph.AddPass(RG_Compute, FRGEvent("Cull Pass"), Descriptor, [&] (ICommandList& CmdList)
        {
            LUMINA_PROFILE_SECTION_COLORED("Cull Pass", tracy::Color::Pink2);
                
            FRHIComputeShaderRef ComputeShader = FShaderLibrary::GetComputeShader("MeshCull.comp");

            FComputePipelineDesc PipelineDesc;
            PipelineDesc.SetComputeShader(ComputeShader);
            PipelineDesc.AddBindingLayout(SceneBindingLayout);
            PipelineDesc.AddBindingLayout(SceneBindlessLayout);
                    
            FRHIComputePipelineRef Pipeline = GRenderContext->CreateComputePipeline(PipelineDesc);
            
            FComputeState State;
            State.SetPipeline(Pipeline);
            State.AddBindingSet(SceneBindingSet);
            State.AddBindingSet(SceneDescriptorTable);
            CmdList.SetComputeState(State);
            
            uint32 Num = (uint32)InstanceData.size();
            uint32 NumWorkGroups = (Num + 255) / 256;
                
            CmdList.Dispatch(NumWorkGroups, 1, 1);
            
        });
    }

    void FForwardRenderScene::DepthPrePass(FRenderGraph& RenderGraph)
    {
        if (DrawCommands.empty())
        {
            return;
        }
        
        FRGPassDescriptor* Descriptor = RenderGraph.AllocDescriptor();
        RenderGraph.AddPass(RG_Raster, FRGEvent("Pre-Depth Pass"), Descriptor, [&] (ICommandList& CmdList)
        {
            LUMINA_PROFILE_SECTION_COLORED("Pre-Depth Pass", tracy::Color::Orange);
        
            FRenderPassDesc::FAttachment Depth; Depth
                .SetImage(GetNamedImage(ENamedImage::DepthAttachment))
                .SetDepthClearValue(0.0f);
            
            FRenderPassDesc RenderPass; RenderPass
                .SetDepthAttachment(Depth)
                .SetRenderArea(GetNamedImage(ENamedImage::HDR)->GetExtent());
            
            FRenderState RenderState; RenderState
                .SetDepthStencilState(FDepthStencilState().SetDepthFunc(EComparisonFunc::Greater))
                .SetRasterState(FRasterState().EnableDepthClip());
            
            
            for (const FMeshDrawCommand& Batch : DrawCommands)
            {
                FRHIVertexShaderRef VertexShader = FShaderLibrary::GetVertexShader("DepthPrePass.vert");
                
                FGraphicsPipelineDesc Desc; Desc
                .SetRenderState(RenderState)
                .AddBindingLayout(SceneBindingLayout)
                .AddBindingLayout(SceneBindlessLayout)
                .SetVertexShader(VertexShader);
                
                FRHIGraphicsPipelineRef Pipeline = GRenderContext->CreateGraphicsPipeline(Desc, RenderPass);
                
                FGraphicsState GraphicsState;
                GraphicsState.SetRenderPass(RenderPass);
                GraphicsState.SetViewportState(SceneViewportState);
                GraphicsState.SetPipeline(Pipeline);
                GraphicsState.AddBindingSet(SceneBindingSet);
                GraphicsState.AddBindingSet(SceneDescriptorTable);
                GraphicsState.SetIndirectParams(GetNamedBuffer(ENamedBuffer::Indirect));
                
                CmdList.SetGraphicsState(GraphicsState);
                
                CmdList.DrawIndirect(Batch.DrawCount, Batch.IndirectDrawOffset * sizeof(FDrawIndirectArguments));
            }
        });
    }

    void FForwardRenderScene::DepthPyramidPass(FRenderGraph& RenderGraph)
    {
        if (DrawCommands.empty())
        {
            return;
        }

        FRGPassDescriptor* Descriptor = RenderGraph.AllocDescriptor();
        Descriptor->SetFlag(ERGExecutionFlags::Async);
        RenderGraph.AddPass(RG_Compute, FRGEvent("Depth Pyramid Pass"), Descriptor, [&](ICommandList& CmdList)
        {
            LUMINA_PROFILE_SECTION_COLORED("Depth Pyramid Pass", tracy::Color::Orange);

            FRHIImage* DepthPyramid = GetNamedImage(ENamedImage::DepthPyramid);
            FRHIComputeShaderRef ComputeShader = FShaderLibrary::GetComputeShader("DepthPyramidMips.comp");
            int MipCount = DepthPyramid->GetDescription().NumMips;

            CmdList.SetEnableUavBarriersForImage(DepthPyramid, false);

            for (int i = 0; i < MipCount; ++i)
            {
                LUMINA_PROFILE_SECTION_COLORED("Process Mip", tracy::Color::Yellow4);

                FBindingLayoutDesc LayoutDesc;
                LayoutDesc.AddItem(FBindingLayoutItem::Texture_SRV(0));
                LayoutDesc.AddItem(FBindingLayoutItem::Texture_UAV(1));
                LayoutDesc.SetVisibility(ERHIShaderType::Compute);
                
                FBindingSetDesc SetDesc;
                if (i == 0)
                {
                    SetDesc.AddItem(FBindingSetItem::TextureSRV(0, GetNamedImage(ENamedImage::DepthAttachment)));
                }
                else
                {
                    FRHISamplerRef Sampler = TStaticRHISampler<true, false, AM_Clamp, AM_Clamp, AM_Clamp, ESamplerReductionType::Minimum>::GetRHI();
                    SetDesc.AddItem(FBindingSetItem::TextureSRV(0, DepthPyramid, Sampler, DepthPyramid->GetFormat(), FTextureSubresourceSet(i - 1, 1, 0, 1)));
                }

                SetDesc.AddItem(FBindingSetItem::TextureUAV(1, DepthPyramid, DepthPyramid->GetFormat(), FTextureSubresourceSet(i, 1, 0, 1)));
                
                FRHIBindingLayout* Layout = BindingCache.GetOrCreateBindingLayout(LayoutDesc);
                FRHIBindingSet* Set = BindingCache.GetOrCreateBindingSet(SetDesc, Layout);

                FComputePipelineDesc PipelineDesc;
                PipelineDesc.AddBindingLayout(Layout);
                PipelineDesc.CS = ComputeShader;
                PipelineDesc.DebugName = "Depth Pyramid Mips";

                FRHIComputePipelineRef Pipeline = GRenderContext->CreateComputePipeline(PipelineDesc);

                FComputeState State;
                State.AddBindingSet(Set);
                State.SetPipeline(Pipeline);

                CmdList.SetComputeState(State);

                uint32 LevelWidth = DepthPyramid->GetSizeX() >> i;
                uint32 LevelHeight = DepthPyramid->GetSizeY() >> i;

                LevelWidth = std::max(LevelWidth, 1u);
                LevelHeight = std::max(LevelHeight, 1u);
                

                glm::vec2 Data = glm::vec2(LevelWidth,LevelHeight);
                TSpan<const Byte> Bytes = AsBytes(Data);
                CmdList.SetPushConstants(Bytes.data(), Bytes.size_bytes());

                uint32 GroupsX = RenderUtils::GetGroupCount(LevelWidth, 32);
                uint32 GroupsY = RenderUtils::GetGroupCount(LevelHeight, 32);
                
                CmdList.Dispatch(GroupsX, GroupsY, 1);
            }

            CmdList.SetEnableUavBarriersForImage(DepthPyramid, true);

        });
    }

    void FForwardRenderScene::ClusterBuildPass(FRenderGraph& RenderGraph)
    {
		if (LightData.NumLights == 0 || DrawCommands.empty())
        {
            return;
        }

        FRGPassDescriptor* Descriptor = RenderGraph.AllocDescriptor();
        RenderGraph.AddPass(RG_Compute, FRGEvent("Cluster Build Pass"), Descriptor, [&] (ICommandList& CmdList)
        {
            LUMINA_PROFILE_SECTION_COLORED("Cluster Build Pass", tracy::Color::Pink2);
                
            FRHIComputeShaderRef ComputeShader = FShaderLibrary::GetComputeShader("ClusterBuild.comp");

            FComputePipelineDesc PipelineDesc;
            PipelineDesc.SetComputeShader(ComputeShader);
            PipelineDesc.AddBindingLayout(SceneBindingLayout);
            PipelineDesc.AddBindingLayout(SceneBindlessLayout);
                    
            FRHIComputePipelineRef Pipeline = GRenderContext->CreateComputePipeline(PipelineDesc);
                
            FComputeState State;
            State.SetPipeline(Pipeline);
            State.AddBindingSet(SceneBindingSet);
            State.AddBindingSet(SceneDescriptorTable);
            CmdList.SetComputeState(State);

            FLightClusterPC ClusterPC;
            ClusterPC.InverseProjection = SceneViewport->GetViewVolume().GetInverseProjectionMatrix();
            ClusterPC.zNearFar = glm::vec2(SceneViewport->GetViewVolume().GetNear(), SceneViewport->GetViewVolume().GetFar());
            ClusterPC.GridSize = glm::vec4(ClusterGridSizeX, ClusterGridSizeY, ClusterGridSizeZ, 0.0f);
            ClusterPC.ScreenSize = glm::vec2(GetNamedImage(ENamedImage::HDR)->GetSizeX(), GetNamedImage(ENamedImage::HDR)->GetSizeY());
                
            CmdList.SetPushConstants(&ClusterPC, sizeof(FLightClusterPC));
                
            CmdList.Dispatch(ClusterGridSizeX, ClusterGridSizeY, ClusterGridSizeZ);
                
        });
    }

    void FForwardRenderScene::LightCullPass(FRenderGraph& RenderGraph)
    {
        if (LightData.NumLights == 0)
        {
            return;
        }

        FRGPassDescriptor* Descriptor = RenderGraph.AllocDescriptor();
        RenderGraph.AddPass(RG_Compute, FRGEvent("Light Cull Pass"), Descriptor, [&] (ICommandList& CmdList)
        {
            LUMINA_PROFILE_SECTION_COLORED("Light Cull Pass", tracy::Color::Pink2);
                
            FRHIComputeShaderRef ComputeShader = FShaderLibrary::GetComputeShader("LightCull.comp");

            FComputePipelineDesc PipelineDesc;
            PipelineDesc.SetComputeShader(ComputeShader);
            PipelineDesc.AddBindingLayout(SceneBindingLayout);
            PipelineDesc.AddBindingLayout(SceneBindlessLayout);
                    
            FRHIComputePipelineRef Pipeline = GRenderContext->CreateComputePipeline(PipelineDesc);
                
            FComputeState State;
            State.SetPipeline(Pipeline);
            State.AddBindingSet(SceneBindingSet);
            State.AddBindingSet(SceneDescriptorTable);
            CmdList.SetComputeState(State);
                
            glm::mat4 ViewProj = SceneViewport->GetViewVolume().GetViewMatrix();
                
            CmdList.SetPushConstants(&ViewProj, sizeof(glm::mat4));
                
            CmdList.Dispatch(27, 1, 1);
                
        });
    }

    void FForwardRenderScene::PointShadowPass(FRenderGraph& RenderGraph)
    {
        if (PackedShadows[(uint32)ELightType::Point].empty() || DrawCommands.empty())
        {
            return;
        }
        
        FRGPassDescriptor* Descriptor = RenderGraph.AllocDescriptor();
        RenderGraph.AddPass(RG_Raster, FRGEvent("Point Light Shadow Pass"), Descriptor, [&](ICommandList& CmdList)
        {
            LUMINA_PROFILE_SECTION_COLORED("Point Light Shadow Pass", tracy::Color::DeepPink2);
            
            FRHIPixelShaderRef PixelShader = FShaderLibrary::GetPixelShader("ShadowMapping.frag");
            
            FRenderState RenderState; RenderState
                    .SetDepthStencilState(FDepthStencilState()
                    .SetDepthFunc(EComparisonFunc::LessOrEqual))
                    .SetRasterState(FRasterState()
                        .SetSlopeScaleDepthBias(1.75f)
                        .SetDepthBias(100)
                        .SetCullFront());
            
            FRenderPassDesc::FAttachment Depth; Depth
                .SetLoadOp(ERenderLoadOp::Clear)
                .SetDepthClearValue(1.0)
                .SetImage(ShadowAtlas.GetImage())
                    .SetNumSlices(6);
            
            FRenderPassDesc RenderPass; RenderPass
                .SetDepthAttachment(Depth)
                .SetViewMask(RenderUtils::CreateViewMask<0u, 1u, 2u, 3u, 4u, 5u>())
                .SetRenderArea(glm::uvec2(GShadowAtlasResolution, GShadowAtlasResolution));
            
            FGraphicsState GraphicsState; GraphicsState
                .SetRenderPass(Move(RenderPass))
                .AddBindingSet(SceneBindingSet)
                .AddBindingSet(SceneDescriptorTable)
                .SetIndirectParams(GetNamedBuffer(ENamedBuffer::Indirect));
                    
            
            const TVector<FLightShadow>& PointShadows = PackedShadows[(uint32)ELightType::Point];
            
            FRHIVertexShaderRef VertexShader = FShaderLibrary::GetVertexShader("ShadowMapping.vert");
            
            for (const FLightShadow& LightShadow : PointShadows)
            {
                LUMINA_PROFILE_SECTION_COLORED("Process Point Light", tracy::Color::DeepPink2);
                
                const FShadowTile& Tile = ShadowAtlas.GetTile(LightShadow.ShadowMapIndex);
                uint32 TilePixelX       = static_cast<uint32>(Tile.UVOffset.x * GShadowAtlasResolution);
                uint32 TilePixelY       = static_cast<uint32>(Tile.UVOffset.y * GShadowAtlasResolution);
                uint32 TileSize         = static_cast<uint32>(Tile.UVScale.x * GShadowAtlasResolution);
                
                FViewport Viewport
                (
                    (float)TilePixelX,
                    (float)TilePixelX + TileSize,
                    (float)TilePixelY,
                    (float)TilePixelY + TileSize,
                    0.0f,
                    1.0f 
                );
                
                // FRect(minX, maxX, minY, maxY)
                FRect Scissor
                (
                    (int)TilePixelX,
                    (int)TilePixelX + TileSize,
                    (int)TilePixelY,
                    (int)TilePixelY + TileSize
                );
                
                GraphicsState.SetViewportState(FViewportState(Viewport, Scissor));
                
                for (const FMeshDrawCommand& Batch : DrawCommands)
                {
                    FGraphicsPipelineDesc Desc; Desc
                        .SetDebugName("Point Light Shadow Pass")
                        .SetRenderState(RenderState)
                        .AddBindingLayout(SceneBindingLayout)
                        .AddBindingLayout(SceneBindlessLayout)
                        .SetVertexShader(VertexShader)
                        .SetPixelShader(PixelShader);
                    
                    FRHIGraphicsPipelineRef Pipeline = GRenderContext->CreateGraphicsPipeline(Desc, RenderPass);
                    
                    GraphicsState.SetPipeline(Pipeline);
                    
                    CmdList.SetGraphicsState(GraphicsState);
                    
                    CmdList.SetPushConstants(&LightShadow.LightIndex, sizeof(uint32));
                    CmdList.DrawIndirect(Batch.DrawCount, Batch.IndirectDrawOffset * sizeof(FDrawIndirectArguments));
                }
            }

            CmdList.EndRenderPass();
        });
    }

    void FForwardRenderScene::SpotShadowPass(FRenderGraph& RenderGraph)
    {
        if (PackedShadows[(uint32)ELightType::Spot].empty() || DrawCommands.empty())
        {
            return;
        }
        
        FRGPassDescriptor* Descriptor = RenderGraph.AllocDescriptor();
        RenderGraph.AddPass(RG_Raster, FRGEvent("Spot Shadow Pass"), Descriptor, [&](ICommandList& CmdList)
        {
            LUMINA_PROFILE_SECTION_COLORED("Spot Shadow Pass", tracy::Color::DeepPink4);
            
            FRHIPixelShaderRef PixelShader = FShaderLibrary::GetPixelShader("ShadowMapping.frag");
            
            FRenderState RenderState; RenderState
                .SetDepthStencilState(FDepthStencilState()
                    .SetDepthFunc(EComparisonFunc::LessOrEqual))
                    .SetRasterState(FRasterState()
                        .SetSlopeScaleDepthBias(1.75f)
                        .SetDepthBias(100)
                        .SetCullFront());
            
            
            const TVector<FLightShadow>& SpotShadows = PackedShadows[(uint32)ELightType::Spot];
            
            FRHIVertexShaderRef VertexShader = FShaderLibrary::GetVertexShader("ShadowMapping.vert");

            for (const FLightShadow& Shadow : SpotShadows)
            {
                LUMINA_PROFILE_SECTION_COLORED("Process Spot Light", tracy::Color::DeepPink);

                const FShadowTile& Tile = ShadowAtlas.GetTile(Shadow.ShadowMapIndex);
                uint32 TilePixelX = static_cast<uint32>(Tile.UVOffset.x * GShadowAtlasResolution);
                uint32 TilePixelY = static_cast<uint32>(Tile.UVOffset.y * GShadowAtlasResolution);
                uint32 TileSize = static_cast<uint32>(Tile.UVScale.x * GShadowAtlasResolution);
                
                FRenderPassDesc::FAttachment Depth; Depth
                    .SetLoadOp(ERenderLoadOp::Clear)
                    .SetDepthClearValue(1.0f)
                    .SetImage(ShadowAtlas.GetImage())
                        .SetArraySlice(6);
                
                FRenderPassDesc RenderPass; RenderPass
                    .SetDepthAttachment(Depth)
                    .SetRenderArea(glm::uvec2(GShadowAtlasResolution, GShadowAtlasResolution));
                
                FViewportState ViewportState;
                ViewportState.SetViewport((FViewport
                (
                    (float)TilePixelX,
                    (float)TilePixelX + TileSize,
                    (float)TilePixelY,
                    (float)TilePixelY + TileSize,
                    0.0f,
                    1.0f 
                )));
                
                ViewportState.SetScissorRect(FRect
                (
                    (int)TilePixelX,
                    (int)TilePixelX + TileSize,
                    (int)TilePixelY,
                    (int)TilePixelY + TileSize
                ));
                
                FGraphicsState GraphicsState; GraphicsState
                    .SetRenderPass(RenderPass)
                    .SetViewportState(ViewportState)
                    .AddBindingSet(SceneBindingSet)
                    .AddBindingSet(SceneDescriptorTable)
                    .SetIndirectParams(GetNamedBuffer(ENamedBuffer::Indirect));                    
                
                
                for (const FMeshDrawCommand& Batch : DrawCommands)
                {
                    FGraphicsPipelineDesc Desc; Desc
                        .SetDebugName("Spot Shadow Pass")
                        .SetRenderState(RenderState)
                        .AddBindingLayout(SceneBindingLayout)
                        .AddBindingLayout(SceneBindlessLayout)
                        .SetVertexShader(VertexShader)
                        .SetPixelShader(PixelShader);
                    
                    FRHIGraphicsPipelineRef Pipeline = GRenderContext->CreateGraphicsPipeline(Desc, RenderPass);
                    
                    GraphicsState.SetPipeline(Pipeline);
                    CmdList.SetGraphicsState(GraphicsState);
                    
                    CmdList.SetPushConstants(&Shadow.LightIndex, sizeof(uint32));
                    CmdList.DrawIndirect(Batch.DrawCount, Batch.IndirectDrawOffset * sizeof(FDrawIndirectArguments));
                }
            }

            CmdList.EndRenderPass();

        });
    }

    void FForwardRenderScene::CascadedShowPass(FRenderGraph& RenderGraph)
    {
        if (!LightData.bHasSun || DrawCommands.empty())
        {
            return;
        }
        
        FRGPassDescriptor* Descriptor = RenderGraph.AllocDescriptor();
        RenderGraph.AddPass(RG_Raster, FRGEvent("Cascaded Shadow Map Pass"), Descriptor, [&](ICommandList& CmdList)
        {
            LUMINA_PROFILE_SECTION_COLORED("Cascaded Shadow Map Pass", tracy::Color::DeepPink2);
            
            FRenderState RenderState; RenderState
                .SetDepthStencilState(FDepthStencilState()
                    .SetDepthFunc(EComparisonFunc::LessOrEqual))
                    .SetRasterState(FRasterState().SetCullFront());
            
            
            FRenderPassDesc::FAttachment Depth; Depth
                .SetLoadOp(ERenderLoadOp::Clear)
                .SetDepthClearValue(1.0)
                .SetImage(GetNamedImage(ENamedImage::Cascade))
                    .SetNumSlices(NumCascades - 1);
            
            FRenderPassDesc RenderPass; RenderPass
                .SetDepthAttachment(Depth)
                .SetViewMask(RenderUtils::CreateViewMask<0u, 1u, 2u>()) // Must match NUM_CASCADES
                .SetRenderArea(glm::uvec2(GCSMResolution, GCSMResolution));
            
            
            FRHIVertexShaderRef VertexShader = FShaderLibrary::GetVertexShader("ShadowMapping.vert");
            
            for (const FMeshDrawCommand& Batch : DrawCommands)
            {
                FGraphicsPipelineDesc Desc; Desc
                    .SetDebugName("Cascaded Shadow Maps")
                    .SetRenderState(RenderState)
                    .AddBindingLayout(SceneBindingLayout)
                    .AddBindingLayout(SceneBindlessLayout)
                    .SetVertexShader(VertexShader);
                
                FRHIGraphicsPipelineRef Pipeline = GRenderContext->CreateGraphicsPipeline(Desc, RenderPass);

                FGraphicsState GraphicsState; GraphicsState
                    .SetRenderPass(RenderPass)
                    .SetViewportState(MakeViewportStateFromImage(GetNamedImage(ENamedImage::Cascade)))
                    .SetPipeline(Pipeline)
                    .AddBindingSet(SceneBindingSet)
                    .AddBindingSet(SceneDescriptorTable)
                    .SetIndirectParams(GetNamedBuffer(ENamedBuffer::Indirect));
                
                CmdList.SetGraphicsState(GraphicsState);
        
                uint32 LightIndex = 0;
                CmdList.SetPushConstants(&LightIndex, sizeof(uint32));
                CmdList.DrawIndirect(Batch.DrawCount, Batch.IndirectDrawOffset * sizeof(FDrawIndirectArguments));
            }
        });
    }

    void FForwardRenderScene::BasePass(FRenderGraph& RenderGraph)
    {
        if (DrawCommands.empty())
        {
            return;
        }
        
        FRGPassDescriptor* Descriptor = RenderGraph.AllocDescriptor();
        RenderGraph.AddPass(RG_Raster, FRGEvent("Forward Base Pass"), Descriptor, [&](ICommandList& CmdList)
        {
            LUMINA_PROFILE_SECTION_COLORED("Forward Base Pass", tracy::Color::Red);
            
            FRenderPassDesc::FAttachment RenderTarget;
            RenderTarget.SetImage(GetNamedImage(ENamedImage::HDR));
            if (RenderSettings.bHasEnvironment)
            {
                RenderTarget.SetLoadOp(ERenderLoadOp::Load);
            }
            
            FRenderPassDesc::FAttachment PickerImageAttachment; PickerImageAttachment
                .SetImage(GetNamedImage(ENamedImage::Picker));
            
            FRenderPassDesc::FAttachment Depth; Depth
                .SetImage(GetNamedImage(ENamedImage::DepthAttachment))
                .SetLoadOp(ERenderLoadOp::Load);
            
            FRenderPassDesc RenderPass; RenderPass
                .AddColorAttachment(RenderTarget)
                .AddColorAttachment(PickerImageAttachment)
                .SetDepthAttachment(Depth)
                .SetRenderArea(GetNamedImage(ENamedImage::HDR)->GetExtent());
            
            
            FRasterState RasterState;
            RasterState.EnableDepthClip();
            RasterState.SetFillMode(RenderSettings.bWireframe ? ERasterFillMode::Wireframe : ERasterFillMode::Solid);
        
            FDepthStencilState DepthState; DepthState
                .SetDepthFunc(EComparisonFunc::Equal)
                .DisableDepthWrite();
            
            FRenderState RenderState;
            RenderState.SetRasterState(RasterState);
            RenderState.SetDepthStencilState(DepthState);
            
            for (const FMeshDrawCommand& Batch : DrawCommands)
            {
                FGraphicsPipelineDesc Desc; Desc
                    .SetDebugName("Forward Base Pass")
                    .SetRenderState(RenderState)
                    .SetVertexShader(Batch.VertexShader)
                    .SetPixelShader(Batch.PixelShader)
                    .AddBindingLayout(SceneBindingLayout)
                    .AddBindingLayout(SceneBindlessLayout);
                
                FGraphicsState GraphicsState; GraphicsState
                    .SetRenderPass(RenderPass)
                    .SetViewportState(SceneViewportState)
                    .SetPipeline(GRenderContext->CreateGraphicsPipeline(Desc, RenderPass))
                    .SetIndirectParams(GetNamedBuffer(ENamedBuffer::Indirect))
                    .AddBindingSet(SceneBindingSet)
                    .AddBindingSet(SceneDescriptorTable);
                
                CmdList.SetGraphicsState(GraphicsState);
                CmdList.DrawIndirect(Batch.DrawCount, Batch.IndirectDrawOffset * sizeof(FDrawIndirectArguments));
            }
        });
    }

    void FForwardRenderScene::BillboardPass(FRenderGraph& RenderGraph)
    {
#if 0
        FRGPassDescriptor* Descriptor = RenderGraph.AllocDescriptor();
        RenderGraph.AddPass(RG_Raster, FRGEvent("Billboard Pass"), Descriptor, [&](ICommandList& CmdList)
        {
            LUMINA_PROFILE_SECTION_COLORED("Billboard Pass", tracy::Color::Red);
            
            FRHIVertexShaderRef VertexShader = FShaderLibrary::GetVertexShader("Billboard.vert");
            FRHIPixelShaderRef PixelShader = FShaderLibrary::GetPixelShader("Billboard.frag");
            
            FRenderPassDesc::FAttachment RenderTarget;
            RenderTarget.SetImage(HDRRenderTarget);
            if (RenderSettings.bHasEnvironment)
            {
                RenderTarget.SetLoadOp(ERenderLoadOp::Load);
            }
            
            FRenderPassDesc::FAttachment PickerImageAttachment; PickerImageAttachment
                .SetImage(PickerImage)
                .SetLoadOp(ERenderLoadOp::Load);
            
            FRenderPassDesc::FAttachment Depth; Depth
                .SetImage(DepthAttachment)
                .SetLoadOp(ERenderLoadOp::Load);
            
            FRenderPassDesc RenderPass; RenderPass
                .AddColorAttachment(RenderTarget)
                .AddColorAttachment(PickerImageAttachment)
                .SetDepthAttachment(Depth)
                .SetRenderArea(HDRRenderTarget->GetExtent());
        
            FDepthStencilState DepthState; DepthState
                .DisableDepthWrite()
                .DisableDepthTest();
            
            FRenderState RenderState; RenderState
                .SetDepthStencilState(DepthState);

            for (const FBillboardInstance& Billboard : BillboardInstances)
            {
                FGraphicsPipelineDesc Desc; Desc
                    .SetDebugName("Billboard Pass")
                    .SetRenderState(RenderState)
                    .SetVertexShader(VertexShader)
                    .SetPixelShader(PixelShader)
                    .AddBindingLayout(BindingLayout)
                    .AddBindingLayout(BindlessLayout);
            
                FGraphicsState GraphicsState; GraphicsState
                    .SetRenderPass(RenderPass)
                    .SetViewportState(MakeViewportStateFromImage(HDRRenderTarget))
                    .SetPipeline(GRenderContext->CreateGraphicsPipeline(Desc, RenderPass))
                    .AddBindingSet(BindingSet)
                    .AddBindingSet(DescriptorTable);
            
                CmdList.SetGraphicsState(GraphicsState);
                CmdList.Draw(6, 1, 0, 0);   
            }
        });
#endif
    }

    void FForwardRenderScene::TransparentPass(FRenderGraph& RenderGraph)
    {
        
    }

    void FForwardRenderScene::EnvironmentPass(FRenderGraph& RenderGraph)
    {
        if (!RenderSettings.bHasEnvironment)
        {
            return;
        }

        FRGPassDescriptor* Descriptor = RenderGraph.AllocDescriptor();
        RenderGraph.AddPass(RG_Raster, FRGEvent("Environment Pass"), Descriptor, [&](ICommandList& CmdList)
        {
            LUMINA_PROFILE_SECTION_COLORED("Environment Pass", tracy::Color::Green3);
        
            FRHIVertexShaderRef VertexShader = FShaderLibrary::GetVertexShader("FullscreenQuad.vert");
            FRHIPixelShaderRef PixelShader = FShaderLibrary::GetPixelShader("Environment.frag");
            if (!VertexShader || !PixelShader)
            {
                return;
            }
        
            FRenderPassDesc::FAttachment Attachment; Attachment
                .SetImage(GetNamedImage(ENamedImage::HDR));
            
            FRenderPassDesc RenderPass; RenderPass
                .AddColorAttachment(Attachment)
                .SetRenderArea(GetNamedImage(ENamedImage::HDR)->GetExtent());
        
            FRasterState RasterState;
            RasterState.SetCullNone();
            
            FRenderState RenderState;
            RenderState.SetRasterState(RasterState);
        
            FGraphicsPipelineDesc Desc;
            Desc.SetDebugName("Environment Pass");
            Desc.SetRenderState(RenderState);
            Desc.AddBindingLayout(SceneBindingLayout);
            Desc.AddBindingLayout(SceneBindlessLayout);
            Desc.SetVertexShader(VertexShader);
            Desc.SetPixelShader(PixelShader);
        
            FRHIGraphicsPipelineRef Pipeline = GRenderContext->CreateGraphicsPipeline(Desc, RenderPass);
        
            FGraphicsState GraphicsState;
            GraphicsState.AddBindingSet(SceneBindingSet);
            GraphicsState.AddBindingSet(SceneDescriptorTable);
            GraphicsState.SetPipeline(Pipeline);
            GraphicsState.SetRenderPass(RenderPass);
            GraphicsState.SetViewportState(SceneViewportState);
            
        
            CmdList.SetGraphicsState(GraphicsState);
        
            CmdList.Draw(3, 1, 0, 0); 
        });
    }

    void FForwardRenderScene::BatchedLineDraw(FRenderGraph& RenderGraph)
    {
        if (SimpleVertices.empty() || LineBatches.empty())
        {
            return;
        }
    
        FRGPassDescriptor* Descriptor = RenderGraph.AllocDescriptor();
        RenderGraph.AddPass(RG_Raster, FRGEvent("Batched Line Draw"), Descriptor, [&](ICommandList& CmdList)
        {
            LUMINA_PROFILE_SECTION_COLORED("Batched Line Draw", tracy::Color::Red2);
    
            FRHIVertexShaderRef VertexShader = FShaderLibrary::GetVertexShader("SimpleElement.vert");
            FRHIPixelShaderRef PixelShader = FShaderLibrary::GetPixelShader("SimpleElement.frag");
            if (!VertexShader || !PixelShader)
            {
                return;
            }
    
            FRenderPassDesc::FAttachment RenderTarget;
            RenderTarget.SetImage(GetNamedImage(ENamedImage::HDR));
            if (!DrawCommands.empty())
            {
                RenderTarget.SetLoadOp(ERenderLoadOp::Load);
            }
    
            FRenderPassDesc::FAttachment Depth; Depth
                .SetImage(GetNamedImage(ENamedImage::DepthAttachment))
                .SetLoadOp(ERenderLoadOp::Load);
    
            FRenderPassDesc RenderPass; RenderPass
                .AddColorAttachment(RenderTarget)
                .SetDepthAttachment(Depth)
                .SetRenderArea(GetNamedImage(ENamedImage::HDR)->GetExtent());
    
            FVertexBufferBinding VertexBinding{GetNamedBuffer(ENamedBuffer::SimpleVertex)};
    
            for (const FLineBatch& Batch : LineBatches)
            {
                FRasterState RasterState; RasterState
                    .SetLineWidth(Batch.Thickness)
                    .EnableDepthClip();
    
                FDepthStencilState DepthState; DepthState
                    .SetDepthFunc(EComparisonFunc::Greater)
                    .EnableDepthWrite();
                
                if (Batch.bDepthTest)
                {
                    DepthState.EnableDepthTest();
                }
                else
                {
                    DepthState.DisableDepthTest();
                }
    
                FRenderState RenderState; RenderState
                    .SetRasterState(RasterState)
                    .SetDepthStencilState(DepthState);
                
                FGraphicsPipelineDesc Desc; Desc
                    .SetDebugName("Batched Line Draw")
                    .SetPrimType(EPrimitiveType::LineList)
                    .SetRenderState(RenderState)
                    .SetInputLayout(SimpleVertexLayoutInput)
                    .AddBindingLayout(SceneBindingLayout)
                    .AddBindingLayout(SceneBindlessLayout)
                    .SetVertexShader(VertexShader)
                    .SetPixelShader(PixelShader);
    
                FGraphicsState GraphicsState; GraphicsState
                    .SetRenderPass(RenderPass)
                    .AddVertexBuffer(VertexBinding)
                    .SetViewportState(SceneViewportState)
                    .SetPipeline(GRenderContext->CreateGraphicsPipeline(Desc, RenderPass))
                    .AddBindingSet(SceneBindingSet)
                    .AddBindingSet(SceneDescriptorTable);
    
                CmdList.SetGraphicsState(GraphicsState);
                CmdList.Draw(Batch.VertexCount, 1, Batch.StartVertex, 0);
            }
        });
    }

    void FForwardRenderScene::SelectionPass(FRenderGraph& RenderGraph)
    {
        if (World->GetSelectedEntities().empty())
        {
            return;
        }
        FRGPassDescriptor* Descriptor = RenderGraph.AllocDescriptor();
        RenderGraph.AddPass(RG_Raster, FRGEvent("Selection Post Process Pass"), Descriptor, [&](ICommandList& CmdList)
        {
            LUMINA_PROFILE_SECTION_COLORED("Selection Post Process Pass", tracy::Color::Red2);
            
            FRHIVertexShaderRef VertexShader = FShaderLibrary::GetVertexShader("FullscreenQuad.vert");
            FRHIPixelShaderRef PixelShader = FShaderLibrary::GetPixelShader("SelectionPostProcess.frag");
            if (!VertexShader || !PixelShader)
            {
                return;
            }
            
            FRenderPassDesc::FAttachment Attachment; Attachment
                .SetImage(GetNamedImage(ENamedImage::HDR))
                .SetLoadOp(ERenderLoadOp::Load);
        
            FRenderPassDesc RenderPass; RenderPass
                .AddColorAttachment(Attachment)
                .SetRenderArea(GetNamedImage(ENamedImage::HDR)->GetExtent());
            
            FRasterState RasterState;
            RasterState.SetCullNone();
            
            FDepthStencilState DepthState;
            DepthState.DisableDepthTest();
            DepthState.DisableDepthWrite();
            
            FRenderState RenderState;
            RenderState.SetRasterState(RasterState);
            RenderState.SetDepthStencilState(DepthState);
            
            FGraphicsPipelineDesc Desc;
            Desc.SetDebugName("Selection Post Process Pass");
            Desc.SetRenderState(RenderState);
            Desc.AddBindingLayout(SceneBindingLayout);
            Desc.AddBindingLayout(SceneBindlessLayout);
            Desc.SetVertexShader(VertexShader);
            Desc.SetPixelShader(PixelShader);
        
            FRHIGraphicsPipelineRef Pipeline = GRenderContext->CreateGraphicsPipeline(Desc, RenderPass);
        
            FGraphicsState GraphicsState;
            GraphicsState.SetPipeline(Pipeline);
            GraphicsState.AddBindingSet(SceneBindingSet);
            GraphicsState.AddBindingSet(SceneDescriptorTable);
            GraphicsState.SetRenderPass(RenderPass);               
            GraphicsState.SetViewportState(MakeViewportStateFromImage(GetNamedImage(ENamedImage::HDR)));
        
            CmdList.SetGraphicsState(GraphicsState);

            auto Selections = World->GetSelectedEntities();
            
            uint32 Push[32];
            Push[0] = PackColor(glm::vec4(255, 0, 0, 255));
            Push[1] = CVarSelectionThickness.GetValue();
            Push[2] = glm::min(29u, (uint32)Selections.size());
            uint32 Start = 3;
            
            for (int i = 0; std::cmp_less(i, Push[2]); ++i)
            {
                Push[Start++] = entt::to_integral(Selections[i]);
            }

            CmdList.SetPushConstants(Push, sizeof(Push));
            CmdList.Draw(3, 1, 0, 0); 
        });
    }

    void FForwardRenderScene::ToneMappingPass(FRenderGraph& RenderGraph)
    {
        FRGPassDescriptor* Descriptor = RenderGraph.AllocDescriptor();
        RenderGraph.AddPass(RG_Raster, FRGEvent("Tone Mapping Pass"), Descriptor, [&](ICommandList& CmdList)
        {
            LUMINA_PROFILE_SECTION_COLORED("Tone Mapping Pass", tracy::Color::Red2);
            
            FRHIVertexShaderRef VertexShader = FShaderLibrary::GetVertexShader("FullscreenQuad.vert");
            FRHIPixelShaderRef PixelShader = FShaderLibrary::GetPixelShader("ToneMapping.frag");
            if (!VertexShader || !PixelShader)
            {
                return;
            }
            
            FRenderPassDesc::FAttachment Attachment; Attachment
                .SetImage(GetRenderTarget());
        
            FRenderPassDesc RenderPass; RenderPass
                .AddColorAttachment(Attachment)
                .SetRenderArea(GetRenderTarget()->GetExtent());
        
        
            FRasterState RasterState;
            RasterState.SetCullNone();
            
            FDepthStencilState DepthState;
            DepthState.DisableDepthTest();
            DepthState.DisableDepthWrite();
            
            FRenderState RenderState;
            RenderState.SetRasterState(RasterState);
            RenderState.SetDepthStencilState(DepthState);
            
            FGraphicsPipelineDesc Desc;
            Desc.SetDebugName("Tone Mapping Pass");
            Desc.SetRenderState(RenderState);
            Desc.AddBindingLayout(SceneBindingLayout);
            Desc.AddBindingLayout(SceneBindlessLayout);
            Desc.SetVertexShader(VertexShader);
            Desc.SetPixelShader(PixelShader);
        
            FRHIGraphicsPipelineRef Pipeline = GRenderContext->CreateGraphicsPipeline(Desc, RenderPass);
        
            FGraphicsState GraphicsState;
            GraphicsState.SetPipeline(Pipeline);
            GraphicsState.AddBindingSet(SceneBindingSet);
            GraphicsState.AddBindingSet(SceneDescriptorTable);
            GraphicsState.SetRenderPass(RenderPass);               
            GraphicsState.SetViewportState(MakeViewportStateFromImage(GetRenderTarget()));
        
            CmdList.SetGraphicsState(GraphicsState);

            glm::vec2 PC;
            PC.x = 1.0;
            PC.y = SceneGlobalData.Time;
            CmdList.SetPushConstants(&PC, sizeof(glm::vec2));
            CmdList.Draw(3, 1, 0, 0); 
        });
    }

    void FForwardRenderScene::DebugDrawPass(FRenderGraph& RenderGraph)
    {
#if 0
        if (RenderSettings.Flags == ERenderSceneDebugFlags::None)
        {
            return;
        }
        
        FRGPassDescriptor* Descriptor = RenderGraph.AllocDescriptor();
        RenderGraph.AddPass(RG_Raster, FRGEvent("Debug Draw Pass"), Descriptor, [&](ICommandList& CmdList)
        {
            LUMINA_PROFILE_SECTION_COLORED("Debug Draw Pass", tracy::Color::Red2);
        
            FRHIVertexShaderRef VertexShader = FShaderLibrary::GetVertexShader("FullscreenQuad.vert");
            FRHIPixelShaderRef PixelShader = FShaderLibrary::GetPixelShader("Debug.frag");
            if (!VertexShader || !PixelShader)
            {
                return;
            }
        
            FRenderPassDesc::FAttachment Attachment; Attachment
                .SetLoadOp(ERenderLoadOp::Load)
                .SetImage(GetRenderTarget());
        
            FRenderPassDesc RenderPass; RenderPass
                .AddColorAttachment(Attachment)
                .SetRenderArea(GetRenderTarget()->GetExtent());
        
        
            FRasterState RasterState;
            RasterState.SetCullNone();
        
            FDepthStencilState DepthState;
            DepthState.DisableDepthTest();
            DepthState.DisableDepthWrite();
        
            FRenderState RenderState;
            RenderState.SetRasterState(RasterState);
            RenderState.SetDepthStencilState(DepthState);
        
            FGraphicsPipelineDesc Desc;
            Desc.SetDebugName("Debug Draw Pass");
            Desc.SetRenderState(RenderState);
            Desc.AddBindingLayout(SceneBindingLayout);
            Desc.AddBindingLayout(DebugPassLayout);
            Desc.SetVertexShader(VertexShader);
            Desc.SetPixelShader(PixelShader);
        
            FRHIGraphicsPipelineRef Pipeline = GRenderContext->CreateGraphicsPipeline(Desc, RenderPass);
        
            FGraphicsState GraphicsState;
            GraphicsState.SetPipeline(Pipeline);
            GraphicsState.AddBindingSet(BindingSet);
            GraphicsState.AddBindingSet(DebugPassSet);
            GraphicsState.SetRenderPass(RenderPass);               
            GraphicsState.SetViewportState(MakeViewportStateFromImage(GetRenderTarget()));
        
            CmdList.SetGraphicsState(GraphicsState);
        
            uint32 Mode = static_cast<uint32>(RenderSettings.Flags);
            CmdList.SetPushConstants(&Mode, sizeof(uint32));
            CmdList.Draw(3, 1, 0, 0); 
        });
#endif
    }
    
    void FForwardRenderScene::InitBuffers()
    {
        {
            FRHIBufferDesc BufferDesc;
            BufferDesc.Size = sizeof(FSceneGlobalData);
            BufferDesc.Usage.SetFlag(BUF_UniformBuffer);
            BufferDesc.bKeepInitialState = true;
            BufferDesc.InitialState = EResourceStates::ShaderResource;
            BufferDesc.DebugName = "Scene Global Data";
            NamedBuffers[(int)ENamedBuffer::Scene] = GRenderContext->CreateBuffer(BufferDesc);
        }
        
        {
            FRHIBufferDesc BufferDesc;
            BufferDesc.Size = sizeof(FMaterialUniforms);
            BufferDesc.Usage.SetFlag(BUF_StorageBuffer);
            BufferDesc.bKeepInitialState = true;
            BufferDesc.InitialState = EResourceStates::ShaderResource;
            BufferDesc.DebugName = "Material Uniform Data";
            NamedBuffers[(int)ENamedBuffer::Materials] = GRenderContext->CreateBuffer(BufferDesc);
        }

        {
            FRHIBufferDesc BufferDesc;
            BufferDesc.Size = sizeof(FInstanceData);
            BufferDesc.Usage.SetFlag(BUF_StorageBuffer);
            BufferDesc.bKeepInitialState = true;
            BufferDesc.InitialState = EResourceStates::ShaderResource;
            BufferDesc.DebugName = "Instance Buffer";
            NamedBuffers[(int)ENamedBuffer::Instance] = GRenderContext->CreateBuffer(BufferDesc);
        }
        
        {
            FRHIBufferDesc BufferDesc;
            BufferDesc.Size = sizeof(glm::mat4) * 255 * 1'000;
            BufferDesc.Usage.SetFlag(BUF_StorageBuffer);
            BufferDesc.bKeepInitialState = true;
            BufferDesc.InitialState = EResourceStates::ShaderResource;
            BufferDesc.DebugName = "Bone Data Buffer";
            NamedBuffers[(int)ENamedBuffer::Bone] = GRenderContext->CreateBuffer(BufferDesc);
        }

        {
            FRHIBufferDesc BufferDesc;
            BufferDesc.Size = sizeof(uint32);
            BufferDesc.Usage.SetFlag(BUF_StorageBuffer);
            BufferDesc.bKeepInitialState = true;
            BufferDesc.InitialState = EResourceStates::UnorderedAccess;
            BufferDesc.DebugName = "Instance Mapping";
            NamedBuffers[(int)ENamedBuffer::InstanceMapping] = GRenderContext->CreateBuffer(BufferDesc);
        }

        {
            FRHIBufferDesc BufferDesc;
            BufferDesc.Size = offsetof(FSceneLightData, Lights);
            BufferDesc.Usage.SetFlag(BUF_StorageBuffer);
            BufferDesc.bKeepInitialState = true;
            BufferDesc.InitialState = EResourceStates::ShaderResource;
            BufferDesc.DebugName = "Light Data Buffer";
            NamedBuffers[(int)ENamedBuffer::Light] = GRenderContext->CreateBuffer(BufferDesc);
        }

        {
            FRHIBufferDesc BufferDesc;
            BufferDesc.Size = sizeof(FCluster) * NumClusters;
            BufferDesc.Usage.SetFlag(BUF_StorageBuffer);
            BufferDesc.bKeepInitialState = true;
            BufferDesc.InitialState = EResourceStates::UnorderedAccess;
            BufferDesc.DebugName = "Cluster SSBO";
            NamedBuffers[(int)ENamedBuffer::Cluster] = GRenderContext->CreateBuffer(BufferDesc);
        }

        {
            FRHIBufferDesc BufferDesc;
            BufferDesc.Size = sizeof(FSimpleElementVertex);
            BufferDesc.Usage.SetFlag(BUF_VertexBuffer);
            BufferDesc.bKeepInitialState = true;
            BufferDesc.InitialState = EResourceStates::VertexBuffer;
            BufferDesc.DebugName = "Simple Element Vertex";
            NamedBuffers[(int)ENamedBuffer::SimpleVertex] = GRenderContext->CreateBuffer(BufferDesc);
        }

        {
            FRHIBufferDesc BufferDesc;
            BufferDesc.Size = sizeof(FDrawIndirectArguments);
            BufferDesc.Stride = sizeof(FDrawIndirectArguments);
            BufferDesc.Usage.SetMultipleFlags(BUF_Indirect, BUF_StorageBuffer);
            BufferDesc.InitialState = EResourceStates::IndirectArgument;
            BufferDesc.bKeepInitialState = true;
            BufferDesc.DebugName = "Indirect Draw Buffer";
            NamedBuffers[(int)ENamedBuffer::Indirect] = GRenderContext->CreateBuffer(BufferDesc);
        }
    }

    static uint32 PreviousPow2(uint32 v)
    {
        uint32_t r = 1;

        while (r * 2 < v)
        {
            r *= 2;
        }

        return r;
    }

    void FForwardRenderScene::InitImages()
    {
        glm::uvec2 Extent = Windowing::GetPrimaryWindowHandle()->GetExtent();
        
        {
            FRHIImageDesc ImageDesc = GetRenderTarget()->GetDescription();
            ImageDesc.Format = EFormat::RGBA16_FLOAT;
            NamedImages[(int)ENamedImage::HDR] = GRenderContext->CreateImage(ImageDesc);
        }
        
        //==================================================================================================
        
        {
            FRHIImageDesc ImageDesc;
            ImageDesc.Extent = Extent;
            ImageDesc.Flags.SetMultipleFlags(EImageCreateFlags::DepthAttachment, EImageCreateFlags::ShaderResource);
            ImageDesc.Format = EFormat::D32;
            ImageDesc.InitialState = EResourceStates::DepthRead;
            ImageDesc.bKeepInitialState = true;
            ImageDesc.Dimension = EImageDimension::Texture2D;
            ImageDesc.DebugName = "Depth Attachment";
        
            NamedImages[(int)ENamedImage::DepthAttachment] = GRenderContext->CreateImage(ImageDesc);
        }

        //==================================================================================================

        {
            uint32 Width = PreviousPow2(Extent.x);
            uint32 Height = PreviousPow2(Extent.y);
            
            FRHIImageDesc ImageDesc;
            ImageDesc.Flags.SetMultipleFlags(EImageCreateFlags::ShaderResource, EImageCreateFlags::Storage);
            ImageDesc.Extent            = glm::uvec2(Width, Height);
            ImageDesc.Format            = EFormat::R32_FLOAT;
            ImageDesc.NumMips           = (uint8)RenderUtils::CalculateMipCount(Width, Height);
            ImageDesc.InitialState      = EResourceStates::ShaderResource;
            ImageDesc.bKeepInitialState = true;
            ImageDesc.Dimension         = EImageDimension::Texture2D;
            ImageDesc.DebugName         = "Depth Pyramid";
            
            NamedImages[(int)ENamedImage::DepthPyramid]                = GRenderContext->CreateImage(ImageDesc);
        }

        //==================================================================================================
        
        {
            FRHIImageDesc ImageDesc;
            ImageDesc.Extent = Extent;
            ImageDesc.Format = EFormat::RG32_UINT;
            ImageDesc.Dimension = EImageDimension::Texture2D;
            ImageDesc.InitialState = EResourceStates::RenderTarget;
            ImageDesc.bKeepInitialState = true;
            ImageDesc.Flags.SetMultipleFlags(EImageCreateFlags::ColorAttachment, EImageCreateFlags::ShaderResource);
            ImageDesc.DebugName = "Picker";
            
            NamedImages[(int)ENamedImage::Picker] = GRenderContext->CreateImage(ImageDesc);
        }
        
        //==================================================================================================
        
        {
            FRHIImageDesc ImageDesc = {};
            ImageDesc.Extent = glm::uvec2(GCSMResolution, GCSMResolution);
            ImageDesc.Format = EFormat::D32;
            ImageDesc.Dimension = EImageDimension::Texture2DArray;
            ImageDesc.InitialState = EResourceStates::DepthWrite;
            ImageDesc.bKeepInitialState = true;
            ImageDesc.ArraySize = NumCascades;
            ImageDesc.Flags.SetMultipleFlags(EImageCreateFlags::DepthAttachment, EImageCreateFlags::ShaderResource);
            ImageDesc.DebugName = "ShadowCascadeMap";
            
            NamedImages[(int)ENamedImage::Cascade] = GRenderContext->CreateImage(ImageDesc);
        }
        
        //==================================================================================================
        
    }

    void FForwardRenderScene::InitFrameResources()
    {
        InitImages();
        
        float SizeY = (float)GetNamedImage(ENamedImage::HDR)->GetSizeY();
        float SizeX = (float)GetNamedImage(ENamedImage::HDR)->GetSizeX();

        SceneViewportState.Viewports.emplace_back(FViewport(SizeX, SizeY));
        SceneViewportState.Scissors.emplace_back(FRect((int)SizeX, (int)SizeY));
        
        FVertexAttributeDesc VertexDesc[2];
        VertexDesc[0].SetElementStride(sizeof(FSimpleElementVertex));
        VertexDesc[0].SetOffset(offsetof(FSimpleElementVertex, Position));
        VertexDesc[0].Format = EFormat::RGBA32_FLOAT;
        
        VertexDesc[1].SetElementStride(sizeof(FSimpleElementVertex));
        VertexDesc[1].SetOffset(offsetof(FSimpleElementVertex, Color));
        VertexDesc[1].Format = EFormat::R32_UINT;
        
        SimpleVertexLayoutInput = GRenderContext->CreateInputLayout(VertexDesc, eastl::size(VertexDesc));
        
        CreateLayouts();
    }

    void FForwardRenderScene::CreateLayouts()
    {
                
        {
            FBindingSetDesc BindingSetDesc;
            BindingSetDesc.AddItem(FBindingSetItem::BufferCBV(0, GetNamedBuffer(ENamedBuffer::Scene)));
            BindingSetDesc.AddItem(FBindingSetItem::BufferUAV(1, GetNamedBuffer(ENamedBuffer::Light)));
            BindingSetDesc.AddItem(FBindingSetItem::BufferUAV(2, GetNamedBuffer(ENamedBuffer::Instance)));
            BindingSetDesc.AddItem(FBindingSetItem::BufferUAV(3, GetNamedBuffer(ENamedBuffer::InstanceMapping)));
            BindingSetDesc.AddItem(FBindingSetItem::BufferUAV(4, GetNamedBuffer(ENamedBuffer::Indirect)));
            BindingSetDesc.AddItem(FBindingSetItem::BufferUAV(5, GetNamedBuffer(ENamedBuffer::Bone)));
            BindingSetDesc.AddItem(FBindingSetItem::BufferUAV(6, GetNamedBuffer(ENamedBuffer::Cluster)));
            BindingSetDesc.AddItem(FBindingSetItem::BufferUAV(7, GetNamedBuffer(ENamedBuffer::Materials)));
            BindingSetDesc.AddItem(FBindingSetItem::TextureSRV(8, GetNamedImage(ENamedImage::Cascade)));
            BindingSetDesc.AddItem(FBindingSetItem::TextureSRV(9, ShadowAtlas.GetImage()));
            BindingSetDesc.AddItem(FBindingSetItem::TextureSRV(10, GetNamedImage(ENamedImage::Picker), TStaticRHISampler<false, false>::GetRHI()));
            BindingSetDesc.AddItem(FBindingSetItem::TextureSRV(11, GetNamedImage(ENamedImage::DepthPyramid), TStaticRHISampler<false, false>::GetRHI()));
            BindingSetDesc.AddItem(FBindingSetItem::TextureSRV(12, GetNamedImage(ENamedImage::HDR)));

            TBitFlags<ERHIShaderType> Visibility;
            Visibility.SetMultipleFlags(ERHIShaderType::Vertex, ERHIShaderType::Fragment, ERHIShaderType::Compute);
            GRenderContext->CreateBindingSetAndLayout(Visibility, 0, BindingSetDesc, SceneBindingLayout, SceneBindingSet);
        }
        
        {
            FBindlessLayoutDesc BindlessLayoutDesc;
            BindlessLayoutDesc.AddBinding(FBindingLayoutItem::Texture_SRV(0));
            BindlessLayoutDesc.SetVisibility(ERHIShaderType::Fragment);
            BindlessLayoutDesc.SetVisibility(ERHIShaderType::Vertex);
            BindlessLayoutDesc.SetMaxCapacity(1024);
            
            SceneBindlessLayout = GRenderContext->CreateBindlessLayout(BindlessLayoutDesc);
            SceneDescriptorTable = GRenderContext->CreateDescriptorTable(SceneBindlessLayout);
        }
    }

    FViewportState FForwardRenderScene::MakeViewportStateFromImage(const FRHIImage* Image)
    {
        float SizeY = (float)Image->GetSizeY();
        float SizeX = (float)Image->GetSizeX();

        FViewportState ViewportState;
        ViewportState.Viewports.emplace_back(FViewport(SizeX, SizeY));
        ViewportState.Scissors.emplace_back(FRect(SizeX, SizeY));

        return ViewportState;
    }

    FRHIImage* FForwardRenderScene::GetRenderTarget() const
    {
        return SceneViewport->GetRenderTarget();
    }

    const FSceneRenderStats& FForwardRenderScene::GetRenderStats() const
    {
        return RenderStats;
    }

    FSceneRenderSettings& FForwardRenderScene::GetSceneRenderSettings()
    {
        return RenderSettings;
    }

    entt::entity FForwardRenderScene::GetEntityAtPixel(uint32 X, uint32 Y) const
    {
        FRHIImage* PickerImage = GetNamedImage(ENamedImage::Picker);
        if (!PickerImage)
        {
            return entt::null;
        }

        FRHICommandListRef CommandList = GRenderContext->CreateCommandList(FCommandListInfo::Graphics());
        CommandList->Open();

        FRHIStagingImageRef StagingImage = GRenderContext->CreateStagingImage(PickerImage->GetDescription(), ERHIAccess::HostRead);
        CommandList->CopyImage(PickerImage, FTextureSlice(), StagingImage, FTextureSlice());

        CommandList->Close();
        GRenderContext->ExecuteCommandList(CommandList);

        size_t RowPitch = 0;
        void* MappedMemory = GRenderContext->MapStagingTexture(StagingImage, FTextureSlice(), ERHIAccess::HostRead, &RowPitch);
        if (!MappedMemory)
        {
            return entt::null;
        }

        const uint32 Width  = PickerImage->GetDescription().Extent.x;
        const uint32 Height = PickerImage->GetDescription().Extent.y;

        if (X >= Width || Y >= Height)
        {
            GRenderContext->UnMapStagingTexture(StagingImage);
            return entt::null;
        }

        uint8* RowStart = static_cast<uint8*>(MappedMemory) + Y * RowPitch;
        glm::uvec2* PixelPtr = reinterpret_cast<glm::uvec2*>(RowStart) + X;
        glm::uvec2 PixelValue = *PixelPtr;

        GRenderContext->UnMapStagingTexture(StagingImage);

        if (PixelValue.x == 0)
        {
            return entt::null;
        }

        return static_cast<entt::entity>(PixelValue.x);
    }

    THashSet<entt::entity> FForwardRenderScene::GetEntitiesInPixelRange(uint32 MinX, uint32 MinY, uint32 MaxX, uint32 MaxY) const
    {
        THashSet<entt::entity> entities;
        
        FRHIImage* PickerImage = GetNamedImage(ENamedImage::Picker);
        if (!PickerImage)
        {
            return entities;
        }
    
        FRHICommandListRef CommandList = GRenderContext->CreateCommandList(FCommandListInfo::Graphics());
        CommandList->Open();
    
        FRHIStagingImageRef StagingImage = GRenderContext->CreateStagingImage(PickerImage->GetDescription(), ERHIAccess::HostRead);
        CommandList->CopyImage(PickerImage, FTextureSlice(), StagingImage, FTextureSlice());
    
        CommandList->Close();
        GRenderContext->ExecuteCommandList(CommandList);
    
        size_t RowPitch = 0;
        void* MappedMemory = GRenderContext->MapStagingTexture(StagingImage, FTextureSlice(), ERHIAccess::HostRead, &RowPitch);
        if (!MappedMemory)
        {
            return entities;
        }
    
        const uint32 Width  = PickerImage->GetDescription().Extent.x;
        const uint32 Height = PickerImage->GetDescription().Extent.y;
    
        MinX = glm::clamp(MinX, 0u, Width - 1);
        MinY = glm::clamp(MinY, 0u, Height - 1);
        MaxX = glm::clamp(MaxX, 0u, Width - 1);
        MaxY = glm::clamp(MaxY, 0u, Height - 1);
    
        if (MinX > MaxX)
        {
            std::swap(MinX, MaxX);
        }
        if (MinY > MaxY)
        {
            std::swap(MinY, MaxY);
        }
    
        struct EntityBounds
        {
            uint32 MinX = UINT32_MAX;
            uint32 MinY = UINT32_MAX;
            uint32 MaxX = 0;
            uint32 MaxY = 0;
        };
        
        THashMap<entt::entity, EntityBounds> entityBoundsMap;
    
        for (uint32 y = 0; y < Height; ++y)
        {
            uint8* RowStart = static_cast<uint8*>(MappedMemory) + y * RowPitch;
            
            for (uint32 x = 0; x < Width; ++x)
            {
                glm::uvec2* PixelPtr = reinterpret_cast<glm::uvec2*>(RowStart) + x;
                glm::uvec2 PixelValue = *PixelPtr;
    
                if (PixelValue.x != 0)
                {
                    entt::entity entity = static_cast<entt::entity>(PixelValue.x);
                    
                    EntityBounds& bounds = entityBoundsMap[entity];
                    bounds.MinX = glm::min(bounds.MinX, x);
                    bounds.MinY = glm::min(bounds.MinY, y);
                    bounds.MaxX = glm::max(bounds.MaxX, x);
                    bounds.MaxY = glm::max(bounds.MaxY, y);
                }
            }
        }
    
        GRenderContext->UnMapStagingTexture(StagingImage);
    
        for (const auto& [entity, bounds] : entityBoundsMap)
        {
            if (bounds.MinX >= MinX && bounds.MaxX <= MaxX &&bounds.MinY >= MinY && bounds.MaxY <= MaxY)
            {
                entities.insert(entity);
            }
        }
    
        return entities;
    }
}
