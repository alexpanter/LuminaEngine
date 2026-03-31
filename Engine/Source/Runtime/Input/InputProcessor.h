#pragma once
#include "Input/Input.h"
#include "Events/EventProcessor.h"

namespace Lumina
{
    class FInputProcessor : public IEventHandler
    {
    public:
    	
    	LE_NO_COPYMOVE(FInputProcessor);
    	FInputProcessor() = default;
    	virtual ~FInputProcessor() = default;

        RUNTIME_API static FInputProcessor& Get();
        
		// IEventHandler interface
        bool OnEvent(FEvent& Event) override;
		// End of IEventHandler interface
    	
		RUNTIME_API double GetMouseX() const { return MouseX; }
		RUNTIME_API double GetMouseY() const { return MouseY; }
        RUNTIME_API double GetMouseZ() const { return MouseZ; }
		RUNTIME_API double GetMouseDeltaX() const { return MouseDeltaX; }
        RUNTIME_API double GetMouseDeltaY() const { return MouseDeltaY; }

        RUNTIME_API Input::EKeyState GetKeyState(EKey KeyCode) const { return KeyStates[static_cast<uint32>(KeyCode)]; }
		RUNTIME_API Input::EMouseState GetMouseButtonState(EMouseKey MouseCode) const { return MouseStates[static_cast<uint32>(MouseCode)]; }

    	RUNTIME_API bool IsKeyDown(EKey KeyCode) const { return GetKeyState(KeyCode) == Input::EKeyState::Pressed || GetKeyState(KeyCode) == Input::EKeyState::Held || GetKeyState(KeyCode) == Input::EKeyState::Repeated; }
    	RUNTIME_API bool IsKeyUp(EKey KeyCode) const { return GetKeyState(KeyCode) == Input::EKeyState::Up; }
    	RUNTIME_API bool IsKeyPressed(EKey KeyCode) const { return GetKeyState(KeyCode) == Input::EKeyState::Pressed; }
    	RUNTIME_API bool IsKeyReleased(EKey KeyCode) const { return GetKeyState(KeyCode) == Input::EKeyState::Released; }
    	RUNTIME_API bool IsKeyRepeated(EKey KeyCode) const { return GetKeyState(KeyCode) == Input::EKeyState::Repeated; }

    	RUNTIME_API bool IsMouseButtonDown(EMouseKey MouseCode) const { return GetMouseButtonState(MouseCode) == Input::EMouseState::Pressed || GetMouseButtonState(MouseCode) == Input::EMouseState::Held; }
    	RUNTIME_API bool IsMouseButtonUp(EMouseKey MouseCode) const { return GetMouseButtonState(MouseCode) == Input::EMouseState::Up; }
    	RUNTIME_API bool IsMouseButtonPressed(EMouseKey MouseCode) const { return GetMouseButtonState(MouseCode) == Input::EMouseState::Pressed; }
    	RUNTIME_API bool IsMouseButtonReleased(EMouseKey MouseCode) const { return GetMouseButtonState(MouseCode) == Input::EMouseState::Released; }
    	RUNTIME_API float GetMouseButtonHeldTime(EMouseKey MouseCode) const { return MouseKeyDownTimes[static_cast<uint32>(MouseCode)]; }

		RUNTIME_API void SetMouseMode(EMouseMode Mode);

        void EndFrame();

    private:
    	
        double MouseX = 0.0;
        double MouseY = 0.0;
        double MouseZ = 0.0;
    	
        double MouseDeltaX = 0.0;
        double MouseDeltaY = 0.0;
    	
    	TArray<float, (uint32)EMouseKey::Num>					MouseKeyDownTimes = {};

        TArray<Input::EKeyState, (uint32)EKey::Num>				KeyStates = {};
        TArray<Input::EMouseState, (uint32)EMouseKey::Num>		MouseStates = {};
    };
}
