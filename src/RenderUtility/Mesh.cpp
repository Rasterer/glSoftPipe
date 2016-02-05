#include "Mesh.h"

#include <cassert>
#include <cstdio>


namespace glsp {

GlspMaterials::GlspMaterials(GLenum TextureTarget, const std::string& FileName)
{
    m_textureTarget = TextureTarget;
    m_fileName      = FileName;
    m_pImage        = NULL;
}

bool GlspMaterials::Load()
{
    try {
		m_pImage = new Magick::Image(m_fileName);
        m_pImage->write(&m_blob, "RGBA");
    }
    catch (Magick::Error& Error) {
        printf("Error loading texture '%s'\n", Error.what());
        return false;
    }

    glGenTextures(1, &m_textureObj);
    glBindTexture(m_textureTarget, m_textureObj);
    glTexImage2D(m_textureTarget, 0, GL_RGBA, m_pImage->columns(), m_pImage->rows(), 0, GL_RGBA, GL_UNSIGNED_BYTE, m_blob.data());
    glTexParameteri(m_textureTarget, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(m_textureTarget, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    return true;
}

void GlspMaterials::Bind(GLenum TextureUnit)
{
    glActiveTexture(TextureUnit);
    glBindTexture(m_textureTarget, m_textureObj);
}


GlspMesh::GlspMeshEntry::GlspMeshEntry()
{
    VB = 0xffffffff;
    IB = 0xffffffff;
    NumIndices  = 0;
    MaterialIndex = 0xffffffff;
};

GlspMesh::GlspMeshEntry::~GlspMeshEntry()
{
    if (VB != 0xffffffff)
    {
        glDeleteBuffers(1, &VB);
    }

    if (IB != 0xffffffff)
    {
        glDeleteBuffers(1, &IB);
    }
}

void GlspMesh::GlspMeshEntry::Init(const std::vector<GlspVertex>& Vertices,
                          const std::vector<unsigned int>& Indices)
{
    NumIndices = Indices.size();

    glGenBuffers(1, &VB);
  	glBindBuffer(GL_ARRAY_BUFFER, VB);
	glBufferData(GL_ARRAY_BUFFER, sizeof(GlspVertex) * Vertices.size(), &Vertices[0], GL_STATIC_DRAW);

    glGenBuffers(1, &IB);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, IB);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int) * NumIndices, &Indices[0], GL_STATIC_DRAW);
}

GlspMesh::GlspMesh()
{
}


GlspMesh::~GlspMesh()
{
    Clear();
}


void GlspMesh::Clear()
{
    for (unsigned int i = 0 ; i < m_Textures.size() ; i++)
    {
        if(m_Textures[i])
        {
            delete m_Textures[i];
            m_Textures[i] = NULL;
        }
    }
}


bool GlspMesh::LoadMesh(const std::string& Filename)
{
    // Release the previously loaded mesh (if it exists)
    Clear();
    
    bool Ret = false;
    Assimp::Importer Importer;

    const aiScene* pScene = Importer.ReadFile(Filename.c_str(), aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_FlipUVs);
    
    if (pScene) {
        Ret = InitFromScene(pScene, Filename);
    }
    else {
        printf("LoadMesh fail: %s\n", Importer.GetErrorString());
        assert(0);
    }

    return Ret;
}

bool GlspMesh::InitFromScene(const aiScene* pScene, const std::string& Filename)
{  
    m_Entries.resize(pScene->mNumMeshes);
    m_Textures.resize(pScene->mNumMaterials);

    // Initialize the meshes in the scene one by one
    for (unsigned int i = 0 ; i < m_Entries.size() ; i++) {
        const aiMesh* paiMesh = pScene->mMeshes[i];
        InitMesh(i, paiMesh);
    }

    return InitMaterials(pScene, Filename);
}

void GlspMesh::InitMesh(unsigned int Index, const aiMesh* paiMesh)
{
    m_Entries[Index].MaterialIndex = paiMesh->mMaterialIndex;
    
    std::vector<GlspVertex> Vertices;
    std::vector<unsigned int> Indices;

    const aiVector3D Zero3D(0.0f, 0.0f, 0.0f);

    for (unsigned int i = 0 ; i < paiMesh->mNumVertices ; i++) {
        const aiVector3D* pPos      = &(paiMesh->mVertices[i]);
        const aiVector3D* pNormal   = &(paiMesh->mNormals[i]);
        const aiVector3D* pTexCoord = paiMesh->HasTextureCoords(0) ? &(paiMesh->mTextureCoords[0][i]) : &Zero3D;

		GlspVertex v(glm::vec3(pPos->x, pPos->y, pPos->z),
                 glm::vec2(pTexCoord->x, pTexCoord->y),
                 glm::vec3(pNormal->x, pNormal->y, pNormal->z));

        Vertices.push_back(v);
    }

    for (unsigned int i = 0 ; i < paiMesh->mNumFaces ; i++) {
        const aiFace& Face = paiMesh->mFaces[i];
        assert(Face.mNumIndices == 3);
        Indices.push_back(Face.mIndices[0]);
        Indices.push_back(Face.mIndices[1]);
        Indices.push_back(Face.mIndices[2]);
    }

    m_Entries[Index].Init(Vertices, Indices);
}

bool GlspMesh::InitMaterials(const aiScene* pScene, const std::string& Filename)
{
    // Extract the directory part from the file name
    std::string::size_type SlashIndex = Filename.find_last_of("/");
    std::string Dir;

    if (SlashIndex == std::string::npos) {
        Dir = ".";
    }
    else if (SlashIndex == 0) {
        Dir = "/";
    }
    else {
        Dir = Filename.substr(0, SlashIndex);
    }

    bool Ret = true;

    // Initialize the materials
    for (unsigned int i = 0 ; i < pScene->mNumMaterials ; i++) {
        const aiMaterial* pMaterial = pScene->mMaterials[i];

        m_Textures[i] = NULL;

        if (pMaterial->GetTextureCount(aiTextureType_DIFFUSE) > 0) {
            aiString Path;

            if (pMaterial->GetTexture(aiTextureType_DIFFUSE, 0, &Path, NULL, NULL, NULL, NULL, NULL) == AI_SUCCESS) {
                std::string FullPath = Dir + "/" + Path.data;
				m_Textures[i] = new GlspMaterials(GL_TEXTURE_2D, FullPath);

                if (!m_Textures[i]->Load()) {
                    printf("Error loading texture '%s'\n", FullPath.c_str());
                    delete m_Textures[i];
                    m_Textures[i] = NULL;
                    Ret = false;
                }
                else {
                    printf("Loaded texture '%s'\n", FullPath.c_str());
                }
            }
        }

        // Load a white texture in case the model does not include its own texture
        if (!m_Textures[i]) {
            printf("No bound texture, load a white texture.\n");
            m_Textures[i] = new GlspMaterials(GL_TEXTURE_2D, GLSP_ROOT "/assets/white.png");

            Ret = m_Textures[i]->Load();
        }
    }

    return Ret;
}

void GlspMesh::Render(bool external_texture)
{
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glEnableVertexAttribArray(2);

    for (unsigned int i = 0 ; i < m_Entries.size() ; i++) {
        glBindBuffer(GL_ARRAY_BUFFER, m_Entries[i].VB);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(GlspVertex), 0);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(GlspVertex), (const GLvoid*)12);
        glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(GlspVertex), (const GLvoid*)20);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_Entries[i].IB);

        const unsigned int MaterialIndex = m_Entries[i].MaterialIndex;

        if (!external_texture && MaterialIndex < m_Textures.size() && m_Textures[MaterialIndex]) {
            m_Textures[MaterialIndex]->Bind(GL_TEXTURE0);
        }

        glDrawElements(GL_TRIANGLES, m_Entries[i].NumIndices, GL_UNSIGNED_INT, 0);
    }

    glDisableVertexAttribArray(0);
    glDisableVertexAttribArray(1);
    glDisableVertexAttribArray(2);
}

} // namespace glsp
