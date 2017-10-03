/*
 * This code is written by Phillip F Knaack. This code is in the
 * public domain. Do anything you want with this code -- compile it,
 * run it, print it out, pass it around, shit on it -- anything you want,
 * except, don't claim you wrote it.  If you modify it, add your name to
 * this comment in the COPYRIGHT file and to this comment in every file
 * you change.
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
            return (p[1]<<8) | p[0];
        };
    void set( UINT16 v )
        {
            p[1] = (v >>  8) & 0xff;
            p[0] =  v    & 0xff;
        };
    int size_of(void) { return 2; }
};

#endif /* __UNIVERSAL_TYPES_H */
