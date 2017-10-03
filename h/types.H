/*
    This file is part of the "pfkutils" tools written by Phil Knaack
    (pknaack1@netscape.net).
    Copyright (C) 2008  Phillip F Knaack

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifndef __UNIVERSAL_TYPES_H
#define __UNIVERSAL_TYPES_H

#define UINT64  unsigned long long
#define UINT32  unsigned int
#define UINT16  unsigned short
#define UINT8   unsigned char
#define UCHAR   unsigned char

#ifdef __cplusplus
class UINT64_t {
private:
    UCHAR p[8];
public:
    UINT64_t( void ) { }
    UINT64_t( UINT64 x ) { set( x ); }
    UINT64 get(void)
        {
            return
                ((UINT64)p[0] << 56) + ((UINT64)p[1] << 48) + 
                ((UINT64)p[2] << 40) + ((UINT64)p[3] << 32) + 
                ((UINT64)p[4] << 24) + ((UINT64)p[5] << 16) + 
                ((UINT64)p[6] <<  8) + ((UINT64)p[7] <<  0);
        }
    UINT32 get_lo(void)
        {
            return
                ((UINT32)p[4] << 24) + ((UINT32)p[5] << 16) + 
                ((UINT32)p[6] <<  8) + ((UINT32)p[7] <<  0);
        }
    UINT32 get_hi(void)
        {
            return
                ((UINT32)p[0] << 24) + ((UINT32)p[1] << 16) + 
                ((UINT32)p[2] <<  8) + ((UINT32)p[3] <<  0);
        }
    void set( UINT64 x )
        {
            p[0] = (x >> 56) & 0xff;  p[1] = (x >> 48) & 0xff;
            p[2] = (x >> 40) & 0xff;  p[3] = (x >> 32) & 0xff;
            p[4] = (x >> 24) & 0xff;  p[5] = (x >> 16) & 0xff;
            p[6] = (x >>  8) & 0xff;  p[7] = (x >>  0) & 0xff;
        }
    int size_of(void) { return 8; }
    void incr(UINT64 v) { set(get()+v); }
    void decr(UINT64 v) { set(get()-v); }
    void incr(void) { incr(1); }
    void decr(void) { decr(1); }
};

class UINT32_t {
private:
    UCHAR p[4];
public:
    UINT32_t( void ) { }
    UINT32_t( UINT32 x ) { set( x ); }
    UINT32 get(void)
        {
            return (p[0]<<24) | (p[1]<<16) | (p[2]<<8) | p[3];
        };
    void set( UINT32 v )
        {
            p[0] = (v >> 24) & 0xff;
            p[1] = (v >> 16) & 0xff;
            p[2] = (v >>  8) & 0xff;
            p[3] =  v    & 0xff;
        };
    int size_of(void) { return 4; }
    void incr(UINT32 v) { set(get()+v); }
    void decr(UINT32 v) { set(get()-v); }
    void incr(void) { incr(1); }
    void decr(void) { decr(1); }
};

class UINT16_t {
private:
    UCHAR p[2];
public:
    UINT16_t( void ) { }
    UINT16_t( UINT16 x ) { set( x ); }
    UINT16 get(void)
        {
            return (p[0]<<8) | p[1];
        };
    void set( UINT16 v )
        {
            p[0] = (v >>  8) & 0xff;
            p[1] =  v    & 0xff;
        };
    int size_of(void) { return 2; }
    void incr(UINT16 v) { set(get()+v); }
    void decr(UINT16 v) { set(get()-v); }
    void incr(void) { incr(1); }
    void decr(void) { decr(1); }
};

class UINT8_t {
private:
    UCHAR p[1];
public:
    UINT8_t( void ) { }
    UINT8_t( UCHAR x ) { set( x ); }
    UINT8 get(void) { return p[0]; }
    void set( UINT8 v ) { p[0] = v; }
    int size_of(void) { return 1; }
    void incr(UINT8 v) { set(get()+v); }
    void decr(UINT8 v) { set(get()-v); }
    void incr(void) { incr(1); }
    void decr(void) { decr(1); }
};

#endif /* __cplusplus */

#endif /* __UNIVERSAL_TYPES_H */
