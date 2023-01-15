/*
	This project serves as a simple demonstration for the article "Combining Raw Input and keyboard Hook to selectively block input from multiple keyboards",
	which you should be able to find in this project folder (HookingRawInput.html), or on the CodeProject website (http://www.codeproject.com/).
	The project is based on the idea shown to me by Petr Medek (http://www.hidmacros.eu/), and is published with his permission, huge thanks to Petr!
	The source code is licensed under The Code Project Open License (CPOL), feel free to adapt it.
	Vít Blecha (sethest@gmail.com), 2014
*/

#include "stdafx.h"
#include <deque>
#include "HookingRawInputDemo.h"
#include "HookingRawInputDemoDLL.h"
#include "SoundboardDisplay.h"

#include <boost/ui.hpp>
#include <thread>

HWND mainHwnd;
namespace ui = boost::ui;

SoundboardDisplay* display = nullptr;

int ui_main()
{
	display = new SoundboardDisplay();
	display->show_modal();

	SoundboardDisplay* tmp = display;
	display = nullptr;
	delete tmp;

	SendMessage(mainHwnd, WM_DESTROY, 0, 0);
		
	return 0;
}

int main(int argc, char* argv[])
{
	// Initialize GUI, call ui_main function, uninitialize GUI, catch exceptions
	return ui::entry(&ui_main, argc, argv);
}

bool processRawInput(USHORT vKey, HANDLE device, USHORT keyPressed) {
	//// ESC =>
	//// vKey == 27;
	//if (vKey == 13 && device == (HANDLE)0x000000000001003F)
	//{
	//	// TODO : Send play sound message to octopus
	//	if (keyPressed)
	//		OutputDebugString(L"Do smth\n"); 

	//	return true;
	//}dsq
	//else {
	//	return false;
	//}


	if (display) {
		if (display->processInput(vKey, device, keyPressed)) {
			return true;
		}
	}

	return false;
}

#define MAX_LOADSTRING 100

// Global Variables:
HINSTANCE hInst;								// current instance
TCHAR szTitle[MAX_LOADSTRING];					// The title bar text
TCHAR szWindowClass[MAX_LOADSTRING];			// the main window class name

UINT const WM_HOOK = WM_APP + 1;
std::deque<DecisionRecord> decisionBuffer;

// Forward declarations of functions included in this code module:
ATOM				MyRegisterClass(HINSTANCE hInstance);
BOOL				InitInstance(HINSTANCE, int);
LRESULT CALLBACK	WndProc(HWND, UINT, WPARAM, LPARAM);

int APIENTRY _tWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPTSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
	//char* test = "test.exe";
	std::thread t1 (main, 0, nullptr);
	//return main(1, &test);

	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

 	// TODO: Place code here.
	MSG msg;
	HACCEL hAccelTable;

	// Initialize global strings
	LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadString(hInstance, IDC_HOOKINGRAWINPUTDEMO, szWindowClass, MAX_LOADSTRING);
	MyRegisterClass(hInstance);

	// Perform application initialization:
	if (!InitInstance (hInstance, nCmdShow))
	{
		return FALSE;
	}

	hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_HOOKINGRAWINPUTDEMO));

	// Main message loop:
	while (GetMessage(&msg, NULL, 0, 0))
	{
		if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}
	
	t1.join();
	return (int) msg.wParam;
}



//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEX wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style			= CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc	= WndProc;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= 0;
	wcex.hInstance		= hInstance;
	wcex.hIcon			= LoadIcon(hInstance, MAKEINTRESOURCE(IDI_HOOKINGRAWINPUTDEMO));
	wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground	= (HBRUSH)(COLOR_WINDOW+1);
	wcex.lpszMenuName	= MAKEINTRESOURCE(IDC_HOOKINGRAWINPUTDEMO);
	wcex.lpszClassName	= szWindowClass;
	wcex.hIconSm		= LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

	return RegisterClassEx(&wcex);
}

//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
	HWND hWnd;

	hInst = hInstance; // Store instance handle in our global variable

	hWnd = CreateWindow (szWindowClass, L"Soundboard_KbHook", WS_OVERLAPPEDWINDOW | WS_VSCROLL, CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, NULL, NULL, hInstance, NULL);

	if (!hWnd)
	{
		return FALSE;
	}

	// Save the HWND
	mainHwnd = hWnd;

	//ShowWindow(hWnd, SW_HIDE);
	//UpdateWindow(hWnd);

	// Register for receiving Raw Input for keyboards
	RAWINPUTDEVICE rawInputDevice[1];
	rawInputDevice[0].usUsagePage = 1;
	rawInputDevice[0].usUsage = 6;
	rawInputDevice[0].dwFlags = RIDEV_INPUTSINK;
	rawInputDevice[0].hwndTarget = hWnd;
	if (!RegisterRawInputDevices(rawInputDevice, 1, sizeof(rawInputDevice[0])))
		return false;

	// Set up the keyboard Hook
	InstallHook (hWnd);

	return TRUE;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;
	PAINTSTRUCT ps;
	HDC hdc;

	switch (message)
	{
	// Raw Input Message
	case WM_INPUT:
	{
		UINT bufferSize;

		// Prepare buffer for the data
		GetRawInputData ((HRAWINPUT)lParam, RID_INPUT, NULL, &bufferSize, sizeof (RAWINPUTHEADER));
		LPBYTE dataBuffer = new BYTE[bufferSize];
		// Load data into the buffer
		GetRawInputData((HRAWINPUT)lParam, RID_INPUT, dataBuffer, &bufferSize, sizeof (RAWINPUTHEADER));

		RAWINPUT* raw = (RAWINPUT*)dataBuffer;
		
		decisionBuffer.push_back (DecisionRecord (raw->data.keyboard.VKey, processRawInput(raw->data.keyboard.VKey, raw->header.hDevice, raw->data.keyboard.Flags & 0x1 ? 0 : 1)));

		delete[] dataBuffer;
		return 0;
	}

	// Message from Hooking DLL
	case WM_HOOK:
	{
		
		USHORT virtualKeyCode = (USHORT)wParam;
		USHORT keyPressed = lParam & 0x80000000 ? 0 : 1;

		// Check the buffer if this Hook message is supposed to be blocked; return 1 if it is
		BOOL blockThisHook = FALSE;
		BOOL recordFound = FALSE;
		int index = 1;
		if (!decisionBuffer.empty ())
		{
			// Search the buffer for the matching record
			std::deque<DecisionRecord>::iterator iterator = decisionBuffer.begin ();
			while (iterator != decisionBuffer.end ())
			{
				if (iterator->virtualKeyCode == virtualKeyCode)
				{
					blockThisHook = iterator->decision;
					recordFound = TRUE;
					// Remove this and all preceding messages from the buffer
					for (int i = 0; i < index; ++i)
					{
						decisionBuffer.pop_front ();
					}
					// Stop looking
					break;
				}
				++iterator;
				++index;
			}
		}

		if (!recordFound) {
			// Wait for the matching Raw Input message if the decision buffer was empty or the matching record wasn't there
			MSG rawMessage;
			while (PeekMessage(&rawMessage, mainHwnd, WM_INPUT, WM_INPUT, PM_REMOVE))
			{
				// The Raw Input message has arrived; decide whether to block the input
				UINT bufferSize;

				// Prepare buffer for the data
				GetRawInputData((HRAWINPUT)rawMessage.lParam, RID_INPUT, NULL, &bufferSize, sizeof(RAWINPUTHEADER));
				LPBYTE dataBuffer = new BYTE[bufferSize];
				// Load data into the buffer
				GetRawInputData((HRAWINPUT)rawMessage.lParam, RID_INPUT, dataBuffer, &bufferSize, sizeof(RAWINPUTHEADER));

				RAWINPUT* raw = (RAWINPUT*)dataBuffer;

				if (virtualKeyCode != raw->data.keyboard.VKey)
				{
					decisionBuffer.push_back(DecisionRecord(raw->data.keyboard.VKey, processRawInput(raw->data.keyboard.VKey, raw->header.hDevice, keyPressed)));
				}
				else
				{
					blockThisHook = processRawInput(raw->data.keyboard.VKey, raw->header.hDevice, keyPressed);
				}
				delete[] dataBuffer;
			}
		}
		
		return blockThisHook;
	}

	case WM_COMMAND:
		wmId    = LOWORD(wParam);
		wmEvent = HIWORD(wParam);
		// Parse the menu selections:
		switch (wmId)
		{
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
		UninstallHook ();
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}