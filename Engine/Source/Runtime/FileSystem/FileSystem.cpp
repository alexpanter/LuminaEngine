#include "pch.h"
#include "FileSystem.h"

#include <ranges>

#include "Containers/Function.h"
#include "Core/Templates/LuminaTemplate.h"
#include "Paths/Paths.h"


namespace Lumina::VFS
{
    static THashMap<FFixedString, TVector<FFileSystem>> FileSystemStorage;
    
    namespace Detail
    {
        template<typename TFunc, typename TFileSystemType = void>
        static auto VisitFileSystems(FStringView Path, TFunc&& Func) -> decltype(Func(eastl::declval<FFileSystem&>()))
        {
            constexpr bool bIsVoid = eastl::is_same_v<decltype(Func(eastl::declval<FFileSystem&>())), void>;

            for (auto& [Alias, FileSystems] : FileSystemStorage)
            {
                if (!Path.starts_with(Alias))
                {
                    continue;
                }
    
                for (FFileSystem& FileSystem : std::ranges::reverse_view(FileSystems))
                {
                    if constexpr (bIsVoid)
                    {
                        Func(FileSystem);
                    }
                    else
                    {
                        if (auto Result = Func(FileSystem))
                        {
                            return Result;
                        }   
                    }
                }
            }
            
            if constexpr (!bIsVoid)
            {
                return {};
            }
        }
    }
    
    bool FFileSystem::ReadFile(TVector<uint8>& Result, FStringView Path)
    {
        return eastl::visit([&](auto& fs) { return fs.ReadFile(Result, Path); }, Storage);
    }

    bool FFileSystem::ReadFile(FString& OutString, FStringView Path)
    {
        return eastl::visit([&](auto& fs) { return fs.ReadFile(OutString, Path); }, Storage);
    }

    bool FFileSystem::WriteFile(FStringView Path, FStringView Data)
    {
        return eastl::visit([&](auto& fs) { return fs.WriteFile(Path, Data); }, Storage);
    }

    bool FFileSystem::WriteFile(FStringView Path, TSpan<const uint8> Data)
    {
        return eastl::visit([&](auto& fs) { return fs.WriteFile(Path, Data); }, Storage);
    }

    bool FFileSystem::Exists(FStringView Path) const
    {
        return eastl::visit([&](const auto& fs) { return fs.Exists(Path); }, Storage);
    }

    bool FFileSystem::IsDirectory(FStringView Path) const
    {
        return eastl::visit([&](const auto& fs) { return fs.IsDirectory(Path); }, Storage);
    }

    bool FFileSystem::CreateDir(FStringView Path) const
    {
        return eastl::visit([&](const auto& fs) { return fs.CreateDir(Path); }, Storage);
    }

    bool FFileSystem::Remove(FStringView Path) const
    {
        return eastl::visit([&](const auto& fs) { return fs.Remove(Path); }, Storage);
    }

    size_t FFileSystem::Size(FStringView Path) const
    {
        return eastl::visit([&](const auto& fs) { return fs.Size(Path); }, Storage);
    }

    bool FFileSystem::RemoveAll(FStringView Path) const
    {
        return eastl::visit([&](const auto& fs) { return fs.RemoveAll(Path); }, Storage);
    }

    bool FFileSystem::Rename(FStringView Old, FStringView New) const
    {
        return eastl::visit([&](const auto& fs) { return fs.Rename(Old, New); }, Storage);
    }

    void FFileSystem::DirectoryIterator(FStringView Path, const TFunction<void(const FFileInfo&)>& Callback) const
    {
        eastl::visit([&](const auto& fs) { fs.DirectoryIterator(Path, Callback); }, Storage);
    }

    void FFileSystem::RecursiveDirectoryIterator(FStringView Path, const TFunction<void(const FFileInfo&)>& Callback) const
    {
        eastl::visit([&](const auto& fs) { fs.RecursiveDirectoryIterator(Path, Callback); }, Storage);
    }

    bool FFileSystem::IsEmpty(FStringView Path) const
    {
        return eastl::visit([&](const auto& fs) { return fs.IsEmpty(Path); }, Storage);
    }

    void FFileSystem::PlatformOpen(FStringView Path) const
    {
        eastl::visit([&](const auto& fs) { return fs.PlatformOpen(Path); }, Storage);
    }

    FStringView FFileSystem::GetAliasPath() const
    {
        return eastl::visit([](const auto& fs) { return fs.GetAliasPath(); }, Storage);
    }

    FStringView FFileSystem::GetBasePath() const
    {
        return eastl::visit([](const auto& fs) { return fs.GetBasePath(); }, Storage);
    }
    
    FStringView Extension(FStringView Path)
    {
        size_t Dot = Path.find_last_of('.');
        if (Dot == FString::npos)
        {
            return {};
        }

        return Path.substr(Dot);
    }

    FStringView FileName(FStringView Path, bool bRemoveExtension)
    {
        size_t LastSlash = Path.find_last_of("/\\");
        if (LastSlash == FString::npos)
        {
            return {};
        }
        
        FStringView FilePart = Path.substr(LastSlash + 1);

        if (bRemoveExtension)
        {
            size_t DotPos = FilePart.find_last_of('.');
            if (DotPos != FString::npos)
            {
                return FilePart.substr(0, DotPos);
            }
        }

        return Move(FilePart);
    }

    bool Remove(FStringView Path)
    {
        bool VisitResult = Detail::VisitFileSystems(Path, [&](FFileSystem& FS)
        {
            return FS.Remove(Path);
        });
        
        return VisitResult;
    }

    size_t Size(FStringView Path)
    {
        return Detail::VisitFileSystems(Path, [&](FFileSystem& FS)
        {
            return FS.Size(Path);
        });
    }

    bool RemoveAll(FStringView Path)
    {
        bool VisitResult = Detail::VisitFileSystems(Path, [&](FFileSystem& FS)
        {
            return FS.RemoveAll(Path);
        });
        
        return VisitResult;
    }

    FFixedString ResolvePath(FStringView Path)
    {
        return {};
    }

    bool CreateDir(FStringView Path)
    {
        bool VisitResult = Detail::VisitFileSystems(Path, [&](FFileSystem& FS)
        {
            return FS.CreateDir(Path);
        });
        
        return VisitResult;
    }

    bool IsUnderDirectory(FStringView Parent, FStringView Path)
    {
        if (!Path.starts_with(Parent))
        {
            return false;
        }
    
        return Path.length() == Parent.length() || 
               Path[Parent.length()] == '/' || 
               Path[Parent.length()] == '\\';
    }

    bool IsDirectory(FStringView Path)
    {
        bool VisitResult = Detail::VisitFileSystems(Path, [&](FFileSystem& FS)
        {
            return FS.IsDirectory(Path);
        });
        
        return VisitResult;
    }

    bool IsLuaAsset(FStringView Path)
    {
        return Extension(Path) == ".luau";
    }

    bool IsLuminaAsset(FStringView Path)
    {
        return Extension(Path) == ".lasset";
    }

    FStringView Parent(FStringView Path, bool bRemoveTrailingSlash)
    {
        size_t Pos = Path.find_last_of('/');
        if (Pos == FString::npos)
        {
            return {};
        }
        
        return Path.substr(0, bRemoveTrailingSlash ? Pos : Pos + 1);
    }

    bool ReadFile(TVector<uint8>& Result, FStringView Path)
    {
        bool VisitResult = Detail::VisitFileSystems(Path, [&](FFileSystem& FS)
        {
            if (FS.Exists(Path))
            {
                if (FS.ReadFile(Result, Path))
                {
                    return true;
                }
            }
            
            return false;
        });
        
        return VisitResult;
    }

    bool ReadFile(FString& OutString, FStringView Path)
    {
        bool VisitResult = Detail::VisitFileSystems(Path, [&](FFileSystem& FS)
        {
            if (FS.Exists(Path))
            {
                if (FS.ReadFile(OutString, Path))
                {
                    return true;
                }
            }
            
            return false;
        });
        
        return VisitResult;   
    }

    bool WriteFile(FStringView Path, FStringView Data)
    {
        bool VisitResult = Detail::VisitFileSystems(Path, [&](FFileSystem& FS)
        {
            return FS.WriteFile(Path, Data);
        });
        
        return VisitResult;
    }

    bool WriteFile(FStringView Path, TSpan<const uint8> Data)
    {
        bool VisitResult = Detail::VisitFileSystems(Path, [&](FFileSystem& FS)
        {
            return FS.WriteFile(Path, Data);
        });
        
        return VisitResult;
    }

    void PlatformOpen(FStringView Path)
    {
        Detail::VisitFileSystems(Path, [&](FFileSystem& FS)
        {
            if (FS.Exists(Path))
            {
                FS.PlatformOpen(Path);
            }
        });
    }

    bool Exists(FStringView Path)
    {
        bool Result = Detail::VisitFileSystems(Path, [&](FFileSystem& FS)
        {
            return FS.Exists(Path);
        });
        
        return Result;
    }
    
    void DirectoryIterator(FStringView Path, const TFunction<void(const FFileInfo&)>& Callback)
    {
        Detail::VisitFileSystems(Path, [&](const FFileSystem& FS)
        {
            FS.DirectoryIterator(Path, Callback);
        });
    }

    void RecursiveDirectoryIterator(FStringView Path, const TFunction<void(const FFileInfo&)>& Callback)
    {
        Detail::VisitFileSystems(Path, [&](const FFileSystem& FS)
        {
            FS.RecursiveDirectoryIterator(Path, Callback);
        });
    }

    bool IsEmpty(FStringView Directory)
    {
        return Detail::VisitFileSystems(Directory, [&](const FFileSystem& FS)
        {
            if (FS.Exists(Directory))
            {
                if (FS.IsEmpty(Directory))
                {
                    return true;
                }
            }
            
            return false;        
        });
    }

    FStringView RemoveExtension(FStringView Path)
    {
        size_t Dot = Path.find_last_of(".");
        if (Dot != FString::npos)
        {
            return Path.substr(0, Dot);
        }

        return Path;
    }

    bool Rename(FStringView Old, FStringView New)
    {
        bool Result = Detail::VisitFileSystems(Old, [&](FFileSystem& FS)
        {
            if (FS.Exists(Old))
            {
                if (FS.Rename(Old, New))
                {
                    return true;
                }
            }
            
            return false;
        });
        
        return Result;
    }

    FFixedString MakeUniqueFilePath(FStringView BasePath)
    {
        FStringView Ext = Extension(BasePath);
        FStringView NoExtensionPath = RemoveExtension(BasePath);
    
        FFixedString ReturnPath(BasePath.begin(), BasePath.end());
    
        if (!Exists(ReturnPath))
        {
            return Move(ReturnPath);
        }
    
        int32 Counter = 1;
        while (Counter <= 25)
        {
            ReturnPath.clear();
            ReturnPath.append(NoExtensionPath.begin(), NoExtensionPath.end());
            ReturnPath.append("_");
            ReturnPath.append_convert(eastl::to_string(Counter));
        
            if (!Ext.empty())
            {
                ReturnPath.append(Ext.begin(), Ext.end());
            }
        
            if (!Exists(ReturnPath))
            {
                return Move(ReturnPath);
            }
        
            Counter++;
        }
    
        return {};
    }
    
    FFileSystem& Detail::AddFileSystemImpl(const FFixedString& Alias, FFileSystem&& System)
    {
        FFixedString Normalized = Paths::Normalize(Alias);
        return FileSystemStorage[Normalized].emplace_back(Move(System));
    }

    TVector<FFileSystem>* Detail::GetFileSystems(const FFixedString& Alias)
    {
        FFixedString Normalized = Paths::Normalize(Alias);

        auto It = FileSystemStorage.find(Normalized);
        if (It == FileSystemStorage.end())
        {
            return nullptr;
        }
        
        return &It->second;
    }

    bool HasExtension(FStringView Path, FStringView Ext)
    {
        Path = Paths::Normalize(Path);
        Ext = Paths::Normalize(Ext);
        size_t Dot = Path.find_last_of('.');
        if (Dot == FString::npos)
        {
            return false;
        }

        FStringView ActualExt = Path.substr(Dot);
        return ActualExt == Ext;
    }
}
