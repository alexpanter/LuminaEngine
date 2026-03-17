#pragma once
#include "ReflectedProperty.h"
#include "Reflector/Clang/Utils.h"

namespace Lumina
{
    class FReflectedObjectProperty : public FReflectedProperty
    {
    public:

        const char* GetTypeName() override
        {
            return "Object";
        }
        
        void AppendDefinition(eastl::string& Stream) const override;

        const char* GetPropertyParamType() const override { return "FObjectPropertyParams"; }
        bool GenerateLuaBinding(eastl::string& Stream) override;
        eastl::string_view GetLuaType() override;

        void DeclareCrossModuleReference(const eastl::string& API, eastl::string& Stream) override
        {
            Stream += API;
            Stream += " Lumina::CClass* Construct_CClass_";
            Stream += ClangUtils::MakeCodeFriendlyNamespace(TypeName);
            Stream += "();\n";
        }
    };
}
