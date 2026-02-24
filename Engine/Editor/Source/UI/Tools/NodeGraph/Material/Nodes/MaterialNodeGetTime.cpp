#include "MaterialNodeGetTime.h"


#include "UI/Tools/NodeGraph/Material/MaterialCompiler.h"

namespace Lumina
{
    void CMaterialNodeGetTime::GenerateDefinition(FMaterialCompiler& Compiler)
    {
        Compiler.Time(FullName);
    }
}
