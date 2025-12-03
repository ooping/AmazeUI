#pragma once

#include "UIUtility.h"
#include "UICamera.h"
#include "UIAnimation.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>


// Static mesh class for loading and rendering 3D models
// Uses Assimp to load FBX/OBJ files and DirectX 12 for rendering
class UIModel {
public:
    UIModel() {
        // Initialize all bone matrices to identity
        DirectX::XMMATRIX identity = DirectX::XMMatrixIdentity();
        for (int i = 0; i < MAX_BONES; i++) {
            DirectX::XMStoreFloat4x4(&_boneMatrices[i], identity);
        }
    }
    ~UIModel() = default;

    // Load model from file
    bool LoadFromFile(const std::wstring& filePath);
    
    // Render the mesh
    void Render(UICameraBase3D* pCamera, const DirectX::XMMATRIX& worldMatrix);
    
protected:
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
        Microsoft::WRL::ComPtr<ID3D12Resource> diffuseTexture;
        D3D12_GPU_DESCRIPTOR_HANDLE diffuseSRV = {};
        bool hasDiffuseTexture = false;
        
        std::wstring emissiveTexturePath;
        Microsoft::WRL::ComPtr<ID3D12Resource> emissiveTexture;
        D3D12_GPU_DESCRIPTOR_HANDLE emissiveSRV = {};
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
    
    // ========== Animation Data Structures (Private) ==========
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
    
    struct BoneAnimChannel {
        std::string boneName;
        std::vector<PositionKey> positions;
        std::vector<RotationKey> rotations;
        std::vector<ScalingKey> scalings;
    };
    
    struct AnimationClip {
        std::string name;
        float duration = 0.0f;
        float ticksPerSecond = 25.0f;
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
    
    // Animation helper methods (protected - for derived classes to use)
    void UpdateBoneTransforms(int animIndex, float time);
    void CalculateBoneTransform(int boneIndex, const DirectX::XMMATRIX& parentTransform);
    DirectX::XMVECTOR InterpolatePosition(const BoneAnimChannel& channel, float time);
    DirectX::XMVECTOR InterpolateRotation(const BoneAnimChannel& channel, float time);
    DirectX::XMVECTOR InterpolateScaling(const BoneAnimChannel& channel, float time);
};







/*
UIModelAnimation - Advanced 3D Model with Animation Control
Inherits from UIModel (rendering) and UIAnimateSecondHelp (animation system)

Features:
- Automatic animation playback with time-based updates
- Supports looping and one-shot animations
- Seamless integration with AmazeUI animation system

Usage:
  UIModelAnimation model;
  model.LoadFromFile(L"character.fbx");
  model.PlayAnimation(0, true, 1.5f);  // Play animation 0, loop, 1.5x speed
  model.Render(camera, worldMatrix);   // Automatic bone updates via animation system
*/
class UIModelAnimation : public UIModel, public UIAnimateSecondHelp {
public:
    UIModelAnimation() = default;
    ~UIModelAnimation() = default;

    // Animation control (essential API only)
    void PlayAnimation(int animIndex = 0, bool loop = true, float speed = 1.0f);
    void PlayAnimation(const std::string& animName, bool loop = true, float speed = 1.0f);

protected:
    // Override from UIAnimateSecondHelp - called every frame by animation system
    void DrawAnimation() override;

private:
    // Animation state (only what's not in UIAnimateSecondHelp)
    int _currentAnimIndex = -1;    // Which animation is playing
    float _animSpeed = 1.0f;       // Playback speed multiplier
    bool _isLooping = true;        // Whether to loop the animation
};


