#pragma once
#include "EASTL/shared_ptr.h"
#include "EASTL/string.h"
#include "EASTL/vector.h"
#include "Reflector/Utils/MetadataUtils.h"
#include "Reflector/Types/PropertyFlags.h"
#include "Reflector/Types/Functions/ReflectedFunction.h"
#include "Reflector/Types/Properties/ReflectedProperty.h"

namespace Lumina::Reflection
{
    class FReflectedHeader;
    class FReflectedProject;
}


namespace Lumina::Reflection
{
    constexpr uint32_t Hash(const char* str)
    {
        uint32_t hash = 5381;
        while (*str)
        {
            hash = ((hash << 5) + hash) + static_cast<unsigned char>(*str++);
        }
        return hash;
    }

    inline EPropertyTypeFlags GetCoreTypeFromName(const char* Name)
    {
        if (Name == nullptr)
        {
            return EPropertyTypeFlags::None;
        }

        switch (Hash(Name))
        {
            case Hash("bool"):                      return EPropertyTypeFlags::Bool;
            case Hash("uint8"):                     return EPropertyTypeFlags::UInt8;
            case Hash("uint16"):                    return EPropertyTypeFlags::UInt16;
            case Hash("uint32"):                    return EPropertyTypeFlags::UInt32;
            case Hash("uint64"):                    return EPropertyTypeFlags::UInt64;
            case Hash("int8"):                      return EPropertyTypeFlags::Int8;
            case Hash("int16"):                     return EPropertyTypeFlags::Int16;
            case Hash("int32"):                     return EPropertyTypeFlags::Int32;
            case Hash("int64"):                     return EPropertyTypeFlags::Int64;
            case Hash("float"):                     return EPropertyTypeFlags::Float;
            case Hash("double"):                    return EPropertyTypeFlags::Double;
            case Hash("entt::entity"):              return EPropertyTypeFlags::Int32;
            case Hash("Lumina::CClass"):            return EPropertyTypeFlags::Class;
            case Hash("Lumina::FName"):             return EPropertyTypeFlags::Name;
            case Hash("Lumina::FString"):           return EPropertyTypeFlags::String;
            case Hash("Lumina::TVector"):           return EPropertyTypeFlags::Vector;
            case Hash("Lumina::TObjectPtr"):        return EPropertyTypeFlags::Object;
            case Hash("Lumina::TWeakObjectPtr"):    return EPropertyTypeFlags::Object;
            case Hash("Lumina::CObject"):           return EPropertyTypeFlags::Object;
            default:                                    return EPropertyTypeFlags::None;
        }
    }
    
    /** Abstract base to all reflected types */
    class FReflectedType
    {
    public:

        enum class EType : uint8_t
        {
            Class,
            Structure,
            Enum,
        };
        
        virtual ~FReflectedType() = default;

        virtual eastl::string GetTypeName() const = 0;
        virtual void DefineConstructionStatics(eastl::string& Stream) = 0;
        virtual void DefineInitialHeader(eastl::string& Stream, const eastl::string& FileID) = 0;
        virtual void DefineSecondaryHeader(eastl::string& Stream, const eastl::string& FileID) = 0;
        virtual void DeclareImplementation(eastl::string& Stream) = 0;
        virtual void DeclareStaticRegistration(eastl::string& Stream) = 0;
        virtual void SetupLuaRegistration(eastl::string& Stream) = 0;
        
        bool HasMetadata(const eastl::string& Meta);

        bool DeclareAccessors(eastl::string& Stream, const eastl::string& FileID);
        void GenerateMetadata(const eastl::string& InMetadata);

        eastl::vector<eastl::unique_ptr<FReflectedProperty>>    Props;
        eastl::vector<eastl::unique_ptr<FReflectedFunction>>    Functions;
        eastl::vector<FMetadataPair>                            Metadata;
        FReflectedHeader*                                       Header;
        eastl::string                                           DisplayName;
        eastl::string                                           QualifiedName;
        eastl::string                                           Namespace;
        uint32_t                                                GeneratedBodyLineNumber;
        uint32_t                                                LineNumber;
        EType                                                   Type;
    };
    

    /** Reflected enumeration. */
    class FReflectedEnum : public FReflectedType
    {
    public:

        struct FConstant
        {
            eastl::string ID;
            eastl::string Label;
            eastl::string Description;
            uint32_t Value;
        };

        FReflectedEnum()
        {
            Type = EType::Enum;
        }

        eastl::string GetTypeName() const override { return "CEnum"; }
        void DefineConstructionStatics(eastl::string& Stream) override;
        void DefineInitialHeader(eastl::string& Stream, const eastl::string& FileID) override;
        void DefineSecondaryHeader(eastl::string& Stream, const eastl::string& FileID) override;
        void DeclareImplementation(eastl::string& Stream) override;
        void DeclareStaticRegistration(eastl::string& Stream) override;
        void SetupLuaRegistration(eastl::string& Stream) override;

        void AddConstant(const FConstant& Constant) { Constants.push_back(Constant); }

        eastl::vector<FConstant>    Constants;
        
    };

    
    /** Reflected structure */
    class FReflectedStruct : public FReflectedType
    {
    public:

        ~FReflectedStruct() override;
        
        FReflectedStruct()
        {
            Type = EType::Structure;
        }

        void PushProperty(eastl::unique_ptr<FReflectedProperty>&& NewProperty);
        void PushFunction(eastl::unique_ptr<FReflectedFunction>&& NewFunction);

        eastl::string GetTypeName() const override { return "CStruct"; }
        
        void DefineConstructionStatics(eastl::string& Stream) override;
        void DefineInitialHeader(eastl::string& Stream, const eastl::string& FileID) override;
        void DefineSecondaryHeader(eastl::string& Stream, const eastl::string& FileID) override;
        void DeclareImplementation(eastl::string& Stream) override;
        void DeclareStaticRegistration(eastl::string& Stream) override;
        void SetupLuaRegistration(eastl::string& Stream) override;
        
        eastl::string Parent;
    };

    
    /** Reflected class */
    class FReflectedClass : public FReflectedStruct
    {
    public:
        FReflectedClass()
        {
            Type = EType::Class;
        }

        eastl::string GetTypeName() const override { return "CClass"; }
        void DefineConstructionStatics(eastl::string& Stream) override;
        void DefineInitialHeader(eastl::string& Stream, const eastl::string& FileID) override;
        void DefineSecondaryHeader(eastl::string& Stream, const eastl::string& FileID) override;
        void DeclareImplementation(eastl::string& Stream) override;
        void DeclareStaticRegistration(eastl::string& Stream) override;

    };
    
}
