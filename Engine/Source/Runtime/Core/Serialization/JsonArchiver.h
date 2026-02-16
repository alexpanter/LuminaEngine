#pragma once
#include "Archiver.h"
#include "nlohmann/json.hpp"

namespace Lumina
{
    class CStruct;

    class RUNTIME_API FJsonArchiver : public FArchive
    {
    public:
        
        ~FJsonArchiver() override;
        void Seek(int64 InPos) override;
        int64 Tell() override;
        int64 TotalSize() override;
        void Serialize(void* V, int64 Length) override;
        inline bool IsWriting() const override;
        inline bool IsReading() const override;
        inline bool HasError() const override;
        
        void SerializeType(CStruct* Type, void* Data);
        
        
        FArchive& operator<<(CObject*& Value) override;
        FArchive& operator<<(FField*& Value) override;
        FArchive& operator<<(FString& Str) override;
        FArchive& operator<<(FName& Name) override;
        
        FString Dump() const;
        
        
        
    private:
        
        nlohmann::json JsonStream;
    };
}
