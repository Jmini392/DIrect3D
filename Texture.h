#pragma once

#include "PCH.h"

class CShader;

// 텍스처 타입 열거형 (alb, ao, nrm, rgn, wlm_r, wlm_g, wlm_b)
enum TextureType {
	TEXTURE_TYPE_ALBEDO = 0,     // alb - Albedo/Diffuse 색상
	TEXTURE_TYPE_AO,             // ao - Ambient Occlusion
	TEXTURE_TYPE_MTL,            // mtl - Metallic 맵
	TEXTURE_TYPE_NORMAL,         // nrm - Normal 맵
	TEXTURE_TYPE_RGN,            // rgn - Region 맵
	TEXTURE_TYPE_WLM_R,          // wlm_r - Wrinkle Map Red
	TEXTURE_TYPE_WLM_G,          // wlm_g - Wrinkle Map Green
	TEXTURE_TYPE_WLM_B,          // wlm_b - Wrinkle Map Blue
	TEXTURE_TYPE_COUNT
};

// 텍스처 타입 접미사 배열 (파일명 생성용)
static const wchar_t* g_TextureSuffixes[TEXTURE_TYPE_COUNT] = {
	L"_alb",      // TEXTURE_TYPE_ALBEDO
	L"_ao",       // TEXTURE_TYPE_AO
	L"_mtl",      // TEXTURE_TYPE_MTL
	L"_nrm",      // TEXTURE_TYPE_NORMAL
	L"_rgn",      // TEXTURE_TYPE_RGN
	L"_wlm_r",    // TEXTURE_TYPE_WLM_R
	L"_wlm_g",    // TEXTURE_TYPE_WLM_G
	L"_wlm_b"     // TEXTURE_TYPE_WLM_B
};

// 텍스처 로딩 함수 선언
ID3D12Resource* CreateTextureResourceFromFile(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList,	const wchar_t* pszFileName,
	ID3D12Resource** ppd3dUploadBuffer, D3D12_RESOURCE_STATES d3dResourceStates = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

class CTexture {
private:
	int m_nReferences = 0;
	
	UINT m_nTextures = 0;
	
	ID3D12Resource** m_ppd3dTextures = nullptr;
	ID3D12Resource** m_ppd3dTextureUploadBuffers = nullptr;
	
	D3D12_GPU_DESCRIPTOR_HANDLE* m_pd3dSrvGpuDescriptorHandles = nullptr;
public:
	CTexture(int nTextures = 1);
	virtual ~CTexture();
	
	void AddRef() { m_nReferences++; }
	void Release() { if (--m_nReferences <= 0) delete this; }

	// 단일 텍스처 로드
	void LoadTextureFromFile(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, const wchar_t* pszFileName, UINT nIndex);

	void ReleaseUploadBuffers();

	ID3D12Resource* GetTexture(UINT nIndex) { return (nIndex < m_nTextures) ? m_ppd3dTextures[nIndex] : nullptr; }
	UINT GetTextureCount() { return m_nTextures; }

	D3D12_GPU_DESCRIPTOR_HANDLE GetGpuDescriptorHandle(UINT nIndex) { return m_pd3dSrvGpuDescriptorHandles[nIndex]; }
	void SetGpuDescriptorHandle(UINT nIndex, D3D12_GPU_DESCRIPTOR_HANDLE d3dSrvGpuDescriptorHandle);
};

// 재질 클래스 (alb, ao, mlt, nrm, rgn, wlm_r, wlm_g, wlm_b 텍스처 세트)
class CMaterial {
public:
	CMaterial();
	virtual ~CMaterial();

	void AddRef() { m_nReferences++; }
	void Release() { if (--m_nReferences <= 0) delete this; }

	void SetTexture(CTexture* pTexture, TextureType type);
	CTexture* GetTexture(TextureType type) { return m_ppTextures[type]; }

	// 텍스처 세트를 한번에 로드 (부위 이름 기반)
	// 예: LoadTextureSet(..., L"texture/head", L".jpg")
	// -> texture/head_alb.jpg, texture/head_ao.jpg, texture/head_nrm.jpg, ...
	void LoadTextureSet(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, const wchar_t* pszBasePath, const wchar_t* pszExtension = L".jpg");

	void PrepareRender(ID3D12GraphicsCommandList* pd3dCommandList);
	void ReleaseUploadBuffers();
private:
	int m_nReferences = 0;

	CTexture* m_ppTextures[TEXTURE_TYPE_COUNT] = { nullptr };
};