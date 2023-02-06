#include "httpd.h"
#include "logger.h"
#include "utils.h"

wcharp2free_t ansi_to_unicode(const char* str)
{
    wchar_t* result = NULL;
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

charp2free_t unicode_to_ansi(const wchar_t* str)
{
    char* result = NULL;
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

wcharp2free_t utf8_to_unicode(const char* str)
{
    wchar_t* result = NULL;
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

charp2free_t unicode_to_utf8(const wchar_t* str)
{
    char* result = NULL;
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

charp2free_t utf8_to_ansi(const char* str)
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

charp2free_t ansi_to_utf8(const char* str)
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

int file_exist(const char *file_name)
{
    FILE* fp = NULL;
    fp = fopen(file_name, "rb");
    if (fp)
    {
        fclose(fp);
    }
    return fp != NULL;
}

int remove_file(const char *file_name)
{
    return DeleteFileA(file_name);
}

const char* file_ext(const char* file_name)
{
    const char *ext = NULL;
    int i = -1;

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

charp2free_t root_path()
{
    static char* root=NULL;
    const int nRootLenBudget = MAX_PATH2;
    uint32_t i = 0;
    int nRootLen = 0;
    int nRootLenIncSlashNull = 0;
    int iTmp = -1;

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
const char* stristr( const char* str1, const char* str2 )
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

    assert(0 <= endidx);

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
            charp2free_t result2 = NULL;
            result_size += 100;
            result2 = (charp2free_t)realloc(result, result_size);
            if (NULL == result2) {
                free(result);
                return NULL;
            }
            result = result2;
        }
    }
    result[used] = (char)NULL;

    return result;
}
//
//int main()
//{
//    const char s[] = "为凸显“红顶商人”地位，肖建华不但平时戴着红帽子，还公开喊话，说自己帮习近平姐姐、曾庆红儿子理财。图片中，#肖建华 在香港四季酒店的楼下广场，装模作样地看着中共高官的回忆录.jpg";
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

static const char g_chars2bescaped[] = { '\t', '\n', '\x20', '!', '"', '#', '$', '%', '&', '\'', '(', ')', '*', '+', ',', '-', '.', '/', ':', ';', '<', '=', '>', '?', '@', '[', '\\', ']', '^', '_', '`', '{', '|', '}', '~' };

static index_t bins2(const char escape_chars[], index_t endidx, char c, Bool_t* b)
{
    const index_t mid_idx = endidx / 2;
    const char mid_str0 = escape_chars[mid_idx];

    assert(0 <= endidx);

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
            charp2free_t result2 = NULL;
            result_size += 100;
            result2 = (charp2free_t)realloc(result, result_size);
            if (NULL == result2) {
                free(result);
                return NULL;
            }
            result = result2;
        }
    }
    result[used] = (char)NULL;

    return result;
}
//
//int main()
//{
//    const char s[] = "为凸显“红顶商人”地位，肖建华不但平时戴着红帽子，还公开喊话，说自己帮习近平姐姐、曾庆红儿子理财。图片中，#肖建华 在香港四季酒店的楼下广场，装模作样地看着中共高官的回忆录.jpg";
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
    memset(dest, c, chars);
    return strcpy_s(dest + chars, destsize - chars, src);
}