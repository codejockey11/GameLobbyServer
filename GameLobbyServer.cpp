#include "framework.h"

#include "resource.h"

#include "CLobbyServer.h"

#define MAX_LOADSTRING 100

enum Controls
{
	BTN_STARTSERVER = 200,
	BTN_STOPSERVER,
	TEXT_EDIT,
	TEXT_MESSAGE
};

CLobbyServer* m_server;
CString* c_port;

HINSTANCE m_hInst;

HRESULT m_hr;

HWND m_hWnd;
HWND m_message;
HWND m_port;
HWND m_startServer;
HWND m_stopServer;

static HFONT s_hFont = NULL;

WCHAR szTitle[MAX_LOADSTRING];
WCHAR szWindowClass[MAX_LOADSTRING];

ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

/*
*/
int APIENTRY wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ int nCmdShow)
{
	hPrevInstance;

	m_hr = OleInitialize(0);

	c_port = new CString(lpCmdLine);

	LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadStringW(hInstance, IDC_GAMELOBBYSERVER, szWindowClass, MAX_LOADSTRING);

	MyRegisterClass(hInstance);

	if (!InitInstance(hInstance, nCmdShow))
	{
		return FALSE;
	}

	LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_GAMELOBBYSERVER));

	MSG msg = {};

	while (msg.message != WM_QUIT)
	{
		if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);

			DispatchMessage(&msg);
		}
		else
		{
			m_server->Frame();
		}
	}

	SAFE_DELETE(m_server);
	SAFE_DELETE(c_port);

	OleUninitialize();

	return (int)msg.wParam;
}

/*
*/
ATOM MyRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEXW wcex = {};

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = WndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hInstance;
	wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_GAMELOBBYSERVER));
	wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wcex.lpszMenuName = MAKEINTRESOURCEW(IDC_GAMELOBBYSERVER);
	wcex.lpszClassName = szWindowClass;
	wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

	return RegisterClassExW(&wcex);
}

/*
*/
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
	m_hInst = hInstance;

	int x = (GetSystemMetrics(SM_CXSCREEN) / 2) - 265 / 2;
	int y = (GetSystemMetrics(SM_CYSCREEN) / 2) - 150 / 2;

	m_hWnd = CreateWindowW(szWindowClass, szTitle, WS_POPUP | WS_SYSMENU | WS_CAPTION | WS_MINIMIZEBOX,
		x, y,
		265, 150, nullptr, nullptr, hInstance, nullptr);

	if (!m_hWnd)
	{
		return FALSE;
	}

	ShowWindow(m_hWnd, nCmdShow);
	UpdateWindow(m_hWnd);

	const TCHAR* fontName = _T("MS Shell Dlg");
	const long nFontSize = 10;

	HDC hdc = GetDC(m_hWnd);

	LOGFONT logFont = { 0 };
	logFont.lfHeight = -MulDiv(nFontSize, GetDeviceCaps(hdc, LOGPIXELSY), 72);
	logFont.lfWeight = FW_NORMAL;
	_tcscpy_s(logFont.lfFaceName, fontName);

	s_hFont = CreateFontIndirect(&logFont);

	ReleaseDC(m_hWnd, hdc);

	m_startServer = CreateWindow(WC_BUTTON, L"Start", WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
		6, 3,
		64, 22,
		m_hWnd,
		(HMENU)Controls::BTN_STARTSERVER,
		m_hInst,
		NULL);

	m_stopServer = CreateWindow(WC_BUTTON, L"Stop", WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
		64 + 6 + 3, 3,
		64, 22,
		m_hWnd,
		(HMENU)Controls::BTN_STOPSERVER,
		m_hInst,
		NULL);

	m_port = CreateWindow(WC_EDIT, L"", WS_CHILD | WS_VISIBLE | WS_BORDER,
		64 + 6 + 3 + 64 + 3 + 6, 3,
		95, 22,
		m_hWnd,
		(HMENU)Controls::TEXT_EDIT,
		m_hInst,
		NULL);

	m_message = CreateWindow(WC_EDIT, L"", WS_CHILD | WS_VISIBLE | WS_BORDER,
		6, 31,
		235, 22,
		m_hWnd,
		(HMENU)Controls::TEXT_MESSAGE,
		m_hInst,
		NULL);

	SendMessage(m_startServer, WM_SETFONT, (WPARAM)s_hFont, (LPARAM)MAKELONG(TRUE, 0));
	SendMessage(m_stopServer, WM_SETFONT, (WPARAM)s_hFont, (LPARAM)MAKELONG(TRUE, 0));
	SendMessage(m_message, WM_SETFONT, (WPARAM)s_hFont, (LPARAM)MAKELONG(TRUE, 0));
	SendMessage(m_port, WM_SETFONT, (WPARAM)s_hFont, (LPARAM)MAKELONG(TRUE, 0));

	m_server = new CLobbyServer();

	if (c_port->m_length > 0)
	{
		m_server->StartServer(c_port->m_text);
		
		SetWindowTextA(m_port, c_port->m_text);
	}
	else
	{
		m_server->StartServer("49153");
		
		SetWindowTextA(m_port, "49153");
	}

	CString* message = new CString("Running on port:");

	message->Append(m_server->m_listenSocket->m_port);

	SetWindowTextA(m_message, message->m_text);

	SAFE_DELETE(message);

	return TRUE;
}

/*
*/
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_COMMAND:
	{
		int wmId = LOWORD(wParam);

		switch (wmId)
		{
		case IDM_ABOUT:
		{
			DialogBox(m_hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);

			break;
		}
		case IDM_EXIT:
		{
			DestroyWindow(hWnd);

			break;
		}
		case Controls::BTN_STARTSERVER:
		{
			if (m_server->m_listenThreadRunning)
			{
				return 0;
			}

			if (c_port)
			{
				SAFE_DELETE(c_port);
			}

			c_port = new CString(32);

			GetWindowTextA(m_port, c_port->m_text, 6);

			m_server->StartServer(c_port->m_text);

			CString* messageString = new CString("Running on port:");

			messageString->Append(m_server->m_listenSocket->m_port);

			SetWindowTextA(m_message, messageString->m_text);

			SAFE_DELETE(messageString);

			return 0;
		}
		case Controls::BTN_STOPSERVER:
		{
			m_server->Stop();

			SetWindowTextA(m_message, "Stopped");

			return 0;
		}
		default:
		{
			return DefWindowProc(hWnd, message, wParam, lParam);
		}
		}

		break;
	}
	case WM_PAINT:
	{
		PAINTSTRUCT ps;
		
		BeginPaint(hWnd, &ps);

		EndPaint(hWnd, &ps);

		break;
	}
	case WM_CLOSE:
	case WM_DESTROY:
	{
		PostQuitMessage(0);
	
		break;
	}
	default:
	{
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	}
	
	return 0;
}

/*
*/
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{
	case WM_INITDIALOG:
	{
		return (INT_PTR)TRUE;
	}
	case WM_COMMAND:
	{
		if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
		{
			EndDialog(hDlg, LOWORD(wParam));
			
			return (INT_PTR)TRUE;
		}

		break;
	}
	}
	
	return (INT_PTR)FALSE;
}