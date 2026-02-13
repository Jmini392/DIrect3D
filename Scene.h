#pragma once

#include "PCH.h"
#include "Timer.h"
#include "Shader.h"
#include "Player.h"
#include "Texture.h"

// 씬 내 렌더링 가능한 오브젝트 정보
struct SRenderObject {
	CGameObject* pObject = nullptr;
	CShader* pShader = nullptr;           // 이 오브젝트가 사용할 셰이더
	CMaterial** ppMaterials = nullptr;     // 재질 배열 (서브메쉬별)
	std::string* pMaterialNames = nullptr; // 재질 이름 (서브메쉬 매칭용)
	int nMaterials = 0;
};

class CScene {
public:
	CScene();
	virtual ~CScene();

	//씬에서 마우스와 키보드 메시지를 처리한다.
	bool OnProcessingMouseMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM	lParam) { return false; };
	bool OnProcessingKeyboardMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam) { return false; };

	virtual void BuildObjects(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList);
	virtual void ReleaseObjects();
	bool ProcessInput(UCHAR* pKeysBuffer);
	virtual void AnimateObjects(float fTimeElapsed);
	virtual void Render(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera);
	virtual void ReleaseUploadBuffers();
	
	//그래픽 루트 시그너쳐를 생성한다.
	ID3D12RootSignature* CreateGraphicsRootSignature(ID3D12Device* pd3dDevice);
	ID3D12RootSignature* GetGraphicsRootSignature() { return(m_pd3dGraphicsRootSignature); };

	// 플레이어/카메라 접근자 (외부에서 참조용)
	CPlayer* GetPlayer() { return m_pPlayer; }
	CCamera* GetCamera() { return m_pCamera; }
	void SetCamera(CCamera* pCamera) { m_pCamera = pCamera; }

protected:
	// 셰이더 관리 (파이프라인 상태만 담당)
	void AddShader(CShader* pShader);
	
	// 메쉬 관리
	void AddMesh(CMesh* pMesh);
	
	// 재질 관리
	void AddMaterial(CMaterial* pMaterial);
	
	// 오브젝트 추가 (셰이더/재질 연결)
	void AddRenderObject(CGameObject* pObject, CShader* pShader,
		CMaterial** ppMaterials = nullptr, std::string* pMaterialNames = nullptr, int nMaterials = 0);
	
	// 서브메쉬 이름으로 재질 인덱스 찾기
	int FindMaterialIndex(SRenderObject& renderObj, const std::string& strMeshName);

protected:
	ID3D12RootSignature* m_pd3dGraphicsRootSignature = NULL;

	// 셰이더 (파이프라인 상태 + SRV 힙)
	std::vector<CShader*> m_vShaders;
	
	// 메쉬 리소스 (소유권)
	std::vector<CMesh*> m_vMeshes;
	
	// 재질 리소스 (소유권)
	std::vector<CMaterial*> m_vMaterials;
	
	// 렌더링 오브젝트 (오브젝트 + 셰이더/재질 바인딩)
	std::vector<SRenderObject> m_vRenderObjects;

	// 플레이어와 카메라 (씬이 소유)
	CPlayer* m_pPlayer = nullptr;
	CCamera* m_pCamera = nullptr;
};