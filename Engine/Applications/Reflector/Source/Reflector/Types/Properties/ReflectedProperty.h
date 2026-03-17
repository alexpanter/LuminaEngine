#pragma once

#include <EASTL/string.h>
#include "EASTL/vector.h"
#include "Reflector/Types/PropertyFlags.h"
#include "Reflector/Types/StructReflectItem.h"
#include "Reflector/Utils/MetadataUtils.h"


namespace Lumina::Reflection
{
    class FReflectedType;
}

namespace Lumina
{
    class FReflectedProperty : public IStructReflectable
    {
    public:

        virtual void AppendDefinition(eastl::string& Stream) const = 0;
        void AppendPropertyDef(eastl::string& Stream, const char* PropertyFlagsStr, const char* TypeFlags, const eastl::string& CustomData = "") const;
        
        virtual const char* GetPropertyParamType() const { return "FPropertyParams"; }
        
        virtual const char* GetTypeName() = 0;
        virtual eastl::string_view GetLuaType() = 0;
        
        eastl::string GetDisplayName() const { return Name; }
        void GenerateMetadata(const eastl::string& InMetadata) override;

        virtual bool CanDeclareCrossModuleReferences() const { return false; }
        virtual void DeclareCrossModuleReference(const eastl::string& API, eastl::string& Stream) { }

        bool GenerateLuaBinding(eastl::string& Stream) override;
        
        virtual bool HasAccessors();
        virtual bool DeclareAccessors(eastl::string& Stream, const eastl::string& FileID);
        virtual bool DefineAccessors(eastl::string& Stream, Reflection::FReflectedType* ReflectedType);

        eastl::vector<FMetadataPair>    Metadata;
        EPropertyFlags                  PropertyFlags;
        eastl::string                   RawTypeName;
        eastl::string                   TypeName;
        eastl::string                   Namespace;
        eastl::string                   Name;
        eastl::string                   Outer;
        eastl::string                   GetterFunc;
        eastl::string                   SetterFunc;
        
        bool                            bInner = false;
    };
}
