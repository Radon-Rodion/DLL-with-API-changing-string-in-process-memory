#include "pch.h" // use stdafx.h in Visual Studio 2017 and earlier
#include <utility>
#include <limits.h>
#include "MemChanger.h"

int fMatchCheck(char* mainstr, int mainstrLen, char* checkstr, int checkstrLen) {
    /*
    Проверка наличия подстроки в строке.
    При этом под "строкой" подразумевается
    просто последовательность байт.
    */
    BOOL fmcret = TRUE;
    int x, y;

    for (x = 0; x < mainstrLen; x++) {
        fmcret = TRUE;

        for (y = 0; y < checkstrLen; y++) {
            if (checkstr[y] != mainstr[x + y]) {
                fmcret = FALSE;
                break;
            }
        }

        if (fmcret)
            return x + checkstrLen;
    }
    return -1;
}

char* getMem(char* buff, size_t buffLen, int from, int to) {
    /*
    Выделяем у себя память, в которой будем хранить
    копию данных из памяти чужой программы.
    */
    size_t ourSize = buffLen * 2;
    char* ret = (char*)malloc(ourSize);

    if (!ret) return ret;

    memset(ret, 0, ourSize);

    memcpy(ret, &buff[from], buffLen - from);
    memset(&ret[to - from], 0, to - from);

    return ret;
}

char* delMem(char* buff, size_t buffLen, int from, int to) {
    /*
    Освобождаем память.
    */
    size_t ourSize = buffLen * 2;
    char* ret = (char*)malloc(ourSize);
    int i, x = 0;
    if (!ret) return ret;

    memset(ret, 0, ourSize);

    for (i = 0; i < buffLen; i++) {
        if (!(i >= from && i < to)) {
            ret[x] = buff[i];
            x++;
        }
    }

    return ret;
}

char* addMem(char* buff, size_t buffLen, char* buffToAdd, size_t addLen, int addFrom) {
    /*
    Запись в память.
    */
    size_t ourSize = (buffLen + addLen) * 2;
    char* ret = (char*)malloc(ourSize);
    int i, x = 0;

    if (!ret) return ret;

    memset(ret, 0, ourSize);

    memcpy(ret, getMem(buff, buffLen, 0, addFrom), addFrom);

    x = 0;
    for (i = addFrom; i < addLen + addFrom; i++) {
        ret[i] = buffToAdd[x];
        x++;
    }

    x = 0;
    for (i; i < addFrom + buffLen; i++) {
        ret[i] = buff[addFrom + x];
        x++;
    }

    return ret;
}

char* replaceMem(char* buff, size_t buffLen, int from, int to, char* replaceBuff, size_t replaceLen) {
    /*
    Заменяем найденную "строку" на свою.
    */
    size_t ourSize = (buffLen) * 2;
    char* ret = (char*)malloc(ourSize);

    if (!ret) return ret;

    memset(ret, 0, ourSize);

    memcpy(ret, buff, buffLen); // copy 'buff' into 'ret'

    ret = delMem(ret, buffLen, from, to); // delete all memory from 'ret' betwen 'from' and 'to'
    ret = addMem(ret, buffLen - to + from, replaceBuff, replaceLen, from);

    return ret;
}

BOOL DoRtlAdjustPrivilege() {
    /*
    Важная функция. Получаем привилегии дебаггера.
    Именно это позволит нам получить нужную информацию
    о доступности памяти.
    */
#define SE_DEBUG_PRIVILEGE 20L
#define AdjustCurrentProcess 0
    BOOL bPrev = FALSE;
    LONG(WINAPI * RtlAdjustPrivilege)(DWORD, BOOL, INT, PBOOL);
    *(FARPROC*)&RtlAdjustPrivilege = GetProcAddress(GetModuleHandle(L"ntdll.dll"), "RtlAdjustPrivilege");
    if (!RtlAdjustPrivilege) return FALSE;
    RtlAdjustPrivilege(SE_DEBUG_PRIVILEGE, TRUE, AdjustCurrentProcess, &bPrev);
    return TRUE;
}

int replaceStringInProcessMemory(long pid, char* lpOrig, char* lpReplacement, int lpOrigLength) {
    /*** VARIABLES ***/
    HANDLE hProc;

    MEMORY_BASIC_INFORMATION mbi;
    SYSTEM_INFO msi;
    ZeroMemory(&mbi, sizeof(mbi));
    GetSystemInfo(&msi);
    /*
    Получаем информацию о памяти в текущей системе.
    */

    DWORD dwRead = 0;

    char* lpData = (char*)GlobalAlloc(GMEM_FIXED, MAX_READ);

    int x, at;
    /*****************/

    if (!lpData)
        return -1;

    ZeroMemory(lpData, MAX_READ);

    // открываем процесс
    hProc = OpenProcess(PROCESS_ALL_ACCESS,
        FALSE, pid);
    if (!hProc) {
        //Cant open process!
        return -1;
    }

    if (DoRtlAdjustPrivilege()) {
        /*
        Привилегии отладчика для работы с памятью.
        */

        //Process opened sucessfully. Scanning memory

        for (LPBYTE lpAddress = (LPBYTE)msi.lpMinimumApplicationAddress; lpAddress <= (LPBYTE)msi.lpMaximumApplicationAddress; lpAddress += mbi.RegionSize) {
            /*
            Этот цикл отвечает как раз за то, что наша программа не совершит
            лишних действий. Память в Windows в процессе делится на "регионы".
            У каждого региона свой уровень доступа: к какому-то доступ запрещен,
            какой-то можно только прочитать. Нам нужны регионы доступные для записи.
            Это позволит в разы ускорить работу поиска по памяти и избежать ошибок
            записи в память. Именно так работает ArtMoney.
            */

            if (VirtualQueryEx(hProc, lpAddress, &mbi, sizeof(mbi))) {
                /*
                Узнаем о текущем регионе памяти.
                */

                if ((mbi.Protect & PAGE_READWRITE) || (mbi.Protect & PAGE_WRITECOPY)) {
                    /*
                    Если он доступен для записи, работаем с ним.
                    */

                    for (lpAddress; lpAddress < (lpAddress + mbi.RegionSize); lpAddress += MAX_READ) {
                        /*
                        Проходим по адресам указателей в памяти чужого процесса от начала, до конца региона
                        и проверяем, не в них ли строка поиска.
                        */

                        dwRead = 0;
                        if (ReadProcessMemory(hProc,
                            (LPCVOID)lpAddress,
                            lpData,
                            MAX_READ, &
                            dwRead) == TRUE) {
                            /*
                            Читаем по 128 байт из памяти чужого процесса от начала
                            и проверяем, не в них ли строка поиска.
                            */

                            if (fMatchCheck(lpData, dwRead, lpOrig, lpOrigLength) != -1) {
                                /*Нашли, сообщим об успехе и поменяем в чужом процессе искомую строку на нашу.*/

                                at = fMatchCheck(lpData,
                                    dwRead,
                                    lpOrig,
                                    lpOrigLength);

                                if (at != -1) {
                                    at -= lpOrigLength;

                                    lpData = replaceMem(lpData,
                                        dwRead,
                                        at,
                                        at + lpOrigLength,
                                        lpReplacement,
                                        /*sizeof(lpReplacement)-1*/
                                        lpOrigLength);

                                    if (!WriteProcessMemory(hProc,
                                        (LPVOID)lpAddress,
                                        lpData,
                                        /*dwRead-sizeof(lpOrig)-1+sizeof(lpReplacement)-1*/
                                        dwRead, &
                                        dwRead)) return -1;
                                }
                                if (hProc)
                                    CloseHandle(hProc);
                                return 0;
                            }
                            else {
                                //puts("Error: not found in: ");
                                //puts(lpData);
                                //puts("\n");
                            }

                        }
                    }

                }
            }
        }
    }

    // // // // //
    if (hProc)
        CloseHandle(hProc);
    /*if (lpData)
        GlobalFree(lpData);*/
        ///////////////
    return 1;
}