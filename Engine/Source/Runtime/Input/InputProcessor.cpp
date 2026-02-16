#include "pch.h"
#include "InputProcessor.h"
#include "Core/Windows/Window.h"
#include "Events/Event.h"
#if WITH_EDITOR
#include "imgui.h"
#endif

namespace Lumina
{
	FInputProcessor& FInputProcessor::Get()
	{
		static FInputProcessor Instance;
		return Instance;
	}

	bool FInputProcessor::OnEvent(FEvent& Event)
	{
		if (Event.IsA<FMouseMovedEvent>())
		{
			FMouseMovedEvent& MouseEvent = Event.As<FMouseMovedEvent>();
			MouseDeltaX += MouseEvent.GetDeltaX();
			MouseDeltaY += MouseEvent.GetDeltaY();

			MouseX = MouseEvent.GetX();
			MouseY = MouseEvent.GetY();
		}
		else if (Event.IsA<FMouseButtonPressedEvent>())
		{
			FMouseButtonEvent& MouseButtonEvent = Event.As<FMouseButtonPressedEvent>();
			uint32 MouseCode = static_cast<uint32>(MouseButtonEvent.GetButton());
			MouseStates[MouseCode] = static_cast<Input::EMouseState>(GLFW_PRESS);
		}
		else if (Event.IsA<FMouseButtonReleasedEvent>())
		{
			FMouseButtonEvent& MouseButtonEvent = Event.As<FMouseButtonPressedEvent>();
			uint32 MouseCode = static_cast<uint32>(MouseButtonEvent.GetButton());
			MouseStates[MouseCode] = static_cast<Input::EMouseState>(GLFW_RELEASE);
		}
		else if (Event.IsA<FKeyPressedEvent>())
		{
			FKeyPressedEvent& KeyEvent = Event.As<FKeyPressedEvent>();
			uint32 KeyCode = static_cast<uint32>(KeyEvent.GetKeyCode());
			KeyStates[KeyCode] = KeyEvent.IsRepeat() ? static_cast<Input::EKeyState>(GLFW_REPEAT) : static_cast<Input::EKeyState>(GLFW_PRESS);
		}
		else if (Event.IsA<FKeyReleasedEvent>())
		{
			FKeyReleasedEvent& KeyEvent = Event.As<FKeyReleasedEvent>();
			uint32 KeyCode = static_cast<uint32>(KeyEvent.GetKeyCode());
			KeyStates[KeyCode] = static_cast<Input::EKeyState>(GLFW_RELEASE);
		}
		else if (Event.IsA<FMouseScrolledEvent>())
		{
			FMouseScrolledEvent& MouseEvent = Event.As<FMouseScrolledEvent>();
			MouseZ = MouseEvent.GetOffset();
			
			uint32 KeyCode = static_cast<uint32>(MouseEvent.GetCode());

			//Gauge direction with offset.
			MouseStates[KeyCode] = MouseZ > 0.0 ? Input::EMouseState::Up : Input::EMouseState::Down;
		}

		// We never want to absorb here.
		return false;
	}
	
	void FInputProcessor::SetMouseMode(EMouseMode Mode)
	{
		int DesiredInputMode = GLFW_CURSOR_NORMAL;
		switch (Mode)
		{
		case EMouseMode::Hidden:
			DesiredInputMode = GLFW_CURSOR_HIDDEN;
			break;
		case EMouseMode::Normal:
			DesiredInputMode = GLFW_CURSOR_NORMAL;
			break;
		case EMouseMode::Captured:
			DesiredInputMode = GLFW_CURSOR_DISABLED;
			break;
		}
		
		glfwSetInputMode(Windowing::GetPrimaryWindowHandle()->GetWindow(), GLFW_CURSOR, DesiredInputMode);
		
		#if WITH_EDITOR
		ImGuiIO& IO = ImGui::GetIO();
		if (Mode == EMouseMode::Captured || Mode == EMouseMode::Hidden)
		{
			IO.ConfigFlags |= ImGuiConfigFlags_NoMouse;
			IO.MouseDrawCursor = false;
		}
		else
		{
			IO.ConfigFlags &= ~ImGuiConfigFlags_NoMouse;
			IO.MouseDrawCursor = true;
		}
		#endif
	}

	void FInputProcessor::EndFrame()
	{
		MouseDeltaX = 0.0;
		MouseDeltaY = 0.0;

		MouseZ = 0.0;
	}
}
