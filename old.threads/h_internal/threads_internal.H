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

//
// - be sure to understand this well -- for instance,
//   whysit gotta be -16.  be sure to understand how stack
//   frames are built, also someday must understand how the
//   root stack frame works (how does gdb know when
//   its done tracing back the stack?)
//

#if defined(__FreeBSD__) && defined(i386)

#define SETUP_STACK( jb, stack, stacksize ) \
        jb->_jb[2] = (int)stack + stacksize - 16

#elif defined(SOLARIS)

#define _setjmp  setjmp
#define _longjmp longjmp

#define SETUP_STACK( jb, stack, stacksize ) \
        jb[1] = (int)stack + stacksize - 64

#elif defined(SUNOS)

#define _setjmp  setjmp
#define _longjmp longjmp

#define SETUP_STACK( jb, stack, stacksize ) \
        jb[2] = (int)stack + stacksize - 192

// #elif defined( __OpenBSD__ ) && defined(i386)
#elif defined( __CYGWIN32__ )

#define SETUP_STACK( jb, stack, stacksize ) \
        jb[7] = (int)stack + stacksize - 64
#define _setjmp  setjmp
#define _longjmp longjmp

#else

#error Need SETUP_STACK macro definition for this architecture

#endif

enum thread_state {
    // these must match statenames table at end of this file
    TH_READY = 1,   TH_DEAD,
    TH_SUSPENDED,   TH_CURR,
    TH_SLEEP,       TH_IOWAIT,
    TH_MSGWAIT,     TH_SEMWAIT,
    TH_NONEXIST,    TH_MAXSTATE
};

struct _Thread {
public:
    LListLinks<_Thread> links[1];
    Threads::tid_t tid;
    int prio;
    thread_state state;
    char * stack;
    int stacksize;
    jmp_buf jb;
    Threads::FUNCPTR func;
    void * arg;
    char * name;
    int switches;  // how many reschedules to this task?
    bool print_header;

    int stackleft( void ) {
        // how many bytes of stack left?
        int i;
        for ( i = 0; i < stacksize; i++ )
            if ( (unsigned char)stack[i] != 0xee )
                break;
        return i;
    }

    static const char *_statenames[];
    const char * statename( thread_state state ) {
        if ( state >= TH_MAXSTATE )
            return _statenames[0];
        return _statenames[state];
    }

    _Thread( int _stacksize, char * _name ) {
        stacksize = _stacksize;
        if ( stacksize != 0 )
        {
            stack = new char[ stacksize ];
            memset( stack, 0xee, stacksize );
        }
        else
            stack = NULL;
        name = new char[ strlen( _name ) + 1 ];
        strcpy( name, _name );
        switches = 0;
        print_header = true;
    }

    ~_Thread( void ) {
        if ( stack != NULL )
            delete[] stack;
        delete[] name;
    }
};

#ifdef _INCLUDE_THREAD_STATENAMES
//static
const char * _Thread::_statenames[] = { 
    // these must match thread_state enum defs at top of this file
    "TH_INVALID",
    "TH_READY",
    "TH_DEAD",
    "TH_SUSPEND",
    "TH_CURR",
    "TH_SLEEP",
    "TH_IOWAIT",
    "TH_MSGWAIT",
    "TH_SEMWAIT",
    "TH_NONEXIST"
};
#endif
