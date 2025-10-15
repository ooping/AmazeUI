#pragma once

#include "pch.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

// Static mesh class for loading and rendering 3D models
// Uses Assimp to load FBX/OBJ files and DirectX 12 for rendering
class StaticMesh {
public:
    StaticMesh() = default;
    ~StaticMesh() = default;

    // Load model from file (ignores bone weights, uses bind pose only)
    bool LoadFromFile(const std::wstring& filePath);
    
    // Render the mesh
    void Render(UICameraBase3D* pCamera, const DirectX::XMMATRIX& worldMatrix);
    
    // Get mesh info
    size_t GetVertexCount() const { return _vertices.size(); }
    size_t GetTriangleCount() const { return _indices.size() / 3; }
    
private:
    // Vertex structure (simplified - no bone weights)
    struct Vertex {
        DirectX::XMFLOAT3 position;
        DirectX::XMFLOAT3 normal;
        DirectX::XMFLOAT2 texcoord;
        
        Vertex() : position(0, 0, 0), normal(0, 1, 0), texcoord(0, 0) {}
    };
    
    // Material structure
    struct Material {
        std::wstring name;
        std::wstring diffuseTexturePath;
        std::wstring emissiveTexturePath;
        Microsoft::WRL::ComPtr<ID3D12Resource> diffuseTexture;
        Microsoft::WRL::ComPtr<ID3D12Resource> emissiveTexture;
        D3D12_GPU_DESCRIPTOR_HANDLE diffuseSRV = {};
        D3D12_GPU_DESCRIPTOR_HANDLE emissiveSRV = {};
        bool hasDiffuseTexture = false;
        bool hasEmissiveTexture = false;
    };
    
    // Sub-mesh structure (each has its own material)
    struct SubMesh {
        uint32_t startIndex;
        uint32_t indexCount;
        uint32_t materialIndex;
    };
    
    // Mesh data
    std::vector<Vertex> _vertices;
    std::vector<uint16_t> _indices;
    std::vector<SubMesh> _subMeshes;
    std::vector<Material> _materials;
    
    // DirectX 12 resources
    Microsoft::WRL::ComPtr<ID3D12Resource> _vertexBuffer;
    Microsoft::WRL::ComPtr<ID3D12Resource> _indexBuffer;
    D3D12_VERTEX_BUFFER_VIEW _vertexBufferView = {};
    D3D12_INDEX_BUFFER_VIEW _indexBufferView = {};
    
    // Descriptor allocation
    static size_t _nextDescriptorIndex;  // Static counter for descriptor allocation
    
    // Helper methods
    void CreateBuffers();
    void ProcessNode(aiNode* node, const aiScene* scene);
    void ProcessMesh(aiMesh* mesh);
    void LoadMaterials(const aiScene* scene, const std::wstring& modelDirectory);
    bool LoadTexture(const std::wstring& texturePath, Microsoft::WRL::ComPtr<ID3D12Resource>& texture, D3D12_GPU_DESCRIPTOR_HANDLE& srvHandle);
};
