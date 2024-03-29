/*
This is free and unencumbered software released into the public domain.

Anyone is free to copy, modify, publish, use, compile, sell, or
distribute this software, either in source code form or as a compiled
binary, for any purpose, commercial or non-commercial, and by any
means.

In jurisdictions that recognize copyright laws, the author or authors
of this software dedicate any and all copyright interest in the
software to the public domain. We make this dedication for the benefit
of the public at large and to the detriment of our heirs and
successors. We intend this dedication to be an overt act of
relinquishment in perpetuity of all present and future rights to this
software under copyright law.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.

For more information, please refer to <http://unlicense.org>
*/

#ifndef __BASE64_H__
#define __BASE64_H__

#include <string>

// note base64 strings are printable char strings and are thus
//      represented by "char".  the binaries being encoded or
//      decoded must be represented by "unsigned char" because
//      it is binary. that is why you see char and unsigned char
//      mixed here.

int b64_is_valid_char( unsigned char c );

// return 4 if ok, 0 if not ok
int b64_encode_quantum( const unsigned char * in3, int in_len,
                        char * out4 );

// return length of bytes decoded (1, 2, or 3), or 0 if not ok
int b64_decode_quantum( const char * in4, unsigned char * out3 );

// init destlen to sizeof dest buffer, on return
// it will be <= that size. destlen must be 4/3 bigger
// than srclen, or this returns false.
bool b64_encode(char *dest, int &destlen,
                const unsigned char *src, int srclen);

// init destlen to sizeof dest buffer, on return
// it will be <= that size. destlen must be at least
// 3/4 of srclen, or this returns false.
// also returns false if src contains non-base64 chars.
bool b64_decode(unsigned char *dest, int &destlen,
                const char *src, int srclen);

// there's no reason this will fail.
void b64_encode(std::string &dest, const std::string &src);

// this will return false if src has non-base64 chars in it.
bool b64_decode(std::string &dest, const std::string &src);

#endif /* __BASE64_H__ */
