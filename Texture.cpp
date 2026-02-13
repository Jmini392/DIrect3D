#include "Texture.h"
#include "Shader.h"

// WIC를 이용한 텍스처 로딩 함수 구현
ID3D12Resource* CreateTextureResourceFromFile(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, const wchar_t* pszFileName,
	ID3D12Resource** ppd3dUploadBuffer, D3D12_RESOURCE_STATES d3dResourceStates) {
	ID3D12Resource* pd3dTexture = nullptr;

	// 파일 존재 확인
	DWORD dwAttrib = GetFileAttributesW(pszFileName);
	if (dwAttrib == INVALID_FILE_ATTRIBUTES) {
		OutputDebugStringW(L"[Texture] File not found: ");
		OutputDebugStringW(pszFileName);
		OutputDebugStringW(L"\n");
		return nullptr;
	}

	// WIC Factory 생성
	IWICImagingFactory* pWICFactory = nullptr;
	HRESULT hr = CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pWICFactory));
	if (FAILED(hr)) {
		OutputDebugStringA("[Texture] Failed to create WIC Factory\n");
		return nullptr;
	}

	// 이미지 파일 디코더 생성
	IWICBitmapDecoder* pDecoder = nullptr;
	hr = pWICFactory->CreateDecoderFromFilename(pszFileName, nullptr, GENERIC_READ, WICDecodeMetadataCacheOnLoad, &pDecoder);
	if (FAILED(hr)) {
		OutputDebugStringW(L"[Texture] Failed to decode: ");
		OutputDebugStringW(pszFileName);
		OutputDebugStringW(L"\n");
		pWICFactory->Release();
		return nullptr;
	}

	// 첫 번째 프레임 가져오기
	IWICBitmapFrameDecode* pFrame = nullptr;
	hr = pDecoder->GetFrame(0, &pFrame);
	if (FAILED(hr)) {
		pDecoder->Release();
		pWICFactory->Release();
		return nullptr;
	}

	// 이미지 크기 가져오기
	UINT nWidth, nHeight;
	pFrame->GetSize(&nWidth, &nHeight);

	// 디버그 출력
	OutputDebugStringW(L"[Texture] Loaded: ");
	OutputDebugStringW(pszFileName);
	wchar_t szBuffer[64];
	swprintf_s(szBuffer, L" (%dx%d)\n", nWidth, nHeight);
	OutputDebugStringW(szBuffer);

	// RGBA 32비트 포맷으로 변환
	IWICFormatConverter* pConverter = nullptr;
	pWICFactory->CreateFormatConverter(&pConverter);
	pConverter->Initialize(pFrame, GUID_WICPixelFormat32bppRGBA, WICBitmapDitherTypeNone, nullptr, 0.0f, WICBitmapPaletteTypeCustom);

	// 픽셀 데이터 읽기
	UINT nRowPitch = nWidth * 4;
	UINT nImageSize = nRowPitch * nHeight;
	BYTE* pImageData = new BYTE[nImageSize];
	pConverter->CopyPixels(nullptr, nRowPitch, nImageSize, pImageData);

	// 텍스처 리소스 생성
	D3D12_RESOURCE_DESC d3dResourceDesc = {};
	d3dResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	d3dResourceDesc.Width = nWidth;
	d3dResourceDesc.Height = nHeight;
	d3dResourceDesc.DepthOrArraySize = 1;
	d3dResourceDesc.MipLevels = 1;
	d3dResourceDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	d3dResourceDesc.SampleDesc.Count = 1;
	d3dResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	d3dResourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

	D3D12_HEAP_PROPERTIES d3dHeapProperties = {};
	d3dHeapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;

	pd3dDevice->CreateCommittedResource(&d3dHeapProperties, D3D12_HEAP_FLAG_NONE, &d3dResourceDesc, 
		D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&pd3dTexture));

	// 업로드 버퍼 생성
	UINT64 nUploadBufferSize = 0;
	pd3dDevice->GetCopyableFootprints(&d3dResourceDesc, 0, 1, 0, nullptr, nullptr, nullptr, &nUploadBufferSize);

	D3D12_HEAP_PROPERTIES d3dUploadHeapProperties = {};
	d3dUploadHeapProperties.Type = D3D12_HEAP_TYPE_UPLOAD;

	D3D12_RESOURCE_DESC d3dUploadResourceDesc = {};
	d3dUploadResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	d3dUploadResourceDesc.Width = nUploadBufferSize;
	d3dUploadResourceDesc.Height = 1;
	d3dUploadResourceDesc.DepthOrArraySize = 1;
	d3dUploadResourceDesc.MipLevels = 1;
	d3dUploadResourceDesc.Format = DXGI_FORMAT_UNKNOWN;
	d3dUploadResourceDesc.SampleDesc.Count = 1;
	d3dUploadResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

	pd3dDevice->CreateCommittedResource(&d3dUploadHeapProperties, D3D12_HEAP_FLAG_NONE, &d3dUploadResourceDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(ppd3dUploadBuffer));

	// 업로드 버퍼에 데이터 복사
	D3D12_PLACED_SUBRESOURCE_FOOTPRINT d3dFootprint;
	pd3dDevice->GetCopyableFootprints(&d3dResourceDesc, 0, 1, 0, &d3dFootprint, nullptr, nullptr, nullptr);

	BYTE* pMappedData = nullptr;
	(*ppd3dUploadBuffer)->Map(0, nullptr, (void**)&pMappedData);
	for (UINT y = 0; y < nHeight; y++) {
		memcpy(pMappedData + y * d3dFootprint.Footprint.RowPitch, pImageData + y * nRowPitch, nRowPitch);
	}
	(*ppd3dUploadBuffer)->Unmap(0, nullptr);

	// 텍스처로 복사
	D3D12_TEXTURE_COPY_LOCATION d3dDstLocation = {};
	d3dDstLocation.pResource = pd3dTexture;
	d3dDstLocation.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
	d3dDstLocation.SubresourceIndex = 0;

	D3D12_TEXTURE_COPY_LOCATION d3dSrcLocation = {};
	d3dSrcLocation.pResource = *ppd3dUploadBuffer;
	d3dSrcLocation.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
	d3dSrcLocation.PlacedFootprint = d3dFootprint;

	pd3dCommandList->CopyTextureRegion(&d3dDstLocation, 0, 0, 0, &d3dSrcLocation, nullptr);

	// 리소스 상태 전환
	D3D12_RESOURCE_BARRIER d3dResourceBarrier = {};
	d3dResourceBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	d3dResourceBarrier.Transition.pResource = pd3dTexture;
	d3dResourceBarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
	d3dResourceBarrier.Transition.StateAfter = d3dResourceStates;
	d3dResourceBarrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	pd3dCommandList->ResourceBarrier(1, &d3dResourceBarrier);

	// 정리
	delete[] pImageData;
	pConverter->Release();
	pFrame->Release();
	pDecoder->Release();
	pWICFactory->Release();

	return pd3dTexture;
}

//=============================================================================
// CTexture
//=============================================================================
CTexture::CTexture(int nTextures) {
	m_nTextures = nTextures;
	m_ppd3dTextures = new ID3D12Resource*[m_nTextures];
	m_ppd3dTextureUploadBuffers = new ID3D12Resource*[m_nTextures];
	m_pd3dSrvGpuDescriptorHandles = new D3D12_GPU_DESCRIPTOR_HANDLE[m_nTextures];

	for (UINT i = 0; i < m_nTextures; i++) {
		m_ppd3dTextures[i] = nullptr;
		m_ppd3dTextureUploadBuffers[i] = nullptr;
		m_pd3dSrvGpuDescriptorHandles[i].ptr = 0;
	}
}

CTexture::~CTexture() {
	for (UINT i = 0; i < m_nTextures; i++) {
		if (m_ppd3dTextures[i]) m_ppd3dTextures[i]->Release();
		if (m_ppd3dTextureUploadBuffers[i]) m_ppd3dTextureUploadBuffers[i]->Release();
	}
	delete[] m_ppd3dTextures;
	delete[] m_ppd3dTextureUploadBuffers;
	delete[] m_pd3dSrvGpuDescriptorHandles;
}

void CTexture::LoadTextureFromFile(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, const wchar_t* pszFileName, UINT nIndex) {
	if (nIndex >= m_nTextures) return;
	
	m_ppd3dTextures[nIndex] = CreateTextureResourceFromFile(pd3dDevice, pd3dCommandList, pszFileName, &m_ppd3dTextureUploadBuffers[nIndex]);
}

void CTexture::ReleaseUploadBuffers() {
	for (UINT i = 0; i < m_nTextures; i++) {
		if (m_ppd3dTextureUploadBuffers[i]) {
			m_ppd3dTextureUploadBuffers[i]->Release();
			m_ppd3dTextureUploadBuffers[i] = nullptr;
		}
	}
}

void CTexture::SetGpuDescriptorHandle(UINT nIndex, D3D12_GPU_DESCRIPTOR_HANDLE d3dSrvGpuDescriptorHandle) {
	if (nIndex < m_nTextures) m_pd3dSrvGpuDescriptorHandles[nIndex] = d3dSrvGpuDescriptorHandle;
}

//=============================================================================
// CMaterial
//=============================================================================
CMaterial::CMaterial() {
	for (int i = 0; i < TEXTURE_TYPE_COUNT; i++) {
		m_ppTextures[i] = nullptr;
	}
}

CMaterial::~CMaterial() {
	for (int i = 0; i < TEXTURE_TYPE_COUNT; i++) {
		if (m_ppTextures[i]) m_ppTextures[i]->Release();
	}
}

void CMaterial::SetTexture(CTexture* pTexture, TextureType type) {
	if (type >= TEXTURE_TYPE_COUNT) return;
	// 기존 텍스처 해제 전에 새 텍스처 참조 추가
	if (pTexture) pTexture->AddRef();
	// 기존 텍스처 해제
	if (m_ppTextures[type]) m_ppTextures[type]->Release();
	m_ppTextures[type] = pTexture;
}

void CMaterial::LoadTextureSet(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, const wchar_t* pszBasePath, const wchar_t* pszExtension) {
	int nLoadedCount = 0;
	int nFailedCount = 0;

	// 각 텍스처 타입별로 파일 로드
	for (int i = 0; i < TEXTURE_TYPE_COUNT; i++) {
		wchar_t szFileName[256];
		swprintf_s(szFileName, L"%s%s%s", pszBasePath, g_TextureSuffixes[i], pszExtension);
		
		// 파일 존재 확인
		DWORD dwAttrib = GetFileAttributesW(szFileName);
		if (dwAttrib != INVALID_FILE_ATTRIBUTES) {
			CTexture* pTexture = new CTexture(1);
			pTexture->LoadTextureFromFile(pd3dDevice, pd3dCommandList, szFileName, 0);
			
			// 텍스처가 실제로 로드되었는지 확인
			if (pTexture->GetTexture(0) != nullptr) {
				// SetTexture 내부에서 AddRef하므로 여기서 Release하지 않음
				if (m_ppTextures[i]) m_ppTextures[i]->Release();
				m_ppTextures[i] = pTexture;  // 직접 할당 (AddRef 불필요, 새로 생성한 것이므로 참조 카운트 1)
				nLoadedCount++;
			}
			else {
				delete pTexture;  // 로드 실패 시 직접 삭제
				nFailedCount++;
			}
		}
		else nFailedCount++;
	}
}

void CMaterial::PrepareRender(ID3D12GraphicsCommandList* pd3dCommandList) {
	// 텍스처들을 셰이더에 바인딩
	// 루트 파라미터 인덱스: 0=World, 1=Camera, 2~8=Textures
	for (int i = 0; i < TEXTURE_TYPE_COUNT; i++) {
		if (m_ppTextures[i]) {
			D3D12_GPU_DESCRIPTOR_HANDLE handle = m_ppTextures[i]->GetGpuDescriptorHandle(0);
			if (handle.ptr != 0) {
				pd3dCommandList->SetGraphicsRootDescriptorTable(2 + i, handle);
			}
		}
	}
}

void CMaterial::ReleaseUploadBuffers() {
	for (int i = 0; i < TEXTURE_TYPE_COUNT; i++) {
		if (m_ppTextures[i]) {
			m_ppTextures[i]->ReleaseUploadBuffers();
		}
	}
}