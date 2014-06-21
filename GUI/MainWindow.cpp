#include "stdafx.h"
#include <io.h>
#include <Fcntl.h>
#include "resource.h"
#include "MainWindow.h"
#include "..\Core\Console.h"
#include "..\Core\Timer.h"
using namespace DirectX;

namespace NES 
{
	MainWindow* MainWindow::Instance = nullptr;

	bool MainWindow::Initialize()
	{
		if(FAILED(InitWindow())) {
			return false;
		}

		if(!_renderer.Initialize(_hInstance, _hWnd)) {
			return false;
		}

		return true;
	}

	void CreateConsole()
	{
		CONSOLE_SCREEN_BUFFER_INFO consoleInfo;
		int consoleHandleR, consoleHandleW;
		long stdioHandle;
		FILE *fptr;

		AllocConsole();
		std::wstring strW = L"Dev Console";
		SetConsoleTitle(strW.c_str());

		EnableMenuItem(GetSystemMenu(GetConsoleWindow(), FALSE), SC_CLOSE, MF_GRAYED);
		DrawMenuBar(GetConsoleWindow());

		GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &consoleInfo);

		stdioHandle = (long)GetStdHandle(STD_INPUT_HANDLE);
		consoleHandleR = _open_osfhandle(stdioHandle, _O_TEXT);
		fptr = _fdopen(consoleHandleR, "r");
		*stdin = *fptr;
		setvbuf(stdin, NULL, _IONBF, 0);

		stdioHandle = (long)GetStdHandle(STD_OUTPUT_HANDLE);
		consoleHandleW = _open_osfhandle(stdioHandle, _O_TEXT);
		fptr = _fdopen(consoleHandleW, "w");
		*stdout = *fptr;
		setvbuf(stdout, NULL, _IONBF, 0);

		stdioHandle = (long)GetStdHandle(STD_ERROR_HANDLE);
		*stderr = *fptr;
		setvbuf(stderr, NULL, _IONBF, 0);
	}

	int MainWindow::Run()
	{
		//#if _DEBUG
		CreateConsole();
		//#endif

		Initialize();

		MSG msg = { 0 };
		Timer timer;
		int frameCount = 0;
		while(WM_QUIT != msg.message) {
			if(PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			} else {
				_renderer.Render();
				frameCount++;
				if(frameCount == 500) {
					double fps = (double)frameCount / (timer.GetElapsedMS() / 1000);
					//std::cout << "FPS: " << fps << std::endl;
					timer.Reset();
					frameCount = 0;
				}
			}
			std::this_thread::sleep_for(std::chrono::duration<int, std::milli>(1));
		}

		return (int)msg.wParam;
	}

	//--------------------------------------------------------------------------------------
	// Register class and create window
	//--------------------------------------------------------------------------------------
	HRESULT MainWindow::InitWindow()
	{
		// Register class
		WNDCLASSEX wcex;
		wcex.cbSize = sizeof(WNDCLASSEX);
		wcex.style = CS_HREDRAW | CS_VREDRAW;
		wcex.lpfnWndProc = WndProc;
		wcex.cbClsExtra = 0;
		wcex.cbWndExtra = 0;
		wcex.hInstance = _hInstance;
		wcex.hIcon = LoadIcon(_hInstance, (LPCTSTR)IDI_GUI);
		wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
		wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
		wcex.lpszMenuName = MAKEINTRESOURCE(IDC_GUI);
		wcex.lpszClassName = L"NESEmu";
		wcex.hIconSm = LoadIcon(wcex.hInstance, (LPCTSTR)IDI_SMALL);
		if(!RegisterClassEx(&wcex))
			return E_FAIL;

		// Create window
		RECT rc = { 0, 0, 260, 270 };
		AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, FALSE);
		_hWnd = CreateWindow(L"NESEmu", L"NESEmu",
			WS_OVERLAPPEDWINDOW,
			CW_USEDEFAULT, CW_USEDEFAULT, rc.right - rc.left, rc.bottom - rc.top, nullptr, nullptr, _hInstance,
			nullptr);
		if(!_hWnd) {
			return E_FAIL;
		}

		ShowWindow(_hWnd, _nCmdShow);

		return S_OK;
	}

	INT_PTR CALLBACK MainWindow::About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
	{
		UNREFERENCED_PARAMETER(lParam);
		switch (message)
		{
		case WM_INITDIALOG:
			return (INT_PTR)TRUE;

		case WM_COMMAND:
			if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
			{
				EndDialog(hDlg, LOWORD(wParam));
				return (INT_PTR)TRUE;
			}
			break;
		}
		return (INT_PTR)FALSE;
	}

	void MainWindow::OpenROM()
	{
		wchar_t buffer[2000];

		OPENFILENAME ofn;
		ZeroMemory(&ofn , sizeof(ofn));
		ofn.lStructSize = sizeof(ofn);
		ofn.hwndOwner = nullptr;
		ofn.lpstrFile = buffer;
		ofn.lpstrFile[0] = '\0';
		ofn.nMaxFile = sizeof(buffer);
		ofn.lpstrFilter = L"NES Roms\0*.NES\0All\0*.*";
		ofn.nFilterIndex = 1;
		ofn.lpstrFileTitle = nullptr;
		ofn.nMaxFileTitle = 0 ;
		ofn.lpstrInitialDir= nullptr;
		ofn.Flags = OFN_PATHMUSTEXIST|OFN_FILEMUSTEXIST ;

		GetOpenFileName(&ofn);
		
		wstring filename = wstring(buffer);

		if(filename.length() > 0) {
			Stop();

			_console.reset(new Console(filename));
			std::thread nesThread(&Console::Run, _console.get());
			nesThread.detach();
		}
	}

	void MainWindow::Stop()
	{
		if(_console) {
			_console->Stop();
			_console.release();
		}
	}

	LRESULT CALLBACK MainWindow::WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
	{
		PAINTSTRUCT ps;
		int wmId, wmEvent;
		HDC hdc;

		switch(message) {
			case WM_COMMAND:
				wmId    = LOWORD(wParam);
				wmEvent = HIWORD(wParam);
				// Parse the menu selections:
				switch (wmId)
				{
					case IDM_FILE_OPEN:
						MainWindow::GetInstance()->OpenROM();
						break;
					case IDM_FILE_RUNTESTS:
						
						break;
					case IDM_ABOUT:
						DialogBox(nullptr, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
						break;
					case IDM_EXIT:
						DestroyWindow(hWnd);
						break;
					default:
						return DefWindowProc(hWnd, message, wParam, lParam);
				}
				break;
			case WM_PAINT:
				hdc = BeginPaint(hWnd, &ps);
				EndPaint(hWnd, &ps);
				break;

			case WM_DESTROY:
				MainWindow::GetInstance()->Stop();
				PostQuitMessage(0);
				break;

			default:
				return DefWindowProc(hWnd, message, wParam, lParam);
		}

		return 0;
	}

}