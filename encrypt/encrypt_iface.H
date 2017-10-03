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

#ifndef __ENCRYPT_IFACE_H_
#define __ENCRYPT_IFACE_H_

#include "types.H"

class encrypt_key {
public:
    int type;
    virtual ~encrypt_key( void ) {};
    // each derived class should implement the following:
    // static bool valid_keystring( char * keystring );
    virtual bool key_parse( char * keystring ) = 0;
    virtual char * key_dump( void ) = 0;
    virtual void random_key( int param ) = 0;
};

class encrypt_iface {
public:
    int type;
    virtual ~encrypt_iface( void ) { delete key; };
    virtual int minsize( void ) = 0;
    virtual bool encrypt( UCHAR * outbuf,
                          UCHAR * inbuf, int numbytes ) = 0;
    virtual bool decrypt( UCHAR * outbuf,
                          UCHAR * inbuf, int numbytes ) = 0;
    encrypt_key * key;
};


encrypt_iface *  parse_key( char * keystring );

#endif /* __ENCRYPT_IFACE_H_ */
