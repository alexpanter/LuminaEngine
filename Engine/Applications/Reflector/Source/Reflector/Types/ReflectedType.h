#pragma once
#include "EASTL/shared_ptr.h"
#include "EASTL/string.h"
#include "EASTL/vector.h"
#include "Reflector/Utils/MetadataUtils.h"
#include "Reflector/Types/Functions/ReflectedFunction.h"
#include "Reflector/Types/Properties/ReflectedProperty.h"

namespace Lumina::Reflection
{
    class FReflectedHeader;
    class FReflectedProject;
}


#define ENUM_CLASS_FLAGS(Enum) \
inline           Enum& operator|=(Enum& Lhs, Enum Rhs) { return Lhs = (Enum)((__underlying_type(Enum))Lhs | (__underlying_type(Enum))Rhs); } \
inline           Enum& operator&=(Enum& Lhs, Enum Rhs) { return Lhs = (Enum)((__underlying_type(Enum))Lhs & (__underlying_type(Enum))Rhs); } \
inline           Enum& operator^=(Enum& Lhs, Enum Rhs) { return Lhs = (Enum)((__underlying_type(Enum))Lhs ^ (__underlying_type(Enum))Rhs); } \
inline constexpr Enum  operator| (Enum  Lhs, Enum Rhs) { return (Enum)((__underlying_type(Enum))Lhs | (__underlying_type(Enum))Rhs); } \
inline constexpr Enum  operator& (Enum  Lhs, Enum Rhs) { return (Enum)((__underlying_type(Enum))Lhs & (__underlying_type(Enum))Rhs); } \
inline constexpr Enum  operator^ (Enum  Lhs, Enum Rhs) { return (Enum)((__underlying_type(Enum))Lhs ^ (__underlying_type(Enum))Rhs); } \
inline constexpr bool  operator! (Enum  E)             { return !(__underlying_type(Enum))E; } \
inline constexpr Enum  operator~ (Enum  E)             { return (Enum)~(__underlying_type(Enum))E; }

template<typename Enum>
constexpr bool EnumHasAllFlags(Enum Flags, Enum Contains)
{
    using UnderlyingType = __underlying_type(Enum);
    return ((UnderlyingType)Flags & (UnderlyingType)Contains) == (UnderlyingType)Contains;
}

template<typename Enum>
constexpr bool EnumHasAnyFlags(Enum Flags, Enum Contains)
{
    using UnderlyingType = __underlying_type(Enum);
    return ((UnderlyingType)Flags & (UnderlyingType)Contains) != 0;
}

template<typename Enum>
void EnumAddFlags(Enum& Flags, Enum FlagsToAdd)
{
    using UnderlyingType = __underlying_type(Enum);
    Flags = (Enum)((UnderlyingType)Flags | (UnderlyingType)FlagsToAdd);
}

template<typename Enum>
void EnumRemoveFlags(Enum& Flags, Enum FlagsToRemove)
{
    using UnderlyingType = __underlying_type(Enum);
    Flags = (Enum)((UnderlyingType)Flags & ~(UnderlyingType)FlagsToRemove);
}


/** This must reflect EPropertyTypeFlags found in ObjectCore.h */
enum class EPropertyTypeFlags : uint64_t
{
    None = 0,

    // Signed integers
    Int8                = 1 << 0,
    Int16               = 1 << 1,
    Int32               = 1 << 2,
    Int64               = 1 << 3,

    // Unsigned integers
    UInt8               = 1 << 4,
    UInt16              = 1 << 5,
    UInt32              = 1 << 6,
    UInt64              = 1 << 7,

    // Floats
    Float               = 1 << 8,
    Double              = 1 << 9,

    // Other types
    Bool                = 1 << 10,
    Object              = 1 << 12,
    Class               = 1 << 13,
    Name                = 1 << 14,
    String              = 1 << 15,
    Enum                = 1 << 16,
    Vector              = 1 << 17,
    Struct              = 1 << 18,
};

/** Must be in-sync with EPropertyFlags in ObjectCore.h */
enum class EPropertyFlags : uint16_t
{
    None                = 0,
    Editable            = 1 << 0,
    ReadOnly            = 1 << 1,
    Transient           = 1 << 2,
    Const               = 1 << 3,
    Private             = 1 << 4,
    Protected           = 1 << 5,
    SubField            = 1 << 6,
    Trivial             = 1 << 7,
    Script              = 1 << 8,
};

ENUM_CLASS_FLAGS(EPropertyFlags)

inline eastl::string PropertyFlagsToString(EPropertyFlags Flags)
{
    if (Flags == EPropertyFlags::None)
    {
        return "Lumina::EPropertyFlags::None";
    }

    eastl::string Result;

    auto AppendFlag = [&](EPropertyFlags Flag, eastl::string_view Name)
    {
        if ((Flags & Flag) != EPropertyFlags::None)
        {
            if (!Result.empty())
            {
                Result += " | ";
            }
            Result.append_convert(Name.begin(), Name.length());
        }
    };

    AppendFlag(EPropertyFlags::Editable,   "Lumina::EPropertyFlags::Editable");
    AppendFlag(EPropertyFlags::ReadOnly,   "Lumina::EPropertyFlags::ReadOnly");
    AppendFlag(EPropertyFlags::Transient,  "Lumina::EPropertyFlags::Transient");
    AppendFlag(EPropertyFlags::Const,      "Lumina::EPropertyFlags::Const");
    AppendFlag(EPropertyFlags::Private,    "Lumina::EPropertyFlags::Private");
    AppendFlag(EPropertyFlags::Protected,  "Lumina::EPropertyFlags::Protected");
    AppendFlag(EPropertyFlags::SubField,   "Lumina::EPropertyFlags::SubField");
    AppendFlag(EPropertyFlags::Trivial,    "Lumina::EPropertyFlags::Trivial");
    AppendFlag(EPropertyFlags::Script,    "Lumina::EPropertyFlags::Script");

    return Result;
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
            default:                                   return EPropertyTypeFlags::None;
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

        void AddConstant(const FConstant& Constant) { Constants.push_back(Constant); }

        eastl::vector<FConstant>    Constants;
        
    };

    
    /** Reflected structure */
    class FReflectedStruct : public FReflectedType
    {
    public:

        virtual ~FReflectedStruct() override;
        
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
