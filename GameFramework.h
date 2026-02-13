#pragma once

#include "PCH.h"
#include "KeyManager.h"
#include "Timer.h"
#include "Scene.h"
#include "Camera.h"
#include "Player.h"

class CGameFramework {
private:
	HINSTANCE m_hInstance;  // 애플리케이션 인스턴스 핸들
	HWND m_hWnd;			// 주 윈도우 핸들

	int m_nWndClientWidth;  // 윈도우 클라이언트 영역 너비
	int m_nWndClientHeight; // 윈도우 클라이언트 영역 높이

	IDXGIFactory4* m_pdxgiFactory;		// DXGI 팩토리 인터페이스 포인터
	IDXGISwapChain3* m_pdxgiSwapChain; 	// DXGI 스왑체인 인터페이스 포인터 (디스플레이 제어)
	ID3D12Device* m_pd3dDevice;			// 다이렉트3D 12 디바이스 인터페이스 포인터 (리소스 생성)

	bool m_bMsaa4xEnable = false;		// MSAA 다중 샘플링 사용 여부
	UINT m_nMsaa4xQualityLevels = 0;	// MSAA 다중 샘플링 활성화

	static const UINT m_nSwapChainBuffers = 2;  // 스왑체인 버퍼 수
	UINT m_nSwapChainBufferIndex;				// 현재 스왑체인 버퍼 인덱스

	ID3D12Resource* m_ppd3dRenderTargetBuffers[m_nSwapChainBuffers];	// 렌더	타겟 버퍼
	ID3D12DescriptorHeap* m_pd3dRtvDescriptorHeap;						// 서술자 힙 인터페이스	포인터
	UINT m_nRtvDescriptorIncrementSize;									// 렌더 타겟 서술자 원소 크기

	ID3D12Resource* m_pd3dDepthStencilBuffer;			// 깊이-스텐실 버퍼
	ID3D12DescriptorHeap* m_pd3dDsvDescriptorHeap;		// 서술자 힙 인터페이스 포인터
	UINT m_nDsvDescriptorIncrementSize;					// 깊이-스텐실 서술자 원소 크기

	ID3D12CommandQueue* m_pd3dCommandQueue;				// 명령 큐
	ID3D12CommandAllocator* m_pd3dCommandAllocator;		// 명령 할당자
	ID3D12GraphicsCommandList* m_pd3dCommandList;		// 명령 리스트

	ID3D12PipelineState* m_pd3dPipelineState;			// 파이프라인 상태 객체 포인터

	ID3D12Fence* m_pd3dFence;							// 펜스 인터페이스 포인터
	UINT64 m_nFenceValue;								// 펜스 값
	HANDLE m_hFenceEvent;								// 펜스 이벤트 핸들

	CGameTimer m_GameTimer;			//게임 프레임워크에서 사용할 타이머
	
	_TCHAR m_pszFrameRate[50];		//프레임 레이트를 주 윈도우의 캡션에 출력하기 위한 문자열
public:
	CGameFramework();	// 생성자
	~CGameFramework();	// 소멸자

	bool OnCreate(HINSTANCE hInstance, HWND hMainWnd);	// 프로그램 생성 시 호출
	void OnDestroy();									// 프로그램 종료 시 호출
	
	void CreateDirect3DDevice();			// 다이렉트3D 디바이스 생성
	void CreateCommandQueueAndList();		// 명령 큐 및 명령 리스트 생성
	void CreateRtvAndDsvDescriptorHeaps();	// RTV 및 DSV 서술자 힙 생성
	void CreateSwapChain();					// 스왑체인 생성
		
	void CreateDepthStencilView();			// 깊이-스텐실 뷰 생성
	void CreateRenderTargetViews();			// 렌더 타겟 뷰 생성

	//void ChangeSwapChainState();			// 전체화면 모드 변경 처리
	
	void BuildObjects();					// 게임 오브젝트 생성
	void ReleaseObjects();					// 게임 오브젝트 해제
	void AnimateObjects();					// 게임 오브젝트 애니메이션 처리

	void FrameAdvance();					// 프레임 진행
	void WaitForGpuComplete();				// GPU 작업 완료 대기
	void MoveToNextFrame();					// 다음 프레임으로 이동

	//void ProcessInput();					// 입력 처리
	//void OnProcessingMouseMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam);				// 마우스 메시지 처리
	//void OnProcessingKeyboardMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam);				// 키보드 메시지 처리
	//LRESULT CALLBACK OnProcessingWindowMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam);	// 윈도우 메시지 처리

	UINT64 m_nFenceValues[m_nSwapChainBuffers];	// 각 스왑체인 버퍼에 대한 펜스 값 배열

	CScene* m_pScene;			// 씬 객체에 대한 포인터
	//CCamera* m_pCamera = NULL;	// 카메라 객체에 대한 포인터
	//CPlayer* m_pPlayer = NULL;	// 플레이어 객체에 대한 포인터
	//POINT m_ptOldCursorPos;		// 이전 마우스 커서 위치

	CKeyManager m_KeyManager;
};