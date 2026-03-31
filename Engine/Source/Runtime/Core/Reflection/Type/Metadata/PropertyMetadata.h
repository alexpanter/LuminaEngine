#pragma once

#include "Containers/Array.h"
#include "Containers/Name.h"

namespace Lumina
{
    class RUNTIME_API FMetaDataPair
    {
    public:

        void AddValue(const FName& Key, const FName& Value);

        bool HasMetadata(const FName& Key) const;
        
        const FName* TryGetMetadata(const FName& Key) const;
        const FName& GetMetadata(const FName& Key) const;
    

    private:

        THashMap<FName, FName> PairParams;
    };
}
