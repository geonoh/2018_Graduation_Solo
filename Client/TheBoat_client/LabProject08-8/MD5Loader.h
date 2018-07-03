#pragma once
#include "Mesh.h"
#include "UploadBuffer.h"
#include <stdio.h>

	bool LoadMD5Model(ID3D12Device *pd3dDevice, ID3D12GraphicsCommandList *pd3dCommandList, ID3D12RootSignature *pd3dGraphicsRootSignature, std::wstring filename, Model3D& MD5Model, std::vector<D3D12_SHADER_RESOURCE_VIEW_DESC*>& shaderResourceViewArray, std::vector<std::wstring> texFileNameArray, CMesh*& pMesh);
	bool LoadMD5Anim(std::wstring filename, Model3D& MD5Model);
	void UpdateMD5Model(Model3D& MD5Model, float deltaTime, int animation, CMesh*& pMesh, int meshnum);


