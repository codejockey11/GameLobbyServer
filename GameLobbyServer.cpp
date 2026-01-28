#include "framework.h"

#include "resource.h"

#include "CLobbyServer.h"

#include "../GameCommon/CCommandLine.h"
#include "../GameCommon/CWindow.h"

enum Controls
{
	BTN_STARTSERVER = 200,
	BTN_STOPSERVER,
	TEXT_EDIT,
	TEXT_MESSAGE
};

CCommandLine* m_commandLine;
CLobbyServer* m_server;
CString* m_port;
CWindow* m_window;

HWND m_hMessage;
HWND m_hPort;

LRESULT CALLBACK    WndProc(HWND, uint32_t, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, uint32_t, WPARAM, LPARAM);

/*
*/
int32_t APIENTRY wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ int32_t nCmdShow)
{
	m_commandLine = new CCommandLine();

	m_commandLine->ServerConstructor(lpCmdLine);

	m_window = new CWindow(hInstance, WndProc, "LobbyServerClass", IDC_GAMELOBBYSERVER, IDI_GAMELOBBYSERVER, IDI_SMALL, "Lobby Server", 265, 120, 0, 0);

	m_window->AddButton(L"Start", 6, 3, 64, 22, (HMENU)Controls::BTN_STARTSERVER);
	m_window->AddButton(L"Stop", 76, 3, 64, 22, (HMENU)Controls::BTN_STOPSERVER);

	m_hPort = m_window->AddTextEdit(L"49153", 146, 3, 95, 22, (HMENU)Controls::TEXT_EDIT);
	m_hMessage = m_window->AddTextEdit(L"Stopped", 6, 31, 235, 22, (HMENU)Controls::TEXT_MESSAGE);

	m_server = new CLobbyServer();

	if (strlen(m_commandLine->m_port) > 0)
	{
		m_window->SetTextForControl(m_hPort, m_commandLine->m_port);
	}

	GetWindowTextA(m_hPort, m_commandLine->m_port, 6);

	m_server->StartServer(m_commandLine->m_port);

	CString* messageString = new CString("Running on port:");

	messageString->Append(m_server->m_listenSocket->m_port);

	m_window->SetTextForControl(m_hMessage, messageString->m_text);

	SAFE_DELETE(messageString);


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
	SAFE_DELETE(m_port);
	SAFE_DELETE(m_commandLine);
	SAFE_DELETE(m_window);

	return (int32_t)msg.wParam;
}

/*
*/
LRESULT CALLBACK WndProc(HWND hWnd, uint32_t message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_COMMAND:
	{
		int32_t wmId = LOWORD(wParam);

		switch (wmId)
		{
		case IDM_ABOUT:
		{
			DialogBox(m_window->m_hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);

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

			GetWindowTextA(m_hPort, m_commandLine->m_port, 6);

			m_server->StartServer(m_commandLine->m_port);

			CString* messageString = new CString("Running on port:");

			messageString->Append(m_server->m_listenSocket->m_port);

			m_window->SetTextForControl(m_hMessage, messageString->m_text);

			SAFE_DELETE(messageString);

			return 0;
		}
		case Controls::BTN_STOPSERVER:
		{
			m_server->Stop();

			m_window->SetTextForControl(m_hMessage, "Stopped");

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
INT_PTR CALLBACK About(HWND hDlg, uint32_t message, WPARAM wParam, LPARAM lParam)
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