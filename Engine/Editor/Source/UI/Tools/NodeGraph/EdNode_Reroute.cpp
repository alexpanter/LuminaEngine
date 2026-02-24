#include "EdNode_Reroute.h"
#include "Core/Object/Cast.h"
#include "Material/MaterialInput.h"
#include "Material/MaterialOutput.h"

namespace Lumina
{
    void CEdNode_Reroute::BuildNode()
    {
        Output = Cast<CMaterialOutput>(CreatePin(CMaterialOutput::StaticClass(), "", ENodePinDirection::Output));
        Output->SetShouldDrawEditor(false);

        Input = Cast<CMaterialInput>(CreatePin(CMaterialInput::StaticClass(), "", ENodePinDirection::Input));
        Input->SetShouldDrawEditor(false); 
    }
}
