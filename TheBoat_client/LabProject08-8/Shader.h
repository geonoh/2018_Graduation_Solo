//-----------------------------------------------------------------------------
// File: Shader.h
//-----------------------------------------------------------------------------

#pragma once

#include "Object.h"
#include "Camera.h"

class CShader
{
public:
	CShader();
	virtual ~CShader();

private:
	int								m_nReferences = 0;

public:
	static int	shootBullet;

	void AddRef() { m_nReferences++; }
	void Release() { if (--m_nReferences <= 0) delete this; }

	virtual D3D12_INPUT_LAYOUT_DESC CreateInputLayout();
	virtual D3D12_RASTERIZER_DESC CreateRasterizerState();
	virtual D3D12_BLEND_DESC CreateBlendState();
	virtual D3D12_DEPTH_STENCIL_DESC CreateDepthStencilState();

	virtual D3D12_SHADER_BYTECODE CreateVertexShader(ID3DBlob **ppd3dShaderBlob);
	virtual D3D12_SHADER_BYTECODE CreatePixelShader(ID3DBlob **ppd3dShaderBlob);
	D3D12_SHADER_BYTECODE CompileShaderFromFile(WCHAR *pszFileName, LPCSTR pszShaderName, LPCSTR pszShaderProfile, ID3DBlob **ppd3dShaderBlob);

	virtual void CreateShader(ID3D12Device *pd3dDevice, ID3D12RootSignature *pd3dGraphicsRootSignature);

	void CreateCbvAndSrvDescriptorHeaps(ID3D12Device *pd3dDevice, ID3D12GraphicsCommandList *pd3dCommandList, int nConstantBufferViews, int nShaderResourceViews);
	void CreateConstantBufferViews(ID3D12Device *pd3dDevice, ID3D12GraphicsCommandList *pd3dCommandList, int nConstantBufferViews, ID3D12Resource *pd3dConstantBuffers, UINT nStride);
	void CreateShaderResourceViews(ID3D12Device *pd3dDevice, ID3D12GraphicsCommandList *pd3dCommandList, CTexture *pTexture, UINT nRootParameterStartIndex, bool bAutoIncrement);

	virtual void CreateShaderVariables(ID3D12Device *pd3dDevice, ID3D12GraphicsCommandList *pd3dCommandList);
	virtual void UpdateShaderVariables(ID3D12GraphicsCommandList *pd3dCommandList);
	virtual void ReleaseShaderVariables();

	virtual void UpdateShaderVariable(ID3D12GraphicsCommandList *pd3dCommandList, XMFLOAT4X4 *pxmf4x4World);

	virtual void BuildObjects(ID3D12Device *pd3dDevice, ID3D12GraphicsCommandList *pd3dCommandList, void *pContext = NULL) { }
	virtual void AnimateObjects(float fTimeElapsed, CCamera *pCamera) { }
	virtual void ReleaseObjects() { }

	virtual void ReleaseUploadBuffers();

	virtual void OnPrepareRender(ID3D12GraphicsCommandList *pd3dCommandList);
	virtual void Render(ID3D12GraphicsCommandList *pd3dCommandList, CCamera *pCamera);
	virtual void Render(ID3D12GraphicsCommandList *pd3dCommandList, CCamera *pCamera, float hp);

	virtual void SetPosition(int id, XMFLOAT3 input);

	D3D12_CPU_DESCRIPTOR_HANDLE GetCPUDescriptorHandleForHeapStart() { return(m_pd3dCbvSrvDescriptorHeap->GetCPUDescriptorHandleForHeapStart()); }
	D3D12_GPU_DESCRIPTOR_HANDLE GetGPUDescriptorHandleForHeapStart() { return(m_pd3dCbvSrvDescriptorHeap->GetGPUDescriptorHandleForHeapStart()); }

	D3D12_CPU_DESCRIPTOR_HANDLE GetCPUCbvDescriptorStartHandle() { return(m_d3dCbvCPUDescriptorStartHandle); }
	D3D12_GPU_DESCRIPTOR_HANDLE GetGPUCbvDescriptorStartHandle() { return(m_d3dCbvGPUDescriptorStartHandle); }
	D3D12_CPU_DESCRIPTOR_HANDLE GetCPUSrvDescriptorStartHandle() { return(m_d3dSrvCPUDescriptorStartHandle); }
	D3D12_GPU_DESCRIPTOR_HANDLE GetGPUSrvDescriptorStartHandle() { return(m_d3dSrvGPUDescriptorStartHandle); }

protected:
	ID3D12PipelineState * *m_ppd3dPipelineStates = NULL;
	int								m_nPipelineStates = 0;

	ID3D12DescriptorHeap			*m_pd3dCbvSrvDescriptorHeap = NULL;

	D3D12_CPU_DESCRIPTOR_HANDLE		m_d3dCbvCPUDescriptorStartHandle;
	D3D12_GPU_DESCRIPTOR_HANDLE		m_d3dCbvGPUDescriptorStartHandle;
	D3D12_CPU_DESCRIPTOR_HANDLE		m_d3dSrvCPUDescriptorStartHandle;
	D3D12_GPU_DESCRIPTOR_HANDLE		m_d3dSrvGPUDescriptorStartHandle;
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
class CPlayerShader : public CShader
{
public:
	CPlayerShader();
	virtual ~CPlayerShader();

	virtual D3D12_INPUT_LAYOUT_DESC CreateInputLayout();
	virtual D3D12_SHADER_BYTECODE CreateVertexShader(ID3DBlob **ppd3dShaderBlob);
	virtual D3D12_SHADER_BYTECODE CreatePixelShader(ID3DBlob **ppd3dShaderBlob);

	virtual void CreateShader(ID3D12Device *pd3dDevice, ID3D12RootSignature *pd3dGraphicsRootSignature);
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
class CIlluminatedShader : public CShader
{
public:
	CIlluminatedShader();
	virtual ~CIlluminatedShader();

	virtual D3D12_INPUT_LAYOUT_DESC CreateInputLayout();

	virtual void CreateShader(ID3D12Device *pd3dDevice, ID3D12RootSignature *pd3dGraphicsRootSignature);

	virtual D3D12_SHADER_BYTECODE CreateVertexShader(ID3DBlob **ppd3dShaderBlob);
	virtual D3D12_SHADER_BYTECODE CreatePixelShader(ID3DBlob **ppd3dShaderBlob);
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
class CTexturedShader : public CShader
{
public:
	CTexturedShader();
	virtual ~CTexturedShader();

	virtual D3D12_INPUT_LAYOUT_DESC CreateInputLayout();

	virtual void CreateShader(ID3D12Device *pd3dDevice, ID3D12RootSignature *pd3dGraphicsRootSignature);

	virtual D3D12_SHADER_BYTECODE CreateVertexShader(ID3DBlob **ppd3dShaderBlob);
	virtual D3D12_SHADER_BYTECODE CreatePixelShader(ID3DBlob **ppd3dShaderBlob);
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
class CIlluminatedTexturedShader : public CTexturedShader
{
public:
	CIlluminatedTexturedShader();
	virtual ~CIlluminatedTexturedShader();

	virtual D3D12_INPUT_LAYOUT_DESC CreateInputLayout();

	virtual void CreateShader(ID3D12Device *pd3dDevice, ID3D12RootSignature *pd3dGraphicsRootSignature);

	virtual D3D12_SHADER_BYTECODE CreateVertexShader(ID3DBlob **ppd3dShaderBlob);
	virtual D3D12_SHADER_BYTECODE CreatePixelShader(ID3DBlob **ppd3dShaderBlob);
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
#define _WITH_BATCH_MATERIAL

class CObjectsShader : public CIlluminatedTexturedShader
{
public:
	CObjectsShader();
	virtual ~CObjectsShader();

	virtual void BuildObjects(ID3D12Device *pd3dDevice, ID3D12GraphicsCommandList *pd3dCommandList, void *pContext = NULL);
	virtual void AnimateObjects(float fTimeElapsed, CCamera *pCamera);
	virtual void ReleaseObjects();

	virtual void CreateShaderVariables(ID3D12Device *pd3dDevice, ID3D12GraphicsCommandList *pd3dCommandList);
	virtual void UpdateShaderVariables(ID3D12GraphicsCommandList *pd3dCommandList);
	virtual void ReleaseShaderVariables();

	virtual void ReleaseUploadBuffers();

	virtual void Render(ID3D12GraphicsCommandList *pd3dCommandList, CCamera *pCamera);

protected:
	CGameObject * *m_ppObjects = 0;
	int								m_nObjects = 0;

	ID3D12Resource					*m_pd3dcbGameObjects = NULL;
	CB_GAMEOBJECT_INFO				*m_pcbMappedGameObjects = NULL;

#ifdef _WITH_BATCH_MATERIAL
	CMaterial						*m_pMaterial = NULL;
#endif
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
class CTerrainShader : public CTexturedShader
{
public:
	CTerrainShader();
	virtual ~CTerrainShader();

	virtual D3D12_INPUT_LAYOUT_DESC CreateInputLayout();

	virtual D3D12_SHADER_BYTECODE CreateVertexShader(ID3DBlob **ppd3dShaderBlob);
	virtual D3D12_SHADER_BYTECODE CreatePixelShader(ID3DBlob **ppd3dShaderBlob);

	virtual void CreateShader(ID3D12Device *pd3dDevice, ID3D12RootSignature *pd3dGraphicsRootSignature);
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
class CSkyBoxShader : public CTexturedShader
{
public:
	CSkyBoxShader();
	virtual ~CSkyBoxShader();

	virtual D3D12_DEPTH_STENCIL_DESC CreateDepthStencilState();


	virtual D3D12_SHADER_BYTECODE CreatePixelShader(ID3DBlob **ppd3dShaderBlob);

	virtual void CreateShader(ID3D12Device *pd3dDevice, ID3D12RootSignature *pd3dGraphicsRootSignature);
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class CTreeShader : public CTexturedShader
{
protected:
	CRotatingObject *				*m_ppTree = 0;
	int								m_nTree = 0;
	int								setRedDot = 0;

#ifdef _WITH_BATCH_MATERIAL
	CMaterial						*m_pMaterial = NULL;
#endif

	ID3D12Resource					*m_pd3dcbGameObjects = NULL;
	CB_GAMEOBJECT_INFO				*m_pcbMappedGameObjects = NULL;

public:
	CTreeShader();
	virtual ~CTreeShader();

	void		 SetRedDot(int input) { setRedDot = input; }
	virtual void BuildObjects(ID3D12Device *pd3dDevice, ID3D12GraphicsCommandList *pd3dCommandList, void *pContext);
	virtual void ReleaseObjects();
	virtual void AnimateObjects(float fTimeElapsed, CCamera *pCamera);
	virtual void ReleaseUploadBuffers();
	virtual void Render(ID3D12GraphicsCommandList *pd3dCommandList, CCamera *pCamera);
	virtual void CreateShaderVariables(ID3D12Device *pd3dDevice, ID3D12GraphicsCommandList *pd3dCommandList);
	virtual void UpdateShaderVariables(ID3D12GraphicsCommandList *pd3dCommandList);
	virtual void ReleaseShaderVariables();
	virtual D3D12_BLEND_DESC CreateBlendState();
	virtual D3D12_DEPTH_STENCIL_DESC CreateDepthStencilState();
	virtual D3D12_SHADER_BYTECODE CreateVertexShader(ID3DBlob **ppd3dShaderBlob);
	virtual D3D12_SHADER_BYTECODE CreatePixelShader(ID3DBlob **ppd3dShaderBlob);
};

class CFlowerShader : public CTexturedShader
{
protected:
	CBillboard * *m_ppTree = 0;
	int								m_nTree = 0;

#ifdef _WITH_BATCH_MATERIAL
	CMaterial						*m_pMaterial = NULL;
#endif

	ID3D12Resource					*m_pd3dcbGameObjects = NULL;
	CB_GAMEOBJECT_INFO				*m_pcbMappedGameObjects = NULL;

public:
	CFlowerShader();
	virtual ~CFlowerShader();

	virtual void BuildObjects(ID3D12Device *pd3dDevice, ID3D12GraphicsCommandList *pd3dCommandList, void *pContext);
	virtual void ReleaseObjects();
	virtual void AnimateObjects(float fTimeElapsed, CCamera *pCamera);
	virtual void ReleaseUploadBuffers();
	virtual void Render(ID3D12GraphicsCommandList *pd3dCommandList, CCamera *pCamera);
	virtual void CreateShaderVariables(ID3D12Device *pd3dDevice, ID3D12GraphicsCommandList *pd3dCommandList);
	virtual void UpdateShaderVariables(ID3D12GraphicsCommandList *pd3dCommandList);
	virtual void ReleaseShaderVariables();
	virtual D3D12_BLEND_DESC CreateBlendState();
	virtual D3D12_DEPTH_STENCIL_DESC CreateDepthStencilState();
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class CBulletShader : public CTexturedShader
{
protected:
	CBillboard * *m_ppBullet = 0;
	int								m_nBullet = 0;
	int								BulletCount = 0;
	bool							death = 1;

#ifdef _WITH_BATCH_MATERIAL
	CMaterial						*m_pMaterial = NULL;
#endif

	ID3D12Resource					*m_pd3dcbGameObjects = NULL;
	CB_GAMEOBJECT_INFO				*m_pcbMappedGameObjects = NULL;

public:
	CBulletShader();
	virtual ~CBulletShader();

	virtual void BuildObjects(ID3D12Device *pd3dDevice, ID3D12GraphicsCommandList *pd3dCommandList, void *pContext);
	virtual void ReleaseObjects();
	virtual void AnimateObjects(float fTimeElapsed, CCamera *pCamera);
	virtual void ReleaseUploadBuffers();
	virtual void Render(ID3D12GraphicsCommandList *pd3dCommandList, CCamera *pCamera);
	virtual void CreateShaderVariables(ID3D12Device *pd3dDevice, ID3D12GraphicsCommandList *pd3dCommandList);
	virtual void UpdateShaderVariables(ID3D12GraphicsCommandList *pd3dCommandList);
	virtual void ReleaseShaderVariables();
	virtual void SetPosition(int id, XMFLOAT3 input);
	virtual D3D12_BLEND_DESC CreateBlendState();
};

class CParticleShader : public CTexturedShader
{
protected:
	int								m_nParticle = 0;
	int								BulletCount = 0;
	bool							death = 1;

	XMFLOAT3						changePosition;

#ifdef _WITH_BATCH_MATERIAL
	CMaterial						*m_pMaterial = NULL;
	CBillboard						**m_ppParticle = 0;

#endif

	ID3D12Resource					*m_pd3dcbGameObjects = NULL;
	CB_GAMEOBJECT_INFO				*m_pcbMappedGameObjects = NULL;

public:
	CParticleShader();
	virtual ~CParticleShader();

	virtual void BuildObjects(ID3D12Device *pd3dDevice, ID3D12GraphicsCommandList *pd3dCommandList, void *pContext);
	virtual void ReleaseObjects();
	virtual void AnimateObjects(float fTimeElapsed, CCamera *pCamera);
	virtual void ReleaseUploadBuffers();
	virtual void Render(ID3D12GraphicsCommandList *pd3dCommandList, CCamera *pCamera);
	virtual void CreateShaderVariables(ID3D12Device *pd3dDevice, ID3D12GraphicsCommandList *pd3dCommandList);
	virtual void UpdateShaderVariables(ID3D12GraphicsCommandList *pd3dCommandList);
	virtual void ReleaseShaderVariables();
	virtual D3D12_BLEND_DESC CreateBlendState();
	virtual void SetPosition(int id, XMFLOAT3 input);

};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class CMiniMapShader : public CTexturedShader
{
protected:
	CRotatingObject					**m_ppTree = 0;
	int								m_nTree = 0;
	int								setRedDot = 0;

#ifdef _WITH_BATCH_MATERIAL
	CMaterial						*m_pMaterial = NULL;
#endif

	ID3D12Resource					*m_pd3dcbGameObjects = NULL;
	CB_GAMEOBJECT_INFO				*m_pcbMappedGameObjects = NULL;

public:
	CMiniMapShader();
	virtual ~CMiniMapShader();

	void		 SetRedDot(int input) { setRedDot = input; }
	virtual void BuildObjects(ID3D12Device *pd3dDevice, ID3D12GraphicsCommandList *pd3dCommandList, void *pContext);
	virtual void ReleaseObjects();
	virtual void AnimateObjects(float fTimeElapsed, CCamera *pCamera);
	virtual void ReleaseUploadBuffers();
	virtual void Render(ID3D12GraphicsCommandList *pd3dCommandList, CCamera *pCamera);
	virtual void CreateShaderVariables(ID3D12Device *pd3dDevice, ID3D12GraphicsCommandList *pd3dCommandList);
	virtual void UpdateShaderVariables(ID3D12GraphicsCommandList *pd3dCommandList);
	virtual void ReleaseShaderVariables();
	virtual D3D12_BLEND_DESC CreateBlendState();
	virtual D3D12_DEPTH_STENCIL_DESC CreateDepthStencilState();
	virtual D3D12_SHADER_BYTECODE CreateVertexShader(ID3DBlob **ppd3dShaderBlob);
	virtual D3D12_SHADER_BYTECODE CreatePixelShader(ID3DBlob **ppd3dShaderBlob);
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class CHpBarShader : public CTexturedShader
{
protected:
	CRotatingObject					**m_ppTree = 0;
	int								m_nTree = 0;
	int								setRedDot = 0;
	float							testHp = 100;

#ifdef _WITH_BATCH_MATERIAL
	CMaterial						*m_pMaterial = NULL;
#endif

	ID3D12Resource					*m_pd3dcbGameObjects = NULL;
	CB_GAMEOBJECT_INFO				*m_pcbMappedGameObjects = NULL;

public:
	CHpBarShader();
	virtual ~CHpBarShader();

	void		 SetRedDot(int input) { setRedDot = input; }
	virtual void BuildObjects(ID3D12Device *pd3dDevice, ID3D12GraphicsCommandList *pd3dCommandList, void *pContext);
	virtual void ReleaseObjects();
	virtual void AnimateObjects(float fTimeElapsed, CCamera *pCamera);
	virtual void ReleaseUploadBuffers();
	virtual void Render(ID3D12GraphicsCommandList *pd3dCommandList, CCamera *pCamera, float hp);
	virtual void CreateShaderVariables(ID3D12Device *pd3dDevice, ID3D12GraphicsCommandList *pd3dCommandList);
	virtual void UpdateShaderVariables(ID3D12GraphicsCommandList *pd3dCommandList);
	virtual void ReleaseShaderVariables();
	virtual D3D12_BLEND_DESC CreateBlendState();
	virtual D3D12_DEPTH_STENCIL_DESC CreateDepthStencilState();
	virtual D3D12_SHADER_BYTECODE CreateVertexShader(ID3DBlob **ppd3dShaderBlob);
	virtual D3D12_SHADER_BYTECODE CreatePixelShader(ID3DBlob **ppd3dShaderBlob);
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class CItemUIShader : public CTexturedShader
{
protected:
	CRotatingObject * *m_ppTree = 0;
	int								m_nTree = 1;

#ifdef _WITH_BATCH_MATERIAL
	CMaterial						*m_pMaterial = NULL;
#endif

	ID3D12Resource					*m_pd3dcbGameObjects = NULL;
	CB_GAMEOBJECT_INFO				*m_pcbMappedGameObjects = NULL;

public:
	CItemUIShader();
	virtual ~CItemUIShader();

	virtual void BuildObjects(ID3D12Device *pd3dDevice, ID3D12GraphicsCommandList *pd3dCommandList, void *pContext);
	virtual void ReleaseObjects();
	virtual void AnimateObjects(float fTimeElapsed, CCamera *pCamera);
	virtual void ReleaseUploadBuffers();
	virtual void Render(ID3D12GraphicsCommandList *pd3dCommandList, CCamera *pCamera);
	virtual void CreateShaderVariables(ID3D12Device *pd3dDevice, ID3D12GraphicsCommandList *pd3dCommandList);
	virtual void UpdateShaderVariables(ID3D12GraphicsCommandList *pd3dCommandList);
	virtual void ReleaseShaderVariables();
	virtual D3D12_BLEND_DESC CreateBlendState();
	virtual D3D12_DEPTH_STENCIL_DESC CreateDepthStencilState();
	virtual D3D12_SHADER_BYTECODE CreateVertexShader(ID3DBlob **ppd3dShaderBlob);
	virtual D3D12_SHADER_BYTECODE CreatePixelShader(ID3DBlob **ppd3dShaderBlob);
};

class CItemUIShader_1 : public CTexturedShader
{
protected:
	CRotatingObject * *m_ppTree = 0;
	int								m_nTree = 1;

#ifdef _WITH_BATCH_MATERIAL
	CMaterial						*m_pMaterial = NULL;
#endif

	ID3D12Resource					*m_pd3dcbGameObjects = NULL;
	CB_GAMEOBJECT_INFO				*m_pcbMappedGameObjects = NULL;

public:
	CItemUIShader_1();
	virtual ~CItemUIShader_1();

	virtual void BuildObjects(ID3D12Device *pd3dDevice, ID3D12GraphicsCommandList *pd3dCommandList, void *pContext);
	virtual void ReleaseObjects();
	virtual void AnimateObjects(float fTimeElapsed, CCamera *pCamera);
	virtual void ReleaseUploadBuffers();
	virtual void Render(ID3D12GraphicsCommandList *pd3dCommandList, CCamera *pCamera);
	virtual void CreateShaderVariables(ID3D12Device *pd3dDevice, ID3D12GraphicsCommandList *pd3dCommandList);
	virtual void UpdateShaderVariables(ID3D12GraphicsCommandList *pd3dCommandList);
	virtual void ReleaseShaderVariables();
	virtual D3D12_BLEND_DESC CreateBlendState();
	virtual D3D12_DEPTH_STENCIL_DESC CreateDepthStencilState();
	virtual D3D12_SHADER_BYTECODE CreateVertexShader(ID3DBlob **ppd3dShaderBlob);
	virtual D3D12_SHADER_BYTECODE CreatePixelShader(ID3DBlob **ppd3dShaderBlob);
};

class CItemUIShader_2 : public CTexturedShader
{
protected:
	CRotatingObject * *m_ppTree = 0;
	int								m_nTree = 1;

#ifdef _WITH_BATCH_MATERIAL
	CMaterial						*m_pMaterial = NULL;
#endif

	ID3D12Resource					*m_pd3dcbGameObjects = NULL;
	CB_GAMEOBJECT_INFO				*m_pcbMappedGameObjects = NULL;

public:
	CItemUIShader_2();
	virtual ~CItemUIShader_2();

	virtual void BuildObjects(ID3D12Device *pd3dDevice, ID3D12GraphicsCommandList *pd3dCommandList, void *pContext);
	virtual void ReleaseObjects();
	virtual void AnimateObjects(float fTimeElapsed, CCamera *pCamera);
	virtual void ReleaseUploadBuffers();
	virtual void Render(ID3D12GraphicsCommandList *pd3dCommandList, CCamera *pCamera);
	virtual void CreateShaderVariables(ID3D12Device *pd3dDevice, ID3D12GraphicsCommandList *pd3dCommandList);
	virtual void UpdateShaderVariables(ID3D12GraphicsCommandList *pd3dCommandList);
	virtual void ReleaseShaderVariables();
	virtual D3D12_BLEND_DESC CreateBlendState();
	virtual D3D12_DEPTH_STENCIL_DESC CreateDepthStencilState();
	virtual D3D12_SHADER_BYTECODE CreateVertexShader(ID3DBlob **ppd3dShaderBlob);
	virtual D3D12_SHADER_BYTECODE CreatePixelShader(ID3DBlob **ppd3dShaderBlob);
};

class CItemUIShader_3 : public CTexturedShader
{
protected:
	CRotatingObject * *m_ppTree = 0;
	int								m_nTree = 1;

#ifdef _WITH_BATCH_MATERIAL
	CMaterial						*m_pMaterial = NULL;
#endif

	ID3D12Resource					*m_pd3dcbGameObjects = NULL;
	CB_GAMEOBJECT_INFO				*m_pcbMappedGameObjects = NULL;

public:
	CItemUIShader_3();
	virtual ~CItemUIShader_3();

	virtual void BuildObjects(ID3D12Device *pd3dDevice, ID3D12GraphicsCommandList *pd3dCommandList, void *pContext);
	virtual void ReleaseObjects();
	virtual void AnimateObjects(float fTimeElapsed, CCamera *pCamera);
	virtual void ReleaseUploadBuffers();
	virtual void Render(ID3D12GraphicsCommandList *pd3dCommandList, CCamera *pCamera);
	virtual void CreateShaderVariables(ID3D12Device *pd3dDevice, ID3D12GraphicsCommandList *pd3dCommandList);
	virtual void UpdateShaderVariables(ID3D12GraphicsCommandList *pd3dCommandList);
	virtual void ReleaseShaderVariables();
	virtual D3D12_BLEND_DESC CreateBlendState();
	virtual D3D12_DEPTH_STENCIL_DESC CreateDepthStencilState();
	virtual D3D12_SHADER_BYTECODE CreateVertexShader(ID3DBlob **ppd3dShaderBlob);
	virtual D3D12_SHADER_BYTECODE CreatePixelShader(ID3DBlob **ppd3dShaderBlob);
};

class CItemUIShader_4 : public CTexturedShader
{
protected:
	CRotatingObject * *m_ppTree = 0;
	int								m_nTree = 1;

#ifdef _WITH_BATCH_MATERIAL
	CMaterial						*m_pMaterial = NULL;
#endif

	ID3D12Resource					*m_pd3dcbGameObjects = NULL;
	CB_GAMEOBJECT_INFO				*m_pcbMappedGameObjects = NULL;

public:
	CItemUIShader_4();
	virtual ~CItemUIShader_4();

	virtual void BuildObjects(ID3D12Device *pd3dDevice, ID3D12GraphicsCommandList *pd3dCommandList, void *pContext);
	virtual void ReleaseObjects();
	virtual void AnimateObjects(float fTimeElapsed, CCamera *pCamera);
	virtual void ReleaseUploadBuffers();
	virtual void Render(ID3D12GraphicsCommandList *pd3dCommandList, CCamera *pCamera);
	virtual void CreateShaderVariables(ID3D12Device *pd3dDevice, ID3D12GraphicsCommandList *pd3dCommandList);
	virtual void UpdateShaderVariables(ID3D12GraphicsCommandList *pd3dCommandList);
	virtual void ReleaseShaderVariables();
	virtual D3D12_BLEND_DESC CreateBlendState();
	virtual D3D12_DEPTH_STENCIL_DESC CreateDepthStencilState();
	virtual D3D12_SHADER_BYTECODE CreateVertexShader(ID3DBlob **ppd3dShaderBlob);
	virtual D3D12_SHADER_BYTECODE CreatePixelShader(ID3DBlob **ppd3dShaderBlob);
};

class CItemEatUIShader : public CTexturedShader
{
protected:
	CRotatingObject * *m_ppTree = 0;
	int								m_nTree = 1;

#ifdef _WITH_BATCH_MATERIAL
	CMaterial						*m_pMaterial = NULL;
#endif

	ID3D12Resource					*m_pd3dcbGameObjects = NULL;
	CB_GAMEOBJECT_INFO				*m_pcbMappedGameObjects = NULL;

public:
	CItemEatUIShader();
	virtual ~CItemEatUIShader();

	virtual void BuildObjects(ID3D12Device *pd3dDevice, ID3D12GraphicsCommandList *pd3dCommandList, void *pContext);
	virtual void ReleaseObjects();
	virtual void AnimateObjects(float fTimeElapsed, CCamera *pCamera);
	virtual void ReleaseUploadBuffers();
	virtual void Render(ID3D12GraphicsCommandList *pd3dCommandList, CCamera *pCamera);
	virtual void CreateShaderVariables(ID3D12Device *pd3dDevice, ID3D12GraphicsCommandList *pd3dCommandList);
	virtual void UpdateShaderVariables(ID3D12GraphicsCommandList *pd3dCommandList);
	virtual void ReleaseShaderVariables();
	virtual D3D12_BLEND_DESC CreateBlendState();
	virtual D3D12_DEPTH_STENCIL_DESC CreateDepthStencilState();
	virtual D3D12_SHADER_BYTECODE CreateVertexShader(ID3DBlob **ppd3dShaderBlob);
	virtual D3D12_SHADER_BYTECODE CreatePixelShader(ID3DBlob **ppd3dShaderBlob);
};