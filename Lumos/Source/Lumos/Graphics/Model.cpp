#include "Precompiled.h"
#include "Model.h"
#include "Mesh.h"
#include "Utilities/StringUtilities.h"
#include "Core/OS/FileSystem.h"

namespace Lumos::Graphics
{
    Model::Model(const std::string& filePath)
        : m_FilePath(filePath)
        , m_PrimitiveType(PrimitiveType::File)
    {
        LoadModel(m_FilePath);
    }

    Model::Model(const SharedPtr<Mesh>& mesh, PrimitiveType type)
        : m_FilePath("Primitive")
        , m_PrimitiveType(type)
    {
        m_Meshes.push_back(mesh);
    }

    Model::Model(PrimitiveType type)
        : m_FilePath("Primitive")
        , m_PrimitiveType(type)
    {
        m_Meshes.push_back(SharedPtr<Mesh>(CreatePrimative(type)));
    }

    Model::~Model()
    {
    }

    void Model::LoadModel(const std::string& path)
    {
        LUMOS_PROFILE_FUNCTION();
        std::string physicalPath;
        if(!Lumos::FileSystem::Get().ResolvePhysicalPath(path, physicalPath))
        {
            LUMOS_LOG_INFO("Failed to load Model - {0}", path);
            return;
        }

        std::string resolvedPath = physicalPath;

        const std::string fileExtension = StringUtilities::GetFilePathExtension(path);

        if(fileExtension == "obj")
            LoadOBJ(resolvedPath);
        else if(fileExtension == "gltf" || fileExtension == "glb")
            LoadGLTF(resolvedPath);
        else if(fileExtension == "fbx" || fileExtension == "FBX")
            LoadFBX(resolvedPath);
        else
            LUMOS_LOG_ERROR("Unsupported File Type : {0}", fileExtension);

        LUMOS_LOG_INFO("Loaded Model - {0}", path);
    }
}
