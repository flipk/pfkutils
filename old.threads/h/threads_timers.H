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

class timerParams;

enum timer_lists {
    TIMER_LIST_QUEUE, TIMER_LIST_HASH, TIMER_NUMLISTS
};

class ThreadTimers {
    LListHash     <timerParams,TIMER_LIST_HASH >  hash;
    LListOrderedQ <timerParams,TIMER_LIST_QUEUE>  oq;

    ThreadSemaphore * timer_sem;

    int tps;
    int tick;
    bool die;
    Threads::tid_t mytid;
    static void _thread( void * arg );
    void thread( void );
    void process_tick( void );
    void setup_helper( void );
    timerParams * new_timer( int ticks );

    int p[2];  // pipe from helper
    int writeretval;
    static void sighand( int s );

public:
    class constructor_failed { };
    ThreadTimers( int _tps, int hash_size );
    ~ThreadTimers( void );
    void kill( void ) { die = true; th->resume( mytid ); }
    void printtimers( FILE * );

    int get_tps() { return tps; }
    int get_tick() { return tick; }
    int get_numtimers() { return hash.get_cnt(); }

    // send message after x ticks, returns timerid
    int set( int ticks, Message * );
    // resume tid after x ticks, returns timerid
    int set( int ticks, Threads::tid_t tid );
    // cancel timerid, returns message if there was one
    bool cancel( int timerid, Message ** m );
    // cancel timerid, deletes message if there was one
    bool cancel( int timerid ) { return cancel( timerid, NULL ); }
};
