#pragma once
#include "PCH.h"
#include "Timer.h"
#include "Scene.h"
#include "Camera.h"
#include "Player.h"

class CGameFramework
{
private:
	HINSTANCE m_hInstance;
	HWND      m_hWnd;

	int    m_nWndClientWidth;
	int    m_nWndClientHeight;

	IDXGIFactory4* m_pdxgiFactory;		// DXGI 팩토리 인터페이스 포인터
	IDXGISwapChain3* m_pdxgiSwapChain; 	// DXGI 스왑체인 인터페이스 포인터 (디스플레이 제어)
	ID3D12Device* m_pd3dDevice;			// 다이렉트3D 12 디바이스 인터페이스 포인터 (리소스 생성)

	bool m_bMsaa4xEnable = false;
	UINT m_nMsaa4xQualityLevels = 0;	// MSAA 다중 샘플링 활성화

	static const UINT m_nSwapChainBuffers = 2;  // 스왑체인 버퍼 수
	UINT m_nSwapChainBufferIndex;				// 현재 스왑체인 버퍼 인덱스

	ID3D12Resource* m_ppd3dRenderTargetBuffers[m_nSwapChainBuffers];	// 렌더	타겟 버퍼
	ID3D12DescriptorHeap* m_pd3dRtvDescriptorHeap;						// 서술자 힙 인터페이스	포인터
	UINT m_nRtvDescriptorIncrementSize;									// 렌더 타겟 서술자 원소 크기

	ID3D12Resource* m_pd3dDepthStencilBuffer;			// 깊이-스텐실 버퍼
	ID3D12DescriptorHeap* m_pd3dDsvDescriptorHeap;		// 서술자 힙 인터페이스 포인터
	UINT m_nDsvDescriptorIncrementSize;				// 깊이-스텐실 서술자 원소 크기

	ID3D12CommandQueue* m_pd3dCommandQueue;				// 명령 큐
	ID3D12CommandAllocator* m_pd3dCommandAllocator;		// 명령 할당자
	ID3D12GraphicsCommandList* m_pd3dCommandList;		// 명령 리스트

	ID3D12PipelineState* m_pd3dPipelineState;			// 파이프라인 상태 객체 포인터

	ID3D12Fence* m_pd3dFence;							// 펜스 인터페이스 포인터
	UINT64 m_nFenceValue;								// 펜스 값
	HANDLE m_hFenceEvent;								// 펜스 이벤트 핸들

	//게임 프레임워크에서 사용할 타이머이다.
	CGameTimer m_GameTimer;
	//프레임 레이트를 주 윈도우의 캡션에 출력하기 위한 문자열이다.
	_TCHAR m_pszFrameRate[50];
public:
	CGameFramework();
	~CGameFramework();

	bool OnCreate(HINSTANCE hInstance, HWND hMainWnd);
	void OnDestroy();
	
	void CreateSwapChain();
	void CreateRtvAndDsvDescriptorHeaps();
	void CreateDirect3DDevice();
	void CreateCommandQueueAndList();

	void CreateRenderTargetViews();
	void CreateDepthStencilView();

	void BuildObjects();
	void ReleaseObjects();

	void ProcessInput();
	void AnimateObjects();
	void FrameAdvance();

	void WaitForGpuComplete();

	void OnProcessingMouseMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam);
	void OnProcessingKeyboardMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam);
	LRESULT CALLBACK OnProcessingWindowMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam);

	void ChangeSwapChainState();

	void MoveToNextFrame();
	UINT64 m_nFenceValues[m_nSwapChainBuffers];
	CScene* m_pScene;

	CCamera* m_pCamera = NULL;

	//플레이어 객체에 대한 포인터이다.
	CPlayer* m_pPlayer = NULL;
	//마지막으로 마우스 버튼을 클릭할 때의 마우스 커서의 위치이다.
	POINT m_ptOldCursorPos;
};

