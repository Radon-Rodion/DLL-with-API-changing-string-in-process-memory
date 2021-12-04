#include "pch.h" // use stdafx.h in Visual Studio 2017 and earlier
#include "GUI.h"
#include "MemChanger.h"

#define IDM_GO_DLL 9375

#define STRING_IN_MEMORY_LENGTH 20//85

HWND hWnd;

HWND hButGo;
HWND pidNo;
HWND searchText;
HWND replacementText;

int k;
unsigned long searchPid;
char* searchStr;
char* replaceStr;
int searchStrLength;

void setWindowElements(HWND hWnd);
int getInputStrings();
int getWStrigFromWindow(HWND window, std::wstring* str);
WCHAR* charToWchar(const char* str);
char* WideStringToAnsi(std::wstring& Str, unsigned int CodePage = CP_ACP);
unsigned long WideStringToULong(std::wstring& Str, unsigned int CodePage);
int GetStrLength(char* str);

LRESULT CALLBACK DLLWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
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
		case IDM_GO_DLL:
		{
			getInputStrings();
			try {
				int res = replaceStringInProcessMemory(searchPid, searchStr, replaceStr, searchStrLength);
				if (res == -1) MessageBox(nullptr, L"Error changing string!", L"Error", MB_ICONERROR);
				if (res == 1) MessageBox(nullptr, L"String not found!", L"Warning", MB_ICONWARNING);
			}
			catch (...) {
				MessageBox(nullptr, L"Memory processing error!", L"Error", MB_ICONERROR);
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

void createWindow() {
	//MessageBox(nullptr, L"Injecting success!", L"Success", MB_ICONINFORMATION);
	WNDCLASSEX wcex;
	MSG msg;
	memset(&wcex, 0, sizeof(WNDCLASSEX));
	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.style = 0;
	wcex.lpfnWndProc = DLLWindowProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	//wcex.hInstance = hInstance;
	wcex.hIcon = 0;
	wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wcex.lpszMenuName = NULL;
	wcex.lpszClassName = L"DLLWindowClass";
	wcex.hIconSm = 0;
	RegisterClassEx(&wcex);

	hWnd = CreateWindowEx(0, L"DLLWindowClass", L"Memory RewriterDLL", WS_OVERLAPPEDWINDOW | WS_VISIBLE, 0, 0, 400, 180, NULL, NULL, NULL/*hInstance*/, NULL);

	while (GetMessage(&msg, 0, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
}

void setWindowElements(HWND hWnd) {
	hButGo = CreateWindow(
		L"BUTTON",
		L"Go",
		WS_CHILD | WS_BORDER | WS_VISIBLE | BS_PUSHBUTTON,
		90, 100, 100, 30, hWnd, reinterpret_cast<HMENU>(IDM_GO_DLL), nullptr, nullptr
	);

	pidNo = CreateWindow(
		L"EDIT",
		L"0",
		WS_CHILD | WS_BORDER | WS_VISIBLE | ES_AUTOHSCROLL,
		10, 0, 360, 20, hWnd, reinterpret_cast<HMENU>(1), nullptr, nullptr
	);

	searchText = CreateWindow(
		L"EDIT",
		L"AAAaaaBBBbbbCCC",
		WS_CHILD | WS_BORDER | WS_VISIBLE | ES_AUTOHSCROLL,
		10, 30, 360, 20, hWnd, reinterpret_cast<HMENU>(2), nullptr, nullptr
	);

	replacementText = CreateWindow(
		L"EDIT",
		L"Replacement",
		WS_CHILD | WS_BORDER | WS_VISIBLE | ES_AUTOHSCROLL,
		10, 60, 360, 20, hWnd, reinterpret_cast<HMENU>(3), nullptr, nullptr
	);
}

int getInputStrings() {
	std::wstring pid{};
	std::wstring search{};
	std::wstring replace{};

	if (getWStrigFromWindow(pidNo, &pid)) return -1;
	if (getWStrigFromWindow(searchText, &search)) return -1;
	if (getWStrigFromWindow(replacementText, &replace)) return -1;

	searchPid = WideStringToULong(pid, 0);
	searchStr = WideStringToAnsi(search, 0);
	replaceStr = WideStringToAnsi(replace, 0);
	searchStrLength = GetStrLength(searchStr);
}

int getWStrigFromWindow(HWND window, std::wstring* str) {
	str->resize(MAX_PATH);
	GetWindowText(window, &(*str)[0], STRING_IN_MEMORY_LENGTH);
	str->erase(remove(begin(*str), end(*str), 0), end(*str));

	if (str->empty()) {
		MessageBox(hWnd, L"Input missed!", L"Err", MB_ICONINFORMATION);
		return -1;
	}
}

/*  GetCurrentProcessId()  */
int GetStrLength(char* str) {
	int i;
	for (i = 0; str[i]; i++);
	return i;
}

WCHAR* charToWchar(const char* str) {
	int i;
	for (i = 0; str[i] != 0; i++) {}
	WCHAR* res = new WCHAR[i + 1];
	for (i = 0; str[i] != 0; i++) {
		res[i] = str[i];
	}
	res[i] = '\0';
	return res;
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

bool isInited = false;

bool libHasBeenInitialized() {
	if (!isInited) {
		isInited = true;
		return false;
	}
	return true;
}