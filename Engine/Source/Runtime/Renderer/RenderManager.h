#pragma once
#include "Core/Delegates/Delegate.h"
#include "Subsystems/Subsystem.h"


namespace Lumina
{
    class FRenderGraph;
    class IImGuiRenderer;
    class IRenderContext;
}

namespace Lumina
{
    class FRenderManager
    {
    public:

        static TMulticastDelegate<void, glm::vec2> OnSwapchainResized;

        FRenderManager();
        ~FRenderManager();
        
        void Initialize();

        void FrameStart(const FUpdateContext& UpdateContext);
        void FrameEnd(const FUpdateContext& UpdateContext, FRenderGraph& RenderGraph);

        void SwapchainResized(glm::vec2 NewSize);


        #if WITH_EDITOR
        IImGuiRenderer* GetImGuiRenderer() const { return ImGuiRenderer; }
        #endif

        uint32 GetCurrentFrameIndex() const { return CurrentFrameIndex; }
        
    private:
        
        #if WITH_EDITOR
        IImGuiRenderer*     ImGuiRenderer = nullptr;
        #endif
        
        uint8               CurrentFrameIndex = 0;
    };
    
    
    RUNTIME_API extern FRenderManager* GRenderManager;
}
