#pragma once
#ifdef MEMORYCHANGINGLIB_EXPORTS
#define MEMORYCHANGINGLIB_API __declspec(dllexport)
#else
#define MEMORYCHANGINGLIB_API __declspec(dllimport)
#endif

#include <windows.h>
// Файлы заголовков среды выполнения C
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include<memoryapi.h>
#include <tchar.h>
#include<processthreadsapi.h>
#include<string>
#include<stdio.h>

extern "C" MEMORYCHANGINGLIB_API void createWindow();
bool libHasBeenInitialized();