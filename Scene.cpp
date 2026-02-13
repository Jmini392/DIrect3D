#include "Scene.h"

CScene::CScene() {}

CScene::~CScene() {
	ReleaseObjects();
}

ID3D12RootSignature* CScene::CreateGraphicsRootSignature(ID3D12Device* pd3dDevice) {
	ID3D12RootSignature* pd3dGraphicsRootSignature = NULL;

	// 텍스처를 위한 Descriptor Range 설정
	D3D12_DESCRIPTOR_RANGE pd3dDescriptorRanges[1];
	pd3dDescriptorRanges[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	pd3dDescriptorRanges[0].NumDescriptors = 8; // alb, ao, mtl, nrm, rgn, wlm_r, wlm_g, wlm_b
	pd3dDescriptorRanges[0].BaseShaderRegister = 0; // t0 ~ t6
	pd3dDescriptorRanges[0].RegisterSpace = 0;
	pd3dDescriptorRanges[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	// 루트 파라미터 설정 (3개: World, Camera, Textures)
	D3D12_ROOT_PARAMETER pd3dRootParameters[3];

	// [0] World 행렬 (32비트 상수 16개)
	pd3dRootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
	pd3dRootParameters[0].Constants.Num32BitValues = 16;
	pd3dRootParameters[0].Constants.ShaderRegister = 0; // b0
	pd3dRootParameters[0].Constants.RegisterSpace = 0;
	pd3dRootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;

	// [1] Camera 행렬 (32비트 상수 32개: View + Projection)
	pd3dRootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
	pd3dRootParameters[1].Constants.Num32BitValues = 32;
	pd3dRootParameters[1].Constants.ShaderRegister = 1; // b1
	pd3dRootParameters[1].Constants.RegisterSpace = 0;
	pd3dRootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;

	// [2] 텍스처 Descriptor Table
	pd3dRootParameters[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	pd3dRootParameters[2].DescriptorTable.NumDescriptorRanges = 1;
	pd3dRootParameters[2].DescriptorTable.pDescriptorRanges = &pd3dDescriptorRanges[0];
	pd3dRootParameters[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	// 정적 샘플러 설정
	D3D12_STATIC_SAMPLER_DESC d3dSamplerDesc;
	::ZeroMemory(&d3dSamplerDesc, sizeof(D3D12_STATIC_SAMPLER_DESC));
	d3dSamplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
	d3dSamplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	d3dSamplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	d3dSamplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	d3dSamplerDesc.MipLODBias = 0;
	d3dSamplerDesc.MaxAnisotropy = 1;
	d3dSamplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
	d3dSamplerDesc.MinLOD = 0;
	d3dSamplerDesc.MaxLOD = D3D12_FLOAT32_MAX;
	d3dSamplerDesc.ShaderRegister = 0; // s0
	d3dSamplerDesc.RegisterSpace = 0;
	d3dSamplerDesc.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	D3D12_ROOT_SIGNATURE_FLAGS d3dRootSignatureFlags =
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;

	D3D12_ROOT_SIGNATURE_DESC d3dRootSignatureDesc;
	::ZeroMemory(&d3dRootSignatureDesc, sizeof(D3D12_ROOT_SIGNATURE_DESC));
	d3dRootSignatureDesc.NumParameters = _countof(pd3dRootParameters);
	d3dRootSignatureDesc.pParameters = pd3dRootParameters;
	d3dRootSignatureDesc.NumStaticSamplers = 1;
	d3dRootSignatureDesc.pStaticSamplers = &d3dSamplerDesc;
	d3dRootSignatureDesc.Flags = d3dRootSignatureFlags;

	ID3DBlob* pd3dSignatureBlob = NULL;
	ID3DBlob* pd3dErrorBlob = NULL;
	HRESULT hr = ::D3D12SerializeRootSignature(&d3dRootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &pd3dSignatureBlob, &pd3dErrorBlob);
	
	if (FAILED(hr) && pd3dErrorBlob) OutputDebugStringA((char*)pd3dErrorBlob->GetBufferPointer());

	pd3dDevice->CreateRootSignature(0, pd3dSignatureBlob->GetBufferPointer(), pd3dSignatureBlob->GetBufferSize(),
		__uuidof(ID3D12RootSignature), (void**)&pd3dGraphicsRootSignature);

	if (pd3dSignatureBlob) pd3dSignatureBlob->Release();
	if (pd3dErrorBlob) pd3dErrorBlob->Release();

	return(pd3dGraphicsRootSignature);
}

//=============================================================================
// 리소스 관리 헬퍼 함수들
//=============================================================================
void CScene::AddShader(CShader* pShader) {
	pShader->AddRef();
	m_vShaders.push_back(pShader);
}

void CScene::AddMesh(CMesh* pMesh) {
	pMesh->AddRef();
	m_vMeshes.push_back(pMesh);
}

void CScene::AddMaterial(CMaterial* pMaterial) {
	pMaterial->AddRef();
	m_vMaterials.push_back(pMaterial);
}

void CScene::AddRenderObject(CGameObject* pObject, CShader* pShader,
	CMaterial** ppMaterials, std::string* pMaterialNames, int nMaterials) {
	SRenderObject renderObj;
	renderObj.pObject = pObject;
	renderObj.pShader = pShader;
	if (pShader) pShader->AddRef();  // 참조 카운트 증가
	renderObj.ppMaterials = ppMaterials;
	renderObj.pMaterialNames = pMaterialNames;
	renderObj.nMaterials = nMaterials;
	m_vRenderObjects.push_back(renderObj);
}

int CScene::FindMaterialIndex(SRenderObject& renderObj, const std::string& strMeshName) {
	for (int i = 0; i < renderObj.nMaterials; i++) {
		if (strMeshName.find(renderObj.pMaterialNames[i]) != std::string::npos) {
			return i;
		}
	}
	return 0;
}

//=============================================================================
// 씬 빌드 - 여기서 오브젝트를 자유롭게 추가/구성
//=============================================================================
void CScene::BuildObjects(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList) {
	m_pd3dGraphicsRootSignature = CreateGraphicsRootSignature(pd3dDevice);

	//--- 1. 셰이더 생성 ---
	CTexturedShader* pTexturedShader = new CTexturedShader();
	pTexturedShader->CreateShader(pd3dDevice, m_pd3dGraphicsRootSignature);
	AddShader(pTexturedShader);

	//--- 2. 재질 생성 ---
	struct TexturePathInfo {
		const char* meshName;
		const wchar_t* texturePath;
	};
	TexturePathInfo texturePaths[] = {
		{ "body", L"Texture/a_body" },
		{ "hair", L"Texture/a_hair" },
		{ "obj",  L"Texture/a_obj" },
		{ "face", L"Texture/a_face" },
		{ "eye",  L"Texture/a_eye" }
	};
	const int nParts = _countof(texturePaths);

	// SRV 디스크립터 힙 생성 (텍스처용)
	pTexturedShader->CreateSrvDescriptorHeap(pd3dDevice, nParts * TEXTURE_TYPE_COUNT);

	CMaterial** ppMaterials = new CMaterial*[nParts];
	std::string* pMaterialNames = new std::string[nParts];

	for (int i = 0; i < nParts; i++) {
		pMaterialNames[i] = texturePaths[i].meshName;
		ppMaterials[i] = new CMaterial();
		ppMaterials[i]->LoadTextureSet(pd3dDevice, pd3dCommandList, texturePaths[i].texturePath, L".png");
		AddMaterial(ppMaterials[i]);

		for (int j = 0; j < TEXTURE_TYPE_COUNT; j++) {
			CTexture* pTexture = ppMaterials[i]->GetTexture((TextureType)j);
			if (pTexture && pTexture->GetTexture(0)) {
				pTexturedShader->CreateShaderResourceViews(pd3dDevice, pTexture, 2);
			}
		}
	}

	//--- 3. 메쉬 생성 ---
	CFBXMeshTextured* pMesh = new CFBXMeshTextured(pd3dDevice, pd3dCommandList, "Model/a.fbx", 1.0f);
	AddMesh(pMesh);

	//--- 4. 오브젝트 생성 및 등록 ---
	CRotatingObject* pObject = new CRotatingObject();
	pObject->SetMesh(pMesh);
	pObject->SetPosition(0.0f, 0.0f, 50.0f);
	pObject->SetRotationAxis(XMFLOAT3(0.0f, 1.0f, 0.0f));
	pObject->SetRotationSpeed(30.0f);

	AddRenderObject(pObject, pTexturedShader, ppMaterials, pMaterialNames, nParts);

	//--- 5. 플레이어 생성 ---
	CAirplanePlayer* pAirplanePlayer = new CAirplanePlayer(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature);
	m_pPlayer = pAirplanePlayer;
	m_pCamera = m_pPlayer->GetCamera();

	// 셰이더 로컬 포인터 해제
	pTexturedShader->Release();
}

void CScene::ReleaseObjects() {
	// 플레이어 해제 (카메라는 플레이어 소멸자에서 해제됨)
	if (m_pPlayer) {
		delete m_pPlayer;
		m_pPlayer = nullptr;
		m_pCamera = nullptr;
	}

	// 렌더 오브젝트 해제
	for (auto& renderObj : m_vRenderObjects) {
		if (renderObj.pObject) delete renderObj.pObject;
		if (renderObj.pShader) renderObj.pShader->Release();
		if (renderObj.ppMaterials) delete[] renderObj.ppMaterials;
		if (renderObj.pMaterialNames) delete[] renderObj.pMaterialNames;
	}
	m_vRenderObjects.clear();

	// 재질 해제
	for (auto pMaterial : m_vMaterials) {
		if (pMaterial) pMaterial->Release();
	}
	m_vMaterials.clear();

	// 메쉬 해제
	for (auto pMesh : m_vMeshes) {
		if (pMesh) pMesh->Release();
	}
	m_vMeshes.clear();

	// 셰이더 해제
	for (auto pShader : m_vShaders) {
		if (pShader) {
			pShader->ReleaseShaderVariables();
			pShader->Release();
		}
	}
	m_vShaders.clear();

	if (m_pd3dGraphicsRootSignature) {
		m_pd3dGraphicsRootSignature->Release();
		m_pd3dGraphicsRootSignature = NULL;
	}
}

void CScene::AnimateObjects(float fTimeElapsed) {
	for (auto& renderObj : m_vRenderObjects) {
		if (renderObj.pObject) renderObj.pObject->Animate(fTimeElapsed);
	}
}

void CScene::Render(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera) {
	// pCamera가 NULL이면 씬 자체 카메라 사용
	CCamera* pActiveCamera = pCamera ? pCamera : m_pCamera;
	if (!pActiveCamera) return;

	pActiveCamera->SetViewportsAndScissorRects(pd3dCommandList);
	pd3dCommandList->SetGraphicsRootSignature(m_pd3dGraphicsRootSignature);
	pActiveCamera->UpdateShaderVariables(pd3dCommandList);

	// 일반 오브젝트 렌더링
	for (auto& renderObj : m_vRenderObjects) {
		if (!renderObj.pObject) continue;

		renderObj.pShader->Render(pd3dCommandList, pActiveCamera);

		XMFLOAT4X4 xmf4x4World = renderObj.pObject->GetWorldMatrix();
		renderObj.pShader->UpdateShaderVariable(pd3dCommandList, &xmf4x4World);

		CMesh* pMesh = renderObj.pObject->GetMesh();
		if (!pMesh) continue;

		pd3dCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		int nSubMeshCount = pMesh->GetSubMeshCount();
		if (nSubMeshCount > 0) {
			// 서브메쉬별 재질 바인딩 후 렌더링
			for (int k = 0; k < nSubMeshCount; k++) {
				SubMeshInfo* pSubMesh = pMesh->GetSubMesh(k);
				if (pSubMesh && renderObj.ppMaterials) {
					int nMatIdx = FindMaterialIndex(renderObj, pSubMesh->strName);
					if (nMatIdx < renderObj.nMaterials && renderObj.ppMaterials[nMatIdx]) {
						CTexture* pAlbedo = renderObj.ppMaterials[nMatIdx]->GetTexture(TEXTURE_TYPE_ALBEDO);
						if (pAlbedo && pAlbedo->GetGpuDescriptorHandle(0).ptr != 0) {
							pd3dCommandList->SetGraphicsRootDescriptorTable(2, pAlbedo->GetGpuDescriptorHandle(0));
						}
					}
				}
				pMesh->Render(pd3dCommandList, k);
			}
		}
		else {
			// 서브메쉬 없으면 기본 렌더링
			CTexturedShader* pTexShader = dynamic_cast<CTexturedShader*>(renderObj.pShader);
			if (pTexShader) {
				pd3dCommandList->SetGraphicsRootDescriptorTable(2, pTexShader->GetSrvGPUDescriptorStartHandle());
			}
			pMesh->Render(pd3dCommandList);
		}
	}

	// 플레이어 렌더링 (3인칭 카메라일 때)
	if (m_pPlayer) m_pPlayer->Render(pd3dCommandList, pActiveCamera);
}

void CScene::ReleaseUploadBuffers() {
	for (auto& renderObj : m_vRenderObjects) {
		if (renderObj.pObject) renderObj.pObject->ReleaseUploadBuffers();
	}
	for (auto pMaterial : m_vMaterials) {
		if (pMaterial) pMaterial->ReleaseUploadBuffers();
	}
	if (m_pPlayer) m_pPlayer->ReleaseUploadBuffers();
}