#include "httpd.h"
#include "logger.h"
#include "utils.h"
#include <sys/stat.h>

wcharp2free_t ansi_to_unicode(const char* str)
{
    wchar_t* result = NULL;
    int len = MultiByteToWideChar(CP_ACP, 0, str, -1, NULL, 0);
    ASSERT(NULL!=str);
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

charp2free_t unicode_to_ansi(const wchar_t* str)
{
    char* result = NULL;
    int len = WideCharToMultiByte(CP_ACP, 0, str, -1, NULL, 0, NULL, NULL);
    ASSERT(NULL!=str);
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

wcharp2free_t utf8_to_unicode(const char* str)
{
    wchar_t* result = NULL;
    int len = MultiByteToWideChar(CP_UTF8, 0, str, -1, NULL, 0);
    ASSERT(NULL!=str);
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

charp2free_t unicode_to_utf8(const wchar_t* str)
{
    char* result = NULL;
    int len = WideCharToMultiByte(CP_UTF8, 0, str, -1, NULL, 0, NULL, NULL);
    ASSERT(NULL!=str);
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

charp2free_t utf8_to_ansi(const char* str)
{
    char* ansi = NULL;
    wchar_t* uni = utf8_to_unicode(str);
    ASSERT(NULL!=str);
    if ( NULL == uni )
    {
        return NULL;
    }
    ansi = unicode_to_ansi(uni);
    free(uni);
    return ansi;
}

charp2free_t ansi_to_utf8(const char* str)
{
    char* utf8 = NULL;
    wchar_t* uni = ansi_to_unicode(str);
    ASSERT(NULL!=str);
    if ( NULL == uni )
    {
        return NULL;
    }
    utf8 = unicode_to_utf8(uni);
    free(uni);
    return utf8;
}

Bool_t file_exist(const char *file_path)
{
    FILE* fp = NULL;
    ASSERT(NULL!=file_path);
    fp = fopen(file_path, "rb");
    if (fp)
    {
        fclose(fp);
    }
    return fp != NULL;
}

// https://stackoverflow.com/questions/12510874/how-can-i-check-if-a-directory-exists
Bool_t folder_exist(const char* folder_path)
{
    struct stat sb;
    if ( stat(folder_path, &sb) == 0 && (S_IFDIR & sb.st_mode) ) {
        return True;
    } else {
        return False;
    }
}

int remove_file(const char *file_name)
{
    ASSERT(NULL!=file_name);
    return DeleteFileA(file_name);
}

const char* file_ext(const char* file_name)
{
    const char *ext = NULL;
    int i = -1;
    
    ASSERT(NULL!=file_name);

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


static char* root_path2()
{
    static char* root = NULL;
    if ( NULL == root )
    {
        // DWORD GetCurrentDirectory( [in] DWORD  nBufferLength, [out] LPTSTR lpBuffer );
        //    To determine the required buffer size, set tlpBuffer to NULL and the nBufferLength parameter to 0.
        //
        // If the function succeeds, the return value specifies the number of 
        //    characters that are written to the buffer, not including the 
        //    terminating null character.

        //    If the function fails, the return value is zero. To get extended error
        //    information, call GetLastError.

        //    If the buffer that is pointed to by lpBuffer is not large enough, the
        //    return value specifies the required size of the buffer, in characters,
        //    including the null-terminating character.
        const size_t nRootLen = GetCurrentDirectoryA(0, NULL);
        if ( 0 == nRootLen )
        {
            log_error( "{%s:%d} GetCurrentDirectoryA fail %d.", __FUNCTION__, __LINE__, GetLastError() );
            return NULL;
        }

        {
            // GetCurrentDirectoryA: If the function succeeds, the return value 
            //   specifies the number of characters that are written to the buffer, NOT
            //   including the terminating null character
            const size_t nRootLenIncSlashNull = nRootLen + 1 + 1;
            root = (char*)malloc( nRootLenIncSlashNull );
            if ( NULL == root )
            {
                log_error("{%s:%d} realloc fail.", __FUNCTION__, __LINE__);
                return NULL;
            }
            memset( root, 0, nRootLenIncSlashNull );
        }
        
        {
            // ... in characters, including the null-terminating character.
            DWORD dwTmp = GetCurrentDirectoryA(nRootLen, root);
            ASSERT( (dwTmp+1) == nRootLen );
        }

        {
            size_t i = 0;
            // ... in characters, including the null-terminating character.
            const size_t tmp = strlen(root);
            ASSERT( (tmp+1) == nRootLen );
            
            for ( i = 0; i < tmp; i++ )
            {
                if (root[i] == '\\')
                {
                    root[i] = '/';
                }
            }
            if (root[tmp-1] != '/')
            {
                root[tmp] = '/';

                // 1. This operation is actually unnecessary, because nRootLen 
                //    in "GetCurrentDirectoryA(nRootLen, root);" limits the writing
                //    of GetCurrentDirectoryA from exceeding the scope
                // 2. "nRootLenIncSlashNull" guarantees that this operation will not overflow
                root[ tmp+1 ] = 0; 
                                 
            }
	    }
    }

    return root;
}

static char* g_root = NULL;
void set_root_path( const char* folder_path )
{
    size_t i = 0;
    const size_t fpLen = strlen(folder_path);
    const size_t bufsize_inc_slash_null = fpLen + 2; // include slash and NULL
    errno_t err = 0;

    ASSERT( NULL == g_root );
    g_root = (char*)malloc( bufsize_inc_slash_null );
    if ( NULL == g_root )
    {
        log_error("{%s:%d} malloc fail.", __FUNCTION__, __LINE__);
        return;
    }
    memset( g_root, 0, bufsize_inc_slash_null );
    err = strcpy_s( g_root, bufsize_inc_slash_null, folder_path );
    ASSERT( 0 == err );

    {
        const size_t tmp = strlen(g_root);
        for ( i = 0; i < tmp; i++ )
        {
            if (g_root[i] == '\\')
            {
                g_root[i] = '/';
            }
        }
        if (g_root[tmp-1] != '/')
        {
            g_root[tmp] = '/';

            // This operation is necessary because "strcpy_s( g_root, bufsize_inc_slash_null, folder_path );" will
            // modify the content of the space other than bufsize_inc_slash_null.
            g_root[tmp+1] = 0;
        }
    }
}

const char* root_path()
{
    return (NULL != g_root) ? g_root : root_path2();
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
const char* stristr( const char* str1, const char* str2 )
{
    const char* p1 = str1 ;
    const char* p2 = str2 ;
    const char* r = *p2 == 0 ? str1 : 0 ;

    ASSERT(NULL!=str1);

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

    return *p2 == 0 ? r : 0 ;
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

ULONGLONG filesize64(DWORD nFileSizeHigh, DWORD nFileSizeLow)
{
    ULARGE_INTEGER ul;
    ul.HighPart = nFileSizeHigh;
    ul.LowPart = nFileSizeLow;
    return ul.QuadPart;
}

/////////////////////////////////////////////////////////////////////////////{{
/////////////////////////////////////////////////////////////////////////////{{
/////////////////////////////////////////////////////////////////////////////{{
static const char g_escape_chars[26][5] =
{
   "\x20%20",
     "\"%22",
      "#%23",
      "$%24",
      "%%25",
      "&%26",
      "'%27",
      "+%2B",
      ",%2C",
      "/%2F",
      ":%3A",
      ";%3B",
      "<%3C",
      "=%3D",
      ">%3E",
      "?%3F",
      "@%40",
      "[%5B",
     "\\%5C",
      "]%5D",
      "^%5E",
      "`%60",
      "{%7B",
      "|%7C",
      "}%7D",
      "~%7E"
};

static index_t url_escape3(const char escape_chars[][5], index_t endidx, char c, Bool_t* b)
{
    const index_t mid_idx = endidx / 2;
    const char mid_str0 = escape_chars[mid_idx][0];

    ASSERT(0 <= endidx && NULL != b);

    if (0 == endidx)
    {
        *b = False;
        return -1;
    }

    if (mid_str0 == c)
    {
        *b = True;
        return mid_idx;
    }
    else if (mid_str0 < c)
    {
        const index_t newidx0 = mid_idx + 1;
        return newidx0 + url_escape3(&escape_chars[newidx0], endidx - newidx0, c, b);
    }
    else
    {
        assert(c < mid_str0);
        return url_escape3(escape_chars, mid_idx, c, b);
    }
    assert(0);
}

static const char* url_escape2(char c)
{
    index_t i = 0;
    Bool_t b = False;
    i = url_escape3(g_escape_chars, 26, c, &b);
    if (False == b) return NULL;
    assert(0 <= i);
    return &g_escape_chars[i][1];
}
//
//int main()
//{
//    char* s = NULL;
//    s = url_escape2('\x20'); if (s) printf("%s\n", s); else printf("error\n");
//    s = url_escape2('\"');   if (s) printf("%s\n", s); else printf("error\n");
//    s = url_escape2('#');    if (s) printf("%s\n", s); else printf("error\n");
//    s = url_escape2('$');    if (s) printf("%s\n", s); else printf("error\n");
//    s = url_escape2('%');    if (s) printf("%s\n", s); else printf("error\n");
//    s = url_escape2('&');    if (s) printf("%s\n", s); else printf("error\n");
//    s = url_escape2('\'');   if (s) printf("%s\n", s); else printf("error\n");
//    s = url_escape2('+');    if (s) printf("%s\n", s); else printf("error\n");
//    s = url_escape2(',');    if (s) printf("%s\n", s); else printf("error\n");
//    s = url_escape2('/');    if (s) printf("%s\n", s); else printf("error\n");
//    s = url_escape2(':');    if (s) printf("%s\n", s); else printf("error\n");
//    s = url_escape2(';');    if (s) printf("%s\n", s); else printf("error\n");
//    s = url_escape2('<');    if (s) printf("%s\n", s); else printf("error\n");
//    s = url_escape2('=');    if (s) printf("%s\n", s); else printf("error\n");
//    s = url_escape2('>');    if (s) printf("%s\n", s); else printf("error\n");
//    s = url_escape2('?');    if (s) printf("%s\n", s); else printf("error\n");
//    s = url_escape2('@');    if (s) printf("%s\n", s); else printf("error\n");
//    s = url_escape2('[');    if (s) printf("%s\n", s); else printf("error\n");
//    s = url_escape2('\\');   if (s) printf("%s\n", s); else printf("error\n");
//    s = url_escape2(']');    if (s) printf("%s\n", s); else printf("error\n");
//    s = url_escape2('^');    if (s) printf("%s\n", s); else printf("error\n");
//    s = url_escape2('`');    if (s) printf("%s\n", s); else printf("error\n");
//    s = url_escape2('{');    if (s) printf("%s\n", s); else printf("error\n");
//    s = url_escape2('|');    if (s) printf("%s\n", s); else printf("error\n");
//    s = url_escape2('}');    if (s) printf("%s\n", s); else printf("error\n");
//    s = url_escape2('~');    if (s) printf("%s\n", s); else printf("error\n");
//    s = url_escape2('\x2');  if (s) printf("%s\n", s); else printf("error\n");
//    s = url_escape2('A');    if (s) printf("%s\n", s); else printf("error\n");
//    s = url_escape2('\x7F'); if (s) printf("%s\n", s); else printf("error\n");
//    return 0;
//}

charp2free_t url_escape(const char* s)
{
    size_t i = 0;
    size_t used = 0;
    charp2free_t result = NULL;
    const size_t sLen = strlen(s);

    size_t result_size = sLen + 3;
    ASSERT(NULL != s);
    result = (charp2free_t)malloc(result_size);
    if (NULL == result) {
        return NULL;
    }

    for (used = 0, i = 0; i < sLen; i++) {
        const char c = s[i];
        const char* hex = url_escape2(c);
        if (NULL != hex) {
	        int iSnprintRet = -1;
            iSnprintRet = strcpy_s(&result[used], result_size-used,hex);
            ASSERT( 0 == iSnprintRet );
            used += 3;
        }
        else {
            result[used++] = c;
        }

        if (result_size <= (used + 4)) { // such as "%20"
            charp2free_t tmp = NULL;
            result_size += 100;
            tmp = (charp2free_t)realloc(result, result_size);
            if (NULL == tmp) {
                free(result);
                return NULL;
            }
            result = tmp;
        }
    }
    result[used] = (char)NULL;

    return result;
}
//
//int main()
//{
//    const char s[] = "abc¡°ABC¡±def£¬a£¬b¡¢c¡£d£¬#efg hij£¬lm.jpg";
//    charp2free_t result = url_escape(s);
//    printf("%s", result);
//    free(result);
//    return 0;
//}
/////////////////////////////////////////////////////////////////////////////}}
/////////////////////////////////////////////////////////////////////////////}}
/////////////////////////////////////////////////////////////////////////////}}


/////////////////////////////////////////////////////////////////////////////{{
/////////////////////////////////////////////////////////////////////////////{{
/////////////////////////////////////////////////////////////////////////////{{
//#include <assert.h>
//#include <string.h>
//#include <memory.h>
//#include <stdio.h>
//#include <stdlib.h>
//
//typedef int index_t;
//typedef int Bool_t;
//typedef char* charp2free_t;
//#define False 0
//#define True 1

static const char g_chars2bescaped[] =
{
    '\t',
    '\n',
    '\x20',
    '!',
    '"',
    '#',
    '$',
    '%',
    '&',
    '\'',
    '(',
    ')',
    '*',
    '+',
    ',',
    '-',
    '.',
    '/',
    ':',
    ';',
    '<',
    '=',
    '>',
    '?',
    '@',
    '[',
    '\\',
    ']',
    '^',
    '_',
    '`',
    '{',
    '|',
    '}',
    '~'
};

static index_t bins2(const char escape_chars[], index_t endidx, char c, Bool_t* b)
{
    const index_t mid_idx = endidx / 2;
    const char mid_str0 = escape_chars[mid_idx];

    ASSERT(0 <= endidx && NULL != b);

    if (0 == endidx) {
        *b = False;
        return -1;
    }

    if (mid_str0 == c) {
        *b = True;
        return mid_idx;
    }
    else if (mid_str0 < c) {
        const index_t newidx0 = mid_idx + 1;
        return newidx0 + bins2(&escape_chars[newidx0], endidx - newidx0, c, b);
    }
    else {
        assert(c < mid_str0);
        return bins2(escape_chars, mid_idx, c, b);
    }
    assert(0);
}

static index_t bins(char c)
{
    index_t i = 0;
    Bool_t b = False;
    i = bins2(g_chars2bescaped, 35, c, &b);
    if (False == b) return -1;
    assert(0 <= i);
    return i;
}

charp2free_t html_escape(const char* s)
{
    size_t i = 0;
    size_t used = 0;
    charp2free_t result = NULL;
    const size_t sLen = strlen(s);

    size_t result_size = sLen + 1 + 5;
    ASSERT(NULL != s);
    result = (charp2free_t)malloc(result_size);
    if (NULL == result) {
        return NULL;
    }

    for (used = 0, i = 0; i < sLen; i++) {
        const char c = s[i];
        const index_t idx = bins(c);
        if (-1 != idx) {
            char s6[7];
            int iSnprintRet = -1;
            iSnprintRet = sprintf_s(s6, sizeof(s6), "&#%d;", c);
            ASSERT( 0 <= iSnprintRet );
            iSnprintRet = strcpy_s(&result[used], result_size-used, s6);
            ASSERT( 0 == iSnprintRet );
            used += strlen(s6);
        }
        else {
            result[used++] = c;
        }

        if (result_size <= (used + 7)) { // such as "&#126;"
            charp2free_t tmp = NULL;
            result_size += 100;
            tmp = (charp2free_t)realloc(result, result_size);
            if (NULL == tmp) {
                free(result);
                return NULL;
            }
            result = tmp;
        }
    }
    result[used] = (char)NULL;

    return result;
}
//
//int main()
//{
//    const char s[] = "abc¡°ABC¡±def£¬a£¬b¡¢c¡£d£¬#efg hij£¬lm.jpg";
//    charp2free_t result = html_escape(s);
//    printf("%s", result);
//    free(result);
//    return 0;
//}

/////////////////////////////////////////////////////////////////////////////}}
/////////////////////////////////////////////////////////////////////////////}}
/////////////////////////////////////////////////////////////////////////////}}

// https://stackoverflow.com/questions/43354488/c-formatted-string-how-to-add-leading-zeros-to-string-value-using-sprintf
// prepend "0" as needed resulting in a string of _minimal_ width.
errno_t prepend_chars(char *dest, int destsize, const char *src, unsigned minimal_width, char c)
{
    const size_t len = strlen(src);
    const size_t chars = (minimal_width < len) ? 0 : (minimal_width-len);
    ASSERT(NULL != dest && NULL != src);
    memset(dest, c, chars);
    return strcpy_s(dest + chars, destsize - chars, src);
}