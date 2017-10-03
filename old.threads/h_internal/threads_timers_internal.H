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

enum timer_action {
    SEND_MESSAGE,
    RESUME_TID
};

class timerParams {
public:
    timerParams( int _timerid, int ticks ) {
        next = NULL;
        timerid = _timerid;
        enabled = true;
        onq = false;
        tickrel = ticks;
        tickarg = ticks;
    }
    ~timerParams( void ) {
        if ( a == SEND_MESSAGE && mtid.m != NULL )
            delete mtid.m;
    }
    void setv( Message * m ) {
        a = SEND_MESSAGE;
        mtid.m = m;
    }
    void setv( int tid ) {
        a = RESUME_TID;
        mtid.tid = tid;
    }
    Message * get_m( void ) {
        Message * ret = NULL;
        if (a == SEND_MESSAGE)
        {
            ret = mtid.m;
            mtid.m = NULL;
        }
        return ret;
    }
    timerParams * next;   // on q list
    timerParams * hnext;  // on hash
    int timerid;
    timer_action a;
    bool onq;      // is it on the q ?
    bool enabled;  // close a race condition with cancel
    int tickarg;
    int tickrel;
    union {
        Threads::tid_t tid;
        Message *m;
    } mtid;
};

enum command_action {
    SET_TIMER,
    CANCEL_TIMER
};

class timerCommand {
public:
    timerCommand( void ) { next = NULL; }
    timerCommand * next;
    command_action a;
    timerParams * p;
};
