// Window.cpp : Определяет точку входа для приложения.
//

#include "framework.h"
#include "Window.h"

#include <windowsx.h>
#include <tlhelp32.h>
#include <tchar.h>
#include <stdarg.h>

#define DYNAMIC_DLL_IMPORT

#ifndef DYNAMIC_DLL_IMPORT
#include "MemChanger.h"
#include "GUI.h"
#endif

#define MAX_LOADSTRING 2

// Глобальные переменные:
HINSTANCE hInst;                                // текущий экземпляр
WCHAR szTitle[MAX_LOADSTRING];                  // Текст строки заголовка
WCHAR szWindowClass[MAX_LOADSTRING];            // имя класса главного окна

#define DLL_NAME "D:\\MemoryChangingLib.dll"

#define IDM_GO 9275
#define IDM_INJECT 9276

HWND hWnd;

HWND hButGo;
HWND hButInject;
HWND pidNo;

int k;
unsigned long searchPid;
//Локальные функции:
int injectDLLIntoProcess(unsigned long pid);
void setWindowElements(HWND hWnd);
int getInputStrings();
int getWStrigFromWindow(HWND window, std::wstring* str);
WCHAR* charToWchar(const char* str);
char* WideStringToAnsi(std::wstring& Str, unsigned int CodePage = CP_ACP);
unsigned long WideStringToULong(std::wstring& Str, unsigned int CodePage);
int GetStrLength(char* str);

WCHAR* NumToText(DWORD pid) {
	int pidLength;
	unsigned long inversedPid = 0;
	for (pidLength = 0; pid; pidLength++) {
		inversedPid *= 10;
		inversedPid += pid % 10;
		pid /= 10;
	}
	WCHAR* res = new WCHAR[pidLength + 1]; int i;
	for (i = 0; inversedPid; i++) {
		res[i] = '0' + (inversedPid % 10);
		inversedPid /= 10;
	}
	for (; i < pidLength; i++) {
		res[i] = '0';
	}
	res[pidLength] = '\0';
	return res;
}

LRESULT CALLBACK WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	HDC hDC;
	switch (uMsg) {
		case WM_CREATE:
			setWindowElements(hWnd);
			break;
		case WM_DESTROY:
			PostQuitMessage(0);
			break;
		case WM_COMMAND:
		{
			// Разобрать выбор:
			switch (LOWORD(wParam))
			{
				case IDM_GO:
					{
#ifndef DYNAMIC_DLL_IMPORT
						createWindow();
#endif
#ifdef DYNAMIC_DLL_IMPORT
						LoadLibrary(L"D:\\MemoryChangingLib.dll");
#endif
						break;
					}
				case IDM_INJECT:
					{
						try {
							getInputStrings();
							if (!injectDLLIntoProcess(searchPid)) {
								
								MessageBox(hWnd, L"Can not inject DLL", NumToText(GetLastError()) , MB_ICONERROR);
							}
						}
						catch (...) {
							MessageBox(nullptr, L"Injecting error!", L"Error", MB_ICONERROR);
						}
						break;
					}
				default:
					return DefWindowProc(hWnd, uMsg, wParam, lParam);
			}
		}
	}
	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

int APIENTRY wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR  lpCmdLine, _In_ int  nCmdShow) {
	hInst = hInstance;
	WNDCLASSEX wcex;
	MSG msg;
	memset(&wcex, 0, sizeof(WNDCLASSEX));
	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.style = 0;
	wcex.lpfnWndProc = WindowProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hInstance;
	wcex.hIcon = 0;
	wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wcex.lpszMenuName = NULL;
	wcex.lpszClassName = L"MyWindowClass";
	wcex.hIconSm = 0;
	RegisterClassEx(&wcex);

	hWnd = CreateWindowEx(0, L"MyWindowClass", L"Memory Rewriter", WS_OVERLAPPEDWINDOW | WS_VISIBLE, 0, 0, 400, 180, NULL, NULL, hInstance, NULL);

	while (GetMessage(&msg, 0, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	return msg.wParam;

	return 0;
}

void setWindowElements(HWND hWnd) {
	hButGo = CreateWindow(
		L"BUTTON",
		L"Go",
		WS_CHILD | WS_BORDER | WS_VISIBLE | BS_PUSHBUTTON,
		90, 100, 100, 30, hWnd, reinterpret_cast<HMENU>(IDM_GO), nullptr, nullptr
	);

	hButInject = CreateWindow(
		L"BUTTON",
		L"Inject",
		WS_CHILD | WS_BORDER | WS_VISIBLE | BS_PUSHBUTTON,
		190, 100, 100, 30, hWnd, reinterpret_cast<HMENU>(IDM_INJECT), nullptr, nullptr
	);

	pidNo = CreateWindow(
		L"EDIT",
		L"0",
		WS_CHILD | WS_BORDER | WS_VISIBLE | ES_AUTOHSCROLL,
		10, 0, 360, 20, hWnd, reinterpret_cast<HMENU>(1), nullptr, nullptr
	);
}

int injectDLLIntoProcess(unsigned long pid) {
	HANDLE process = OpenProcess(PROCESS_ALL_ACCESS, false, pid);
	if (process == NULL) return false;

	// "Вытягивание" функции из системной библиотеки для динамической  
	// подгрузки DLL в адресное пространство открытого процесса
	LPVOID fp = (LPVOID)GetProcAddress(GetModuleHandle(L"kernel32.dll"), "LoadLibraryA");
	if (fp == NULL) return false;

	// Выделение участка памяти размером strlen(_dll_name) для последующей 
	// записи имени библеотеки в память процесса.
	LPVOID alloc = (LPVOID)VirtualAllocEx(process, 0, strlen(DLL_NAME), MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
	if (alloc == NULL) return false;

	// Запись имени инжектируемой DLL в память
	BOOL w = WriteProcessMemory(process, (LPVOID)alloc, DLL_NAME, strlen(DLL_NAME), 0);
	if (w == NULL) return false;

	// Создание "удаленного" потока в адресном пространстве
	// открытого процесса и последующая подгрузка нашей DLL.
	HANDLE thread = CreateRemoteThread(process, 0, 0, (LPTHREAD_START_ROUTINE)fp, (LPVOID)alloc, 0, 0);
	if (thread == NULL) {
		MessageBox(hWnd, L"Can not inject DLL", NumToText(GetLastError()), MB_ICONERROR);
		return false;
	}

	CloseHandle(process);
	return true;
}

int getInputStrings() {
	std::wstring pid{};
	std::wstring search{};
	std::wstring replace{};

	if (getWStrigFromWindow(pidNo, &pid)) return -1;

	searchPid = WideStringToULong(pid, 0);
}

int getWStrigFromWindow(HWND window, std::wstring* str) {
	str->resize(MAX_PATH);
	GetWindowText(window, &(*str)[0], 20);
	str->erase(remove(begin(*str), end(*str), 0), end(*str));

	if (str->empty()) {
		MessageBox(hWnd, L"Input missed!", L"Err", MB_ICONINFORMATION);
		return -1;
	}
}

unsigned long WideStringToULong(std::wstring& Str, unsigned int CodePage) {
	char* tempAnsiStr = WideStringToAnsi(Str, CodePage);

	unsigned long res = 0;
	for (int i = 0; tempAnsiStr[i] != '\0'; i++) {
		res *= 10;
		res += tempAnsiStr[i] - '0';
	}
	return res;
}

char* WideStringToAnsi(std::wstring& Str, unsigned int CodePage)
{
	DWORD BuffSize = WideCharToMultiByte(CodePage, 0, Str.c_str(), -1, NULL, 0, NULL, NULL);
	if (!BuffSize)
		return NULL;
	char* Buffer = new char[BuffSize];

	if (!WideCharToMultiByte(CodePage, 0, Str.c_str(), -1, Buffer, BuffSize, NULL, NULL))
		return NULL;
	return (Buffer);
}