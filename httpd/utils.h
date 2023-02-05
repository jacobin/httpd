#ifndef __UTILS_H__
#define __UTILS_H__

#define MAX_PATH2 (2*MAX_PATH)

wchar_t* ansi_to_unicode(char* str);
char* unicode_to_ansi(wchar_t* str);
wchar_t* utf8_to_unicode(char* str);
char* unicode_to_utf8(wchar_t* str);
char* utf8_to_ansi(char* str);
char* ansi_to_utf8(char* str);
int file_exist(char *file_name);
int remove_file(char *file_name);
char* file_ext(char* file_name);
char* root_path();
char* uint32_to_str(uint32_t n);
void mySleep(int sleepMs);
char* stristr( const char* str1, const char* str2 );
ULONGLONG filesize64( DWORD nFileSizeHigh, DWORD nFileSizeLow );

#endif