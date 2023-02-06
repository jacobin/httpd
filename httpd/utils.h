#ifndef __UTILS_H__
#define __UTILS_H__

#define MAX_PATH2 (2*MAX_PATH)

typedef char* charp2free_t;
typedef wchar_t* wcharp2free_t;

typedef int index_t;
typedef int Bool_t;
#define False 0
#define True 1

#define MAXIMAL_64BIT_UNSIGN_DEC 20
#define MAXIMAL_32BIT_UNSIGN_DEC 10

wcharp2free_t ansi_to_unicode(char* str);
charp2free_t unicode_to_ansi(wchar_t* str);
wcharp2free_t utf8_to_unicode(char* str);
charp2free_t unicode_to_utf8(wchar_t* str);
charp2free_t utf8_to_ansi(char* str);
charp2free_t ansi_to_utf8(char* str);
int file_exist(char *file_name);
int remove_file(char *file_name);
char* file_ext(char* file_name);
charp2free_t root_path();
char* uint32_to_str(uint32_t n);
void mySleep(int sleepMs);
char* stristr( const char* str1, const char* str2 );
ULONGLONG filesize64( DWORD nFileSizeHigh, DWORD nFileSizeLow );
charp2free_t html_escape(const char* s);
charp2free_t url_escape(const char* s);
errno_t prepend_chars(char *dest, int destsize, const char *src, unsigned minimal_width, char c);

#endif