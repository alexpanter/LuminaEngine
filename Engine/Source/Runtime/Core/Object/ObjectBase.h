#pragma once

#include <Containers/Name.h>
#include <Containers/String.h>
#include <Core/LuminaMacros.h>
#include <Core/Assertions/Assert.h>
#include <Platform/GenericPlatform.h>
#include "ObjectFlags.h"
#include "GUID/GUID.h"
#include "Initializer/ObjectInitializer.h"

namespace Lumina
{
    struct FMetaDataPairParam;
    class CObjectBase;
    class CClass;
    
    /** Low level implementation of a CObject */
    class CObjectBase
    {
    public:
        friend class FCObjectArray;
        friend class CPackage;
        
        RUNTIME_API CObjectBase();
        RUNTIME_API virtual ~CObjectBase();

        // Non-movable, Non-copyable.
        CObjectBase(const CObjectBase&) = delete;
        CObjectBase(CObjectBase&&) = delete;

        CObjectBase& operator=(const CObjectBase&) = delete;
        CObjectBase& operator=(CObjectBase&&) = delete;
        
        RUNTIME_API virtual void ConstructInternal(const FObjectInitializer& OI);
        
        RUNTIME_API CObjectBase(EObjectFlags InFlags);
        RUNTIME_API CObjectBase(CClass* InClass, EObjectFlags InFlags, CPackage* Package, FName InName, const FGuid& GUID);
        
        RUNTIME_API void BeginRegister();
        RUNTIME_API void FinishRegister(CClass* InClass, const TCHAR* InName);

        RUNTIME_API EObjectFlags GetFlags() const { return ObjectFlags; }

        RUNTIME_API void ClearFlags(EObjectFlags Flags) const { EnumRemoveFlags(ObjectFlags, Flags); }
        RUNTIME_API void SetFlag(EObjectFlags Flags) const { EnumAddFlags(ObjectFlags, Flags); }
        RUNTIME_API bool HasAnyFlag(EObjectFlags Flag) const { return EnumHasAnyFlags(ObjectFlags, Flag); }
        RUNTIME_API bool HasAllFlags(EObjectFlags Flags) const { return EnumHasAllFlags(ObjectFlags, Flags); }

        RUNTIME_API void ForceDestroyNow();
        RUNTIME_API void ConditionalBeginDestroy();
        RUNTIME_API int32 GetStrongRefCount() const;

        /** Low level object rename, will remove and reload into hash buckets. Only call this if you've verified it's safe */
        RUNTIME_API void HandleNameChange(const FName& NewName, CPackage* NewPackage = nullptr) noexcept;

        /** Called just before the destructor is called and the memory is freed */
        RUNTIME_API virtual void OnDestroy() { }

        /** Internal index into the CObjectArray */
        RUNTIME_API int32 GetInternalIndex() const { return InternalIndex; }

        /** Adds the object the root set, rooting an object will ensure it will not be destroyed */
        RUNTIME_API void AddToRoot();

        /** Removes the object from the root set. */
        RUNTIME_API void RemoveFromRoot();
        
        /** Strips away common CObject prefixes */
        RUNTIME_API virtual FFixedString MakeDisplayName() const;
        
    private:

        RUNTIME_API void AddObject();
        
    public:
        
        /** Force any base classes to be registered first. */
        RUNTIME_API virtual void RegisterDependencies() { }

        /** Returns the CClass that defines the fields of this object */
        CClass* GetClass() const
        {
            return ClassPrivate;
        }

        /** Get the internal low level name of this object */
        FName GetName() const
        {
            return NamePrivate;
        }

        /** Get the internal globally unique ID for this CObject */
        const FGuid& GetGUID() const
        {
            return GUIDPrivate;
        }

        /** Get the internal package this object came from (script, plugin, package, etc.). */
        CPackage* GetPackage() const
        {
            return PackagePrivate;
        }

        /** Loader index from asset loading */
        RUNTIME_API int64 GetLoaderIndex() const
        {
            return LoaderIndex;
        }
    
    private:
        
        template<typename TClassType>
        static bool IsAHelper(const TClassType* Class, const TClassType* TestClass)
        {
            return Class->IsChildOf(TestClass);
        }
        
    public:

        // This exists to fix the cyclical dependency between CObjectBase and CClass.
        template<typename OtherClassType>
        bool IsA(OtherClassType Base) const
        {
            const CClass* SomeBase = Base;
            (void)SomeBase;

            const CClass* ThisClass = GetClass();

            // Stop compiler from doing un-necessary branching for nullptr checks.
            ASSUME(SomeBase);
            ASSUME(ThisClass);

            return IsAHelper(ThisClass, SomeBase);
            
        }
        
        template<typename T>
        bool IsA() const
        {
            return IsA(T::StaticClass());
        }

    private:

        /** Flags used to track various object states. */
        mutable EObjectFlags    ObjectFlags;

        /** Class this object belongs to */
        CClass*                 ClassPrivate = nullptr;

        /** Package to represent on disk */
        CPackage*               PackagePrivate = nullptr;
        
        /** Logical name of this object. */
        FName                   NamePrivate;

        /** Globally unique identifier for this object */
        FGuid                   GUIDPrivate;
        
        /** Internal index into the global object array. */
        int32                   InternalIndex = -1;

        /** Index into this object's package export map */
        int32                   LoaderIndex = 0;
    };

//---------------------------------------------------------------------------------------------------
    

    /** Helper for static registration in LRT code */
    struct FRegisterCompiledInInfo
    {
        template<typename ... TArgs>
        FRegisterCompiledInInfo(TArgs&& ... Args)
        {
            RegisterCompiledInInfo(std::forward<TArgs>(Args)...);
        }
    };

    struct FClassRegisterCompiledInInfo
    {
        class CClass* (*RegisterFn)();
        const TCHAR* Package;
        const TCHAR* Name;
        
        uint16 NumMetaData;
        const FMetaDataPairParam* MetaDataArray;
    };

    struct FStructRegisterCompiledInInfo
    {
        class CStruct* (*RegisterFn)();
        const TCHAR* Name;
        
        uint16 NumMetaData;
        const FMetaDataPairParam* MetaDataArray;
    };

    struct FEnumRegisterCompiledInInfo
    {
        class CEnum* (*RegisterFn)();
        const TCHAR* Name;
        
        uint16 NumMetaData;
        const FMetaDataPairParam* MetaDataArray;
    };


    RUNTIME_API void InitializeCObjectSystem();
    RUNTIME_API void ShutdownCObjectSystem();

    /** Invoked from static constructor in generated code */
    RUNTIME_API void RegisterCompiledInInfo(CClass* (*RegisterFn)(), const TCHAR* Package, const TCHAR* Name);

    RUNTIME_API void RegisterCompiledInInfo(CEnum* (*RegisterFn)(), const FEnumRegisterCompiledInInfo& Info);

    RUNTIME_API void RegisterCompiledInInfo(CStruct* (*RegisterFn)(), const FStructRegisterCompiledInInfo& Info);
    
    RUNTIME_API void RegisterCompiledInInfo(const FClassRegisterCompiledInInfo* Info, size_t NumClassInfo);

    RUNTIME_API void RegisterCompiledInInfo(const FEnumRegisterCompiledInInfo* EnumInfo, size_t NumEnumInfo, const FClassRegisterCompiledInInfo* ClassInfo, size_t NumClassInfo);

    RUNTIME_API void RegisterCompiledInInfo(const FEnumRegisterCompiledInInfo* EnumInfo, size_t NumEnumInfo, const FClassRegisterCompiledInInfo* ClassInfo, size_t NumClassInfo, const FStructRegisterCompiledInInfo* StructInfo, size_t NumStructInfo);

    RUNTIME_API CEnum* GetStaticEnum(CEnum* (*RegisterFn)(), const TCHAR* Name);
    
    RUNTIME_API void CObjectForceRegistration(CObjectBase* Object);

    
    RUNTIME_API void ProcessNewlyLoadedCObjects();
}
