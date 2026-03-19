#include "pch.h"
#include "PropertyMetadata.h"

namespace Lumina
{
    void FMetaDataPair::AddValue(const FName& Key, const FName& Value)
    {
        PairParams.emplace(Key, Value);
    }

    bool FMetaDataPair::HasMetadata(const FName& Key) const
    {
        return PairParams.find(Key) != PairParams.end();
    }

    const FName* FMetaDataPair::TryGetMetadata(const FName& Key) const
    {
        auto It = PairParams.find(Key);
        if (It != PairParams.end())
        {
            return &It->second;
        }
        
        return nullptr;
    }

    const FName& FMetaDataPair::GetMetadata(const FName& Key) const
    {
        return PairParams.at(Key);
    }
}
