#pragma once

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
HINSTANCE g_hInst;
LPSTR lpszClass = "GITADORA Simulator";

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance
	, LPSTR lpszCmdParam, int nCmdShow)
{
	HWND hWnd;
	MSG Message;
	WNDCLASS WndClass;
	g_hInst = hInstance;

	ULONG_PTR tok;
	GdiplusStartupInput gsi;
	GdiplusStartup(&tok, &gsi, NULL);

	WndClass.cbClsExtra = 0;
	WndClass.cbWndExtra = 0;
	WndClass.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
	WndClass.hCursor = LoadCursor(NULL, IDC_ARROW);
	WndClass.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	WndClass.hInstance = hInstance;
	WndClass.lpfnWndProc = (WNDPROC)WndProc;
	WndClass.lpszClassName = lpszClass;
	WndClass.lpszMenuName = NULL;
	WndClass.style = CS_HREDRAW | CS_VREDRAW;
	RegisterClass(&WndClass);

	hWnd = CreateWindow(lpszClass, lpszClass, WS_OVERLAPPEDWINDOW,
		40, 10, GAME_RESOLUTION_WIDTH, GAME_RESOLUTION_HEIGHT + TASKBAR_HEIGHT,
		NULL, (HMENU)NULL, hInstance, NULL);
	ShowWindow(hWnd, nCmdShow);

	InitSleep();
	int done = 0;
	while (!done)
	{
		// PeekMessage는 윈도우 메시지를 확인하는 논블로킹 메소드다
		if (PeekMessage(&Message, NULL, 0, 0, PM_REMOVE))
		{
			// 종료 메시지를 찾는다
			if (Message.message == WM_QUIT)
				done = 1;
			// 해석한 뒤 메시지를 WinProc에 전달한다
			TranslateMessage(&Message);
			DispatchMessage(&Message);
		}
		else
			GameRunMain(hWnd);		// 게임 루프를 실행한다(반복적으로 호출)
	}

	GdiplusShutdown(tok);
	return Message.wParam;
}