#pragma once
#include "Reflector/ReflectionCore/ReflectionDatabase.h"

namespace Lumina::Reflection
{
    class FReflectedWorkspace;
    class FReflectedDatabase;

    class FCodeGenerator
    {
    public:

        FCodeGenerator(FReflectedWorkspace* InWorkspace, const FReflectionDatabase& Database);
        
        void GenerateCode();

        void GenerateReflectionCodeForHeader(FReflectedHeader* Header);
        void GenerateReflectionCodeForSource(FReflectedHeader* Header);
        void GenerateReflectionCodeForLuaAPI(eastl::string& Stream, FReflectedHeader* Header);
    
    private:

        void GenerateCodeHeader(eastl::string& Stream, FReflectedHeader* Header);
        void GenerateCodeSource(eastl::string& Stream, FReflectedHeader* Header);


    private:
        
        FReflectedWorkspace*                    Workspace;
        const FReflectionDatabase*              ReflectionDatabase;
    };
}
