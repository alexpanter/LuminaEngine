#include "LuminaEditor.h"
#include <fstream>
#include <Core/Engine/Engine.h>
#include <Memory/Memory.h>
#include <Tools/UI/DevelopmentToolUI.h>
#include "Config/Config.h"
#include "FileSystem/FileSystem.h"
#include "Paths/Paths.h"
#include "UI/EditorUI.h"
#include "World/WorldManager.h"

namespace Lumina
{
    EDITOR_API FEditorEngine* GEditorEngine = nullptr;
    
    bool FEditorEngine::Init()
    {
        VFS::Mount<VFS::FNativeFileSystem>("/Editor", Paths::Combine(Paths::GetEngineDirectory(), "Editor"));
        
        GConfig->LoadPath("/Editor/Config");
        
        bool bSuccess = FEngine::Init();
        
        entt::locator<entt::meta_ctx>::reset(GetEngineMetaService());

        return bSuccess;
    }

    bool FEditorEngine::Shutdown()
    {
        return FEngine::Shutdown();
    }

    CWorld* FEditorEngine::GetCurrentEditorWorld() const
    {
        return nullptr;
    }

    IDevelopmentToolUI* FEditorEngine::CreateDevelopmentTools()
    {
        return Memory::New<FEditorUI>();
    }

    void FEditorEngine::CreateProject(FStringView NewProjectName, FStringView NewProjectPath)
    {
        FFixedString BlankProjectPath = Paths::Combine(Paths::GetEngineInstallDirectory(), "Templates", "Blank");
        
        FFixedString Combined = Paths::Combine(NewProjectPath, NewProjectName);
        std::filesystem::create_directories(Combined.c_str());
        
        for (auto& Entry : std::filesystem::recursive_directory_iterator(BlankProjectPath.c_str()))
        {
            std::filesystem::path RelativePath = std::filesystem::relative(Entry.path(), BlankProjectPath.c_str());
            
            FFixedString RelativePathStr = RelativePath.string().c_str();
            eastl::replace(RelativePathStr.begin(), RelativePathStr.end(), '\\', '/');
            
            size_t Pos = 0;
            while ((Pos = RelativePathStr.find("$PROJECTNAME", Pos)) != FFixedString::npos)
            {
                RelativePathStr.replace(Pos, 12, NewProjectName.data());
                Pos += NewProjectName.size();
            }
            
            FFixedString DestPath = Paths::Combine(Combined, RelativePathStr);
            
            if (Entry.is_directory())
            {
                std::filesystem::create_directories(DestPath.c_str());
            }
            else if (Entry.is_regular_file())
            {
                std::filesystem::path DestPathFS(DestPath.c_str());
                std::filesystem::create_directories(DestPathFS.parent_path());
                
                std::ifstream InputFile(Entry.path(), std::ios::binary);
                if (!InputFile.is_open())
                {
                    continue;
                }
    
                std::string FileContent((std::istreambuf_iterator<char>(InputFile)), std::istreambuf_iterator<char>());
                InputFile.close();
                
                Pos = 0;
                while ((Pos = FileContent.find("$PROJECTNAME", Pos)) != std::string::npos)
                {
                    FileContent.replace(Pos, 12, NewProjectName.data());
                    Pos += NewProjectName.size();
                }
                
                std::ofstream OutputFile(DestPath.c_str(), std::ios::binary);
                if (OutputFile.is_open())
                {
                    OutputFile.write(FileContent.data(), FileContent.size());
                    OutputFile.close();
                }
            }
        }
    }
}
