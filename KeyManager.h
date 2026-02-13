#pragma once
#include "PCH.h"
#include "Player.h"
#include "Camera.h"
#include "Timer.h"

class CKeyManager {
private:
	int m_nWndClientWidth;
	int m_nWndClientHeight;
public:
	CKeyManager();
	~CKeyManager();

	void OnCreate(HWND hMainWnd, CGameTimer* pTimer);

	void OnceKey(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam);
	void OnceMouse(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam);
	LRESULT CALLBACK ProcessingMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam);
	void ProcessInput();

	HWND m_hWnd = NULL;
	POINT m_ptOldCursorPos;
	CCamera* m_pCamera = NULL;
	CPlayer* m_pPlayer = NULL;
	CGameTimer* m_pGameTimer = NULL;
};