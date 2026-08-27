/*
 * compare file timestamps
 */
#include <stdio.h>
#include "Windows.h"
#include "fileapi.h"
#include "errhandlingapi.h"
#include <tchar.h>
#include <shlwapi.h> // TODO: use Raylib GetFileModTime instead of windows-specific stuff

/**
 * print usage message to stdout
 */
void print_usage(const char* name) {
    printf("USAGE: %s PATH1 PATH2\n\nExits with code 1 if the file at PATH1 was modified more recently than the one at PATH2, or if one of them does not exist. Exits with code 0 otherwise.\n\n", name);
}

/**
 * print usage message to stderr
 */
void print_last_error() {
    DWORD err_code = GetLastError();
    LPVOID lpMsgBuf;
    DWORD fmt_msg_res = FormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | 
        FORMAT_MESSAGE_FROM_SYSTEM |
        FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        err_code,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPTSTR) &lpMsgBuf,
        0, NULL
    );
    if (fmt_msg_res==0) {
        fprintf(stderr, "unknown system error");
        return;
    }
    _ftprintf(stderr, (LPCTSTR)lpMsgBuf);
    LocalFree(lpMsgBuf);
}

/**
 * Check time of the most recent change to a file.
 * Returns non-zero on success. If it fails, you can use GetLastError() to learn more.
 */
BOOL get_m_time(const char* path, LPFILETIME lpTime) {
    if (lpTime == NULL) {
        return 1;
    }
    HANDLE file = CreateFileA(path, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (file == INVALID_HANDLE_VALUE) {
        return 0;
    }
    if (0 == GetFileTime(file, NULL, NULL, lpTime)) {
        return 0;
    }
    return CloseHandle(file);
}

/**
 * entry point
 */
int main(int argc, char ** argv) {
    if (argc != 3) {
        print_usage(argv[0]);
        return 1;
    }
    char *path1 = argv[1];
    char *path2 = argv[2];
    if (!PathFileExistsA(path1) || PathIsDirectoryA(path1)) {
        return 1;
    }
    if (!PathFileExistsA(path2) || PathIsDirectoryA(path2)) {
        return 1;
    }
    FILETIME time1;
    if (!get_m_time(path1, &time1)) {
        print_last_error();
        return 0;
    }
    FILETIME time2;
    if (!get_m_time(path2, &time2)) {
        print_last_error();
        return 0;
    }
    if (time1.dwHighDateTime > time2.dwHighDateTime) {
        return 1;
    }
    if (time1.dwHighDateTime < time2.dwHighDateTime) {
        return 0;
    }
    return time1.dwLowDateTime > time2.dwLowDateTime;
}