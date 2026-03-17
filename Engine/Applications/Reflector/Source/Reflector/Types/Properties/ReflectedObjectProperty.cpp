#include "ReflectedObjectProperty.h"
#include "Reflector/Types/ReflectedType.h"

namespace Lumina
{
    void FReflectedObjectProperty::AppendDefinition(eastl::string& Stream) const
    {
        eastl::string PropertyFlagStr = PropertyFlagsToString(PropertyFlags); \

        eastl::string CustomData = "Construct_CClass_" + ClangUtils::MakeCodeFriendlyNamespace(TypeName);
        AppendPropertyDef(Stream, PropertyFlagStr.c_str(), "Lumina::EPropertyTypeFlags::Object", CustomData);
    }

    bool FReflectedObjectProperty::GenerateLuaBinding(eastl::string& Stream)
    {
        return true;
    }

    eastl::string_view FReflectedObjectProperty::GetLuaType()
    {
        size_t Pos = TypeName.find_last_of(':');
        if (Pos != eastl::string::npos)
        {
            return eastl::string_view(TypeName).substr(Pos + 1);
        }
        return TypeName;
    }
}
