/* md5.h */
/*
    This file is part of the AVR-Crypto-Lib.
    Copyright (C) 2008  Daniel Otte (daniel.otte@rub.de)

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
/* 
 * File:	md5.h
 * Author:	Daniel Otte
 * Date: 	31.07.2006
 * License: GPL
 * Description: Implementation of the MD5 hash algorithm as described in RFC 1321
 * 
 */


#ifndef MD5_H_
#define MD5_H_

#include <stdint.h>

namespace WebAppServer {

#define WEBSOCKET_MD5_HASH_BITS  128
#define WEBSOCKET_MD5_HASH_BYTES (WEBSOCKET_MD5_HASH_BITS/8)
#define WEBSOCKET_MD5_BLOCK_BITS 512
#define WEBSOCKET_MD5_BLOCK_BYTES (WEBSOCKET_MD5_BLOCK_BITS/8)

typedef struct websocket_md5_ctx_st {
    uint32_t a[4];
    uint32_t counter;
} websocket_md5_ctx_t;

typedef uint8_t websocket_md5_hash_t[WEBSOCKET_MD5_HASH_BYTES];
 
void websocket_md5_init(websocket_md5_ctx_t *s);
void websocket_md5_nextBlock(websocket_md5_ctx_t *state, const void* block);
void websocket_md5_lastBlock(websocket_md5_ctx_t *state, const void* block,
                             uint16_t length);
void websocket_md5_ctx2hash(websocket_md5_hash_t dest,
                            const websocket_md5_ctx_t* state);
void websocket_md5(websocket_md5_hash_t dest, const void* msg, uint32_t length_b);

} // namespace WebAppServer

#endif /*MD5_H_*/
