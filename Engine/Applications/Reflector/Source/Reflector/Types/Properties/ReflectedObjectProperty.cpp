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
        Stream += "\t\t\"" + GetDisplayName() + "\", "
        "sol::property([](" + Outer + "& Self) { return nullptr; })";//\n"
        //"\t\t[](" + Outer + "& Self, " + TypeName + "* Obj) { Self." + Name + " = Obj; })";

        return true;
    }
}
