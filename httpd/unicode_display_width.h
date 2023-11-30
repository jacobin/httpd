#include "types.h" 

int mk_wcwidth(wchar_t ucs);

int mk_wcswidth(const wchar_t* pwcs, size_t n);

/*
 * The following functions are the same as mk_wcwidth() and
 * mk_wcswidth(), except that spacing characters in the East Asian
 * Ambiguous (A) category as defined in Unicode Technical Report #11
 * have a column width of 2. This variant might be useful for users of
 * CJK legacy encodings who want to migrate to UCS without changing
 * the traditional terminal character-width behaviour. It is not
 * otherwise recommended for general use.
 */
int mk_wcwidth_cjk(wchar_t ucs);

int mk_wcswidth_cjk(const wchar_t* pwcs, size_t n);