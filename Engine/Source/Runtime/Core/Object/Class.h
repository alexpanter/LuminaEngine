#pragma once

#include "Lumina.h"
#include "Object.h"
#include "Class/StructTraits.h"
#include "Containers/Function.h"
#include "Core/Reflection/Type/Metadata/PropertyMetadata.h"
#include "Core/Templates/Align.h"
#include "Initializer/ObjectInitializer.h"
#include "Memory/SmartPtr.h"

namespace Lumina
{
    class FProperty;
    struct FTransform;
}

namespace Lumina
{
    
    //--------------------------------------------------------------------------------
    // CField
    //--------------------------------------------------------------------------------
    
    class CField : public CObject
    {
    public:

        DECLARE_CLASS(Lumina, CField, CObject, "/Script/Engine", RUNTIME_API)
        DEFINE_CLASS_FACTORY(CField)
        
        CField() = default;
        
        
        CField(CPackage* Package, const FName& InName, uint32 InSize, uint32 InAlignment, EObjectFlags InFlags)
            : CObject(nullptr, InFlags, Package, InName, FGuid::New())
            , Size(InSize)
            , Alignment(InAlignment)
        {}
        //~ End Internal Use Only Constructors

        FProperty* LinkedProperty = nullptr;

        RUNTIME_API uint32 GetAlignedSize() const { return Align(Size, Alignment); }
        RUNTIME_API uint32 GetSize() const { return Size; }
        RUNTIME_API uint32 GetAlignment() const { return Alignment; }
        
        RUNTIME_API bool HasMeta(const FName& Key) const;
        RUNTIME_API const FName& GetMeta(const FName& Key) const;

        
        uint32 Size = 0;
        uint32 Alignment = 0;
        
        FMetaDataPair Metadata;
    };


    //--------------------------------------------------------------------------------
    // CEnum
    //--------------------------------------------------------------------------------
    
    class CEnum : public CField
    {
    public:
        
        DECLARE_CLASS(Lumina, CEnum, CField, "/Script/Engine", RUNTIME_API)
        DEFINE_CLASS_FACTORY(CEnum)

        CEnum() = default;

        RUNTIME_API FName GetNameAtValue(uint64 Value);
        RUNTIME_API uint64 GetEnumValueByName(FName Name);
        void AddEnum(FName Name, uint64 Value);

        void ForEachEnum(TFunction<void(const TPair<FName, uint64>&)> Functor);
        FFixedString MakeDisplayName() const override;

        TVector<TPair<FName, uint64>> Names;
        
    };
    

    //--------------------------------------------------------------------------------
    // CStruct
    //--------------------------------------------------------------------------------
    
    
    /** Base class for any data structure that holds fields */
    class CStruct : public CField
    {
        friend RUNTIME_API void ConstructCStruct(CStruct** OutStruct, const FStructParams& Params);
        
        DECLARE_CLASS(Lumina, CStruct, CField, "/Script/Engine", RUNTIME_API)
        DEFINE_CLASS_FACTORY(CStruct)

    public:
        
        CStruct() = default;

        // Begin Internal Use Only Constructors 
        CStruct(CPackage* Package, const FName& InName, uint32 InSize, uint32 InAlignment, EObjectFlags InFlags)
            : CField(Package, InName, InSize, InAlignment, InFlags)
        {}
        //~ End Internal Use Only Constructors
        
        virtual void SetSuperStruct(CStruct* InSuper);
        
        void RegisterDependencies() override;
        
        /** Struct this inherits from, may be null */
        CStruct* GetSuperStruct() const { return SuperStruct; }

        RUNTIME_API FProperty* GetProperty(const FName& Name) const;
        RUNTIME_API virtual void AddProperty(FProperty* Property);

        RUNTIME_API void SerializeTaggedProperties(FArchive& Ar, void* Data);
        
        void Serialize(FArchive& Ar) override { }
        void Serialize(IStructuredArchive::FRecord Slot) override { }
        
        template<typename PropertyType>
        PropertyType* GetProperty(const FName& Name)
        {
            return static_cast<PropertyType*>(GetProperty(Name));
        }

        template<typename PropertyType, typename TFunc>
        requires (eastl::is_base_of_v<FProperty, PropertyType> && eastl::is_invocable_v<TFunc, PropertyType*>)
        void ForEachProperty(TFunc&& Func)
        {
            PropertyType* Current = static_cast<PropertyType*>(LinkedProperty);
            while (Current != nullptr)
            {
                eastl::invoke(Func, Current);
                Current = static_cast<PropertyType*>(Current->Next);
            }
        }
        
        template<class T>
        bool IsChildOf() const
        {
            return IsChildOf(T::StaticClass());
        }
        
        RUNTIME_API bool IsChildOf(const CStruct* Base) const;

        /** Links a derived to it's parent (if one exists) and will link properties. */
        RUNTIME_API virtual void Link();
        
        FFixedString MakeDisplayName() const override;
        
    private:

        TUniquePtr<FStructOps> StructOps;
        
        /** Parent struct */
        CStruct* SuperStruct = nullptr;
        
        bool bLinked = false;
    };



    //--------------------------------------------------------------------------------
    // CClass
    //--------------------------------------------------------------------------------
    
    
    /** Final class for fields and functions. */
    class CClass final : public CStruct
    {
    public:

        DECLARE_CLASS(Lumina, CClass, CStruct, "/Script/Engine", RUNTIME_API)
        DEFINE_CLASS_FACTORY(CClass)

        using FactoryFunctionType = CObject*(*)(void*);
                
        CClass() = default;

        // Begin Internal Use Only Constructors 
        CClass(CPackage* Package, const FName& InName, uint32 InSize, uint32 InAlignment, EObjectFlags InFlags, FactoryFunctionType InFactory)
            : CStruct(Package, InName, InSize, InAlignment, InFlags)
            , FactoryFunction(InFactory)
        {}
        //~ End Internal Use Only Constructors


        RUNTIME_API CObject* EmplaceInstance(void* Memory) const;
        
        RUNTIME_API CClass* GetSuperClass() const;
        
        RUNTIME_API CObject* GetDefaultObject() const;

        template<typename T>
        T* GetDefaultObject() const
        {
            return static_cast<T*>(GetDefaultObject());
        }


        mutable int32   ClassUnique = 0;

        FactoryFunctionType FactoryFunction = nullptr;

    protected:

        RUNTIME_API CObject* CreateDefaultObject();

    private:

        CObject*        ClassDefaultObject = nullptr;

    };

    template<class T>
    void InternalConstructor(const FObjectInitializer& IO)
    { 
        T::__DefaultConstructor(IO);
    }

    template<class T>
    void InternalAllocator(const FObjectInitializer& IO)
    { 
        T::__DefaultAllocator(IO);
    }

    RUNTIME_API void AllocateStaticClass(const TCHAR* Package, const TCHAR* Name, CClass** OutClass, uint32 Size, uint32 Alignment, CClass* (*SuperClassFn)(), CClass::FactoryFunctionType FactoryFunc);
    

    template<typename Class>
    FORCEINLINE FString GetClassName()
    {
    	return Class::StaticClass()->GetName();
    }
    
    template <typename T>
    struct TBaseStructureBase
    {
        static CStruct* Get()
        {
            return T::StaticStruct();
        }
    };

    template <typename T>
    struct TBaseStructure : TBaseStructureBase<T>
    {
    };

    template<> struct TBaseStructure<glm::vec2> 
    {
        static RUNTIME_API CStruct* Get(); 
    };

    template<> struct TBaseStructure<glm::vec3> 
    {
        static RUNTIME_API CStruct* Get(); 
    };
    
    template<> struct TBaseStructure<glm::vec4> 
    {
        static RUNTIME_API CStruct* Get(); 
    };

    template<> struct TBaseStructure<glm::quat> 
    {
        static RUNTIME_API CStruct* Get(); 
    };
}
