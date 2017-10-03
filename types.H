/*
 * This code is originally written by Phillip F Knaack.
 * There are absolutely no restrictions on the use of this code.  It is
 * NOT GPL!  If you use this code in a product, you do NOT have to release
 * your alterations, nor do you have to acknowledge in any way that you
 * are using this software.
 * The only thing I ask is this: do not claim you wrote it.  My name
 * should appear in this file forever as the original author, and every
 * person who modifies this file after me should also place their names
 * in this file (so those that follow know who broke what).
 * This restriction is not enforced in any way by any license terms, it
 * is merely a personal request from me to you.  If you wanted, you could
 * even completely remove this entire comment and place a new one with your
 * company's confidential proprietary license on it-- but if you are a good
 * internet citizen, you will comply with my request out of the goodness
 * of your heart.
 * If you do use this code and you make a buttload of money off of it,
 * I would appreciate a little kickback-- but again this is a personal
 * request and is not required by any licensing of this code.  (Of course
 * in the offchance that anyone every actually DOES make a lot of money 
 * using this code, I know I'm going to regret that statement, but at
 * this point in time it feels like the right thing to say.)
 * 
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR(S) ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR(S) BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef __UNIVERSAL_TYPES_H
#define __UNIVERSAL_TYPES_H

#ifndef NULL
#define NULL 0
#endif

#define UINT    unsigned int
#define UINT32  unsigned int
#define UINT64  unsigned long long
#define UINT16  unsigned short
#define UINT8   unsigned char
#define UCHAR   unsigned char

#define uint    UINT32
#define ushort  UINT16
#define uchar   UCHAR

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
};

#endif /* __cplusplus */

#ifndef MAX
#define MAX(a,b) (((a)>(b))?(a):(b))
#define MIN(a,b) (((a)<(b))?(a):(b))
#endif

#endif /* __UNIVERSAL_TYPES_H */
