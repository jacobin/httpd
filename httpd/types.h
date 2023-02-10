#ifndef __TYPE_S_H_
#define __TYPE_S_H_

typedef char                int8_t;
typedef short               int16_t;
typedef long                int32_t;
typedef long long           int64_t;
typedef unsigned char       uint8_t;
typedef unsigned short      uint16_t;
typedef unsigned long       uint32_t;
typedef unsigned long long  uint64_t;

#define INT8_MAX	0x7f
#define INT16_MAX	0x7fff
#define INT32_MAX	0x7fffffff
#define INT64_MAX	0x7fffffffffffffff
#define UINT8_MAX	0xff
#define UINT16_MAX	0xffff
#define UINT32_MAX	0xffffffff
#define UINT64_MAX	0xffffffffffffffff

typedef enum
{
    SUCC,
    FAIL,
    PARA,
    FULL,
    EXIS,
    NEXI,
    DISC
} ret_code_t;

typedef char* charp2free_t;
typedef wchar_t* wcharp2free_t;

typedef int index_t;
typedef int Bool_t;
#define False 0
#define True 1

#endif