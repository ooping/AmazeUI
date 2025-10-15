#include "StaticMesh.h"
#include <codecvt>
#include <locale>

// DirectXTK12 headers for texture loading
#include "ResourceUploadBatch.h"
#include "WICTextureLoader.h"
#include "DDSTextureLoader.h"

using namespace DirectX;
using namespace Microsoft::WRL;
using namespace std;


// Load model from file using Assimp
// XXX\\source\\xxx.fbx
// XXX\\textures\\xxx.png
bool StaticMesh::LoadFromFile(const wstring& filePath) {
    // Locate model file
    wchar_t strFilePath[MAX_PATH] = {};
    DX::FindMediaFile(strFilePath, MAX_PATH, filePath.c_str());
    
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
    
    return true;
}

// Process scene node recursively
void StaticMesh::ProcessNode(aiNode* node, const aiScene* scene) {
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
void StaticMesh::ProcessMesh(aiMesh* mesh) {
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
void StaticMesh::CreateBuffers() {
    auto device = UIDXFoundation::GetSingletonInstance()->GetD3DDevice();
    
    // Calculate buffer sizes
    const UINT vertexBufferSize = static_cast<UINT>(_vertices.size() * sizeof(Vertex));
    const UINT indexBufferSize = static_cast<UINT>(_indices.size() * sizeof(uint16_t));
    
    // Create vertex buffer (using Upload heap for simplicity)
    CD3DX12_HEAP_PROPERTIES uploadHeapProps(D3D12_HEAP_TYPE_UPLOAD);
    CD3DX12_RESOURCE_DESC vertexBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(vertexBufferSize);
    
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
    CD3DX12_RANGE readRange(0, 0); // We don't intend to read from this resource on the CPU
    _vertexBuffer->Map(0, &readRange, &pVertexData);
    memcpy(pVertexData, _vertices.data(), vertexBufferSize);
    _vertexBuffer->Unmap(0, nullptr);
    
    // Create vertex buffer view
    _vertexBufferView.BufferLocation = _vertexBuffer->GetGPUVirtualAddress();
    _vertexBufferView.SizeInBytes = vertexBufferSize;
    _vertexBufferView.StrideInBytes = sizeof(Vertex);
    
    // Create index buffer
    CD3DX12_RESOURCE_DESC indexBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(indexBufferSize);
    
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
void StaticMesh::Render(UICameraBase3D* pCamera, const DirectX::XMMATRIX& worldMatrix) {
    if (_vertices.empty() || _indices.empty()) {
        return;
    }
    
    auto* pDX12 = UIDXFoundation::GetSingletonInstance();
    auto commandList = pDX12->GetDeviceResources()->GetCommandList();
    auto* effect = pDX12->Get3DShapeEffect();
    
    if (!effect) {
        return;
    }
    
    // Set matrices
    effect->SetWorld(worldMatrix);
    effect->SetView(pCamera->GetViewMatrix());
    effect->SetProjection(pCamera->GetProjectionMatrix());
    
    // Setup lighting
    effect->EnableDefaultLighting();
    effect->SetLightEnabled(0, true);
    effect->SetLightDiffuseColor(0, DirectX::XMVectorSet(1.0f, 1.0f, 1.0f, 1.0f));
    effect->SetLightDirection(0, DirectX::XMVectorSet(-0.5773f, -0.5773f, -0.5773f, 0.0f));
    
    effect->SetLightEnabled(1, true);
    effect->SetLightDiffuseColor(1, DirectX::XMVectorSet(0.5f, 0.5f, 0.6f, 1.0f));
    effect->SetLightDirection(1, DirectX::XMVectorSet(0.5773f, -0.5773f, -0.5773f, 0.0f));
    
    effect->SetLightEnabled(2, true);
    effect->SetLightDiffuseColor(2, DirectX::XMVectorSet(0.4f, 0.4f, 0.3f, 1.0f));
    effect->SetLightDirection(2, DirectX::XMVectorSet(0.0f, 0.7071f, -0.7071f, 0.0f));
    
    // Set material
    effect->SetDiffuseColor(DirectX::XMVectorSet(1.0f, 1.0f, 1.0f, 1.0f));
    effect->SetSpecularColor(DirectX::XMVectorSet(0.3f, 0.3f, 0.3f, 1.0f));
    effect->SetSpecularPower(16.0f);
    effect->SetAmbientLightColor(DirectX::XMVectorSet(0.7f, 0.7f, 0.7f, 1.0f));
    
    // Set buffers
    commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    commandList->IASetVertexBuffers(0, 1, &_vertexBufferView);
    commandList->IASetIndexBuffer(&_indexBufferView);
    
    // Render submeshes
    if (_subMeshes.empty()) {
        effect->Apply(commandList);
        commandList->DrawIndexedInstanced(static_cast<UINT>(_indices.size()), 1, 0, 0, 0);
    } else {
        for (const SubMesh& submesh : _subMeshes) {
            if (submesh.materialIndex < _materials.size()) {
                const Material& mat = _materials[submesh.materialIndex];
                
                if (mat.hasDiffuseTexture) {
                    effect->SetTexture(mat.diffuseSRV, pDX12->p_states->LinearWrap());
                }
                
                effect->Apply(commandList);
                commandList->DrawIndexedInstanced(submesh.indexCount, 1, submesh.startIndex, 0, 0);
            }
        }
    }
}

// Load materials from Assimp scene
void StaticMesh::LoadMaterials(const aiScene* scene, const wstring& modelDirectory) {
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
                wstring wtexPath = STR_TO_WSTR(texPath.C_Str());
                
                mat.diffuseTexturePath = modelDirectory + wtexPath;
                
                if (GetFileAttributesW(mat.diffuseTexturePath.c_str()) == INVALID_FILE_ATTRIBUTES) {
                    mat.diffuseTexturePath += L".png";
                }
                
                mat.hasDiffuseTexture = LoadTexture(mat.diffuseTexturePath, mat.diffuseTexture, mat.diffuseSRV);
            }
        }
        
        // Load emissive texture
        if (material->GetTextureCount(aiTextureType_EMISSIVE) > 0) {
            aiString texPath;
            if (material->GetTexture(aiTextureType_EMISSIVE, 0, &texPath) == AI_SUCCESS) {
                wstring wtexPath = STR_TO_WSTR(texPath.C_Str());
                
                mat.emissiveTexturePath = modelDirectory + wtexPath;
                
                if (GetFileAttributesW(mat.emissiveTexturePath.c_str()) == INVALID_FILE_ATTRIBUTES) {
                    mat.emissiveTexturePath += L".png";
                }
                
                mat.hasEmissiveTexture = LoadTexture(mat.emissiveTexturePath, mat.emissiveTexture, mat.emissiveSRV);
            }
        }
    }
}

size_t StaticMesh::_nextDescriptorIndex = 0;

// Load a single texture file
bool StaticMesh::LoadTexture(const wstring& texturePath, ComPtr<ID3D12Resource>& texture, D3D12_GPU_DESCRIPTOR_HANDLE& srvHandle) {
    auto foundation = UIDXFoundation::GetSingletonInstance();
    if (!foundation) {
        return false;
    }
    
    auto device = foundation->GetD3DDevice();
    auto commandQueue = foundation->p_deviceResources->GetCommandQueue();
    
    wchar_t strTexturePath[MAX_PATH] = {};
    DX::FindMediaFile(strTexturePath, MAX_PATH, texturePath.c_str());
    
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
    size_t descriptorIndex = UIDXFoundation::Descriptors::ModelTexturesStart + _nextDescriptorIndex;
    _nextDescriptorIndex++;
    
    if (_nextDescriptorIndex >= UIDXFoundation::Descriptors::ModelTexturesCount) {
        return false;
    }
    
    // Create SRV
    DirectX::CreateShaderResourceView(
        device,
        texture.Get(),
        foundation->p_resourceDescriptors->GetCpuHandle(descriptorIndex)
    );
    
    srvHandle = foundation->p_resourceDescriptors->GetGpuHandle(descriptorIndex);
    
    // Wait for upload
    auto uploadFinished = resourceUpload.End(commandQueue);
    uploadFinished.wait();
    
    return true;
}
