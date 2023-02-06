#include "httpd.h"
#include "utils.h"
#include <Dbghelp.h> 


#pragma comment(lib, "Dbghelp.lib")

LONG __stdcall crush_callback(struct _EXCEPTION_POINTERS* ep)
{
    time_t t = 0;
    struct tm *p = NULL;
    char fname[MAX_PATH2] = {0};
    MINIDUMP_EXCEPTION_INFORMATION    exceptioninfo;
    HANDLE hFile = INVALID_HANDLE_VALUE;
    int iSnprintRet = -1;

    t = time(NULL) + 8 * 3600;
    p = gmtime(&t);

    iSnprintRet = sprintf_s(fname, sizeof(fname), "dump_%d-%d-%d_%d_%d_%d.DMP", 1900+p->tm_year, 1+p->tm_mon, p->tm_mday, (p->tm_hour)%24, p->tm_min, p->tm_sec);
    ASSERT( 0 <= iSnprintRet );
    hFile = CreateFileA(fname,
        GENERIC_WRITE,
        0,
        NULL,
        CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        NULL);
    if (hFile == INVALID_HANDLE_VALUE)
    {
        return EXCEPTION_CONTINUE_SEARCH;
    }
    
    exceptioninfo.ExceptionPointers = ep;
    exceptioninfo.ThreadId          = GetCurrentThreadId();
    exceptioninfo.ClientPointers    = FALSE;

    if (!MiniDumpWriteDump(GetCurrentProcess(),
        GetCurrentProcessId(),
        hFile,
        MiniDumpWithFullMemory,
        &exceptioninfo,
        NULL,
        NULL))
    {
        return EXCEPTION_CONTINUE_SEARCH;
    }
    CloseHandle(hFile);
    return EXCEPTION_EXECUTE_HANDLER;
}

int main()
{
    UINT16 port = 80;

start_again:
    port = 80;
    SetUnhandledExceptionFilter(crush_callback);
    
    http_startup(&port);

    mySleep(2000);

    goto start_again;

    return 0;
}
