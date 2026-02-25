#include "ReflectedType.h"
#include "Functions/ReflectedFunction.h"
#include "Properties/ReflectedProperty.h"
#include "Reflector/Clang/Utils.h"
#include "Reflector/ReflectionCore/ReflectedProject.h"


namespace Lumina::Reflection
{

    static bool IsManualReflectFile(const eastl::string& HeaderID)
    {
        return HeaderID.find("manualreflecttypes") != eastl::string::npos;
    }
    
    bool FReflectedType::DeclareAccessors(eastl::string& Stream, const eastl::string& FileID)
    {
        // First pass: check if any property has accessors
        bool bHasAnyAccessor = false;
        for (const auto& Prop : Props)
        {
            if (Prop->HasAccessors())
            {
                bHasAnyAccessor = true;
                break;
            }
        }

        if (!bHasAnyAccessor)
        {
            return false;
        }

        Stream += "#define " + FileID + "_" + eastl::to_string(GeneratedBodyLineNumber) + "_ACCESSORS \\\n";

        for (const auto& Prop : Props)
        {
            Prop->DeclareAccessors(Stream, FileID);
        }

        // Remove "\\\n from last accessor."
        Stream.pop_back();
        Stream.pop_back();
        
        Stream += "\n\n";
        
        return true;
    }

    void FReflectedType::GenerateMetadata(const eastl::string& InMetadata)
    {
        FMetadataParser Parser(InMetadata);
        Metadata = eastl::move(Parser.Metadata);
    }

    void FReflectedEnum::DefineConstructionStatics(eastl::string& Stream)
    {
    }

    void FReflectedEnum::DefineInitialHeader(eastl::string& Stream, const eastl::string& FileID)
    {
        Stream += "enum class " + DisplayName + " : uint8;\n";

        eastl::string ProjectAPI = Header->Project->Name + "_api";
        ProjectAPI.make_upper();

        Stream += ProjectAPI;
        Stream += " ";
        Stream += "Lumina::CEnum* Construct_CEnum_" + Namespace + "_" + DisplayName + "();\n";
        
        Stream += "template<> Lumina::CEnum* StaticEnum<" +  DisplayName + ">();\n\n";
    }

    void FReflectedEnum::DefineSecondaryHeader(eastl::string& Stream, const eastl::string& FileID)
    {
    }

    void FReflectedEnum::DeclareImplementation(eastl::string& Stream)
    {
        Stream += "static Lumina::FEnumRegistrationInfo Registration_Info_CEnum_" + Namespace + "_" + DisplayName + ";\n\n";

        Stream += "struct Construct_CEnum_" + Namespace + "_" + DisplayName + "_Statics\n";
        Stream +="{\n";
        
        if (!Metadata.empty())
        {
            Stream += "\tstatic constexpr Lumina::FMetaDataPairParam " + ClangUtils::MakeCodeFriendlyNamespace(QualifiedName) +  + "_Metadata[] = {\n";

            for (const FMetadataPair& Metadata : Metadata)
            {
                Stream += "\t\t{ \"" + Metadata.Key + "\", \"" + Metadata.Value + "\" },\n";    
            }
            
            Stream += "\t};\n";
        }
        
        Stream += "\tstatic constexpr Lumina::FEnumeratorParam Enumerators[] = {\n";
        for (const FReflectedEnum::FConstant& Constant : Constants)
        {
            Stream += "\t\t{ \"" + DisplayName + "::" + Constant.Label + "\", " + eastl::to_string(Constant.Value) + " },\n";
        }
        Stream += "\t};\n\n";

        Stream += "\tstatic const Lumina::FEnumParams EnumParams;\n";
        Stream += "};\n";
        Stream += "const Lumina::FEnumParams Construct_CEnum_" + Namespace + "_" + DisplayName + "_Statics::EnumParams = {\n";
        Stream += "\t\"" + DisplayName + "\",\n";
        Stream += "\tEnumerators,\n";
        Stream += "\t(uint32)std::size(Enumerators)";
        
        if (!Metadata.empty())
        {
            eastl::string MetaDataName = Namespace + "_" + DisplayName + "_Metadata";
            Stream += ",\n";
            Stream += "\t(uint32)std::size(" + MetaDataName + "),\n";
            Stream += "\t" + MetaDataName;
        }
        
        Stream += "\n};\n\n";
        
        Stream += "Lumina::CEnum* Construct_CEnum_" + Namespace + "_" + DisplayName + "()\n";
        Stream += "{\n";
        Stream += "\tif(!Registration_Info_CEnum_" + Namespace + "_" + DisplayName + ".InnerSingleton)" + "\n";
        Stream += "\t{\n";
        Stream += "\t\tLumina::ConstructCEnum(&Registration_Info_CEnum_" + Namespace + "_" + DisplayName + ".InnerSingleton, Construct_CEnum_" + Namespace + "_" + DisplayName + "_Statics::EnumParams);" + "\n";
        Stream += "\t}\n";
        Stream += "\treturn Registration_Info_CEnum_" + Namespace + "_" + DisplayName + ".InnerSingleton;" + "\n";
        Stream += "}\n";
        Stream += "\n";
        Stream += "template<> Lumina::CEnum* StaticEnum<" + DisplayName + ">()\n";
        Stream += "{\n";
        Stream += "\tif (!Registration_Info_CEnum_" + Namespace + "_" + DisplayName + ".OuterSingleton)\n";
        Stream += "\t{\n";
        Stream += "\t\tRegistration_Info_CEnum_" + Namespace + "_" + DisplayName + ".OuterSingleton = Construct_CEnum_" + Namespace + "_" + DisplayName + "();\n";
        Stream += "\t}\n";
        Stream += "\treturn Registration_Info_CEnum_" + Namespace + "_" + DisplayName + ".OuterSingleton;\n";
        Stream += "}\n";
    }

    void FReflectedEnum::DeclareStaticRegistration(eastl::string& Stream)
    {
        eastl::string FriendlyName = ClangUtils::MakeCodeFriendlyNamespace(QualifiedName);
        Stream += "\t{ Construct_CEnum_" + FriendlyName + ", TEXT(\"" + DisplayName + "\") },\n";
    }


    //---------------------------------------------------------------------------------------------------------------------

    
    
    FReflectedStruct::~FReflectedStruct()
    {
    }

    void FReflectedStruct::PushProperty(eastl::unique_ptr<FReflectedProperty>&& NewProperty)
    {
        if (Namespace.empty())
        {
            NewProperty->Outer = DisplayName;
        }
        else
        {
            NewProperty->Outer = Namespace + "::" + DisplayName;
        }
        Props.push_back(eastl::move(NewProperty));
    }

    void FReflectedStruct::PushFunction(eastl::unique_ptr<FReflectedFunction>&& NewFunction)
    {
        if (Namespace.empty())
        {
            NewFunction->Outer = DisplayName;
        }
        else
        {
            NewFunction->Outer = Namespace + "::" + DisplayName;
        }
        Functions.push_back(eastl::move(NewFunction));
    }

    void FReflectedStruct::DefineConstructionStatics(eastl::string& Stream)
    {
        Stream += "struct Construct_CStruct_" + Namespace + "_" + DisplayName + "_Statics\n{\n\n";
    }

    void FReflectedStruct::DefineInitialHeader(eastl::string& Stream, const eastl::string& FileID)
    {
        if (!Namespace.empty())
        {
            Stream += "namespace " + Namespace + " { struct " + DisplayName + "; }\n";
        }
        else
        {
            Stream += "\tclass " + DisplayName + ";\n";
        }
        eastl::string ProjectAPI = Header->Project->Name + "_api";
        ProjectAPI.make_upper();

        Stream += ProjectAPI;
        Stream += " ";
        Stream += "Lumina::CStruct* Construct_CStruct_" + Namespace + "_" + DisplayName + "();\n";

        
    }

    void FReflectedStruct::DefineSecondaryHeader(eastl::string& Stream, const eastl::string& FileID)
    {
        if (IsManualReflectFile(FileID))
        {
            Stream += "\n\n";
            return;
        }

        bool bDeclared = DeclareAccessors(Stream, FileID);
        
        Stream += "#define " + FileID + "_" + eastl::to_string(GeneratedBodyLineNumber) + "_GENERATED_BODY \\\n";
        if (bDeclared)
        {
            Stream += FileID + "_" + eastl::to_string(GeneratedBodyLineNumber) + "_ACCESSORS \\\n";
        }
        Stream += "\tstatic class Lumina::CStruct* StaticStruct();\\\n";
        if (!Parent.empty())
        {
            Stream += "\tusing Super = " + Namespace + "::" + Parent + ";\\\n";
        }

        for (const FMetadataPair& Data : Metadata)
        {
            if (Data.Key == "Component")
            {
                Stream += "\tstatic constexpr auto in_place_delete = true;\\\n";
            }
        }
        
        Stream += "\n\n";
    }

    void FReflectedStruct::DeclareImplementation(eastl::string& Stream)
    {
        Stream += "\n\n";
        
        Stream += "// Begin " + DisplayName + "\n";
        Stream += "static Lumina::FStructRegistrationInfo Registration_Info_CStruct_" + Namespace + "_" + DisplayName + ";\n\n";
        
        DefineConstructionStatics(Stream);
        
        eastl::string FriendlyName = ClangUtils::MakeCodeFriendlyNamespace(QualifiedName);
        
        Stream += "\tstatic Lumina::FStructOps* GetStructOps()\n";
        Stream += "\t{\n";
        Stream += "\t\treturn Lumina::MakeStructOps<" + QualifiedName + ">();\n";
        Stream += "\t}\n";
        
        
        if (!Metadata.empty())
        {
            Stream += "\tstatic constexpr Lumina::FMetaDataPairParam " + FriendlyName +  + "_Metadata[] = {\n";

            for (const FMetadataPair& Data : Metadata)
            {
                Stream += "\t\t{ \"" + Data.Key + "\", \"" + Data.Value + "\" },\n";    
            }
            
            Stream += "\t};\n";
        }   

        for (const auto& Prop : Props)
        {
            if (Prop->Metadata.empty())
            {
                continue;
            }
            
            Stream += "\tstatic constexpr Lumina::FMetaDataPairParam " + Prop->Name + "_Metadata[] = {\n";

            for (const FMetadataPair& Metadata : Prop->Metadata)
            {
                Stream += "\t\t{ \"" + Metadata.Key + "\", \"" + Metadata.Value + "\" },\n";    
            }
            
            Stream += "\t};\n";
        }
        
        Stream += "\n";

        for (const auto& Prop : Props)
        {
            Stream += "\tstatic const Lumina::" + eastl::string(Prop->GetPropertyParamType()) + " " + Prop->Name + ";\n";
        }
        
        Stream += "\t//...\n\n";
        
        Stream += "\tstatic const Lumina::FStructParams StructParams;\n";
        if (!Props.empty())
        {
            Stream += "\tstatic const Lumina::FPropertyParams* const PropPointers[];\n";
        }
        Stream += "};\n\n";
        
        Stream += "Lumina::CStruct* Construct_CStruct_" + FriendlyName + "()\n";
        Stream += "{\n";
        Stream += "\tif (!Registration_Info_CStruct_" + FriendlyName + ".InnerSingleton)\n";
        Stream += "\t{\n";
        Stream += "\t\tLumina::ConstructCStruct(&Registration_Info_CStruct_" + FriendlyName + ".InnerSingleton, Construct_CStruct_" + FriendlyName + "_Statics::StructParams);\n";
        
        for (const FMetadataPair& Data : Metadata)
        {
            if (Data.Key == "Component")
            {
                Stream += "\t\t::Lumina::Meta::RegisterComponentMeta<" + QualifiedName + ">();\n";
            }
            
            if (Data.Key == "System")
            {
                Stream += "\t\t::Lumina::Meta::RegisterECSSystem<" + QualifiedName + ">();\n";
            }
            
            if (Data.Key == "Event")
            {
                Stream += "\t\t::Lumina::Meta::RegisterECSEvent<" + QualifiedName + ">();\n";
            }
        }
        
        Stream += "\t}\n";
        Stream += "\treturn Registration_Info_CStruct_" + FriendlyName + ".InnerSingleton;\n";
        Stream += "}\n\n";

        if (!IsManualReflectFile(Header->HeaderPath))
        {
            Stream += "class Lumina::CStruct* " + QualifiedName + "::StaticStruct()\n";
            Stream += "{\n";
            Stream += "\tif (!Registration_Info_CStruct_" + FriendlyName + ".OuterSingleton)\n";
            Stream += "\t{\n";
            Stream += "\t\tRegistration_Info_CStruct_" + FriendlyName + ".OuterSingleton = Construct_CStruct_" + FriendlyName + "();\n";
            Stream += "\t}\n";
            Stream += "\treturn Registration_Info_CStruct_" + FriendlyName + ".OuterSingleton;\n";
            Stream += "}\n";
        }

        if (!Props.empty())
        {
            for (const auto& Prop : Props)
            {
                Prop->DefineAccessors(Stream, this);
            }
            
            for (const auto& Prop : Props)
            {
                Stream += "const Lumina::" + eastl::string(Prop->GetPropertyParamType()) + " Construct_CStruct_" + FriendlyName + "_Statics::" + Prop->Name + " = ";
                Prop->AppendDefinition(Stream);
            }
            
            Stream += "\n";
            Stream += "const Lumina::FPropertyParams* const Construct_CStruct_" + FriendlyName + "_Statics::PropPointers[] = {\n";
        
            for (const auto& Prop : Props)
            {
                Stream += "\t(const Lumina::FPropertyParams*)&Construct_CStruct_" + FriendlyName + "_Statics::" + Prop->Name + ",\n";
            }
        
            Stream += "};\n\n";
        }

        Stream += "const Lumina::FStructParams Construct_CStruct_" + FriendlyName + "_Statics::StructParams = {\n";
        if (Parent.empty())
        {
            Stream += "\tnullptr,\n";
        }
        else
        {
            Stream += "\t" + QualifiedName + "::" + "Super::StaticStruct,\n";
        }
        
        Stream += "&GetStructOps,\n";
        
        Stream += "\t\"" + DisplayName + "\",\n";
        
        if (!Props.empty())
        {
            Stream += "\tPropPointers,\n";
            Stream += "\t(uint32)std::size(PropPointers),\n";
        }
        else
        {
            Stream += "\tnullptr,\n";
            Stream += "\t0,\n";
        }

        Stream += "\tsizeof(" + QualifiedName + "),\n";
        Stream += "\talignof(" + QualifiedName + ")";
        
        if (!Metadata.empty())
        {
            eastl::string MetaDataName = FriendlyName + "_Metadata";
            Stream += ",\n";
            Stream += "\t(uint32)std::size(" + MetaDataName + "),\n";
            Stream += "\t" + MetaDataName;
        }
        
        
        Stream += "\n};\n\n";

        Stream += "//~ End " + DisplayName + "\n\n";
        Stream += "//------------------------------------------------------------\n\n";
    }

    void FReflectedStruct::DeclareStaticRegistration(eastl::string& Stream)
    {
        eastl::string FriendlyName = ClangUtils::MakeCodeFriendlyNamespace(QualifiedName);
        Stream += "\t{ Construct_CStruct_" + FriendlyName + ", TEXT(\"" + DisplayName + "\") },\n";
    }


    //---------------------------------------------------------------------------------------------------------------------


    
    void FReflectedClass::DefineConstructionStatics(eastl::string& Stream)
    {
        Stream += "struct Construct_CClass_" + Namespace + "_" + DisplayName + "_Statics\n{\n";
    }

    void FReflectedClass::DefineInitialHeader(eastl::string& Stream, const eastl::string& FileID)
    {
        eastl::string LowerProject = Header->Project->Name;
        LowerProject.make_lower();
        
        eastl::string PackageName = "/Script/";
        if (LowerProject == "runtime")
        {
            PackageName += "Engine";
        }
        else
        {
            PackageName += Header->Project->Name;
        }
        

        if (!Namespace.empty())
        {
            Stream += "namespace " + Namespace + " { class " + DisplayName + "; }\n";
        }
        else
        {
            Stream += "\tclass " + DisplayName + ";\n";
        }

        eastl::string ProjectAPI = Header->Project->Name + "_api";
        ProjectAPI.make_upper();

        Stream += ProjectAPI;
        Stream += " ";
        Stream += "Lumina::CClass* Construct_CClass_" + Namespace + "_" + DisplayName + "();\n";
        
        Stream += "#define " + FileID + "_" + eastl::to_string(LineNumber) + "_CLASS \\\n";
        Stream += "private: \\\n";
        Stream += "\\\n";
        Stream += "public: \\\n";
        Stream += "\tDECLARE_CLASS(" + Namespace + ", " + DisplayName + ", " + Parent + ", \"" + PackageName.c_str() + "\", NO_API" + ") \\\n";
        Stream += "\tDEFINE_CLASS_FACTORY(" + Namespace + "::" + DisplayName + ") \\\n";
        Stream += "\tDECLARE_SERIALIZER(" + Namespace + ", " + DisplayName + ") \\\n";
        Stream += "\n\n";
    }

    void FReflectedClass::DefineSecondaryHeader(eastl::string& Stream, const eastl::string& FileID)
    {
        bool bDeclared = DeclareAccessors(Stream, FileID);
        
        Stream += "#define " + FileID + "_" + eastl::to_string(GeneratedBodyLineNumber) + "_GENERATED_BODY \\\n";
        Stream += "public: \\\n";
        if (bDeclared)
        {
            Stream += FileID + "_" + eastl::to_string(GeneratedBodyLineNumber) + "_ACCESSORS \\\n";
        }
        Stream += "\t" + FileID + "_" + eastl::to_string(LineNumber) + "_CLASS \\\n";
        Stream += "private: \\\n";
        Stream += "\n\n";
    }

    void FReflectedClass::DeclareImplementation(eastl::string& Stream)
    {
        
        Stream += "// Begin " + DisplayName + "\n";
        Stream += "IMPLEMENT_CLASS(" + Namespace + ", " + DisplayName + ")\n";
        DefineConstructionStatics(Stream);
        
        if (!Metadata.empty())
        {
            Stream += "\tstatic constexpr Lumina::FMetaDataPairParam " + ClangUtils::MakeCodeFriendlyNamespace(QualifiedName) +  + "_Metadata[] = {\n";

            for (const FMetadataPair& Metadata : Metadata)
            {
                Stream += "\t\t{ \"" + Metadata.Key + "\", \"" + Metadata.Value + "\" },\n";    
            }
            
            Stream += "\t};\n";
        }
        
        for (const auto& Prop : Props)
        {
            if (Prop->Metadata.empty())
            {
                continue;
            }
            
            Stream += "\tstatic constexpr Lumina::FMetaDataPairParam " + Prop->Name + "_Metadata[] = {\n";

            for (const FMetadataPair& Metadata : Prop->Metadata)
            {
                Stream += "\t\t{ \"" + Metadata.Key + "\", \"" + Metadata.Value + "\" },\n";    
            }
            
            Stream += "\n\t};\n";
        }

        Stream += "\n";
        
        for (const auto& Prop : Props)
        {
            Stream += "\tstatic const Lumina::" + eastl::string(Prop->GetPropertyParamType()) + " " + Prop->Name + ";\n";
        }
        
        Stream += "\t//...\n\n";
        
        Stream += "\tstatic const Lumina::FClassParams ClassParams;\n";
        if (!Props.empty())
        {
            Stream += "\tstatic const Lumina::FPropertyParams* const PropPointers[];\n";
        }
        
        
        Stream += "};\n\n";
        
        Stream += "Lumina::CClass* Construct_CClass_" + Namespace + "_" + DisplayName + "()\n";
        Stream += "{\n";
        Stream += "\tif (!Registration_Info_CClass_" + Namespace + "_" + DisplayName + ".OuterSingleton)\n";
        Stream += "\t{\n";
        Stream += "\t\tLumina::ConstructCClass(&Registration_Info_CClass_" + Namespace + "_" + DisplayName + ".OuterSingleton, Construct_CClass_" + Namespace + "_" + DisplayName + "_Statics::ClassParams);\n";
        Stream += "\t}\n";
        Stream += "\treturn Registration_Info_CClass_" + Namespace + "_" + DisplayName + ".OuterSingleton;\n";
        Stream += "}\n\n";
        
        if (!Props.empty())
        {
            for (const auto& Prop : Props)
            {
                Prop->DefineAccessors(Stream, this);
            }
            
            for (const auto& Prop : Props)
            {
                Stream += "const Lumina::" + eastl::string(Prop->GetPropertyParamType()) + " Construct_CClass_" + Namespace + "_" + DisplayName + "_Statics::" + Prop->Name + " = ";
                Prop->AppendDefinition(Stream);
            }
            
            
            Stream += "\n";
            Stream += "const Lumina::FPropertyParams* const Construct_CClass_" + Namespace + "_" + DisplayName + "_Statics::PropPointers[] = {\n";
        
            for (const auto& Prop : Props)
            {
                Stream += "\t(const Lumina::FPropertyParams*)&Construct_CClass_" + Namespace + "_" + DisplayName + "_Statics::" + Prop->Name + ",\n";
            }
        
            Stream += "};\n\n";
        }
        
        
        Stream += "const Lumina::FClassParams Construct_CClass_" + Namespace + "_" + DisplayName + "_Statics::ClassParams = {\n";
        Stream += "\t&" + Namespace + "::" + DisplayName + "::StaticClass,\n";
        if (!Props.empty())
        {
            Stream += "\tPropPointers,\n";
            Stream += "\t(uint32)std::size(PropPointers),";
        }
        else
        {
            Stream += "\tnullptr,\n";
            Stream += "\t0";
        }
        
        if (!Metadata.empty())
        {
            eastl::string MetaDataName = Namespace + "_" + DisplayName + "_Metadata";
            Stream += ",\n";
            Stream += "\t(uint32)std::size(" + MetaDataName + "),\n";
            Stream += "\t" + MetaDataName;
        }
        
        
        Stream += "\n};\n\n";
        
        
        Stream += "//~ End " + DisplayName + "\n\n";
        Stream += "//------------------------------------------------------------\n\n";
    }

    void FReflectedClass::DeclareStaticRegistration(eastl::string& Stream)
    {
        eastl::string FriendlyName = ClangUtils::MakeCodeFriendlyNamespace(QualifiedName);
        Stream += "\t{ Construct_CClass_" + FriendlyName + ", " + R"(TEXT("/Script"), TEXT(")" + DisplayName + "\") },\n";
    }
}
