#include "ReflectedFunction.h"

namespace Lumina
{
    void FReflectedFunction::GenerateMetadata(const eastl::string& InMetadata)
    {
        if (InMetadata.empty())
        {
            return;
        }

        FMetadataParser Parser(InMetadata);
        Metadata = eastl::move(Parser.Metadata);
    }

    void FReflectedFunction::AppendDefinition(eastl::string& Stream)
    {
        Stream += "+[](lua_State* L)\n";
        Stream += "{\n";
        Stream += "\treturn Lumina::Lua::Invoker<&" + Outer + "::" + Name + ">(L);\n";
        Stream += "};\n";
    }
}
