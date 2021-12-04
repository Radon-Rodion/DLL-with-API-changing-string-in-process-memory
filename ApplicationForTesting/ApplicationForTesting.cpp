#include<Windows.h>
#include<WindowsX.h>
#include<stdio.h>

WCHAR* GetItsPidText();
WCHAR* charToWchar(const char* str);
char szText[] = "InformationAAAaaaBBBbbbCCCcccDdH!";

WCHAR* pidText;
//const char* lineInMemory = "This id first (not changed) string in memory.\nAAAaaaBBBbbbCCCcccDdH!";

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR lpCmdLine, INT show) {
	pidText = GetItsPidText();

	for (int i = 0; i < 10; i++) {
		MessageBox(NULL, charToWchar(szText), pidText, MB_ICONINFORMATION);//charToWchar(lineInMemory)
	}

	return 0;
}

WCHAR* GetItsPidText() {
	unsigned long pid = GetCurrentProcessId(); 
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