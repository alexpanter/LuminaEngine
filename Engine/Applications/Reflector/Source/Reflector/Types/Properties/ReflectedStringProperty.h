#pragma once
#include "ReflectedProperty.h"
#include "Reflector/Types/ReflectedType.h"

namespace Lumina
{
    class FReflectedStringProperty : public FReflectedProperty
    {
    public:

        const char* GetTypeName() override { return "FString"; }

        void AppendDefinition(eastl::string& Stream) const override
        {
            eastl::string PropertyFlagStr = PropertyFlagsToString(PropertyFlags);

            AppendPropertyDef(Stream, PropertyFlagStr.c_str(), "Lumina::EPropertyTypeFlags::String");
        }

        bool GenerateLuaBinding(eastl::string& Stream) override;
        
        virtual const char* GetPropertyParamType() const override { return "FStringPropertyParams"; } \

    };

    class FReflectedNameProperty : public FReflectedProperty
    {
    public:

        const char* GetTypeName() override { return "FName"; }

        void AppendDefinition(eastl::string& Stream) const override
        {
            eastl::string PropertyFlagStr = PropertyFlagsToString(PropertyFlags);

            AppendPropertyDef(Stream, PropertyFlagStr.c_str(), "Lumina::EPropertyTypeFlags::Name");
        }

        virtual const char* GetPropertyParamType() const override { return "FNamePropertyParams"; } \

    };
}
