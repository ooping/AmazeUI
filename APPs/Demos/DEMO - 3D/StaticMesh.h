#pragma once

#include "pch.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

// Static mesh class for loading and rendering 3D models
// Uses Assimp to load FBX/OBJ files and DirectX 12 for rendering
class StaticMesh {
public:
    StaticMesh() {
        // Initialize all bone matrices to identity
        DirectX::XMMATRIX identity = DirectX::XMMatrixIdentity();
        for (int i = 0; i < MAX_BONES; i++) {
            DirectX::XMStoreFloat4x4(&_boneMatrices[i], identity);
        }
    }
    ~StaticMesh() = default;

    // Load model from file (ignores bone weights, uses bind pose only)
    bool LoadFromFile(const std::wstring& filePath);
    
    // Render the mesh
    void Render(UICameraBase3D* pCamera, const DirectX::XMMATRIX& worldMatrix);
    
    // Animation control
    void PlayAnimation(int animIndex, bool loop = true);
    void PlayAnimation(const std::string& animName, bool loop = true);
    void StopAnimation();
    void UpdateAnimation(float deltaTime);
    void SetAnimationSpeed(float speed) { _animSpeed = speed; }
    bool IsPlaying() const { return _isPlaying; }
    
    // Get mesh info
    size_t GetVertexCount() const { return _vertices.size(); }
    size_t GetTriangleCount() const { return _indices.size() / 3; }
    size_t GetBoneCount() const { return _bones.size(); }
    size_t GetAnimationCount() const { return _animations.size(); }
    
private:
    // Vertex structure (with bone weights for skeletal animation)
    struct Vertex {
        DirectX::XMFLOAT3 position;
        DirectX::XMFLOAT3 normal;
        DirectX::XMFLOAT2 texcoord;
        uint8_t boneIndices[4];   // Up to 4 bones can influence a vertex
        float boneWeights[4];     // Corresponding weights (sum to 1.0)
        
        Vertex() : position(0, 0, 0), normal(0, 1, 0), texcoord(0, 0) {
            boneIndices[0] = boneIndices[1] = boneIndices[2] = boneIndices[3] = 0;
            boneWeights[0] = 1.0f;  // Default: fully influenced by bone 0
            boneWeights[1] = boneWeights[2] = boneWeights[3] = 0.0f;
        }
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
    
    // Bone structure
    struct Bone {
        std::string name;
        int parentIndex = -1;  // -1 for root bones
        DirectX::XMFLOAT4X4 offsetMatrix;      // Bone offset (inverse bind pose)
        DirectX::XMFLOAT4X4 localTransform;    // Current local transform
        DirectX::XMFLOAT4X4 globalTransform;   // Current global transform
    };
    
    // Animation key frames
    struct PositionKey {
        float time;
        DirectX::XMFLOAT3 value;
    };
    
    struct RotationKey {
        float time;
        DirectX::XMFLOAT4 value;  // Quaternion (x, y, z, w)
    };
    
    struct ScalingKey {
        float time;
        DirectX::XMFLOAT3 value;
    };
    
    // Bone animation channel (per-bone animation data)
    struct BoneAnimChannel {
        std::string boneName;
        std::vector<PositionKey> positions;
        std::vector<RotationKey> rotations;
        std::vector<ScalingKey> scalings;
    };
    
    // Animation clip
    struct AnimationClip {
        std::string name;
        float duration = 0.0f;          // Duration in ticks
        float ticksPerSecond = 25.0f;   // Animation speed
        std::vector<BoneAnimChannel> channels;
    };
    
    // Mesh data
    std::vector<Vertex> _vertices;
    std::vector<uint16_t> _indices;
    std::vector<SubMesh> _subMeshes;
    std::vector<Material> _materials;
    
    // Skeleton data
    std::vector<Bone> _bones;
    std::map<std::string, int> _boneNameToIndex;  // Quick lookup
    std::vector<AnimationClip> _animations;
    
    // Animation state
    int _currentAnimIndex = -1;
    float _currentTime = 0.0f;
    float _animSpeed = 1.0f;
    bool _isPlaying = false;
    bool _isLooping = true;
    
    // Bone matrices for GPU (final transforms)
    static constexpr size_t MAX_BONES = 128;
    DirectX::XMFLOAT4X4 _boneMatrices[MAX_BONES];
    
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
    
    // Bone helper methods
    int GetOrCreateBoneIndex(const std::string& boneName);
    void AddBoneWeight(Vertex& vertex, int boneIndex, float weight);
    void LoadBoneHierarchy(aiNode* node, int parentIndex);
    void LoadAnimations(const aiScene* scene);
    
    // Animation helper methods
    void UpdateBoneTransforms(float time);
    void CalculateBoneTransform(int boneIndex, const DirectX::XMMATRIX& parentTransform);
    DirectX::XMVECTOR InterpolatePosition(const BoneAnimChannel& channel, float time);
    DirectX::XMVECTOR InterpolateRotation(const BoneAnimChannel& channel, float time);
    DirectX::XMVECTOR InterpolateScaling(const BoneAnimChannel& channel, float time);
};
