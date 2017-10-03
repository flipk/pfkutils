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

#include <netinet/in.h>
#include <netdb.h>

class MessagesUdp : public Thread {

    int connid;
    encrypt_iface * cr;
    bool verbose;
    bool die;

    int fd;

    struct other_eid_defs;
    struct other_eid_defs * others;
    int * mqids; // 0 is fd active mqid
    int nummqids;

    char body[ 16384 ];

    void entry( void );
    void read_packet( void );

    static const int MY_PRIO = NUM_PRIOS-1;
    static const int MY_STACK = 65536;

public:
    MessagesUdp( int connid, encrypt_iface * _cr,
                 int port, bool _verbose );
    ~MessagesUdp( void );

    bool register_entity( unsigned long remote_ip,
                          int remote_port,
                          int eid );
    bool unregister_entity( int eid );
    void kill( void );
};
