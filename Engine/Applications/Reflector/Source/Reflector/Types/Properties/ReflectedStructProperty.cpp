#include "ReflectedStructProperty.h"

#include "Reflector/Types/ReflectedType.h"

namespace Lumina
{
    void FReflectedStructProperty::AppendDefinition(eastl::string& Stream) const
    {
        eastl::string PropertyFlagStr = PropertyFlagsToString(PropertyFlags);

        eastl::string CustomData = "Construct_CStruct_" + ClangUtils::MakeCodeFriendlyNamespace(TypeName);
        AppendPropertyDef(Stream, PropertyFlagStr.c_str(), "Lumina::EPropertyTypeFlags::Struct", CustomData);
    }
}
