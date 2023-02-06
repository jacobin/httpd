#ifndef __UTILS_H__
#define __UTILS_H__

#define MAX_PATH2 (2*MAX_PATH)

wcharp2free_t ansi_to_unicode(const char* str);
charp2free_t unicode_to_ansi(const wchar_t* str);
wcharp2free_t utf8_to_unicode(const char* str);
charp2free_t unicode_to_utf8(const wchar_t* str);
charp2free_t utf8_to_ansi(const char* str);
charp2free_t ansi_to_utf8(const char* str);
int file_exist(const char *file_name);
int remove_file(const char *file_name);
const char* file_ext(const char* file_name);
charp2free_t root_path();
char* uint32_to_str(uint32_t n);
void mySleep(int sleepMs);
const char* stristr( const char* str1, const char* str2 );
ULONGLONG filesize64( DWORD nFileSizeHigh, DWORD nFileSizeLow );
charp2free_t html_escape(const char* s);
charp2free_t url_escape(const char* s);
errno_t prepend_chars(char *dest, int destsize, const char *src, unsigned minimal_width, char c);

#endif