#ifndef __UTILS_H__
#define __UTILS_H__

#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>

#define MAX_PATH2 (2*MAX_PATH)

wcharp2free_t ansi_to_unicode(const char* str);
charp2free_t unicode_to_ansi(const wchar_t* str);
wcharp2free_t utf8_to_unicode(const char* str);
charp2free_t unicode_to_utf8(const wchar_t* str);
charp2free_t utf8_to_ansi(const char* str);
charp2free_t ansi_to_utf8(const char* str);
Bool_t file_exist(const char *file_path);
Bool_t folder_exist(const char* folder_path);
int remove_file(const char *file_name);
const char* file_ext(const char* file_name);
const char* root_path();
void set_root_path( const char* folder_path );
void free_root_path();
void mySleep(int sleepMs);
const char* stristr( const char* str1, const char* str2 );
ULONGLONG filesize64( DWORD nFileSizeHigh, DWORD nFileSizeLow );
charp2free_t html_escape(const char* s);
charp2free_t url_escape(const char* s);
errno_t prepend_chars(char *dest, int destsize, const char *src, unsigned minimal_width, char c);

#endif