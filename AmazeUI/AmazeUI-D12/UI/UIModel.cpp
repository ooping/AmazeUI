#include "UIModel.h"
#include "UIDXFoundation.h"


using namespace DirectX;
using namespace Microsoft::WRL;
using namespace std;

namespace {
    enum Descriptors {
        // System reserved (0-9)
        WindowsLogo = 0,
        MSYHFont = 1,
        SystemReservedCount = 10,

        // UI textures (10-9999)
        UITexturesStart = 10,
        UITexturesCount = 9990,

        // Font textures (10000-19999)
        FontTexturesStart = 10000,
        FontTexturesCount = 10000,

        // Model textures (20000-29999)
        ModelTexturesStart = 20000,
        ModelTexturesCount = 10000,

        TotalDescriptorCount = 30000
    };
};

// Load model from file using Assimp
// XXX\\source\\xxx.fbx
// XXX\\textures\\xxx.png
bool UIModel::LoadFromFile(const wstring& filePath) {
    // Locate model file
    wchar_t strFilePath[MAX_PATH] = {};
    FindMediaFile(strFilePath, MAX_PATH, filePath.c_str());
    
    // Convert to string for Assimp
    string filePathStr = WSTR_TO_STR(strFilePath);
    
    // Load with Assimp
    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(filePathStr,
        aiProcess_Triangulate | 
        aiProcess_ConvertToLeftHanded | 
        aiProcess_CalcTangentSpace | 
        aiProcess_GenSmoothNormals | 
        aiProcess_JoinIdenticalVertices |
        aiProcess_ImproveCacheLocality | 
        aiProcess_OptimizeMeshes | 
        aiProcess_SortByPType
    );
    
    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
        return false;
    }
    
    // Clear existing data
    _vertices.clear();
    _indices.clear();
    _subMeshes.clear();
    _materials.clear();
    
    // Process scene
    ProcessNode(scene->mRootNode, scene);
    
    if (_vertices.empty() || _indices.empty()) {
        return false;
    }
    
    CreateBuffers();
    
    // Load materials - get parent directory for textures
    wstring dir = filePath;
    // find last '/' or '\'
    auto pos1 = dir.find_last_of(L"/\\");
    if (pos1 != wstring::npos) {
        // find last '/' or '\' before pos1
        auto pos2 = dir.find_last_of(L"/\\", pos1 - 1);
        if (pos2 != wstring::npos) {
            dir.resize(pos2 + 1);
        }
    }
    
    LoadMaterials(scene, dir + L"textures\\");
    
    // Load bone hierarchy
    LoadBoneHierarchy(scene->mRootNode, -1);
    
    // Load animations
    LoadAnimations(scene);
    
    return true;
}

// Process scene node recursively
void UIModel::ProcessNode(aiNode* node, const aiScene* scene) {
    // Process all meshes in this node
    for (unsigned int i = 0; i < node->mNumMeshes; i++) {
        aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
        ProcessMesh(mesh);
    }
    
    // Recursively process children
    for (unsigned int i = 0; i < node->mNumChildren; i++) {
        ProcessNode(node->mChildren[i], scene);
    }
}

// Process a single mesh
void UIModel::ProcessMesh(aiMesh* mesh) {
    uint16_t baseVertex = static_cast<uint16_t>(_vertices.size());
    uint32_t startIndex = static_cast<uint32_t>(_indices.size());
    
    // Extract vertices
    for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
        Vertex vertex;
        
        // Position
        vertex.position.x = mesh->mVertices[i].x;
        vertex.position.y = mesh->mVertices[i].y;
        vertex.position.z = mesh->mVertices[i].z;
        
        // Normal
        if (mesh->HasNormals()) {
            DirectX::XMFLOAT3 normal(mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z);
            DirectX::XMVECTOR normalVec = DirectX::XMLoadFloat3(&normal);
            normalVec = DirectX::XMVector3Normalize(normalVec);
            DirectX::XMStoreFloat3(&vertex.normal, normalVec);
        } else {
            vertex.normal = DirectX::XMFLOAT3(0.0f, 1.0f, 0.0f);
        }
        
        // Texture coordinates
        if (mesh->mTextureCoords[0]) {
            vertex.texcoord.x = mesh->mTextureCoords[0][i].x;
            vertex.texcoord.y = mesh->mTextureCoords[0][i].y;
        }
        
        _vertices.push_back(vertex);
    }
    
    // Process bone weights
    for (unsigned int boneIdx = 0; boneIdx < mesh->mNumBones; boneIdx++) {
        aiBone* bone = mesh->mBones[boneIdx];
        int boneIndex = GetOrCreateBoneIndex(bone->mName.C_Str());
        
        // Skip bones beyond MaxBones limit (DirectXTK12 limitation: 72 bones max)
        if (boneIndex >= DirectX::IEffectSkinning::MaxBones) {
            continue;
        }
        
        // Store bone offset matrix (inverse bind pose)
        if (boneIndex < _bones.size()) {
            aiMatrix4x4& m = bone->mOffsetMatrix;
            _bones[boneIndex].offsetMatrix = DirectX::XMFLOAT4X4(
                m.a1, m.b1, m.c1, m.d1,
                m.a2, m.b2, m.c2, m.d2,
                m.a3, m.b3, m.c3, m.d3,
                m.a4, m.b4, m.c4, m.d4
            );
        }
        
        // Add bone weights to vertices
        for (unsigned int weightIdx = 0; weightIdx < bone->mNumWeights; weightIdx++) {
            unsigned int vertexId = baseVertex + bone->mWeights[weightIdx].mVertexId;
            float weight = bone->mWeights[weightIdx].mWeight;
            
            if (vertexId < _vertices.size()) {
                AddBoneWeight(_vertices[vertexId], boneIndex, weight);
            }
        }
    }
    
    // Extract indices
    for (unsigned int i = 0; i < mesh->mNumFaces; i++) {
        aiFace face = mesh->mFaces[i];
        for (unsigned int j = 0; j < face.mNumIndices; j++) {
            _indices.push_back(baseVertex + static_cast<uint16_t>(face.mIndices[j]));
        }
    }
    
    // Record submesh
    SubMesh submesh;
    submesh.startIndex = startIndex;
    submesh.indexCount = static_cast<uint32_t>(_indices.size()) - startIndex;
    submesh.materialIndex = mesh->mMaterialIndex;
    _subMeshes.push_back(submesh);
}

// Create DirectX 12 vertex and index buffers
void UIModel::CreateBuffers() {
    auto device = UIGraphicsSystem::GetSingletonInstance()->_graphicsDevice->GetD3DDevice();
    
    // Calculate buffer sizes
    const UINT vertexBufferSize = static_cast<UINT>(_vertices.size() * sizeof(Vertex));
    const UINT indexBufferSize = static_cast<UINT>(_indices.size() * sizeof(uint16_t));
    
    // Create vertex buffer (using Upload heap for simplicity)
    D3D12_HEAP_PROPERTIES uploadHeapProps = {};
    uploadHeapProps.Type = D3D12_HEAP_TYPE_UPLOAD;
    uploadHeapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    uploadHeapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    uploadHeapProps.CreationNodeMask = 1;
    uploadHeapProps.VisibleNodeMask = 1;
    
    D3D12_RESOURCE_DESC vertexBufferDesc = {};
    vertexBufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    vertexBufferDesc.Alignment = 0;
    vertexBufferDesc.Width = vertexBufferSize;
    vertexBufferDesc.Height = 1;
    vertexBufferDesc.DepthOrArraySize = 1;
    vertexBufferDesc.MipLevels = 1;
    vertexBufferDesc.Format = DXGI_FORMAT_UNKNOWN;
    vertexBufferDesc.SampleDesc.Count = 1;
    vertexBufferDesc.SampleDesc.Quality = 0;
    vertexBufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    vertexBufferDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
    
    device->CreateCommittedResource(
        &uploadHeapProps,
        D3D12_HEAP_FLAG_NONE,
        &vertexBufferDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&_vertexBuffer)
    );
    
    // Upload vertex data
    void* pVertexData;
    D3D12_RANGE readRange = {};
    readRange.Begin = 0;
    readRange.End = 0;  // We don't intend to read from this resource on the CPU
    _vertexBuffer->Map(0, &readRange, &pVertexData);
    memcpy(pVertexData, _vertices.data(), vertexBufferSize);
    _vertexBuffer->Unmap(0, nullptr);
    
    // Create vertex buffer view
    _vertexBufferView.BufferLocation = _vertexBuffer->GetGPUVirtualAddress();
    _vertexBufferView.SizeInBytes = vertexBufferSize;
    _vertexBufferView.StrideInBytes = sizeof(Vertex);
    
    // Create index buffer
    D3D12_RESOURCE_DESC indexBufferDesc = {};
    indexBufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    indexBufferDesc.Alignment = 0;
    indexBufferDesc.Width = indexBufferSize;
    indexBufferDesc.Height = 1;
    indexBufferDesc.DepthOrArraySize = 1;
    indexBufferDesc.MipLevels = 1;
    indexBufferDesc.Format = DXGI_FORMAT_UNKNOWN;
    indexBufferDesc.SampleDesc.Count = 1;
    indexBufferDesc.SampleDesc.Quality = 0;
    indexBufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    indexBufferDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
    
    device->CreateCommittedResource(
        &uploadHeapProps,
        D3D12_HEAP_FLAG_NONE,
        &indexBufferDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&_indexBuffer)
    );
    
    // Upload index data
    void* pIndexData;
    _indexBuffer->Map(0, &readRange, &pIndexData);
    memcpy(pIndexData, _indices.data(), indexBufferSize);
    _indexBuffer->Unmap(0, nullptr);
    
    // Create index buffer view
    _indexBufferView.BufferLocation = _indexBuffer->GetGPUVirtualAddress();
    _indexBufferView.SizeInBytes = indexBufferSize;
    _indexBufferView.Format = DXGI_FORMAT_R16_UINT;
}

// Render the mesh
void UIModel::Render(UICameraBase3D* pCamera, const DirectX::XMMATRIX& worldMatrix) {
    if (_vertices.empty() || _indices.empty()) {
        return;
    }
    
    auto graphicsSystem = UIGraphicsSystem::GetSingletonInstance();
    auto commandList = graphicsSystem->_graphicsDevice->GetCommandList();
    
    if (!_bones.empty()) {
        // Check if we should use SkinnedEffect (for models with bones)
        auto pSkinnedEffect = graphicsSystem->_graphicsDevice->_effectManager.p_skinnedEffect3D.get();
        
        // Set matrices
        pSkinnedEffect->SetWorld(worldMatrix);
        pSkinnedEffect->SetView(pCamera->GetViewMatrix());
        pSkinnedEffect->SetProjection(pCamera->GetProjectionMatrix());
        
        // Set bone transforms (max 72 bones)
        size_t boneCount = min(_bones.size(), size_t(DirectX::IEffectSkinning::MaxBones));
        XMMATRIX boneTransforms[DirectX::IEffectSkinning::MaxBones];
        for (size_t i = 0; i < boneCount; i++) {
            boneTransforms[i] = XMLoadFloat4x4(&_boneMatrices[i]);
        }
        pSkinnedEffect->SetBoneTransforms(boneTransforms, boneCount);
        
        // Set buffers
        commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        commandList->IASetVertexBuffers(0, 1, &_vertexBufferView);
        commandList->IASetIndexBuffer(&_indexBufferView);
        
        // Render submeshes
        if (_subMeshes.empty()) {
            pSkinnedEffect->Apply(commandList);
            commandList->DrawIndexedInstanced(static_cast<UINT>(_indices.size()), 1, 0, 0, 0);
        } else {
            for (const SubMesh& submesh : _subMeshes) {
                if (submesh.materialIndex < _materials.size()) {
                    const Material& mat = _materials[submesh.materialIndex];
                    
                    if (mat.hasDiffuseTexture && mat.diffuseSRV.ptr != 0) {
                        pSkinnedEffect->SetTexture(mat.diffuseSRV, graphicsSystem->_graphicsDevice->p_states.get()->LinearWrap());
                    }
                    pSkinnedEffect->Apply(commandList);
                    commandList->DrawIndexedInstanced(submesh.indexCount, 1, submesh.startIndex, 0, 0);
                }
            }
        }
    }
    else {
        // Static mesh rendering (BasicEffect)
        auto* pShapeEffect = graphicsSystem->_graphicsDevice->_effectManager.p_shapeEffect3D.get();

        // Set matrices
        pShapeEffect->SetWorld(worldMatrix);
        pShapeEffect->SetView(pCamera->GetViewMatrix());
        pShapeEffect->SetProjection(pCamera->GetProjectionMatrix());
        
        // Setup lighting
        pShapeEffect->EnableDefaultLighting();
        pShapeEffect->SetLightEnabled(0, true);
        pShapeEffect->SetLightDiffuseColor(0, DirectX::XMVectorSet(1.0f, 1.0f, 1.0f, 1.0f));
        pShapeEffect->SetLightDirection(0, DirectX::XMVectorSet(-0.5773f, -0.5773f, -0.5773f, 0.0f));
        
        pShapeEffect->SetLightEnabled(1, true);
        pShapeEffect->SetLightDiffuseColor(1, DirectX::XMVectorSet(0.5f, 0.5f, 0.6f, 1.0f));
        pShapeEffect->SetLightDirection(1, DirectX::XMVectorSet(0.5773f, -0.5773f, -0.5773f, 0.0f));
        
        pShapeEffect->SetLightEnabled(2, true);
        pShapeEffect->SetLightDiffuseColor(2, DirectX::XMVectorSet(0.4f, 0.4f, 0.3f, 1.0f));
        pShapeEffect->SetLightDirection(2, DirectX::XMVectorSet(0.0f, 0.7071f, -0.7071f, 0.0f));
        
        // Set material
        pShapeEffect->SetDiffuseColor(DirectX::XMVectorSet(1.0f, 1.0f, 1.0f, 1.0f));
        pShapeEffect->SetSpecularColor(DirectX::XMVectorSet(0.3f, 0.3f, 0.3f, 1.0f));
        pShapeEffect->SetSpecularPower(16.0f);
        pShapeEffect->SetAmbientLightColor(DirectX::XMVectorSet(0.7f, 0.7f, 0.7f, 1.0f));
        
        // Set buffers
        commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        commandList->IASetVertexBuffers(0, 1, &_vertexBufferView);
        commandList->IASetIndexBuffer(&_indexBufferView);
        
        // Render submeshes
        if (_subMeshes.empty()) {
            pShapeEffect->Apply(commandList);
            commandList->DrawIndexedInstanced(static_cast<UINT>(_indices.size()), 1, 0, 0, 0);
        } else {
            for (const SubMesh& submesh : _subMeshes) {
                if (submesh.materialIndex < _materials.size()) {
                    const Material& mat = _materials[submesh.materialIndex];
                    
                    if (mat.hasDiffuseTexture) {
                        pShapeEffect->SetTexture(mat.diffuseSRV, graphicsSystem->_graphicsDevice->p_states.get()->LinearWrap());
                    }
                    
                    pShapeEffect->Apply(commandList);
                    commandList->DrawIndexedInstanced(submesh.indexCount, 1, submesh.startIndex, 0, 0);
                }
            }
        }
    }
}

// Load materials from Assimp scene
void UIModel::LoadMaterials(const aiScene* scene, const wstring& modelDirectory) {
    _materials.resize(scene->mNumMaterials);
    
    for (unsigned int i = 0; i < scene->mNumMaterials; i++) {
        aiMaterial* material = scene->mMaterials[i];
        Material& mat = _materials[i];
        
        // Get material name
        aiString materialName;
        if (material->Get(AI_MATKEY_NAME, materialName) == AI_SUCCESS) {
            mat.name = STR_TO_WSTR(materialName.C_Str());
        }
        
        // Load diffuse texture
        if (material->GetTextureCount(aiTextureType_DIFFUSE) > 0) {
            aiString texPath;
            if (material->GetTexture(aiTextureType_DIFFUSE, 0, &texPath) == AI_SUCCESS) {
                std::string texPathStr = texPath.C_Str();
        
                // Extract filename only (remove directory path)
                size_t lastSlash = texPathStr.find_last_of("/\\");
                if (lastSlash != std::string::npos) {
                    texPathStr = texPathStr.substr(lastSlash + 1);
                }
                
                wstring wtexPath = STR_TO_WSTR(texPathStr);
                mat.diffuseTexturePath = modelDirectory + wtexPath;
                
                if (GetFileAttributesW(mat.diffuseTexturePath.c_str()) == INVALID_FILE_ATTRIBUTES) {
                    // Check if filename already ends with .png or .PNG
                    bool hasPngExtension = false;
                    if (mat.diffuseTexturePath.size() >= 4) {
                        wstring ext = mat.diffuseTexturePath.substr(mat.diffuseTexturePath.size() - 4);
                        hasPngExtension = (ext == L".png" || ext == L".PNG" || ext == L".jpg" || ext == L".JPG");
                    }
                    
                    if (!hasPngExtension) {
                        mat.diffuseTexturePath += L".png";
                    }
                }
                
                mat.hasDiffuseTexture = LoadTexture(mat.diffuseTexturePath, mat.diffuseTexture, mat.diffuseSRV);
            }
        }
        
        // Load emissive texture
        if (material->GetTextureCount(aiTextureType_EMISSIVE) > 0) {
            aiString texPath;
            if (material->GetTexture(aiTextureType_EMISSIVE, 0, &texPath) == AI_SUCCESS) {
                std::string texPathStr = texPath.C_Str();
                
                // Extract filename only (remove directory path)
                size_t lastSlash = texPathStr.find_last_of("/\\");
                if (lastSlash != std::string::npos) {
                    texPathStr = texPathStr.substr(lastSlash + 1);
                }
                
                wstring wtexPath = STR_TO_WSTR(texPathStr);
                mat.emissiveTexturePath = modelDirectory + wtexPath;
                
                if (GetFileAttributesW(mat.emissiveTexturePath.c_str()) == INVALID_FILE_ATTRIBUTES) {
                    // Check if filename already ends with .png or .PNG
                    bool hasPngExtension = false;
                    if (mat.emissiveTexturePath.size() >= 4) {
                        wstring ext = mat.emissiveTexturePath.substr(mat.emissiveTexturePath.size() - 4);
                        hasPngExtension = (ext == L".png" || ext == L".PNG" || ext == L".jpg" || ext == L".JPG");
                    }
                    
                    if (!hasPngExtension) {
                        mat.emissiveTexturePath += L".png";
                    }
                }
                
                mat.hasEmissiveTexture = LoadTexture(mat.emissiveTexturePath, mat.emissiveTexture, mat.emissiveSRV);
            }
        }
    }
}

size_t UIModel::_nextDescriptorIndex = 0;

// Load a single texture file
bool UIModel::LoadTexture(const wstring& texturePath, ComPtr<ID3D12Resource>& texture, D3D12_GPU_DESCRIPTOR_HANDLE& srvHandle) {
    auto graphicsSystem = UIGraphicsSystem::GetSingletonInstance();
    auto device = graphicsSystem->_graphicsDevice->GetD3DDevice();
    auto commandQueue = graphicsSystem->_graphicsDevice->GetCommandQueue();
    
    wchar_t strTexturePath[MAX_PATH] = {};
    FindMediaFile(strTexturePath, MAX_PATH, texturePath.c_str());
    
    // Load texture with ResourceUploadBatch
    DirectX::ResourceUploadBatch resourceUpload(device);
    resourceUpload.Begin();
    
    HRESULT hr = DirectX::CreateWICTextureFromFile(
        device,
        resourceUpload,
        strTexturePath,
        texture.ReleaseAndGetAddressOf()
    );
    
    if (FAILED(hr)) {
        return false;
    }
    
    // Allocate descriptor
    size_t descriptorIndex = Descriptors::ModelTexturesStart + _nextDescriptorIndex;
    _nextDescriptorIndex++;
    
    if (_nextDescriptorIndex >= Descriptors::ModelTexturesCount) {
        return false;
    }
    
    // Create SRV
    DirectX::CreateShaderResourceView(device, texture.Get(), graphicsSystem->_graphicsDevice->p_resourceDescriptors->GetCpuHandle(descriptorIndex));
    
    srvHandle = graphicsSystem->_graphicsDevice->p_resourceDescriptors->GetGpuHandle(descriptorIndex);
    
    // Wait for upload
    auto uploadFinished = resourceUpload.End(commandQueue);
    uploadFinished.wait();
    
    return true;
}

// Get or create bone index
int UIModel::GetOrCreateBoneIndex(const std::string& boneName) {
    auto it = _boneNameToIndex.find(boneName);
    if (it != _boneNameToIndex.end()) {
        return it->second;
    }
    
    // Create new bone
    int index = static_cast<int>(_bones.size());
    _boneNameToIndex[boneName] = index;
    
    Bone bone;
    bone.name = boneName;
    _bones.push_back(bone);
    
    return index;
}

// Add bone weight to vertex (up to 4 bones per vertex)
void UIModel::AddBoneWeight(Vertex& vertex, int boneIndex, float weight) {
    // Safety clamp: ensure bone index is within valid range
    if (boneIndex >= DirectX::IEffectSkinning::MaxBones) {
        boneIndex = 0;  // Fallback to root bone
        weight *= 0.1f; // Reduce visual impact
    }
    
    // Find empty slot or smallest weight to replace
    for (int i = 0; i < 4; i++) {
        if (vertex.boneWeights[i] == 0.0f) {
            vertex.boneIndices[i] = static_cast<uint8_t>(boneIndex);
            vertex.boneWeights[i] = weight;
            return;
        }
    }
    
    // All slots occupied, replace smallest weight if new weight is larger
    int minIndex = 0;
    float minWeight = vertex.boneWeights[0];
    for (int i = 1; i < 4; i++) {
        if (vertex.boneWeights[i] < minWeight) {
            minWeight = vertex.boneWeights[i];
            minIndex = i;
        }
    }
    
    if (weight > minWeight) {
        vertex.boneIndices[minIndex] = static_cast<uint8_t>(boneIndex);
        vertex.boneWeights[minIndex] = weight;
    }
}

// Load bone hierarchy from scene nodes
void UIModel::LoadBoneHierarchy(aiNode* node, int parentIndex) {
    if (!node) return;
    
    // Check if this node is a bone
    std::string nodeName = node->mName.C_Str();
    auto it = _boneNameToIndex.find(nodeName);
    
    int currentBoneIndex = parentIndex;
    if (it != _boneNameToIndex.end()) {
        // This node is a bone
        currentBoneIndex = it->second;
        _bones[currentBoneIndex].parentIndex = parentIndex;
        
        // Store local transform
        aiMatrix4x4& t = node->mTransformation;
        _bones[currentBoneIndex].localTransform = DirectX::XMFLOAT4X4(
            t.a1, t.b1, t.c1, t.d1,
            t.a2, t.b2, t.c2, t.d2,
            t.a3, t.b3, t.c3, t.d3,
            t.a4, t.b4, t.c4, t.d4
        );
    }
    
    // Recursively process children
    for (unsigned int i = 0; i < node->mNumChildren; i++) {
        LoadBoneHierarchy(node->mChildren[i], currentBoneIndex);
    }
}

// Load animations from scene
void UIModel::LoadAnimations(const aiScene* scene) {
    _animations.resize(scene->mNumAnimations);
    
    for (unsigned int animIdx = 0; animIdx < scene->mNumAnimations; animIdx++) {
        aiAnimation* anim = scene->mAnimations[animIdx];
        AnimationClip& clip = _animations[animIdx];
        
        clip.name = anim->mName.C_Str();
        clip.duration = static_cast<float>(anim->mDuration);
        clip.ticksPerSecond = static_cast<float>(anim->mTicksPerSecond > 0 ? anim->mTicksPerSecond : 25.0f);
        
        // Load all bone animation channels
        clip.channels.resize(anim->mNumChannels);
        
        for (unsigned int chanIdx = 0; chanIdx < anim->mNumChannels; chanIdx++) {
            aiNodeAnim* channel = anim->mChannels[chanIdx];
            BoneAnimChannel& boneChannel = clip.channels[chanIdx];
            
            boneChannel.boneName = channel->mNodeName.C_Str();
            
            // Load position keys
            boneChannel.positions.resize(channel->mNumPositionKeys);
            for (unsigned int i = 0; i < channel->mNumPositionKeys; i++) {
                boneChannel.positions[i].time = static_cast<float>(channel->mPositionKeys[i].mTime);
                boneChannel.positions[i].value = DirectX::XMFLOAT3(
                    channel->mPositionKeys[i].mValue.x,
                    channel->mPositionKeys[i].mValue.y,
                    channel->mPositionKeys[i].mValue.z
                );
            }
            
            // Load rotation keys (quaternions)
            boneChannel.rotations.resize(channel->mNumRotationKeys);
            for (unsigned int i = 0; i < channel->mNumRotationKeys; i++) {
                boneChannel.rotations[i].time = static_cast<float>(channel->mRotationKeys[i].mTime);
                boneChannel.rotations[i].value = DirectX::XMFLOAT4(
                    channel->mRotationKeys[i].mValue.x,
                    channel->mRotationKeys[i].mValue.y,
                    channel->mRotationKeys[i].mValue.z,
                    channel->mRotationKeys[i].mValue.w
                );
            }
            
            // Load scaling keys
            boneChannel.scalings.resize(channel->mNumScalingKeys);
            for (unsigned int i = 0; i < channel->mNumScalingKeys; i++) {
                boneChannel.scalings[i].time = static_cast<float>(channel->mScalingKeys[i].mTime);
                boneChannel.scalings[i].value = DirectX::XMFLOAT3(
                    channel->mScalingKeys[i].mValue.x,
                    channel->mScalingKeys[i].mValue.y,
                    channel->mScalingKeys[i].mValue.z
                );
            }
        }
    }
}

// ============================================================================
// Bone Transform Calculation
// ============================================================================

// Update all bone transforms for current animation time
void UIModel::UpdateBoneTransforms(int animIndex, float time) {
    if (animIndex < 0 || animIndex >= _animations.size()) {
        return;
    }
    
    AnimationClip& anim = _animations[animIndex];
    
    // Update bone local transforms from animation
    for (const BoneAnimChannel& channel : anim.channels) {
        auto it = _boneNameToIndex.find(channel.boneName);
        if (it == _boneNameToIndex.end()) continue;
        
        int boneIndex = it->second;
        Bone& bone = _bones[boneIndex];
        
        // Interpolate transformation components
        DirectX::XMVECTOR position = InterpolatePosition(channel, time);
        DirectX::XMVECTOR rotation = InterpolateRotation(channel, time);
        DirectX::XMVECTOR scaling = InterpolateScaling(channel, time);
        
        // Build transformation matrix
        DirectX::XMMATRIX transform = DirectX::XMMatrixScalingFromVector(scaling) *
                                     DirectX::XMMatrixRotationQuaternion(rotation) *
                                     DirectX::XMMatrixTranslationFromVector(position);
        
        DirectX::XMStoreFloat4x4(&bone.localTransform, transform);
    }
    
    // Calculate global transforms recursively
    for (size_t i = 0; i < _bones.size(); i++) {
        if (_bones[i].parentIndex == -1) {
            CalculateBoneTransform(static_cast<int>(i), DirectX::XMMatrixIdentity());
        }
    }
    
    // Generate final bone matrices for GPU
    for (size_t i = 0; i < _bones.size() && i < MAX_BONES; i++) {
        DirectX::XMMATRIX offset = DirectX::XMLoadFloat4x4(&_bones[i].offsetMatrix);
        DirectX::XMMATRIX global = DirectX::XMLoadFloat4x4(&_bones[i].globalTransform);
        DirectX::XMMATRIX finalTransform = offset * global;
        DirectX::XMStoreFloat4x4(&_boneMatrices[i], DirectX::XMMatrixTranspose(finalTransform));
    }
}

// Calculate bone global transform recursively
void UIModel::CalculateBoneTransform(int boneIndex, const DirectX::XMMATRIX& parentTransform) {
    Bone& bone = _bones[boneIndex];
    
    // Global = Parent * Local
    DirectX::XMMATRIX local = DirectX::XMLoadFloat4x4(&bone.localTransform);
    DirectX::XMMATRIX global = local * parentTransform;
    DirectX::XMStoreFloat4x4(&bone.globalTransform, global);
    
    // Recursively update children
    for (size_t i = 0; i < _bones.size(); i++) {
        if (_bones[i].parentIndex == boneIndex) {
            CalculateBoneTransform(static_cast<int>(i), global);
        }
    }
}

// ============================================================================
// Keyframe Interpolation Functions
// ============================================================================

// Interpolate position between keyframes
DirectX::XMVECTOR UIModel::InterpolatePosition(const BoneAnimChannel& channel, float time) {
    if (channel.positions.empty()) {
        return DirectX::XMVectorSet(0, 0, 0, 0);
    }
    
    if (channel.positions.size() == 1) {
        return DirectX::XMLoadFloat3(&channel.positions[0].value);
    }
    
    // Find keyframes to interpolate between
    for (size_t i = 0; i < channel.positions.size() - 1; i++) {
        if (time < channel.positions[i + 1].time) {
            float t0 = channel.positions[i].time;
            float t1 = channel.positions[i + 1].time;
            float factor = (time - t0) / (t1 - t0);
            
            DirectX::XMVECTOR v0 = DirectX::XMLoadFloat3(&channel.positions[i].value);
            DirectX::XMVECTOR v1 = DirectX::XMLoadFloat3(&channel.positions[i + 1].value);
            
            return DirectX::XMVectorLerp(v0, v1, factor);
        }
    }
    
    // Use last keyframe
    return DirectX::XMLoadFloat3(&channel.positions.back().value);
}

// Interpolate rotation (quaternion slerp)
DirectX::XMVECTOR UIModel::InterpolateRotation(const BoneAnimChannel& channel, float time) {
    if (channel.rotations.empty()) {
        return DirectX::XMQuaternionIdentity();
    }
    
    if (channel.rotations.size() == 1) {
        return DirectX::XMLoadFloat4(&channel.rotations[0].value);
    }
    
    // Find keyframes to interpolate between
    for (size_t i = 0; i < channel.rotations.size() - 1; i++) {
        if (time < channel.rotations[i + 1].time) {
            float t0 = channel.rotations[i].time;
            float t1 = channel.rotations[i + 1].time;
            float factor = (time - t0) / (t1 - t0);
            
            DirectX::XMVECTOR q0 = DirectX::XMLoadFloat4(&channel.rotations[i].value);
            DirectX::XMVECTOR q1 = DirectX::XMLoadFloat4(&channel.rotations[i + 1].value);
            
            return DirectX::XMQuaternionSlerp(q0, q1, factor);
        }
    }
    
    // Use last keyframe
    return DirectX::XMLoadFloat4(&channel.rotations.back().value);
}

// Interpolate scaling
DirectX::XMVECTOR UIModel::InterpolateScaling(const BoneAnimChannel& channel, float time) {
    if (channel.scalings.empty()) {
        return DirectX::XMVectorSet(1, 1, 1, 0);
    }
    
    if (channel.scalings.size() == 1) {
        return DirectX::XMLoadFloat3(&channel.scalings[0].value);
    }
    
    // Find keyframes to interpolate between
    for (size_t i = 0; i < channel.scalings.size() - 1; i++) {
        if (time < channel.scalings[i + 1].time) {
            float t0 = channel.scalings[i].time;
            float t1 = channel.scalings[i + 1].time;
            float factor = (time - t0) / (t1 - t0);
            
            DirectX::XMVECTOR v0 = DirectX::XMLoadFloat3(&channel.scalings[i].value);
            DirectX::XMVECTOR v1 = DirectX::XMLoadFloat3(&channel.scalings[i + 1].value);
            
            return DirectX::XMVectorLerp(v0, v1, factor);
        }
    }
    
    // Use last keyframe
    return DirectX::XMLoadFloat3(&channel.scalings.back().value);
}







void UIModelAnimation::PlayAnimation(int animIndex, bool loop, float speed) {
    // Validate animation index
    if (animIndex < 0 || animIndex >= static_cast<int>(_animations.size())) {
        return;
    }

    _currentAnimIndex = animIndex;
    _animSpeed = speed;
    _isLooping = loop;

    // Start permanent animation (duration = -1 means it never auto-stops)
    // Explicitly call UIAnimateSecondHelp::PlayAnimation to avoid recursion
    UIAnimateSecondHelp::PlayAnimation(-1.0f);
}

void UIModelAnimation::PlayAnimation(const std::string& animName, bool loop, float speed) {
    // Find animation by name
    for (int i = 0; i < static_cast<int>(_animations.size()); ++i) {
        if (_animations[i].name == animName) {
            PlayAnimation(i, loop, speed);
            return;
        }
    }
}

void UIModelAnimation::DrawAnimation() {
    // Check if animation is running (using UIAnimateSecondHelp's state)
    if (!IsAnimationRun() || _currentAnimIndex < 0) {
        return;
    }

    // Validate animation index
    if (_currentAnimIndex >= static_cast<int>(_animations.size())) {
        return;
    }

    // Get current animation time from UIAnimateSecondHelp
    float currentTime = GetElapsedTime();
    
    // Apply speed multiplier
    float animTime = currentTime * _animSpeed;

    // Get current animation duration
    const AnimationClip& currentAnim = _animations[_currentAnimIndex];
    
    if (animTime >= currentAnim.duration) {
        if (_isLooping) {
            // Loop the animation - reset elapsed time
            animTime = fmod(animTime, currentAnim.duration);
        } else {
            // Stop at the end
            animTime = currentAnim.duration;
            UIAnimateSecondHelp::StopAnimation();  // Call parent class method
        }
    }

    // Update bone transforms for current animation
    UpdateBoneTransforms(_currentAnimIndex, animTime);
}
