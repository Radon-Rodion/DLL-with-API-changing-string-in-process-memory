#pragma once
#ifdef MEMORYCHANGINGLIB_EXPORTS
#define MEMORYCHANGINGLIB_API __declspec(dllexport)
#else
#define MEMORYCHANGINGLIB_API __declspec(dllimport)
#endif

#include <windows.h>
#include <tlhelp32.h>

#define MAX_READ 128

 extern "C" MEMORYCHANGINGLIB_API int replaceStringInProcessMemory(long pid, char* lpOrig, char* lpReplacement, int lpOrigLength);