#pragma once

#include "PCH.h"

//정점을 표현하기 위한 클래스를 선언한다.
class CVertex {
protected:
	//정점의 위치 벡터이다(모든 정점은 최소한 위치 벡터를 가져야 한다).
	XMFLOAT3 m_xmf3Position;
public:
	CVertex() { m_xmf3Position = XMFLOAT3(0.0f, 0.0f, 0.0f); }
	CVertex(XMFLOAT3 xmf3Position) { m_xmf3Position = xmf3Position; }
	~CVertex() {}
};

class CDiffusedVertex : public CVertex {
protected:
	//정점의 색상이다.
	XMFLOAT4 m_xmf4Diffuse;
public:
	CDiffusedVertex() {	m_xmf3Position = XMFLOAT3(0.0f, 0.0f, 0.0f); m_xmf4Diffuse = XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f); }
	CDiffusedVertex(float x, float y, float z, XMFLOAT4 xmf4Diffuse) { m_xmf3Position = XMFLOAT3(x, y, z); m_xmf4Diffuse = xmf4Diffuse; }
	CDiffusedVertex(XMFLOAT3 xmf3Position, XMFLOAT4 xmf4Diffuse) { m_xmf3Position = xmf3Position; m_xmf4Diffuse = xmf4Diffuse; }
	~CDiffusedVertex() {}
};

class CTexturedVertex : public CVertex {
protected:
	XMFLOAT2 m_xmf2TexCoord;
public:
	CTexturedVertex() { 
		m_xmf3Position = XMFLOAT3(0.0f, 0.0f, 0.0f); 
		m_xmf2TexCoord = XMFLOAT2(0.0f, 0.0f); 
	}
	CTexturedVertex(float x, float y, float z, XMFLOAT2 xmf2TexCoord) { 
		m_xmf3Position = XMFLOAT3(x, y, z); 
		m_xmf2TexCoord = xmf2TexCoord; 
	}
	CTexturedVertex(XMFLOAT3 xmf3Position, XMFLOAT2 xmf2TexCoord) { 
		m_xmf3Position = xmf3Position; 
		m_xmf2TexCoord = xmf2TexCoord; 
	}
	~CTexturedVertex() {}
};

class CTexturedNormalVertex : public CVertex {
protected:
	XMFLOAT3 m_xmf3Normal;
	XMFLOAT2 m_xmf2TexCoord;
public:
	CTexturedNormalVertex() { 
		m_xmf3Position = XMFLOAT3(0.0f, 0.0f, 0.0f);
		m_xmf3Normal = XMFLOAT3(0.0f, 1.0f, 0.0f);
		m_xmf2TexCoord = XMFLOAT2(0.0f, 0.0f); 
	}
	CTexturedNormalVertex(XMFLOAT3 xmf3Position, XMFLOAT3 xmf3Normal, XMFLOAT2 xmf2TexCoord) { 
		m_xmf3Position = xmf3Position;
		m_xmf3Normal = xmf3Normal;
		m_xmf2TexCoord = xmf2TexCoord; 
	}
	~CTexturedNormalVertex() {}
};

// 서브메쉬 정보 구조체
struct SubMeshInfo {
	std::string strName;      // 메쉬 이름 (body, hair 등)
	UINT nStartIndex;         // 인덱스 버퍼 시작 위치
	UINT nIndexCount;         // 인덱스 개수
	int nMaterialIndex;       // 재질 인덱스 (-1이면 없음)
};

class CMesh {
public:
	CMesh(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList);
	virtual ~CMesh();
private:
	int m_nReferences = 0;
public:
	void AddRef() { m_nReferences++; }
	void Release() { if (--m_nReferences <= 0) delete this; }
	void ReleaseUploadBuffers();
protected:
	ID3D12Resource* m_pd3dVertexBuffer = NULL;
	ID3D12Resource* m_pd3dVertexUploadBuffer = NULL;
	D3D12_VERTEX_BUFFER_VIEW m_d3dVertexBufferView;
	D3D12_PRIMITIVE_TOPOLOGY m_d3dPrimitiveTopology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	UINT m_nSlot = 0;
	UINT m_nVertices = 0;
	UINT m_nStride = 0;
	UINT m_nOffset = 0;
public:
	virtual void Render(ID3D12GraphicsCommandList* pd3dCommandList);
	virtual void Render(ID3D12GraphicsCommandList* pd3dCommandList, int nSubMeshIndex);
	
	// 서브메쉬 정보
	int GetSubMeshCount() { return (int)m_vSubMeshes.size(); }
	SubMeshInfo* GetSubMesh(int nIndex) { return (nIndex < (int)m_vSubMeshes.size()) ? &m_vSubMeshes[nIndex] : nullptr; }
	
protected:
	ID3D12Resource* m_pd3dIndexBuffer = NULL;
	ID3D12Resource* m_pd3dIndexUploadBuffer = NULL;
	/*인덱스 버퍼(인덱스의 배열)와 인덱스 버퍼를 위한 업로드 버퍼에 대한 인터페이스 포인터이다. 인덱스 버퍼는 정점
	버퍼(배열)에 대한 인덱스를 가진다.*/
	D3D12_INDEX_BUFFER_VIEW m_d3dIndexBufferView;
	UINT m_nIndices = 0;
	//인덱스 버퍼에 포함되는 인덱스의 개수이다. 
	UINT m_nStartIndex = 0;
	//인덱스 버퍼에서 메쉬를 그리기 위해 사용되는 시작 인덱스이다. 
	int m_nBaseVertex = 0;
	
	// 서브메쉬 목록
	std::vector<SubMeshInfo> m_vSubMeshes;
};

class CTriangleMesh : public CMesh {
public:
	CTriangleMesh(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList);
	virtual ~CTriangleMesh() {}
};

class CCubeMeshDiffused : public CMesh {
public:
	//직육면체의 가로, 세로, 깊이의 길이를 지정하여 직육면체 메쉬를 생성한다.
	CCubeMeshDiffused(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, float fWidth = 2.0f, float fHeight = 2.0f, float fDepth = 2.0f);
	virtual ~CCubeMeshDiffused();
};

class CAirplaneMeshDiffused : public CMesh {
public:
	CAirplaneMeshDiffused(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList,
		float fWidth = 20.0f, float fHeight = 20.0f, float fDepth = 4.0f, XMFLOAT4 xmf4Color = XMFLOAT4(1.0f, 1.0f, 0.0f, 0.0f));
	virtual ~CAirplaneMeshDiffused();
};

class CFBXMeshDiffused : public CMesh {
public:
	CFBXMeshDiffused(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, const char* pstrFileName,
		float fScale = 1.0f, XMFLOAT4 xmf4Color = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f));
	virtual ~CFBXMeshDiffused();
private:
	void ProcessNode(FbxNode* pNode, std::vector<CDiffusedVertex>& vertices, std::vector<UINT>& indices, 
		float fScale, XMFLOAT4& xmf4Color);
	void ProcessMesh(FbxMesh* pMesh, std::vector<CDiffusedVertex>& vertices, std::vector<UINT>& indices, 
		float fScale, XMFLOAT4& xmf4Color);
private:
	ID3D12Device* m_pd3dDevice = nullptr;
	ID3D12GraphicsCommandList* m_pd3dCommandList = nullptr;
	float m_fScale = 1.0f;
};

class CFBXMeshTextured : public CMesh {
public:
	CFBXMeshTextured(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, const char* pstrFileName,
		float fScale = 1.0f);
	virtual ~CFBXMeshTextured();
private:
	void ProcessNode(FbxNode* pNode, std::vector<CTexturedNormalVertex>& vertices, std::vector<UINT>& indices, float fScale);
	void ProcessMesh(FbxNode* pNode, FbxMesh* pMesh, std::vector<CTexturedNormalVertex>& vertices, std::vector<UINT>& indices, float fScale);
	XMFLOAT2 GetUV(FbxMesh* pMesh, int nControlPointIndex, int nVertexCounter, int nUVIndex);
	XMFLOAT3 GetNormal(FbxMesh* pMesh, int nControlPointIndex, int nVertexCounter);
private:
	ID3D12Device* m_pd3dDevice = nullptr;
	ID3D12GraphicsCommandList* m_pd3dCommandList = nullptr;
	float m_fScale = 1.0f;
};