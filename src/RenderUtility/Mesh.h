#pragma once

#include <string>
#include <vector>
#include <assimp/Importer.hpp>      // C++ importer interface
#include <assimp/scene.h>       // Output data structure
#include <assimp/postprocess.h> // Post processing flags
#include <glm/glm.hpp>
#include <Magick++.h>


namespace glsp {
#include "khronos/GL/glcorearb.h"

class GlspMaterials
{
public:
	GlspMaterials(GLenum TextureTarget, const std::string& FileName);

    bool Load();

    void Bind(GLenum TextureUnit);

private:
    std::string m_fileName;
    GLenum m_textureTarget;
    GLuint m_textureObj;
    Magick::Image* m_pImage;
    Magick::Blob m_blob;
};

struct GlspVertex
{
    glm::vec3 m_pos;
    glm::vec2 m_tex;
    glm::vec3 m_normal;

    GlspVertex() {}

    GlspVertex(const glm::vec3& pos, const glm::vec2& tex, const glm::vec3& normal)
    {
        m_pos    = pos;
        m_tex    = tex;
        m_normal = normal;
    }
};

class GlspMesh
{
public:
    GlspMesh();
    ~GlspMesh();

    bool LoadMesh(const std::string& Filename);
    virtual void Render(bool external_texture = false);

private:
    bool InitFromScene(const aiScene* pScene, const std::string& Filename);
    void InitMesh(unsigned int Index, const aiMesh* paiMesh);
    bool InitMaterials(const aiScene* pScene, const std::string& Filename);
    void Clear();

#define INVALID_MATERIAL 0xFFFFFFFF

    struct GlspMeshEntry {
        GlspMeshEntry();

        ~GlspMeshEntry();

        void Init(const std::vector<GlspVertex>& Vertices,
                  const std::vector<unsigned int>& Indices);

        GLuint VB;
        GLuint IB;
        unsigned int NumIndices;
        unsigned int MaterialIndex;
    };

    std::vector<GlspMeshEntry>  m_Entries;
	std::vector<GlspMaterials*> m_Textures;
};

} // namespace glsp
