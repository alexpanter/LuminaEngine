#pragma once
#include "ReflectedProperty.h"

namespace Lumina
{
    class FReflectedArrayProperty : public FReflectedProperty
    {
    public:

        const char* GetPropertyParamType() const override { return "FArrayPropertyParams"; }
        void AppendDefinition(eastl::string& Stream) const override;
        const char* GetTypeName() override { return nullptr; }
        eastl::string_view GetLuaType() override { return eastl::string_view{}; }
        
        
        bool HasAccessors() override;
        bool DeclareAccessors(eastl::string& Stream, const eastl::string& FileID) override;
        bool DefineAccessors(eastl::string& Stream, Reflection::FReflectedType* ReflectedType) override;
        bool GenerateLuaBinding(eastl::string& Stream) override;

        eastl::string ElementTypeName;
    };
}
