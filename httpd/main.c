
#include "httpd.h"
#include "utils.h"
#include <Dbghelp.h> 
#include "getopt.h"


#pragma comment(lib, "Dbghelp.lib")

LONG __stdcall crush_callback(struct _EXCEPTION_POINTERS* ep)
{
    time_t t = 0;
    struct tm m;
    struct tm *p = &m;
    char fname[MAX_PATH2] = {0};
    MINIDUMP_EXCEPTION_INFORMATION    exceptioninfo;
    HANDLE hFile = INVALID_HANDLE_VALUE;
    int iSnprintRet = -1;
    errno_t err = -1;

    ASSERT(NULL != ep);

    t = time(NULL);
    err = localtime_s(&m, &t);
    ASSERT( 0 == err );

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

Bool_t parse_command_line( int argc, char* argv[], charp2free_t* root_path, UINT16* port )
{
    int    verbose_flag = 0;
    Bool_t help = False;
    char*  root_path2 = NULL;
    UINT16 port2 = 80;

    const struct option long_options[]  =
    {
        { "verbose",   ARG_NONE, &verbose_flag,        1 },
        { "brief",     ARG_NONE, &verbose_flag,        0 },
        { "help",      ARG_NONE,             0,      'h' },
        { "root_path",  ARG_REQ,             0,      'r' },
        { "port", 	    ARG_REQ,             0,      'p' },
        { ARG_NULL,    ARG_NULL,      ARG_NULL, ARG_NULL }
    };

    while ( 1 )
    {
        int option_index = 0;
        const int c = getopt_long( argc, argv, "r:p:h", long_options, &option_index );

        // Check for end of operation or error
        if ( -1 == c )
        {
            break;
        }

        // Handle options
        switch ( c )
        {
        case 0:
            // If this option set a flag, do nothing else now.
            if ( 0 != long_options[option_index].flag )
            {
                break;
            }

            fprintf(stderr, "option %s", long_options[option_index].name );
            if ( optarg )
            {
                fprintf(stderr, " with argument %s", optarg );
            }
            fprintf(stderr, "\n" );
            break;

        case 'r':
            root_path2 = _strdup( optarg );
            break;

        case 'p':
            port2 = atoi( optarg );
            break;

        case 'h':
            help = True;
            break;

        case '?':
            // getopt_long already printed an error message.
            break;

        default:
            abort();
        }
    }

    if ( verbose_flag )
    {
        fprintf(stderr, "verbose flag is set\n" );
    }

    if ( optind < argc )
    {
        fprintf(stderr, "non-option ARGV-elements: " );
        while ( optind < argc )
        {
            fprintf(stderr, "%s ", argv[optind++] );
        }
        fprintf(stderr, "\n" );
    }

    if ( help )
    {
        fprintf(stderr, "httpd [-h/--help]\x20"
            "[-rRootPath/--root_path2 RootPath]\x20"
            "[-pPort/--port Port]\x20" );
        return False;
    }

    *root_path = root_path2;
    *port = port2;

    return True;
}

BOOL WINAPI ConsoleHandler(DWORD CEvent)
{
    switch(CEvent)
    {
    case CTRL_C_EVENT:
        MessageBox(NULL,
            "CTRL+C received!","CEvent",MB_OK);
        break;
    case CTRL_BREAK_EVENT:
        MessageBox(NULL,
            "CTRL+BREAK received!","CEvent",MB_OK);
        break;
    case CTRL_CLOSE_EVENT:
        _CrtDumpMemoryLeaks();
        MessageBox(NULL,
            "Program being closed!","CEvent",MB_OK);
        break;
    case CTRL_LOGOFF_EVENT:
        _CrtDumpMemoryLeaks();
        MessageBox(NULL,
            "User is logging off!","CEvent",MB_OK);
        break;
    case CTRL_SHUTDOWN_EVENT:
        _CrtDumpMemoryLeaks();
        MessageBox(NULL,
            "User is logging off!","CEvent",MB_OK);
        break;

    }
    return TRUE;
}

int main( int argc, char* argv[] )
{
    UINT16 port = 80;

    charp2free_t root_path2 = NULL;
    UINT16 port2 = INVALID_PORT;

    _CrtSetDbgFlag ( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );

    if (SetConsoleCtrlHandler(
        (PHANDLER_ROUTINE)ConsoleHandler,TRUE)==FALSE)
    {
        // unable to install handler... 
        // display message to the user
        printf("Unable to install handler!\n");
        return -1;
    }

    if ( False == parse_command_line( argc, argv, &root_path2, &port2 ) )
    {
        return 1;
    }

    if ( NULL != root_path2 )
    {
        if ( !folder_exist(root_path2) )
        {
            fprintf( stdout, "The specified root directory \"%s\" does not exist\n", root_path2 );
            free( root_path2 );
            return 2;
        }
        set_root_path(root_path2);
        free( root_path2 );
    }

    if ( port2 != INVALID_PORT )
    {
        port = port2;
    }
    SetUnhandledExceptionFilter(crush_callback);
    
    if ( SUCC != http_startup(&port) )
    {
        return 3;
    }

    return 0;
}
