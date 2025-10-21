#pragma once

#include "Texture.h"

#include "tiny_gltf.h" 

#include <GL/glew.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <string>
#include <fstream>
#include <vector>
#include <iostream>
#include <sstream>
#include <map>
#include <stdexcept>
#include <cstdlib>

namespace Renderer
{
    class Model
    {
    public:
        Model();
        Model(const std::string& _path);
        Model(const Model& _copy);
        Model& operator=(const Model& _assign);
        virtual ~Model();

        GLsizei vertex_count() const;
        GLuint vao_id();

        void Unload();

        float get_width() const;
        float get_height() const;
        float get_length() const;

        struct Vertex
        {
            //Vertex();
            glm::vec3 position = glm::vec3(0, 0, 0);
            glm::vec2 texcoord = glm::vec2(0, 0);
            glm::vec3 normal = glm::vec3(0, 0, 0);
        };

        struct Face
        {
            Vertex a;
            Vertex b;
            Vertex c;
        };

        const std::vector<Model::Face>& GetFaces() const;

        // Returns true if the model was loaded with material support.
        bool usesMaterials() const { return m_useMaterials; }

        struct PBRMaterial
        {
            enum class AlphaMode { AlphaOpaque = 0, AlphaMask = 1, AlphaBlend = 2 };

            int baseColorTexIndex = -1;            // index into m_embeddedTextures
            int normalTexIndex = -1;
            int metallicRoughnessTexIndex = -1;
            int occlusionTexIndex = -1;
            int emissiveTexIndex = -1;

			// Material properties, filled with default values if not specified
            glm::vec4 baseColorFactor = glm::vec4(1.0f);
            float metallicFactor = 0.0f;
            float roughnessFactor = 1.0f;

            AlphaMode alphaMode = AlphaMode::AlphaOpaque;
            float alphaCutoff = 0.5f;

            bool doubleSided = false;

            // Normal/AO/emissive scalar controls from core glTF
            float normalScale = 1.0f;
            float occlusionStrength = 1.0f;
            glm::vec3 emissiveFactor = glm::vec3(0.0f);

            // KHR materials (specular/glossiness extension)
            float transmissionFactor = 0.0f;
            int transmissionTexIndex = -1;
			float ior = 1.5f;
        };

        // Structure for material-specific geometry.
        struct MaterialGroup
        {
            std::string materialName;
            std::vector<Face> faces;
            std::string texturePath; // Diffuse texture from the MTL file.

            glm::vec3 boundsCenterMS = glm::vec3(0.0f);
            glm::vec3 boundsHalfExtentsMS = glm::vec3(0.0f);
            float boundsSphereRadiusMS = 0.0f;


            PBRMaterial pbr;

            GLuint vao = 0;
            GLuint vbo = 0;
        };

        // Returns the material groups (for multi-textured models).
        const std::vector<MaterialGroup>& GetMaterialGroups() const { return m_materialGroups; }

		const std::vector<Texture>& GetEmbeddedTextures() const { return m_embeddedTextures; }

    private:
        // Legacy face data.
        std::vector<Face> m_faces;
        // Material groups when using multi-material mode.
        std::vector<MaterialGroup> m_materialGroups;

        GLuint m_vaoid = 0;
        GLuint m_vboid = 0;
        bool m_dirty = true;

        float m_width = 0.0f;
        float m_height = 0.0f;
        float m_length = 0.0f;

        // Flag indicating if the model uses materials.
        bool m_useMaterials = false;

        std::vector<Texture> m_embeddedTextures;

        void split_string_whitespace(const std::string& _input, std::vector<std::string>& _output);
        void split_string(const std::string& _input, char _splitter, std::vector<std::string>& _output);
        void calculate_dimensions();

        // Helper to load a MTL file mapping material names to diffuse texture paths.
        void LoadMTL(const std::string& mtlFilePath, std::map<std::string, std::string>& materialToTextureMap);

        bool LoadGLTF(const std::string& path);
    };

    inline Model::Model()
    {
    }

    inline Model::Model(const std::string& _path)
    {
        // extract extension
        std::string ext;
        size_t dot = _path.find_last_of('.');
        if (dot != std::string::npos)
            ext = _path.substr(dot + 1);

        if (ext == "glb" || ext == "gltf")
        {
            if (!LoadGLTF(_path))
                throw std::runtime_error("Failed to load GLTF model: " + _path);
            calculate_dimensions();
            m_dirty = false;
            return;
        }

        std::vector<glm::vec3> positions;
        std::vector<glm::vec2> tcs;
        std::vector<glm::vec3> normals;
        std::string currentLine;

        std::string directory;
        size_t found = _path.find_last_of("/\\");
        if (found != std::string::npos)
            directory = _path.substr(0, found + 1);

        std::ifstream file(_path.c_str());
        if (!file.is_open())
        {
            std::cout << "Failed to open file: " << _path << std::endl;
            throw std::runtime_error("Failed to open model file");
        }

        std::map<std::string, std::string> materialToTexture;
        m_useMaterials = false;
        std::string currentMaterial = "default";
        std::map<std::string, size_t> materialGroupIndices;

        while (std::getline(file, currentLine))
        {
            if (currentLine.empty()) continue;
            std::vector<std::string> tokens;
            split_string_whitespace(currentLine, tokens);
            if (tokens.empty()) continue;

            if (tokens.at(0) == "v" && tokens.size() >= 4)
            {
                glm::vec3 p(atof(tokens.at(1).c_str()),
                    atof(tokens.at(2).c_str()),
                    atof(tokens.at(3).c_str()));
                positions.push_back(p);
            }
            else if (tokens.at(0) == "vt" && tokens.size() >= 3)
            {
                glm::vec2 tc(atof(tokens.at(1).c_str()),
                    1.0f - atof(tokens.at(2).c_str()));
                tcs.push_back(tc);
            }
            else if (tokens.at(0) == "vn" && tokens.size() >= 4)
            {
                glm::vec3 n(atof(tokens.at(1).c_str()),
                    atof(tokens.at(2).c_str()),
                    atof(tokens.at(3).c_str()));
                normals.push_back(n);
            }
            else if (tokens.at(0) == "mtllib" && tokens.size() >= 2)
            {
                std::string mtlFilename;
                for (size_t i = 1; i < tokens.size(); ++i)
                {
                    if (i > 1) mtlFilename += " ";
                    mtlFilename += tokens[i];
                }

                std::string mtlPath = directory + mtlFilename;
                LoadMTL(mtlPath, materialToTexture);
            }
            else if (tokens.at(0) == "usemtl" && tokens.size() >= 2)
            {
                m_useMaterials = true;
                currentMaterial = tokens.at(1);
                if (materialGroupIndices.find(currentMaterial) == materialGroupIndices.end())
                {
                    MaterialGroup group;
                    group.materialName = currentMaterial;
                    if (materialToTexture.find(currentMaterial) != materialToTexture.end())
                        group.texturePath = materialToTexture[currentMaterial];
                    materialGroupIndices[currentMaterial] = m_materialGroups.size();
                    m_materialGroups.push_back(group);
                }
            }
            else if (tokens.at(0) == "f" && tokens.size() >= 4)
            {
                Face f;
                std::vector<std::string> sub;
                split_string(tokens.at(1), '/', sub);
                if (sub.size() >= 1) f.a.position = positions.at(atoi(sub.at(0).c_str()) - 1);
                if (sub.size() >= 2 && !sub.at(1).empty()) {
                    int texIndex = atoi(sub.at(1).c_str()) - 1;
                    if (texIndex >= 0 && texIndex < tcs.size())
                        f.a.texcoord = tcs.at(texIndex);
                }
                if (sub.size() >= 3) f.a.normal = normals.at(atoi(sub.at(2).c_str()) - 1);

                for (size_t ti = 2; ti < tokens.size(); ti++)
                {
                    split_string(tokens.at(ti - 1), '/', sub);
                    if (sub.size() >= 1) f.b.position = positions.at(atoi(sub.at(0).c_str()) - 1);
                    if (sub.size() >= 2 && !sub.at(1).empty()) {
                        int texIndex = atoi(sub.at(1).c_str()) - 1;
                        if (texIndex >= 0 && texIndex < tcs.size())
                            f.b.texcoord = tcs.at(texIndex);
                    }
                    if (sub.size() >= 3) f.b.normal = normals.at(atoi(sub.at(2).c_str()) - 1);

                    split_string(tokens.at(ti), '/', sub);
                    if (sub.size() >= 1) f.c.position = positions.at(atoi(sub.at(0).c_str()) - 1);
                    if (sub.size() >= 2 && !sub.at(1).empty()) {
                        int texIndex = atoi(sub.at(1).c_str()) - 1;
                        if (texIndex >= 0 && texIndex < tcs.size())
                            f.c.texcoord = tcs.at(texIndex);
                    }
                    if (sub.size() >= 3) f.c.normal = normals.at(atoi(sub.at(2).c_str()) - 1);

                    if (m_useMaterials)
                    {
                        size_t index = materialGroupIndices[currentMaterial];
                        m_materialGroups[index].faces.emplace_back(f);
                        m_faces.emplace_back(f);
                    }
                    else
                    {
                        m_faces.push_back(f);
                    }
                }
            }
        }

        file.close();

        if (!m_useMaterials)
        {
            if (m_faces.empty())
                throw std::runtime_error("Model is empty");

            glGenBuffers(1, &m_vboid);
            if (!m_vboid)
                throw std::runtime_error("Failed to generate vertex buffer");

            glGenVertexArrays(1, &m_vaoid);
            if (!m_vaoid)
                throw std::runtime_error("Failed to generate vertex array");

            std::vector<GLfloat> data;
            for (size_t fi = 0; fi < m_faces.size(); ++fi)
            {
                data.push_back(m_faces[fi].a.position.x);
                data.push_back(m_faces[fi].a.position.y);
                data.push_back(m_faces[fi].a.position.z);
                data.push_back(m_faces[fi].a.texcoord.x);
                data.push_back(m_faces[fi].a.texcoord.y);
                data.push_back(m_faces[fi].a.normal.x);
                data.push_back(m_faces[fi].a.normal.y);
                data.push_back(m_faces[fi].a.normal.z);

                data.push_back(m_faces[fi].b.position.x);
                data.push_back(m_faces[fi].b.position.y);
                data.push_back(m_faces[fi].b.position.z);
                data.push_back(m_faces[fi].b.texcoord.x);
                data.push_back(m_faces[fi].b.texcoord.y);
                data.push_back(m_faces[fi].b.normal.x);
                data.push_back(m_faces[fi].b.normal.y);
                data.push_back(m_faces[fi].b.normal.z);

                data.push_back(m_faces[fi].c.position.x);
                data.push_back(m_faces[fi].c.position.y);
                data.push_back(m_faces[fi].c.position.z);
                data.push_back(m_faces[fi].c.texcoord.x);
                data.push_back(m_faces[fi].c.texcoord.y);
                data.push_back(m_faces[fi].c.normal.x);
                data.push_back(m_faces[fi].c.normal.y);
                data.push_back(m_faces[fi].c.normal.z);
            }

            glBindBuffer(GL_ARRAY_BUFFER, m_vboid);
            glBufferData(GL_ARRAY_BUFFER, data.size() * sizeof(GLfloat), &data.at(0), GL_STATIC_DRAW);
            glBindBuffer(GL_ARRAY_BUFFER, 0);

            glBindVertexArray(m_vaoid);
            glBindBuffer(GL_ARRAY_BUFFER, m_vboid);

            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (void*)0);
            glEnableVertexAttribArray(0);
            glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (void*)(3 * sizeof(GLfloat)));
            glEnableVertexAttribArray(1);
            glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (void*)(5 * sizeof(GLfloat)));
            glEnableVertexAttribArray(2);

            glBindVertexArray(0);
            glBindBuffer(GL_ARRAY_BUFFER, 0);
        }
        else
        {
            // New: Set up buffers for each material group.
            for (size_t gi = 0; gi < m_materialGroups.size(); ++gi)
            {
                MaterialGroup& group = m_materialGroups[gi];
                if (group.faces.empty())
                    continue;

                std::vector<GLfloat> data;
                for (size_t fi = 0; fi < group.faces.size(); ++fi)
                {
                    data.push_back(group.faces[fi].a.position.x);
                    data.push_back(group.faces[fi].a.position.y);
                    data.push_back(group.faces[fi].a.position.z);
                    data.push_back(group.faces[fi].a.texcoord.x);
                    data.push_back(group.faces[fi].a.texcoord.y);
                    data.push_back(group.faces[fi].a.normal.x);
                    data.push_back(group.faces[fi].a.normal.y);
                    data.push_back(group.faces[fi].a.normal.z);

                    data.push_back(group.faces[fi].b.position.x);
                    data.push_back(group.faces[fi].b.position.y);
                    data.push_back(group.faces[fi].b.position.z);
                    data.push_back(group.faces[fi].b.texcoord.x);
                    data.push_back(group.faces[fi].b.texcoord.y);
                    data.push_back(group.faces[fi].b.normal.x);
                    data.push_back(group.faces[fi].b.normal.y);
                    data.push_back(group.faces[fi].b.normal.z);

                    data.push_back(group.faces[fi].c.position.x);
                    data.push_back(group.faces[fi].c.position.y);
                    data.push_back(group.faces[fi].c.position.z);
                    data.push_back(group.faces[fi].c.texcoord.x);
                    data.push_back(group.faces[fi].c.texcoord.y);
                    data.push_back(group.faces[fi].c.normal.x);
                    data.push_back(group.faces[fi].c.normal.y);
                    data.push_back(group.faces[fi].c.normal.z);
                }

                glGenBuffers(1, &group.vbo);
                if (!group.vbo)
                    throw std::runtime_error("Failed to generate vertex buffer for material group");
                glGenVertexArrays(1, &group.vao);
                if (!group.vao)
                    throw std::runtime_error("Failed to generate vertex array for material group");

                glBindBuffer(GL_ARRAY_BUFFER, group.vbo);
                glBufferData(GL_ARRAY_BUFFER, data.size() * sizeof(GLfloat), &data.at(0), GL_STATIC_DRAW);
                glBindBuffer(GL_ARRAY_BUFFER, 0);

                glBindVertexArray(group.vao);
                glBindBuffer(GL_ARRAY_BUFFER, group.vbo);

                glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (void*)0);
                glEnableVertexAttribArray(0);
                glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (void*)(3 * sizeof(GLfloat)));
                glEnableVertexAttribArray(1);
                glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (void*)(5 * sizeof(GLfloat)));
                glEnableVertexAttribArray(2);

                glBindVertexArray(0);
                glBindBuffer(GL_ARRAY_BUFFER, 0);
            }

            std::cout << _path << " uses: " << std::endl;
            for (auto& group : GetMaterialGroups())
            {
                std::cout << "  Material: " << group.materialName << std::endl;
            }
        }

        m_dirty = false;
        calculate_dimensions();
    }

    inline Model::~Model()
    {
        Unload();
    }

    inline Model::Model(const Model& _copy)
    {
        m_faces = _copy.m_faces;
        m_materialGroups = _copy.m_materialGroups;
    }

    inline Model& Model::operator=(const Model& _assign)
    {
        m_faces = _assign.m_faces;
        m_materialGroups = _assign.m_materialGroups;
        m_dirty = true;
        return *this;
    }

    inline void Model::split_string_whitespace(const std::string& _input, std::vector<std::string>& _output)
    {
        _output.clear();
        std::istringstream iss(_input);
        std::string token;
        while (iss >> token)
        {
            _output.push_back(token);
        }
    }

    inline void Model::split_string(const std::string& _input, char _splitter, std::vector<std::string>& _output)
    {
        _output.clear();
        std::stringstream ss(_input);
        std::string item;
        while (std::getline(ss, item, _splitter))
        {
            _output.push_back(item);
        }
    }

    inline void Model::LoadMTL(const std::string& mtlFilePath, std::map<std::string, std::string>& materialToTextureMap)
    {
        std::ifstream mtlFile(mtlFilePath.c_str());
        if (!mtlFile.is_open())
        {
            std::cout << "Failed to open MTL file: " << mtlFilePath << std::endl;
            return;
        }

        std::string line;
        std::string currentMat;
        while (std::getline(mtlFile, line))
        {
            std::vector<std::string> tokens;
            split_string_whitespace(line, tokens);
            if (tokens.empty())
                continue;
            if (tokens[0] == "newmtl" && tokens.size() >= 2)
            {
                currentMat = tokens[1];
            }
            else if (tokens[0] == "map_Kd" && tokens.size() >= 2)
            {
                materialToTextureMap[currentMat] = tokens[1];
            }
        }
        mtlFile.close();
    }

    inline bool Model::LoadGLTF(const std::string& path)
    {
		std::cout << "Loading glTF model from: " << path << std::endl;
        tinygltf::TinyGLTF loader;
        tinygltf::Model scene;
        std::string err, warn;
        bool ok = (path.find(".glb") != std::string::npos)
            ? loader.LoadBinaryFromFile(&scene, &err, &warn, path)
            : loader.LoadASCIIFromFile(&scene, &err, &warn, path);
        if (!warn.empty()) std::cerr << "glTF warning: " << warn << "\n";
        if (!err.empty())  std::cerr << "glTF error:   " << err << "\n";
        if (!ok) return false;

        m_embeddedTextures.clear();
        for (const auto& img : scene.images)
            m_embeddedTextures.emplace_back(img.image.data(), img.width, img.height, img.component);

        m_faces.clear();
        m_materialGroups.clear();
        m_useMaterials = false;

        for (const auto& mesh : scene.meshes)
        {
            for (const auto& prim : mesh.primitives)
            {
                if (prim.mode != TINYGLTF_MODE_TRIANGLES)
                    throw std::runtime_error("Only TRIANGLES are supported");

                if (!prim.attributes.count("POSITION"))
                    throw std::runtime_error("Missing POSITION");

                const auto& posAcc = scene.accessors.at(prim.attributes.at("POSITION"));
                if (posAcc.type != TINYGLTF_TYPE_VEC3 || posAcc.componentType != TINYGLTF_COMPONENT_TYPE_FLOAT)
                    throw std::runtime_error("POSITION must be float vec3");
                const auto& posView = scene.bufferViews.at(posAcc.bufferView);
                const auto& posBuf = scene.buffers.at(posView.buffer);
                const unsigned char* posBase = posBuf.data.data() + posView.byteOffset + posAcc.byteOffset;
                size_t posStride = posView.byteStride ? posView.byteStride : 3 * sizeof(float);

                const float* normBase = nullptr; size_t normStride = 0;
                if (prim.attributes.count("NORMAL"))
                {
                    const auto& nAcc = scene.accessors.at(prim.attributes.at("NORMAL"));
                    if (nAcc.type == TINYGLTF_TYPE_VEC3 && nAcc.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT)
                    {
                        const auto& nView = scene.bufferViews.at(nAcc.bufferView);
                        const auto& nBuf = scene.buffers.at(nView.buffer);
                        normBase = reinterpret_cast<const float*>(nBuf.data.data() + nView.byteOffset + nAcc.byteOffset);
                        normStride = nView.byteStride ? nView.byteStride : 3 * sizeof(float);
                    }
                }

                const float* uvBase = nullptr; size_t uvStride = 0;
                if (prim.attributes.count("TEXCOORD_0"))
                {
                    const auto& tAcc = scene.accessors.at(prim.attributes.at("TEXCOORD_0"));
                    if (tAcc.type == TINYGLTF_TYPE_VEC2 && tAcc.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT)
                    {
                        const auto& tView = scene.bufferViews.at(tAcc.bufferView);
                        const auto& tBuf = scene.buffers.at(tView.buffer);
                        uvBase = reinterpret_cast<const float*>(tBuf.data.data() + tView.byteOffset + tAcc.byteOffset);
                        uvStride = tView.byteStride ? tView.byteStride : 2 * sizeof(float);
                    }
                }

                if (prim.indices < 0)
                    throw std::runtime_error("Indexed geometry required");

                const auto& iAcc = scene.accessors.at(prim.indices);
                if (iAcc.type != TINYGLTF_TYPE_SCALAR)
                    throw std::runtime_error("indices accessor must be SCALAR");
                const auto& iView = scene.bufferViews.at(iAcc.bufferView);
                const auto& iBuf = scene.buffers.at(iView.buffer);
                const unsigned char* iBase = iBuf.data.data() + iView.byteOffset + iAcc.byteOffset;
                size_t icSize = tinygltf::GetComponentSizeInBytes(iAcc.componentType);
                size_t iStride = iView.byteStride ? iView.byteStride : icSize;

                int matIdx = prim.material;
                bool useMat = (matIdx >= 0);
                if (useMat) m_useMaterials = true;

                size_t groupIndex = 0;
                if (useMat)
                {
                    const std::string& mname = scene.materials[matIdx].name;
                    auto it = std::find_if(m_materialGroups.begin(), m_materialGroups.end(),
                        [&](const MaterialGroup& g) { return g.materialName == mname; });
                    if (it == m_materialGroups.end())
                    {
                        MaterialGroup mg;
                        mg.materialName = mname;
                        groupIndex = m_materialGroups.size();
                        m_materialGroups.push_back(mg);
                    }
                    else groupIndex = size_t(std::distance(m_materialGroups.begin(), it));

                    auto& group = m_materialGroups[groupIndex];
                    const auto& mat = scene.materials[matIdx];
                    const auto& pbr = mat.pbrMetallicRoughness;

                    if (pbr.baseColorTexture.index >= 0)
                        group.pbr.baseColorTexIndex = scene.textures[pbr.baseColorTexture.index].source;
                    group.pbr.baseColorFactor = glm::make_vec4(pbr.baseColorFactor.data());

                    if (pbr.metallicRoughnessTexture.index >= 0)
                        group.pbr.metallicRoughnessTexIndex = scene.textures[pbr.metallicRoughnessTexture.index].source;
                    group.pbr.metallicFactor = pbr.metallicFactor;
                    group.pbr.roughnessFactor = pbr.roughnessFactor;

                    if (mat.normalTexture.index >= 0)
                        group.pbr.normalTexIndex = scene.textures[mat.normalTexture.index].source;
                    if (mat.occlusionTexture.index >= 0)
                        group.pbr.occlusionTexIndex = scene.textures[mat.occlusionTexture.index].source;
                    if (mat.emissiveTexture.index >= 0)
                        group.pbr.emissiveTexIndex = scene.textures[mat.emissiveTexture.index].source;

                    const std::string& am = mat.alphaMode;
                    if (am == "BLEND") {
                        group.pbr.alphaMode = PBRMaterial::AlphaMode::AlphaBlend;
                    }
                    else if (am == "MASK") {
                        group.pbr.alphaMode = PBRMaterial::AlphaMode::AlphaMask;
                        group.pbr.alphaCutoff = float(mat.alphaCutoff); // default 0.5 in glTF spec
                    }
                    else {
                        group.pbr.alphaMode = PBRMaterial::AlphaMode::AlphaOpaque;
                    }

                    auto toLower = [](std::string s) { for (auto& c : s) c = (char)tolower(c); return s; };
                    std::string lname = toLower(group.materialName);

                    // If author exported trees/fences as BLEND, prefer MASK cutout:
                    bool looksLikeFoliage =
                        lname.find("tree") != std::string::npos ||
						lname.find("trees") != std::string::npos ||
                        lname.find("leaf") != std::string::npos ||
                        lname.find("leaves") != std::string::npos ||
                        lname.find("bush") != std::string::npos ||
                        lname.find("hedge") != std::string::npos ||
                        lname.find("grass") != std::string::npos ||
                        lname.find("fence") != std::string::npos ||
						lname.find("fencing") != std::string::npos ||
						lname.find("fences") != std::string::npos ||
                        lname.find("chain") != std::string::npos; // chain-link fence etc.

                    if (looksLikeFoliage && group.pbr.alphaMode == Renderer::Model::PBRMaterial::AlphaMode::AlphaBlend) {
                        group.pbr.alphaMode = Renderer::Model::PBRMaterial::AlphaMode::AlphaMask;
                        group.pbr.alphaCutoff = 0.5f;
                    }

                    group.pbr.doubleSided = mat.doubleSided;

                    if (mat.normalTexture.index >= 0) {
                        group.pbr.normalScale = static_cast<float>(mat.normalTexture.scale);
                    }
                    if (mat.occlusionTexture.index >= 0) {
                        group.pbr.occlusionStrength = static_cast<float>(mat.occlusionTexture.strength);
                    }
                    if (mat.emissiveFactor.size() == 3) {
                        group.pbr.emissiveFactor = glm::vec3(
                            static_cast<float>(mat.emissiveFactor[0]),
                            static_cast<float>(mat.emissiveFactor[1]),
                            static_cast<float>(mat.emissiveFactor[2]));
                    }

                    auto extItTr = mat.extensions.find("KHR_materials_transmission");
                    if (extItTr != mat.extensions.end()) {
                        const tinygltf::Value& ext = extItTr->second;

                        auto tfIt = ext.Get("transmissionFactor");
                        if (tfIt.IsNumber()) {
                            group.pbr.transmissionFactor = static_cast<float>(tfIt.Get<double>());
                        }

                        auto ttIt = ext.Get("transmissionTexture");
                        if (ttIt.IsObject()) {
                            auto idxIt = ttIt.Get("index");
                            if (idxIt.IsInt()) {
                                int texIndex = idxIt.Get<int>();
                                if (texIndex >= 0 && texIndex < static_cast<int>(scene.textures.size())) {
                                    int img = scene.textures[texIndex].source;
                                    if (img >= 0 && img < static_cast<int>(scene.images.size())) {
                                        group.pbr.transmissionTexIndex = img;
                                    }
                                }
                            }
                        }
                    }

                    auto extItIor = mat.extensions.find("KHR_materials_ior");
                    if (extItIor != mat.extensions.end()) {
                        const tinygltf::Value& ext = extItIor->second;

                        auto iorIt = ext.Get("ior");
                        if (iorIt.IsNumber()) {
                            group.pbr.ior = static_cast<float>(iorIt.Get<double>());
                        }
                    }
                }

                auto fetchVert = [&](Vertex& v, size_t vi)
                    {
                        const float* p = reinterpret_cast<const float*>(posBase + vi * posStride);
                        v.position = { p[0], p[1], p[2] };
                        if (normBase)
                        {
                            const float* n = reinterpret_cast<const float*>(reinterpret_cast<const unsigned char*>(normBase) + vi * normStride);
                            v.normal = { n[0], n[1], n[2] };
                        }
                        if (uvBase)
                        {
                            const float* t = reinterpret_cast<const float*>(reinterpret_cast<const unsigned char*>(uvBase) + vi * uvStride);
                            v.texcoord = { t[0], t[1] };
                        }
                    };

                for (size_t f = 0; f + 2 < iAcc.count; f += 3)
                {
                    uint32_t i0 = 0, i1 = 0, i2 = 0;
                    switch (iAcc.componentType)
                    {
                    case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
                        i0 = *(const uint8_t*)(iBase + (f + 0) * iStride);
                        i1 = *(const uint8_t*)(iBase + (f + 1) * iStride);
                        i2 = *(const uint8_t*)(iBase + (f + 2) * iStride);
                        break;
                    case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
                        i0 = *(const uint16_t*)(iBase + (f + 0) * iStride);
                        i1 = *(const uint16_t*)(iBase + (f + 1) * iStride);
                        i2 = *(const uint16_t*)(iBase + (f + 2) * iStride);
                        break;
                    case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:
                        i0 = *(const uint32_t*)(iBase + (f + 0) * iStride);
                        i1 = *(const uint32_t*)(iBase + (f + 1) * iStride);
                        i2 = *(const uint32_t*)(iBase + (f + 2) * iStride);
                        break;
                    default:
                        throw std::runtime_error("Unsupported index type");
                    }

                    Face face;
                    fetchVert(face.a, i0);
                    fetchVert(face.b, i1);
                    fetchVert(face.c, i2);

                    if (useMat) m_materialGroups[groupIndex].faces.push_back(face);
                    m_faces.push_back(face);
                }
            }
        }

        // Compute model-space bounds per material group (center, half-extents, sphere radius)
        for (auto& group : m_materialGroups)
        {
            if (group.faces.empty())
                continue;

            bool first = true;
            glm::vec3 minv(0.0f), maxv(0.0f);

            auto processPos = [&](const glm::vec3& p)
                {
                    if (first) { minv = maxv = p; first = false; }
                    else {
                        minv = glm::min(minv, p);
                        maxv = glm::max(maxv, p);
                    }
                };

            for (const auto& f : group.faces)
            {
                processPos(f.a.position);
                processPos(f.b.position);
                processPos(f.c.position);
            }

            group.boundsCenterMS = 0.5f * (minv + maxv);
            group.boundsHalfExtentsMS = 0.5f * (maxv - minv);
            group.boundsSphereRadiusMS = glm::length(group.boundsHalfExtentsMS); // AABB-based sphere
        }

        if (!m_useMaterials)
        {
            glGenBuffers(1, &m_vboid);
            glGenVertexArrays(1, &m_vaoid);
            std::vector<GLfloat> data;
            data.reserve(m_faces.size() * 3 * 8);
            for (auto& f : m_faces)
            {
                auto push = [&](const Vertex& v) {
                    data.insert(data.end(), { v.position.x, v.position.y, v.position.z,
                                              v.texcoord.x, v.texcoord.y,
                                              v.normal.x,   v.normal.y,   v.normal.z });
                    };
                push(f.a); push(f.b); push(f.c);
            }
            glBindBuffer(GL_ARRAY_BUFFER, m_vboid);
            glBufferData(GL_ARRAY_BUFFER, data.size() * sizeof(GLfloat), data.data(), GL_STATIC_DRAW);
            glBindVertexArray(m_vaoid);
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (void*)0);
            glEnableVertexAttribArray(0);
            glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (void*)(3 * sizeof(GLfloat)));
            glEnableVertexAttribArray(1);
            glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (void*)(5 * sizeof(GLfloat)));
            glEnableVertexAttribArray(2);
            glBindVertexArray(0);
            glBindBuffer(GL_ARRAY_BUFFER, 0);
            m_dirty = false;
        }
        else
        {
            for (auto& group : m_materialGroups)
            {
                if (group.faces.empty()) continue;
                glGenBuffers(1, &group.vbo);
                glGenVertexArrays(1, &group.vao);
                std::vector<GLfloat> data;
                data.reserve(group.faces.size() * 3 * 8);
                for (auto& f : group.faces)
                {
                    auto push = [&](const Vertex& v) {
                        data.insert(data.end(), { v.position.x, v.position.y, v.position.z,
                                                  v.texcoord.x, v.texcoord.y,
                                                  v.normal.x,   v.normal.y,   v.normal.z });
                        };
                    push(f.a); push(f.b); push(f.c);
                }
                glBindBuffer(GL_ARRAY_BUFFER, group.vbo);
                glBufferData(GL_ARRAY_BUFFER, data.size() * sizeof(GLfloat), data.data(), GL_STATIC_DRAW);
                glBindVertexArray(group.vao);
                glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (void*)0);
                glEnableVertexAttribArray(0);
                glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (void*)(3 * sizeof(GLfloat)));
                glEnableVertexAttribArray(1);
                glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (void*)(5 * sizeof(GLfloat)));
                glEnableVertexAttribArray(2);
                glBindVertexArray(0);
                glBindBuffer(GL_ARRAY_BUFFER, 0);
            }
        }

        return true;
    }

    inline GLuint Model::vao_id()
    {
        if (!m_useMaterials)
        {
            if (m_faces.empty())
                throw std::runtime_error("Model is empty");
            if (!m_vboid)
            {
                glGenBuffers(1, &m_vboid);
                if (!m_vboid)
                    throw std::runtime_error("Failed to generate vertex buffer");
            }
            if (!m_vaoid)
            {
                glGenVertexArrays(1, &m_vaoid);
                if (!m_vaoid)
                    throw std::runtime_error("Failed to generate vertex array");
            }
            if (m_dirty)
            {
                std::vector<GLfloat> data;
                for (size_t fi = 0; fi < m_faces.size(); ++fi)
                {
                    data.push_back(m_faces[fi].a.position.x);
                    data.push_back(m_faces[fi].a.position.y);
                    data.push_back(m_faces[fi].a.position.z);
                    data.push_back(m_faces[fi].a.texcoord.x);
                    data.push_back(m_faces[fi].a.texcoord.y);
                    data.push_back(m_faces[fi].a.normal.x);
                    data.push_back(m_faces[fi].a.normal.y);
                    data.push_back(m_faces[fi].a.normal.z);

                    data.push_back(m_faces[fi].b.position.x);
                    data.push_back(m_faces[fi].b.position.y);
                    data.push_back(m_faces[fi].b.position.z);
                    data.push_back(m_faces[fi].b.texcoord.x);
                    data.push_back(m_faces[fi].b.texcoord.y);
                    data.push_back(m_faces[fi].b.normal.x);
                    data.push_back(m_faces[fi].b.normal.y);
                    data.push_back(m_faces[fi].b.normal.z);

                    data.push_back(m_faces[fi].c.position.x);
                    data.push_back(m_faces[fi].c.position.y);
                    data.push_back(m_faces[fi].c.position.z);
                    data.push_back(m_faces[fi].c.texcoord.x);
                    data.push_back(m_faces[fi].c.texcoord.y);
                    data.push_back(m_faces[fi].c.normal.x);
                    data.push_back(m_faces[fi].c.normal.y);
                    data.push_back(m_faces[fi].c.normal.z);
                }

                glBindBuffer(GL_ARRAY_BUFFER, m_vboid);
                glBufferData(GL_ARRAY_BUFFER, data.size() * sizeof(GLfloat), &data.at(0), GL_STATIC_DRAW);
                glBindBuffer(GL_ARRAY_BUFFER, 0);

                glBindVertexArray(m_vaoid);
                glBindBuffer(GL_ARRAY_BUFFER, m_vboid);

                glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (void*)0);
                glEnableVertexAttribArray(0);
                glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (void*)(3 * sizeof(GLfloat)));
                glEnableVertexAttribArray(1);
                glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (void*)(5 * sizeof(GLfloat)));
                glEnableVertexAttribArray(2);

                glBindVertexArray(0);
                glBindBuffer(GL_ARRAY_BUFFER, 0);

                m_dirty = false;
            }
            return m_vaoid;
        }
        else
        {
            return 0;
        }
    }

    inline void Model::Unload()
    {
        if (!m_useMaterials)
        {
            if (m_vaoid)
            {
                glDeleteVertexArrays(1, &m_vaoid);
                m_vaoid = 0;
                m_dirty = true;
            }
            if (m_vboid)
            {
                glDeleteBuffers(1, &m_vboid);
                m_vboid = 0;
                m_dirty = true;
            }
        }
        else
        {
            for (auto& group : m_materialGroups)
            {
                if (group.vao)
                {
                    glDeleteVertexArrays(1, &group.vao);
                    group.vao = 0;
                }
                if (group.vbo)
                {
                    glDeleteBuffers(1, &group.vbo);
                    group.vbo = 0;
                }
            }
        }
    }

    inline GLsizei Model::vertex_count() const
    {
        if (!m_useMaterials)
            return static_cast<GLsizei>(m_faces.size() * 3);
        else
        {
            GLsizei count = 0;
            for (const auto& group : m_materialGroups)
                count += static_cast<GLsizei>(group.faces.size() * 3);
            return count;
        }
    }

    inline void Model::calculate_dimensions()
    {
        bool first = true;
        glm::vec3 min_pos, max_pos;
        auto processVertex = [&](const glm::vec3& pos)
            {
                if (first)
                {
                    min_pos = pos;
                    max_pos = pos;
                    first = false;
                }
                else
                {
                    min_pos = glm::min(min_pos, pos);
                    max_pos = glm::max(max_pos, pos);
                }
            };

        if (!m_useMaterials)
        {
            for (const auto& face : m_faces)
            {
                processVertex(face.a.position);
                processVertex(face.b.position);
                processVertex(face.c.position);
            }
        }
        else
        {
            for (const auto& group : m_materialGroups)
            {
                for (const auto& face : group.faces)
                {
                    processVertex(face.a.position);
                    processVertex(face.b.position);
                    processVertex(face.c.position);
                }
            }
        }

        m_width = max_pos.x - min_pos.x;
        m_height = max_pos.y - min_pos.y;
        m_length = max_pos.z - min_pos.z;
    }

    inline float Model::get_width() const { return m_width; }
    inline float Model::get_height() const { return m_height; }
    inline float Model::get_length() const { return m_length; }

    inline const std::vector<Model::Face>& Model::GetFaces() const
    {
        return m_faces;
    }
}
