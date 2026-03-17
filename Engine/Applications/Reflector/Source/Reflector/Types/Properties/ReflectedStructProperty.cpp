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

    eastl::string_view FReflectedStructProperty::GetLuaType()
    {
        size_t Pos = TypeName.find_last_of(':');
        if (Pos != eastl::string::npos)
        {
            return eastl::string_view(TypeName).substr(Pos + 1);
        }
        return TypeName;
    }
}
