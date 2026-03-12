#pragma once
#include "Input/Input.h"
#include "Events/EventProcessor.h"

namespace Lumina
{
    class FInputProcessor : public IEventHandler
    {
    public:

        RUNTIME_API static FInputProcessor& Get();
        
		// IEventHandler interface
        bool OnEvent(FEvent& Event) override;
		// End of IEventHandler interface
    	
		RUNTIME_API FORCEINLINE double GetMouseX() const { return MouseX; }
		RUNTIME_API FORCEINLINE double GetMouseY() const { return MouseY; }
        RUNTIME_API FORCEINLINE double GetMouseZ() const { return MouseZ; }
		RUNTIME_API FORCEINLINE double GetMouseDeltaX() const { return MouseDeltaX; }
        RUNTIME_API FORCEINLINE double GetMouseDeltaY() const { return MouseDeltaY; }

        RUNTIME_API FORCEINLINE Input::EKeyState GetKeyState(EKey KeyCode) const { return KeyStates[static_cast<uint32>(KeyCode)]; }
		RUNTIME_API FORCEINLINE Input::EMouseState GetMouseButtonState(EMouseKey MouseCode) const { return MouseStates[static_cast<uint32>(MouseCode)]; }

        RUNTIME_API FORCEINLINE bool IsKeyDown(EKey KeyCode) const { return GetKeyState(KeyCode) == Input::EKeyState::Down || GetKeyState(KeyCode) == Input::EKeyState::Repeated; }
		RUNTIME_API FORCEINLINE bool IsKeyUp(EKey KeyCode) const { return GetKeyState(KeyCode) == Input::EKeyState::Up; }
    	RUNTIME_API FORCEINLINE bool IsKeyPressed(EKey KeyCode) const { return GetKeyState(KeyCode) == Input::EKeyState::Down; }
		RUNTIME_API FORCEINLINE bool IsKeyRepeated(EKey KeyCode) const { return GetKeyState(KeyCode) == Input::EKeyState::Repeated; }

		RUNTIME_API FORCEINLINE bool IsMouseButtonDown(EMouseKey MouseCode) const { return GetMouseButtonState(MouseCode) == Input::EMouseState::Down; }
		RUNTIME_API FORCEINLINE bool IsMouseButtonUp(EMouseKey MouseCode) const { return GetMouseButtonState(MouseCode) == Input::EMouseState::Up; }

		RUNTIME_API void SetMouseMode(EMouseMode Mode);

        void EndFrame();

    private:
    	
        double MouseX = 0.0;
        double MouseY = 0.0;
        double MouseZ = 0.0;
    	
        double MouseDeltaX = 0.0;
        double MouseDeltaY = 0.0;

        TArray<Input::EKeyState, (uint32)EKey::Num>			KeyStates = {};
        TArray<Input::EMouseState, (uint32)EMouseKey::Num>		MouseStates = {};
    };
}
