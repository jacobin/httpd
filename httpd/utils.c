#include "httpd.h"
#include "logger.h"

wchar_t* ansi_to_unicode(char* str)
{
    wchar_t* result;
    int len = MultiByteToWideChar(CP_ACP, 0, str, -1, NULL, 0);
    result = (wchar_t*)malloc((len+1)*sizeof(wchar_t));
    if ( NULL == result )
    {
        log_error("{%s:%d} malloc fail.", __FUNCTION__, __LINE__);
        return NULL;
    }
    memset(result, 0, (len+1)*sizeof(wchar_t));
    MultiByteToWideChar(CP_ACP, 0, str, -1, (LPWSTR)result, len);
    return result;
}

char* unicode_to_ansi(wchar_t* str)
{
    char* result;
    int len = WideCharToMultiByte(CP_ACP, 0, str, -1, NULL, 0, NULL, NULL);
    result = (char*)malloc((len+1)*sizeof(char));
    if ( NULL == result )
    {
        log_error("{%s:%d} malloc fail.", __FUNCTION__, __LINE__);
        return NULL;
    }
    memset(result, 0, (len+1)*sizeof(char));
    WideCharToMultiByte(CP_ACP, 0, str, -1, result, len, NULL, NULL);
    return result;
}

wchar_t* utf8_to_unicode(char* str)
{
    wchar_t* result;
    int len = MultiByteToWideChar(CP_UTF8, 0, str, -1, NULL, 0);
    result = (wchar_t*)malloc((len+1)*sizeof(wchar_t));
    if ( NULL == result )
    {
        log_error("{%s:%d} malloc fail.", __FUNCTION__, __LINE__);
        return NULL;
    }
    memset(result, 0, (len+1)*sizeof(wchar_t));
    MultiByteToWideChar(CP_UTF8, 0, str, -1, (LPWSTR)result, len);
    return result;
}

char* unicode_to_utf8(wchar_t* str)
{
    char* result;
    int len = WideCharToMultiByte(CP_UTF8, 0, str, -1, NULL, 0, NULL, NULL);
    result =(char*)malloc((len+1)*sizeof(char));
    if ( NULL == result )
    {
        log_error("{%s:%d} malloc fail.", __FUNCTION__, __LINE__);
        return NULL;
    }
    memset(result, 0, (len+1)*sizeof(char));
    WideCharToMultiByte(CP_UTF8, 0, str, -1, result, len, NULL, NULL);
    return result;
}

char* utf8_to_ansi(char* str)
{
    char* ansi = NULL;
    wchar_t* uni = utf8_to_unicode(str);
    if ( NULL == uni )
    {
        return NULL;
    }
    ansi = unicode_to_ansi(uni);
    if ( NULL == ansi )
    {
        free(uni);
        return NULL;
    }
    free(uni);
    return ansi;
}

char* ansi_to_utf8(char* str)
{
    char* utf8 = NULL;
    wchar_t* uni = ansi_to_unicode(str);
    if ( NULL == uni )
    {
        return NULL;
    }
    utf8 = unicode_to_utf8(uni);
    if ( NULL == utf8 )
    {
        free(uni);
        return NULL;
    }
    free(uni);
    return utf8;
}

int file_exist(char *file_name)
{
    FILE* fp = NULL;
    fp = fopen(file_name, "rb");
    if (fp)
    {
        fclose(fp);
    }
    return fp != NULL;
}

int remove_file(char *file_name)
{
    return DeleteFileA(file_name);
}

char* file_ext(char* file_name)
{
    char *ext = NULL;
    int i;

    for (i=strlen(file_name)-1; i>=0; i--)
    {
        if (file_name[i] == '.')
        {
            ext = file_name+i+1;
            break;
        }
    }
    return ext;
}

char* root_path()
{
    static char* root=NULL;
    const int nRootLenBudget = MAX_PATH;
    uint32_t i;
    int nRootLen = 0;
    int nRootLenIncSlashNull = 0;
    int iTmp = 0;

    if ( NULL != root )
    {
        return root;
    }

    root = (char*)malloc( nRootLenBudget );
    if ( NULL == root )
    {
        log_error("{%s:%d} malloc fail.", __FUNCTION__, __LINE__);
        return NULL;
    }
    memset( root, 0, nRootLenBudget );

    ASSERT( NULL != root );
    if ( 0 == ( nRootLen = GetCurrentDirectoryA(nRootLenBudget, root) ) )
    {
        free( root );
        root = NULL;
        return NULL;
    }

    nRootLenIncSlashNull = nRootLen +2;
    if ( nRootLenBudget < nRootLenIncSlashNull )
    {
        root = (char*)realloc( root, nRootLenIncSlashNull );
        if ( NULL == root )
        {
            log_error("{%s:%d} realloc fail.", __FUNCTION__, __LINE__);
            return NULL;
        }
        memset( root, 0, nRootLenIncSlashNull );

        iTmp = GetCurrentDirectoryA(nRootLen, root);
        ASSERT( iTmp == nRootLen );
    }

    ASSERT( NULL != root );
    for (i=0; i<strlen(root); i++)
    {
        if (root[i] == '\\')
        {
            root[i] = '/';
        }
    }
    if (root[strlen(root)-1] != '/')
    {
        root[strlen(root)] = '/';
    }

    return root;
}

char* uint32_to_str(uint32_t n)
{
    static char buf[16] = {0};
    memset(buf, 0, sizeof(buf));
    _itoa(n, buf, 10);
    return buf;
}

///////////////////////////////////////////////////////////////////////////////
# ifdef LINUX
#    include <unistd.h>
# endif
# ifdef WINDOWS
#    include <windows.h>
# endif

// https://stackoverflow.com/questions/10918206/cross-platform-sleep-function-for-c
void mySleep(int sleepMs)
{
# ifdef LINUX
    usleep(sleepMs * 1000);   // usleep takes sleep time in us (1 millionth of a second)
# endif
# ifdef WINDOWS
    Sleep(sleepMs);
# endif
}

///////////////////////////////////////////////////////////////////////////////
// https://stackoverflow.com/questions/27303062/strstr-function-like-that-ignores-upper-or-lower-case
#include <ctype.h>
char* stristr( const char* str1, const char* str2 )
{
    const char* p1 = str1 ;
    const char* p2 = str2 ;
    const char* r = *p2 == 0 ? str1 : 0 ;

    while( *p1 != 0 && *p2 != 0 )
    {
        if( tolower( (unsigned char)*p1 ) == tolower( (unsigned char)*p2 ) )
        {
            if( r == 0 )
            {
                r = p1 ;
            }

            p2++ ;
        }
        else
        {
            p2 = str2 ;
            if( r != 0 )
            {
                p1 = r + 1 ;
            }

            if( tolower( (unsigned char)*p1 ) == tolower( (unsigned char)*p2 ) )
            {
                r = p1 ;
                p2++ ;
            }
            else
            {
                r = 0 ;
            }
        }

        p1++ ;
    }

    return *p2 == 0 ? (char*)r : 0 ;
}
//
////
//#include <stdio.h>
//int main(void) 
//{
//    char* test = stristr( "One TTwo Three", "two" ) ;
//    printf( "%s\n", test == 0 ? "NULL" : test  ) ;
//
//    test = stristr( "One Two Three", "two" ) ;
//    printf( "%s\n", test == 0 ? "NULL" : test  ) ;
//
//    test = stristr( "One wot Three", "two" ) ;
//    printf( "%s\n", test == 0 ? "NULL" : test  ) ;
//
//    test = stristr( "abcdefg", "cde" ) ;
//    printf( "%s\n", test == 0 ? "NULL" : test  ) ;
//
//    test = stristr( "ABCDEFG", "cde" ) ;
//    printf( "%s\n", test == 0 ? "NULL" : test  ) ;
//
//    test = stristr( "AbCdEfG", "cde" ) ;
//    printf( "%s\n", test == 0 ? "NULL" : test  ) ;
//
//    test = stristr( "1234567", "cde" ) ;
//    printf( "%s\n", test == 0 ? "NULL" : test  ) ;
//
//    test = stristr( "zzzz", "zz" ) ;
//    printf( "%s\n", test == 0 ? "NULL" : test  ) ;
//
//    test = stristr( "zz", "zzzzz" ) ;
//    printf( "%s\n", test == 0 ? "NULL" : test  ) ;
//
//    test = stristr( "", "" ) ;
//    printf( "%s\n", test == 0 ? "NULL" : test  ) ;
//
//    test = stristr( "zzzzz", "" ) ;
//    printf( "%s\n", test == 0 ? "NULL" : test  ) ;
//
//    test = stristr( "", "zzzz" ) ;
//    printf( "%s\n", test == 0 ? "NULL" : test  ) ;
//
//    test = stristr("AAABCDX","AABC") ;
//    printf( "%s\n", test == 0 ? "NULL" : test  ) ;
//
//    return 0;
//}
