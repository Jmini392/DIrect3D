#include "GameFramework.h"

CGameFramework::CGameFramework() {
	m_pdxgiFactory = NULL;
	m_pdxgiSwapChain = NULL;
	m_pd3dDevice = NULL;

	m_pd3dCommandAllocator = NULL;
	m_pd3dCommandQueue = NULL;
	m_pd3dPipelineState = NULL;
	m_pd3dCommandList = NULL;

	for (int i = 0; i < m_nSwapChainBuffers; i++) {
		m_ppd3dRenderTargetBuffers[i] = NULL;
	}
	m_pd3dRtvDescriptorHeap = NULL;
	m_nRtvDescriptorIncrementSize = 0;

	m_pd3dDepthStencilBuffer = NULL;
	m_pd3dDsvDescriptorHeap = NULL;
	m_nDsvDescriptorIncrementSize = 0;

	m_nSwapChainBufferIndex = 0;

	m_hFenceEvent = NULL;
	m_pd3dFence = NULL;
	m_nFenceValue = 0;

	m_nWndClientHeight = FRAME_BUFFER_HEIGHT;
	m_nWndClientWidth = FRAME_BUFFER_WIDTH;

	_tcscpy_s(m_pszFrameRate, _T("LapProject ("));

	for (int i = 0; i < m_nSwapChainBuffers; i++) {
		m_nFenceValues[i] = 0;
	}
	m_pScene = NULL;
}

CGameFramework::~CGameFramework() {}

bool CGameFramework::OnCreate(HINSTANCE hInstance, HWND hMainWnd) { // 실행시 한번만 호출됨
	m_hInstance = hInstance;	// 윈도우 인스턴스 핸들 저장
	m_hWnd = hMainWnd;			// 윈도우 메인 핸들 저장

	CreateDirect3DDevice();				// 다이렉트3D 디바이스 생성
	CreateCommandQueueAndList();		// 명령 큐 및 명령 리스트 생성
	CreateRtvAndDsvDescriptorHeaps();	// RTV 및 DSV 서술자 힙 생성
	CreateSwapChain();					// 스왑체인 생성
	CreateDepthStencilView();			// 깊이-스텐실 뷰 생성

	// KeyManager 초기화 (BuildObjects 전에 hWnd와 타이머 설정)
	m_KeyManager.OnCreate(hMainWnd, &m_GameTimer);
	
	BuildObjects();						// 게임 오브젝트 생성

	return(true);
}

void CGameFramework::OnDestroy() {
	WaitForGpuComplete();		  // GPU 작업 완료 대기

	ReleaseObjects();			  // 게임 오브젝트 해제
	
	::CloseHandle(m_hFenceEvent); // 펜스 이벤트 핸들 닫기

	for (int i = 0; i < m_nSwapChainBuffers; i++) {
		// 스왑체인 버퍼 해제
		if (m_ppd3dRenderTargetBuffers[i]) m_ppd3dRenderTargetBuffers[i]->Release();
	}
	// RTV 서술자 힙 해제
	if (m_pd3dRtvDescriptorHeap) m_pd3dRtvDescriptorHeap->Release();
	
	// 깊이-스텐실 버퍼 및 DSV 서술자 힙 해제
	if (m_pd3dDepthStencilBuffer) m_pd3dDepthStencilBuffer->Release();
	if (m_pd3dDsvDescriptorHeap) m_pd3dDsvDescriptorHeap->Release();

	// 명령 큐, 명령 할당자, 파이프라인 상태 객체 및 명령 리스트 해제
	if (m_pd3dCommandAllocator) m_pd3dCommandAllocator->Release();
	if (m_pd3dCommandQueue) m_pd3dCommandQueue->Release();
	if (m_pd3dPipelineState) m_pd3dPipelineState->Release();
	if (m_pd3dCommandList) m_pd3dCommandList->Release();

	// 펜스 해제
	if (m_pd3dFence) m_pd3dFence->Release();

	// 전체화면 모드 해제
	m_pdxgiSwapChain->SetFullscreenState(FALSE, NULL);

	// 스왑체인, 다이렉트3D 디바이스 및 DXGI 팩토리 해제
	if (m_pdxgiSwapChain) m_pdxgiSwapChain->Release();
	if (m_pd3dDevice) m_pd3dDevice->Release();
	if (m_pdxgiFactory) m_pdxgiFactory->Release();

	// 디버그 객체를 통해 릴리즈되지 않은 리소스가 있는지 검사
#if defined(_DEBUG)
	IDXGIDebug1* pdxgiDebug = NULL;
	DXGIGetDebugInterface1(0, __uuidof(IDXGIDebug1), (void**)&pdxgiDebug);
	HRESULT hResult = pdxgiDebug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_DETAIL);
	pdxgiDebug->Release();
#endif
}

void CGameFramework::CreateSwapChain() {
	RECT rcClient; // 클라이언트 영역 사각형
	::GetClientRect(m_hWnd, &rcClient); // 클라이언트 영역 사각형 정보 얻기
	m_nWndClientWidth = rcClient.right - rcClient.left;  // 클라이언트 영역 너비 계산
	m_nWndClientHeight = rcClient.bottom - rcClient.top; // 클라이언트 영역 높이 계산

	DXGI_SWAP_CHAIN_DESC dxgiSwapChainDesc; // 스왑체인 서술자
	::ZeroMemory(&dxgiSwapChainDesc, sizeof(dxgiSwapChainDesc)); // 스왑체인 서술자 초기화
	dxgiSwapChainDesc.BufferCount = m_nSwapChainBuffers; // 스왑체인 버퍼 수 설정
	dxgiSwapChainDesc.BufferDesc.Width = m_nWndClientWidth; // 스왑체인 버퍼 너비 설정
	dxgiSwapChainDesc.BufferDesc.Height = m_nWndClientHeight; // 스왑체인 버퍼 높이 설정
	dxgiSwapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM; // 스왑체인 버퍼 포맷 설정

	dxgiSwapChainDesc.BufferDesc.RefreshRate.Numerator = 60; // 스왑체인 버퍼 새로고침 빈도 설정
	dxgiSwapChainDesc.BufferDesc.RefreshRate.Denominator = 1; // 스왑체인 버퍼 새로고침 분모 설정
	dxgiSwapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT; // 스왑체인 버퍼 사용 용도 설정
	dxgiSwapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD; // 스왑체인 교체 효과 설정

	dxgiSwapChainDesc.OutputWindow = m_hWnd; // 스왑체인 출력 윈도우 설정
	dxgiSwapChainDesc.SampleDesc.Count = (m_bMsaa4xEnable) ? 4 : 1; // 스왑체인 샘플링 카운트 설정
	dxgiSwapChainDesc.SampleDesc.Quality = (m_bMsaa4xEnable) ? (m_nMsaa4xQualityLevels - 1) : 0; // 스왑체인 샘플링 품질 설정
	dxgiSwapChainDesc.Windowed = TRUE; // 스왑체인 창 모드 설정

	//전체화면 모드에서 바탕화면의 해상도를 스왑체인(후면버퍼)의 크기에 맞게 변경한다.
	dxgiSwapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
	HRESULT hResult = m_pdxgiFactory->CreateSwapChain(m_pd3dCommandQueue, &dxgiSwapChainDesc, (IDXGISwapChain**)&m_pdxgiSwapChain);
	m_nSwapChainBufferIndex = m_pdxgiSwapChain->GetCurrentBackBufferIndex();
	hResult = m_pdxgiFactory->MakeWindowAssociation(m_hWnd, DXGI_MWA_NO_ALT_ENTER);

	// 스왑체인 버퍼에 대한 렌더 타겟 뷰 생성
#ifndef _WITH_SWAPCHAIN_FULLSCREEN_STATE
	CreateRenderTargetViews();
#endif
}

void CGameFramework::CreateDirect3DDevice() {
	HRESULT hResult; // 결과 코드
	UINT nDXGIFactoryFlags = 0; // DXGI 팩토리 플래그
#if defined(_DEBUG)
	ID3D12Debug* pd3dDebugController = NULL;
	hResult = D3D12GetDebugInterface(__uuidof(ID3D12Debug), (void**)&pd3dDebugController);
	if (pd3dDebugController) {
		pd3dDebugController->EnableDebugLayer();
		pd3dDebugController->Release();
	}
	nDXGIFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
#endif
	hResult = ::CreateDXGIFactory2(nDXGIFactoryFlags, __uuidof(IDXGIFactory4), (void**)&m_pdxgiFactory);
	IDXGIAdapter1* pd3dAdapter = NULL;
	for (UINT i = 0; DXGI_ERROR_NOT_FOUND != m_pdxgiFactory->EnumAdapters1(i, &pd3dAdapter); i++) {
		DXGI_ADAPTER_DESC1 dxgiAdapterDesc;
		pd3dAdapter->GetDesc1(&dxgiAdapterDesc);
		if (dxgiAdapterDesc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) continue;
		if (SUCCEEDED(D3D12CreateDevice(pd3dAdapter, D3D_FEATURE_LEVEL_12_0, __uuidof(ID3D12Device), (void**)&m_pd3dDevice))) break;
	}
	if (!pd3dAdapter) {
		m_pdxgiFactory->EnumWarpAdapter(__uuidof(IDXGIAdapter1), (void**)&pd3dAdapter);
		D3D12CreateDevice(pd3dAdapter, D3D_FEATURE_LEVEL_11_0, __uuidof(ID3D12Device), (void**)&m_pd3dDevice);
	}
	D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS d3dMsaaQualityLevels;
	d3dMsaaQualityLevels.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	d3dMsaaQualityLevels.SampleCount = 4;
	d3dMsaaQualityLevels.Flags = D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG_NONE;
	d3dMsaaQualityLevels.NumQualityLevels = 0;
	m_pd3dDevice->CheckFeatureSupport(D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS,	&d3dMsaaQualityLevels, sizeof(D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS));
	m_nMsaa4xQualityLevels = d3dMsaaQualityLevels.NumQualityLevels;
	m_bMsaa4xEnable = (m_nMsaa4xQualityLevels > 1) ? true : false;

	hResult = m_pd3dDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, __uuidof(ID3D12Fence), (void**)&m_pd3dFence);
	m_nFenceValue = 0;
	m_hFenceEvent = ::CreateEvent(NULL, FALSE, FALSE, NULL);

	if (pd3dAdapter) pd3dAdapter->Release();
}

void CGameFramework::CreateCommandQueueAndList() {
	D3D12_COMMAND_QUEUE_DESC d3dCommandQueueDesc;
	::ZeroMemory(&d3dCommandQueueDesc, sizeof(D3D12_COMMAND_QUEUE_DESC));
	d3dCommandQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	d3dCommandQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	HRESULT hResult = m_pd3dDevice->CreateCommandQueue(&d3dCommandQueueDesc, __uuidof(ID3D12CommandQueue), (void**)&m_pd3dCommandQueue);

	hResult = m_pd3dDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, __uuidof(ID3D12CommandAllocator), (void**)&m_pd3dCommandAllocator);
	
	hResult = m_pd3dDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_pd3dCommandAllocator, NULL,
		__uuidof(ID3D12GraphicsCommandList), (void**)&m_pd3dCommandList);
	
	hResult = m_pd3dCommandList->Close();
}

void CGameFramework::CreateRtvAndDsvDescriptorHeaps() {
	D3D12_DESCRIPTOR_HEAP_DESC d3dDescriptorHeapDesc;
	::ZeroMemory(&d3dDescriptorHeapDesc, sizeof(D3D12_DESCRIPTOR_HEAP_DESC));
	d3dDescriptorHeapDesc.NumDescriptors = m_nSwapChainBuffers;
	d3dDescriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	d3dDescriptorHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	d3dDescriptorHeapDesc.NodeMask = 0;
	HRESULT hResult = m_pd3dDevice->CreateDescriptorHeap(&d3dDescriptorHeapDesc, __uuidof(ID3D12DescriptorHeap), (void**)&m_pd3dRtvDescriptorHeap);
	m_nRtvDescriptorIncrementSize = m_pd3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	d3dDescriptorHeapDesc.NumDescriptors = 1;
	d3dDescriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	hResult = m_pd3dDevice->CreateDescriptorHeap(&d3dDescriptorHeapDesc, __uuidof(ID3D12DescriptorHeap), (void**)&m_pd3dDsvDescriptorHeap);
	m_nDsvDescriptorIncrementSize = m_pd3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
}

void CGameFramework::CreateRenderTargetViews() {
	D3D12_CPU_DESCRIPTOR_HANDLE d3dRtvCPUDescriptorHandle = m_pd3dRtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	for (UINT i = 0; i < m_nSwapChainBuffers; i++) {
		m_pdxgiSwapChain->GetBuffer(i, __uuidof(ID3D12Resource), (void**)&m_ppd3dRenderTargetBuffers[i]);
		m_pd3dDevice->CreateRenderTargetView(m_ppd3dRenderTargetBuffers[i], NULL, d3dRtvCPUDescriptorHandle);
		d3dRtvCPUDescriptorHandle.ptr += m_nRtvDescriptorIncrementSize;
	}
}

void CGameFramework::CreateDepthStencilView() {
	D3D12_RESOURCE_DESC d3dResourceDesc;
	d3dResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	d3dResourceDesc.Alignment = 0;
	d3dResourceDesc.Width = m_nWndClientWidth;
	d3dResourceDesc.Height = m_nWndClientHeight;
	d3dResourceDesc.DepthOrArraySize = 1;
	d3dResourceDesc.MipLevels = 1;
	d3dResourceDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	d3dResourceDesc.SampleDesc.Count = m_bMsaa4xEnable ? 4 : 1;
	d3dResourceDesc.SampleDesc.Quality = m_bMsaa4xEnable ? (m_nMsaa4xQualityLevels - 1) : 0;
	d3dResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	d3dResourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

	D3D12_HEAP_PROPERTIES d3dHeapProperties;
	::ZeroMemory(&d3dHeapProperties, sizeof(D3D12_HEAP_PROPERTIES));
	d3dHeapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;
	d3dHeapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	d3dHeapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	d3dHeapProperties.CreationNodeMask = 1;
	d3dHeapProperties.VisibleNodeMask = 1;

	D3D12_CLEAR_VALUE d3dClearValue;
	d3dClearValue.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	d3dClearValue.DepthStencil.Depth = 1.0f;
	d3dClearValue.DepthStencil.Stencil = 0;
	HRESULT hResult = m_pd3dDevice->CreateCommittedResource(&d3dHeapProperties, D3D12_HEAP_FLAG_NONE, &d3dResourceDesc,
		D3D12_RESOURCE_STATE_DEPTH_WRITE, &d3dClearValue, __uuidof(ID3D12Resource), (void**)&m_pd3dDepthStencilBuffer);

	D3D12_CPU_DESCRIPTOR_HANDLE d3dDsvCPUDescriptorHandle = m_pd3dDsvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	m_pd3dDevice->CreateDepthStencilView(m_pd3dDepthStencilBuffer, NULL, d3dDsvCPUDescriptorHandle);
}

void CGameFramework::BuildObjects() {
	m_pd3dCommandList->Reset(m_pd3dCommandAllocator, NULL);

	// 씬이 플레이어 포함 모든 오브젝트를 생성
	m_pScene = new CScene();
	if (m_pScene) m_pScene->BuildObjects(m_pd3dDevice, m_pd3dCommandList);
	
	// KeyManager에 씬의 플레이어/카메라 참조 연결
	m_KeyManager.m_pPlayer = m_pScene->GetPlayer();
	m_KeyManager.m_pCamera = m_pScene->GetCamera();

	m_pd3dCommandList->Close();
	ID3D12CommandList* ppd3dCommandLists[] = { m_pd3dCommandList };
	m_pd3dCommandQueue->ExecuteCommandLists(1, ppd3dCommandLists);
	WaitForGpuComplete();
	if (m_pScene) m_pScene->ReleaseUploadBuffers();
	m_GameTimer.Reset();
}

void CGameFramework::ReleaseObjects() {
	if (m_pScene) m_pScene->ReleaseObjects();
	if (m_pScene) delete m_pScene;
}

void CGameFramework::AnimateObjects() {
	if (m_pScene) m_pScene->AnimateObjects(m_GameTimer.GetTimeElapsed());
}

void CGameFramework::WaitForGpuComplete() {
	UINT64 nFenceValue = ++m_nFenceValues[m_nSwapChainBufferIndex];
	HRESULT hResult = m_pd3dCommandQueue->Signal(m_pd3dFence, nFenceValue);
	if (m_pd3dFence->GetCompletedValue() < nFenceValue) {
		hResult = m_pd3dFence->SetEventOnCompletion(nFenceValue, m_hFenceEvent);
		::WaitForSingleObject(m_hFenceEvent, INFINITE);
	}
}

void CGameFramework::FrameAdvance() { // 매 프레임마다 호출됨
	m_GameTimer.Tick(0.0f); // 타이머 갱신

	//ProcessInput();   // 입력 처리
	m_KeyManager.ProcessInput(); // KeyManager로 폴링 입력 처리
	// 카메라 변경이 반영되도록 동기화
	//m_pCamera = m_KeyManager.m_pCamera;

	AnimateObjects(); // 게임 오브젝트 애니메이션
	
	// 명령 할당자 및 명령 리스트 초기화
	HRESULT hResult = m_pd3dCommandAllocator->Reset();
	hResult = m_pd3dCommandList->Reset(m_pd3dCommandAllocator, NULL);
	
	// 스왑체인 버퍼를 렌더 타겟 상태로 전환
	D3D12_RESOURCE_BARRIER d3dResourceBarrier;
	::ZeroMemory(&d3dResourceBarrier, sizeof(D3D12_RESOURCE_BARRIER));
	d3dResourceBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	d3dResourceBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	d3dResourceBarrier.Transition.pResource = m_ppd3dRenderTargetBuffers[m_nSwapChainBufferIndex];
	d3dResourceBarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
	d3dResourceBarrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
	d3dResourceBarrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	m_pd3dCommandList->ResourceBarrier(1, &d3dResourceBarrier);
	
	// 현재 렌더 타겟 뷰 설정
	D3D12_CPU_DESCRIPTOR_HANDLE d3dRtvCPUDescriptorHandle = m_pd3dRtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	d3dRtvCPUDescriptorHandle.ptr += (m_nSwapChainBufferIndex * m_nRtvDescriptorIncrementSize);
	
	// 배경색으로 렌더 타겟 뷰를 다시 그림
	float pfClearColor[4] = { 0.0f, 0.125f, 0.3f, 1.0f }; // 배경색 (RGBA - 청색 /*Colors::Azure*/) 
	m_pd3dCommandList->ClearRenderTargetView(d3dRtvCPUDescriptorHandle, pfClearColor, 0, NULL);
	
	// 깊이-스텐실 뷰를 지우고 렌더 타겟 및 깊이-스텐실 뷰 설정
	D3D12_CPU_DESCRIPTOR_HANDLE d3dDsvCPUDescriptorHandle = m_pd3dDsvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	m_pd3dCommandList->ClearDepthStencilView(d3dDsvCPUDescriptorHandle, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, NULL);
	m_pd3dCommandList->OMSetRenderTargets(1, &d3dRtvCPUDescriptorHandle, TRUE, &d3dDsvCPUDescriptorHandle);
	
	// 씬이 플레이어 포함 모든 것을 렌더링 (카메라도 씬이 갖고 있으므로 NULL 전달 가능)
	if (m_pScene) m_pScene->Render(m_pd3dCommandList, nullptr);

	d3dResourceBarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
	d3dResourceBarrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
	d3dResourceBarrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	m_pd3dCommandList->ResourceBarrier(1, &d3dResourceBarrier);
	hResult = m_pd3dCommandList->Close();
	
	// 명령 리스트 실행
	ID3D12CommandList* ppd3dCommandLists[] = { m_pd3dCommandList };
	m_pd3dCommandQueue->ExecuteCommandLists(1, ppd3dCommandLists);
	
	WaitForGpuComplete(); // GPU 작업 완료 대기
	
	m_pdxgiSwapChain->Present(0, 0); // 스왑체인 버퍼 교체
	
	MoveToNextFrame(); // 다음 프레임으로 이동
	
	// 프레임을 표시
	m_GameTimer.GetFrameRate(m_pszFrameRate + 12, 37); // 프레임률 문자열 생성
	::SetWindowText(m_hWnd, m_pszFrameRate); // 윈도우 타이틀에 프레임률 표시
}

void CGameFramework::MoveToNextFrame() {
	m_nSwapChainBufferIndex = m_pdxgiSwapChain->GetCurrentBackBufferIndex();
	
	UINT64 nFenceValue = ++m_nFenceValues[m_nSwapChainBufferIndex];
	
	HRESULT hResult = m_pd3dCommandQueue->Signal(m_pd3dFence, nFenceValue);
	
	if (m_pd3dFence->GetCompletedValue() < nFenceValue) {
		hResult = m_pd3dFence->SetEventOnCompletion(nFenceValue, m_hFenceEvent);
		::WaitForSingleObject(m_hFenceEvent, INFINITE);
	}
}