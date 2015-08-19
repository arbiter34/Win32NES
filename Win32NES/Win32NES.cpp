// Win32NES.cpp : Defines the entry point for the application.
//
#include "stdafx.h"
#include <conio.h>
#include <Windows.h>
#include <ShObjIdl.h>
#include <ShlObj.h>
#include <shlwapi.h>
#include <mmsystem.h>
#include "Win32NES.h"
#include "CPU.h"
#include "PPU.h"
#include "Cartridge.h"
#include "Controller.h"

#define MAX_LOADSTRING 100

#define HERTZ 1789773
#define TIMER_HERTZ 60

// Global Variables:
HINSTANCE hInst;								// current instance
TCHAR szTitle[MAX_LOADSTRING];					// The title bar text
TCHAR szWindowClass[MAX_LOADSTRING];			// the main window class name

MMRESULT cpuTimerId;
MMRESULT decrementTimerId;
MMRESULT updateScreenId;

CPU *cpu;
PPU *ppu;
Controller *controller;
Cartridge *cartridge;

PWSTR romPath;
static volatile BOOL running = false;
BOOL paused = false;

HANDLE threadHandle;
DWORD emulationThreadId;

// Forward declarations of functions included in this code module:
ATOM				MyRegisterClass(HINSTANCE hInstance);
BOOL				InitInstance(HINSTANCE, int);
LRESULT CALLBACK	WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK	About(HWND, UINT, WPARAM, LPARAM);
PWSTR LoadFile();
BOOL Run(HWND hWnd);
BOOL Restart(HWND hWnd);
BOOL Pause(HWND hWnd);
BOOL Stop(HWND hWnd);
void StartTimers(HWND hWnd);
void StopTimers();
void DrawScreen(HDC hdc);
void UpdateScreen();

DWORD WINAPI emulationThread(LPVOID lpParameter);

void CALLBACK cpuCycle(UINT uTimerID, UINT uMsg, DWORD_PTR dwUser, DWORD_PTR dw1, DWORD_PTR dw2);
void CALLBACK decrementTimers(UINT uTimerID, UINT uMsg, DWORD_PTR dwUser, DWORD_PTR dw1, DWORD_PTR dw2);
void CALLBACK updateScreen(UINT uTimerID, UINT uMsg, DWORD_PTR dwUser, DWORD_PTR dw1, DWORD_PTR dw2);
void OnKeyDown(WPARAM wParam);

int APIENTRY _tWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPTSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	ppu = new PPU();
	controller = new Controller();
	cartridge = new Cartridge();
	cpu = new CPU(ppu, controller, cartridge);
	ppu->setVar(cartridge, cpu);

	MSG msg;
	HACCEL hAccelTable;

	// Initialize global strings
	LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadString(hInstance, IDC_WIN32NES, szWindowClass, MAX_LOADSTRING);
	MyRegisterClass(hInstance);

	// Perform application initialization:
	if (!InitInstance (hInstance, nCmdShow))
	{
		return FALSE;
	}

	hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_WIN32NES));

	// Main message loop:
	while (GetMessage(&msg, NULL, 0, 0))
	{
		if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

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
	wcex.hIcon			= LoadIcon(hInstance, MAKEINTRESOURCE(IDI_WIN32NES));
	wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground	= (HBRUSH)(COLOR_WINDOW+1);
	wcex.lpszMenuName	= MAKEINTRESOURCE(IDC_WIN32NES);
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

   hWnd = CreateWindow(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT, 0, 550, 550, NULL, NULL, hInstance, NULL);

   if (!hWnd)
   {
      return FALSE;
   }

   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   return TRUE;
}

//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE:  Processes messages for the main window.
//
//  WM_COMMAND	- process the application menu
//  WM_PAINT	- Paint the main window
//  WM_DESTROY	- post a quit message and return
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;
	PAINTSTRUCT ps;
	HDC hdc;

	switch (message)
	{
	case WM_COMMAND:
		wmId    = LOWORD(wParam);
		wmEvent = HIWORD(wParam);
		// Parse the menu selections:
		switch (wmId)
		{
			case IDM_RUN:
				Run(hWnd);
				break;
			case IDM_PAUSE:
				Pause(hWnd);
				break;
			case IDM_RESTART:
				Restart(hWnd);
				break;
			case IDM_STOP:
				Stop(hWnd);
				break;
			case IDM_ABOUT:
				DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
				break;
			case IDM_OPEN:
				Stop(hWnd);
				romPath = LoadFile();
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
		if (running) {
			DrawScreen(hdc);
		}
		EndPaint(hWnd, &ps);
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

// Message handler for about box.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
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

PWSTR LoadFile() {
	IFileOpenDialog *pFileOpen;
	PWSTR pszFilePath = NULL;

	// Create the FileOpenDialog object.
	HRESULT hr = CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_ALL,
		IID_IFileOpenDialog, reinterpret_cast<void**>(&pFileOpen));
	if (SUCCEEDED(hr))
	{
		//IShellItem *psiDocuments = NULL;
		//hr = SHCreateItemInKnownFolder(FOLDERID_Documents, 0, NULL, IID_PPV_ARGS(&psiDocuments));

		//if (SUCCEEDED(hr)) {
		//	hr = pFileOpen->SetFolder(psiDocuments);
		//	psiDocuments->Release();
		//}
		// Show the Open dialog box.
		hr = pFileOpen->Show(NULL);

		// Get the file name from the dialog box.
		if (SUCCEEDED(hr))
		{
			IShellItem *pItem;
			hr = pFileOpen->GetResult(&pItem);
			if (SUCCEEDED(hr))
			{
				hr = pItem->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath);

				pItem->Release();
			}
		}
		pFileOpen->Release();
	}
	return pszFilePath;
}

BOOL Run(HWND hWnd) {
	if (running) {
		return false;
	}
	if (romPath == NULL) {
		romPath = LoadFile();
		if (romPath == NULL) {
			return false;
		}
		//romPath = L"C:\\Users\\alperst\\Documents\\Visual Studio 2013\\Projects\\Win32NES\\rom_singles\\01-basics.nes";
	}
	cartridge->loadRom(romPath);
	cpu->reset();
	running = true;
	paused = false;
	StartTimers(hWnd);
	return true;
}

BOOL Pause(HWND hWnd) {
	if (!running) {
		return false;
	}
	if (paused) {
		paused = !paused;
		StartTimers(hWnd);
	}
	else {
		StopTimers();
	}
	paused = !paused;
	return true;
}

BOOL Restart(HWND hWnd) {
	if (running) {
		Stop(hWnd);
		Sleep(1);
		Run(hWnd);
		return true;
	}
	return false;
}

BOOL Stop(HWND hWnd) {
	StopTimers();
	cpu->reset();
	InvalidateRect(hWnd, NULL, FALSE);
	running = false;
	return true;
}

void StartTimers(HWND hWnd) {
	updateScreenId = timeSetEvent(1000 / TIMER_HERTZ, 0, (LPTIMECALLBACK)&updateScreen, (DWORD)hWnd, TIME_PERIODIC | TIME_CALLBACK_FUNCTION | TIME_KILL_SYNCHRONOUS);
	threadHandle = CreateThread(0, 0, emulationThread, NULL, 0, &emulationThreadId);
}

void StopTimers() {
	timeKillEvent(cpuTimerId);
	timeKillEvent(updateScreenId);
}

void __cdecl odprintfs(const char *format, ...)
{
	char    buf[4096], *p = buf;
	va_list args;
	int     n;

	va_start(args, format);
	n = _vsnprintf(p, sizeof buf - 3, format, args); // buf-3 is room for CR/LF/NUL
	va_end(args);

	p += (n < 0) ? sizeof buf - 3 : n;

	while (p > buf  &&  isspace(p[-1]))
		*--p = '\0';

	*p++ = '\r';
	*p++ = '\n';
	*p = '\0';

	OutputDebugStringA(buf);
}

DWORD WINAPI emulationThread(LPVOID lpParameter) {
	LARGE_INTEGER StartingTime, EndingTime, ElapsedMicroseconds, Frequency, TestTime;
	QueryPerformanceFrequency(&Frequency);
	QueryPerformanceCounter(&TestTime);
	QueryPerformanceCounter(&StartingTime);
	while (running) {
		int ppuCycles = cpu->execute() * 3;
		for (int i = 0; i < ppuCycles; i++) {
			ppu->Step();
		}

		//High-res sleep
		//while ((QueryPerformanceCounter(&EndingTime)) && (((EndingTime.QuadPart - StartingTime.QuadPart) * 1000000000) / (Frequency.QuadPart * 1000)) < ((1000000000 / HERTZ) * (cpu->cycleCount - previousCycleCount))) {
		//	Sleep(0);
		//}
		//QueryPerformanceCounter(&StartingTime);
	}
	return 0;
}

void CALLBACK updateScreen(UINT uTimerID, UINT uMsg, DWORD_PTR dwUser, DWORD_PTR dw1, DWORD_PTR dw2) {
	InvalidateRect(((HWND)dwUser), NULL, FALSE);
}

void DrawScreen(HDC hdc)
{
	RECT rect;

	int border = 10;

	//Paint Background 
	GetClientRect(WindowFromDC(hdc), &rect);
	int width = abs(rect.left - rect.right);
	int height = abs(rect.top - rect.bottom);
	int widthFactor = width / SCREEN_WIDTH;
	int heightFactor = height / SCREEN_HEIGHT;

	HDC hdcMem = CreateCompatibleDC(hdc);
	HBITMAP hbmMem = CreateCompatibleBitmap(hdc, width, height);

	HANDLE hOld = SelectObject(hdcMem, hbmMem);

	HBRUSH brush = CreateSolidBrush(RGB(0, 0, 0));
	FillRect(hdcMem, &rect, brush);
	DeleteObject(brush);

	for (int i = 0; i < SCREEN_HEIGHT; i++) {
		for (int j = 0; j < SCREEN_WIDTH; j++) {
			int r = ppu->screen[i * SCREEN_WIDTH + j].r;
			int g = ppu->screen[i * SCREEN_WIDTH + j].g;
			int b = ppu->screen[i * SCREEN_WIDTH + j].b;
			HBRUSH brush = CreateSolidBrush(RGB(r, g, b));
			rect.left = border + j * widthFactor;
			rect.top = border + i * heightFactor;
			rect.bottom = rect.top + heightFactor;
			rect.right = rect.left + widthFactor;
			FillRect(hdcMem, &rect, brush);
			DeleteObject(brush);
		}
	}
	BitBlt(hdc, 0, 0, width, height, hdcMem, 0, 0, SRCCOPY);
	SelectObject(hdcMem, hOld);
	DeleteObject(hbmMem);
	DeleteDC(hdcMem);
	DeleteObject(brush);
}