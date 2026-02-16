#include "pch.h"
#include "JsonArchiver.h"

#include "Core/Object/Object.h"

namespace Lumina
{
    using json = nlohmann::json;
    
    FJsonArchiver::~FJsonArchiver()
    {
    }

    void FJsonArchiver::Seek(int64 InPos)
    {
        FArchive::Seek(InPos);
    }

    int64 FJsonArchiver::Tell()
    {
        return FArchive::Tell();
    }

    int64 FJsonArchiver::TotalSize()
    {
        return JsonStream.size();
    }

    void FJsonArchiver::Serialize(void* V, int64 Length)
    {
        FArchive::Serialize(V, Length);
    }

    bool FJsonArchiver::IsWriting() const
    {
        return FArchive::IsWriting();
    }

    bool FJsonArchiver::IsReading() const
    {
        return FArchive::IsReading();
    }

    bool FJsonArchiver::HasError() const
    {
        return FArchive::HasError();
    }

    void FJsonArchiver::SerializeType(CStruct* Type, void* Data)
    {
        
    }

    FArchive& FJsonArchiver::operator<<(CObject*& Value)
    {
        JsonStream[Value->GetName().c_str()] = json::object();

        for (int i = 0; i < 10; ++i)
        {
            JsonStream[i] = i;
        }
        
        return FArchive::operator<<(Value);
    }

    FArchive& FJsonArchiver::operator<<(FField*& Value)
    {
        return FArchive::operator<<(Value);
    }

    FArchive& FJsonArchiver::operator<<(FString& Str)
    {
        return FArchive::operator<<(Str);
    }

    FArchive& FJsonArchiver::operator<<(FName& Name)
    {
        return FArchive::operator<<(Name);
    }

    FString FJsonArchiver::Dump() const
    {
        return JsonStream.dump(4).c_str();
    }
}
