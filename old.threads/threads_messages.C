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

#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/time.h>
#include <signal.h>

#include "threads.H"
#include "threads_internal.H"
#include "threads_messages_internal.H"
#include "threads_timers_internal.H"

ThreadMessages :: ThreadMessages( int _max_mqids, int _my_eid,
                                  int _max_eids, int _max_fds )
{
    int i;

    max_mqids = _max_mqids;
    my_eid = _my_eid;
    max_eids = _max_eids;
    max_fds = _max_fds;

    mqs = new messageQueue*[max_mqids];

    for ( i = 0; i < max_mqids; i++ )
        mqs[i] = NULL;

    eid_mqids = new int[max_eids];

    for ( i = 0; i < max_eids; i++ )
        eid_mqids[i] = -1;

    fd_mqids = new fd_mqids_t[max_fds];

    // a thread which provides a nifty file-descriptor-to-mailbox
    // gateway. this thread will select on any fds you wish and
    // will send you indications when activity is required on them.
    // thanks to this, threads which deal with multiple file descriptors
    // *and* use messages, can do it all from one main event loop that
    // just waits for messages.

    fd_helper_tid = th->create( "msgsfd", Threads::NUM_PRIOS-1,
                                16384, false,
                                msgs_fd, (void*)this );

    mqs[0] = new messageQueue( (char*)lookup_mqname );

    // a task which does nothing but respond to 
    // lookup request messages.

    lookup_helper_tid = th->create( "lookupmq", Threads::NUM_PRIOS-1,
                                    16384, false,
                                    lookup_thd, (void*)this );
}

const char * ThreadMessages::lookup_mqname = "Lookups";

ThreadMessages :: ~ThreadMessages( void )
{
    int i;

    for ( i = 0; i < max_mqids; i++ )
        if ( mqs[i] != NULL )
            delete mqs[i];

    delete[] eid_mqids;
    delete[] fd_mqids;
    delete[] mqs;
}

void
ThreadMessages :: printmqs( FILE * f )
{
    int i;
    messageQueue * q;

#define FORMAT1 "mqid","mqname","msgs","waiter"
#define FORMAT2 "%5s %10s %4s %6s\n"
#define FORMAT3 "%5d %10s %4d %6d\n"
#define FORMAT4 "----------------------------\n"

    fprintf( f, FORMAT2, FORMAT1 );
    fprintf( f, FORMAT4 );

    for ( i = 0; i < max_mqids; i++ )
        if (( q = mqs[i] ) != NULL )
            fprintf( f, FORMAT3, i,
                     q->mqname, q->get_count(),
                     q->wait == NULL ? -1 :
                     q->wait->tid );

#undef  FORMAT1
#undef  FORMAT2
#undef  FORMAT3
}

bool
ThreadMessages :: send( Message * m, MessageAddress * addr )
{
    int mqid = -1;
    int eid = addr->eid.get();

    if ( th->debug & ThreadParams::DEBUG_MSGSCONTENTS )
    {
        char * dest_mid_name = get_mqname( m->dest.mid.get() );
        if ( dest_mid_name == NULL   ||
             strcmp( dest_mid_name, "printer" ) != 0 )
        {
            printf( "thread %d(%s) sending msg\n",
                    th->tid(), th->tid_name(th->tid()));
            printf( "  type %x src %d:%d(%s) dest %d:%d(%s)\n"
                    "    ",
                    m->type.get(),
                    m->src.eid.get(), m->src.mid.get(),
                    get_mqname( m->src.mid.get() ),
                    m->dest.eid.get(), m->dest.mid.get(),
                    get_mqname( m->dest.mid.get() ));

            UCHAR * body = m->get_body();
            int s = m->get_size();
            for ( int i = 0; i < s; i++ )
            {
                printf( "%02x ", body[i] );
                if (( i & 15 ) == 15 )
                    printf( "\n    " );
            }
            printf( "\n" );
        }
    }

    if ( eid == my_eid || eid == 0 )
    {
        mqid = addr->mid.get();
    }
    else
    {
        if ( eid < max_eids )
            mqid = eid_mqids[eid];
        else
        {
            TH_DEBUG( DEBUG_MSGS,
                      ( 0, "thmsgs", "send : invalid eid %d", eid ));
            return false;
        }
    }

    if ( mqid == -1 || (unsigned)mqid >= (unsigned)max_mqids )
    {
        TH_DEBUG( DEBUG_MSGS,
                  ( 0, "thmsgs", "send : invalid "
                    "mqid %d (eid was %d)", mqid, eid ));
        return false;
    }

    if ( mqs[mqid] == NULL )
    {
        TH_DEBUG( DEBUG_MSGS,
                  ( 0, "thmsgs", "send : mqid %d doesn't exist", mqid ));
        return false;
    }

    m->source_tid.set( th->tid() );
    mqs[mqid]->enqueue( m );
    if ( mqs[mqid]->wait != NULL &&
         mqs[mqid]->wait->whichwoke == -1 )
    {
        mqs[mqid]->wait->whichwoke = mqid;
        th->resume( mqs[mqid]->wait->tid );
    }

    return true;
}

Message *
ThreadMessages :: recv( int numqids, int * mqids,
                        int * mqid_out, int timeout )
{
    waiter wtr;
    int i, mqid;

    wtr.tid = th->tid();
    wtr.whichwoke = -1;

    for ( i = 0; i < numqids; i++ )
    {
        mqid = mqids[i];
        if ( (unsigned)mqid >= (unsigned)max_mqids )
        {
            TH_DEBUG( DEBUG_MSGS,
                      ( 0, "thmsgs", "recv : mqid %d invalid value", mqid ));
            continue; // skip this mqid
        }
        if ( mqs[mqid] == NULL )
        {
            TH_DEBUG( DEBUG_MSGS,
                      ( 0, "thmsgs", "recv : mqid %d doesn't exist", mqid ));
            continue; // skip
        }
        if ( mqs[mqid]->get_count() > 0 )
        {
            wtr.whichwoke = mqid;
            numqids = i;
            break;
        }
        if ( mqs[mqid]->wait != NULL )
        {
            TH_DEBUG( DEBUG_MSGS,
                      ( 0, "thmsgs", "recv : mqid %d already "
                        "has waiter (tid %d)", mqid, mqs[mqid]->wait->tid ));
            continue; // skip
        }
        mqs[mqid]->wait = &wtr;
    }

    if ( wtr.whichwoke == -1 )
    {
        if ( timeout != Threads::NO_WAIT )
        {
            int timerid = -1;
            if ( timeout != Threads::WAIT_FOREVER )
                timerid = th->timers->set( timeout, th->current->tid );
            th->current->state = TH_MSGWAIT;
            th->reschedule();
            if ( timerid != -1 )
                (void) th->timers->cancel( timerid );
        }
    }

    for ( i = 0; i < numqids; i++ )
    {
        mqid = mqids[i];
        if ( mqs[mqid] != NULL && mqs[mqid]->wait == &wtr )
            mqs[mqid]->wait = NULL;
    }

    Message * ret = NULL;

    if ( wtr.whichwoke != -1 )
    {
        if ( mqid_out != NULL )
            *mqid_out = wtr.whichwoke;
        ret = mqs[wtr.whichwoke]->dequeue();
    }

    if ( ret && ( th->debug & ThreadParams::DEBUG_MSGSCONTENTS ))
    {
        char * dest_mq_name = get_mqname( ret->dest.mid.get() );
        if ( dest_mq_name == NULL   ||
             strcmp( dest_mq_name, "printer" ) != 0 )
        {
            printf( "thread %d(%s) received msg from %d(%s):\n",
                    th->tid(), th->tid_name(th->tid()),
                    ret->source_tid.get(),
                    th->tid_name(ret->source_tid.get()));
            printf( "  type %x src %d:%d(%s) dest %d:%d(%s)\n"
                    "    ",
                    ret->type.get(),
                    ret->src.eid.get(), ret->src.mid.get(),
                    get_mqname( ret->src.mid.get() ),
                    ret->dest.eid.get(), ret->dest.mid.get(),
                    get_mqname( ret->dest.mid.get() ));
            UCHAR * body = ret->get_body();
            int s = ret->get_size();
            for ( int i = 0; i < s; i++ )
            {
                printf( "%02x ", body[i] );
                if (( i & 15 ) == 15 )
                    printf( "\n    " );
            }
            printf( "\n" );
        }
    }

    return ret;
}

bool
ThreadMessages :: register_mq( int &mqid, char * mqname )
{
    for ( mqid = 1; mqid < max_mqids; mqid++ )
        if ( mqs[mqid] == NULL )
            break;

    if ( mqid == max_mqids )
    {
        TH_DEBUG_ALL(( 0, "thmsgs", "register_mq : out of mqids" ));
        return false;
    }

    if ( mqs[mqid] != NULL )
    {
        TH_DEBUG( DEBUG_MSGS,
                  ( 0, "thmsgs", "mq %d (%s) exists (making %s)",
                    mqid, mqs[mqid]->mqname, mqname ));
        return false;
    }

    mqs[mqid] = new messageQueue( mqname );
    return true;
}

char *
ThreadMessages :: get_mqname( int mqid )
{
    if ( mqid < max_mqids && mqs[mqid] != NULL )
        return mqs[mqid]->mqname;
    return NULL;
}

bool
ThreadMessages :: lookup_mq( int eid, int &ret_mqid, char * mqname )
{
    bool ret = false;
    if ( eid == 0 || eid == my_eid )
    {
        int i;
        for ( i = 0; i < max_mqids; i++ )
            if ( mqs[i] && strcmp( mqs[i]->mqname, mqname ) == 0 )
                break;
        if ( i == max_mqids )
            return false;
        ret_mqid = i;
        ret = true;
    }
    else
    {
        int mqid;

        if ( register_mq( mqid, "lookup temp reply" ) == false )
        {
            th->printf( "could not register lookup "
                        "temp reply mq\n" );
            return false;
        }

        int retries = 0;
        bool send_req = true;

        while ( retries < 3 )
        {
            union {
                Message * m;
                LookupMqMsg * lmm;
                LookupMqReplyMsg * lrm;
            } m;

            if ( send_req )
            {
                m.lmm = new(mqname) LookupMqMsg(mqname);
                m.lmm->src.set( my_eid, mqid );
                m.lmm->dest.set( eid, 0 );
                if ( send( m.lmm, &m.lmm->dest ) == false )
                {
                    th->printf( "could not send lookup request\n" );
                    delete m.lmm;
                    break;
                }
                send_req = false;
            }

            m.m = recv( 1, &mqid, NULL, th->timers->get_tps() );
            if ( m.m == NULL )
            {
                th->printf( "lookup reply timeout\n" );
                retries++;
                send_req = true;
                continue;
            }

            switch ( m.m->type.get() )
            {
            case LookupMqReplyMsg::TYPE:
                retries = 99;
                ret_mqid = m.lrm->mqid.get();
                if ( ret_mqid == -1 )
                    th->printf( "lookup reply : mq %s not found\n", mqname );
                else
                    ret = true;
                break;

            default:
                th->printf( "lookup reply was type %x!\n", 
                            m.m->type.get() );
            }

            delete m.m;
        }

        (void) unregister_mq( mqid );
    }
    return ret;
}

bool
ThreadMessages :: unregister_mq( int mqid )
{
    if ( (unsigned)mqid >= (unsigned)max_mqids )
    {
        TH_DEBUG( DEBUG_MSGS,
                  ( 0, "thmsgs", "unregister_mq : invalid mqid %d", mqid ));
        return false;
    }

    if ( mqs[mqid] == NULL )
    {
        TH_DEBUG( DEBUG_MSGS,
                  ( 0, "thmsgs", "unregister_mq : mqid %d "
                    "doesn't exist", mqid ));
        return false;
    }

    delete mqs[mqid];
    mqs[mqid] = NULL;

    return true;
}

bool
ThreadMessages :: register_eid( int eid, int mqid )
{
    if ( (unsigned)eid > (unsigned)max_eids )
    {
        TH_DEBUG_ALL(( 0, "thmsgs", "register_eid : invalid eid %d", eid ));
        return false;
    }

    if ( eid_mqids[eid] != -1 )
    {
        TH_DEBUG( DEBUG_MSGS,
                  ( 0, "thmsgs", "register_eid : eid %d already "
                    "registered!", eid ));
        return false;
    }

    eid_mqids[eid] = mqid;
    return true;
}

bool
ThreadMessages :: unregister_eid( int eid )
{
    if ( (unsigned)eid > (unsigned)max_eids )
    {
        TH_DEBUG_ALL(( 0, "thmsgs", "unregister_eid : invalid eid %d", eid ));
        return false;
    }

    if ( eid_mqids[eid] == -1 )
    {
        TH_DEBUG( DEBUG_MSGS,
                  ( 0, "thmsgs", "unregister_eid : eid %d is not "
                    "registered!", eid ));
        return false;
    }

    eid_mqids[eid] = -1;
    return true;
}



bool
ThreadMessages :: register_fd_mq( int fd, void * arg,
                                  ThreadMessages::fd_mq_t activity,
                                  int mqid )
{
    if ( (unsigned)fd >= (unsigned)max_fds )
    {
        TH_DEBUG_ALL(( 0, "thmsgs", "register_read_fd_mq : "
                       "fd %d is too high (> %d)", fd, max_fds ));
        return false;
    }

    if ( fd_mqids[fd].mqid != -1  &&
         ( fd_mqids[fd].mqid != mqid  ||  fd_mqids[fd].arg != arg ))
    {
        TH_DEBUG( DEBUG_MSGS,
                  ( 0, "thmsgs", "register_read_fd_mq : "
                    "fd %d already registered!", fd ));
        return false;
    }

    fd_mqids[fd].mqid = mqid;
    fd_mqids[fd].type = activity;
    fd_mqids[fd].arg  = arg;

    th->resume( fd_helper_tid );
    return true;
}

bool
ThreadMessages :: unregister_fd_mq( int fd )
{
    if ( (unsigned)fd >= (unsigned)max_fds )
    {
        TH_DEBUG_ALL(( 0, "thmsgs", "unregister_read_fd_mq : "
                       "fd %d is too high (> %d)", fd, max_fds ));
        return false;
    }

    if ( fd_mqids[fd].mqid == -1 )
    {
        TH_DEBUG( DEBUG_MSGS,
                  ( 0, "thmsgs", "unregister_read_fd_mq : "
                    "fd %d is not registered!", fd ));
        return false;
    }

    _unregister_fd_mq( fd );
    th->resume( fd_helper_tid );
    return true;
}

void
ThreadMessages :: _unregister_fd_mq( int fd )
{
    fd_mqids[fd].mqid = -1;
    fd_mqids[fd].type = FOR_NOTHING;
    fd_mqids[fd].arg  = NULL;
}



// static
void
ThreadMessages :: msgs_fd( void * arg )
{
    ThreadMessages * tm = (ThreadMessages *)arg;
    tm->_msgs_fd();
}

void
ThreadMessages :: _msgs_fd( void )
{
    int * myrfds = new int[max_fds];
    int * mywfds = new int[max_fds];
    int numrfds;
    int numwfds;
    int i;

    die = false;
    do {
        int r;

        numrfds = numwfds = 0;

        for ( i = 0; i < max_fds; i++ )
            switch ( fd_mqids[i].type )
            {
            case FOR_READ:
                myrfds[numrfds++] = i;
                break;
            case FOR_WRITE:
                mywfds[numwfds++] = i;
                break;
            case FOR_READWRITE:
                myrfds[numrfds++] = i;
                mywfds[numwfds++] = i;
                break;
            case FOR_NOTHING:
            case FOR_ERROR:
                break;
            }


        if ( numrfds == 0 && numwfds == 0 )
        {
            th->suspend( 0 );
        }
        else
        {
            int out[10];
            r = th->select( numrfds, myrfds,
                            numwfds, mywfds,
                            10, out, 
                            Threads :: WAIT_FOREVER );
            TH_DEBUG( DEBUG_MSGSFD,
                      ( 0, "msgs_fd", "select %d(%d)", r, errno ));
            if ( r > 0 )
            {
                send_indications( r, out );
            }
            if ( r < 0 && errno == EBADF )
            {
                send_error_indication( out[0] );
            }
        }

    } while ( die == false );

    delete[] myrfds;
    delete[] mywfds;
}

void
ThreadMessages :: send_indications( int num, int * outs )
{
    for ( num--; num >= 0; num-- )
    {
        int fd = outs[num] & ~Threads::SELECT_FOR_WRITE;

        FdActiveMessage * fam = new FdActiveMessage;

        fam->fd = fd;
        fam->activity = 
            (outs[num] & Threads::SELECT_FOR_WRITE) ?
            FOR_WRITE : FOR_READ;

        fam->dest.set( fd_mqids[fd].mqid );
        fam->arg = fd_mqids[fd].arg;

        // unregister before we've sent the indications
        // so that we don't immediately drop into the loop
        // again and send another (ad infinitum).
        // if we were to unreg after sending the indication
        // a recipient running at the same prio as us might
        // resume immediately and have problems reregistering.

        TH_DEBUG( DEBUG_MSGSFD,
                  ( 0, "thmsgs", "ind for fd %d to mqid %d",
                    fd, fd_mqids[fd].mqid ));

        _unregister_fd_mq( fd );

        if ( send( fam, &fam->dest ) == false )
        {
            TH_DEBUG( DEBUG_MSGSFD,
                      ( 0, "thmsgs", "msgs_fd : "
                        "failure sending message for fd %d", fd ));
            delete fam;
        }
    }
}

void
ThreadMessages :: send_error_indication( int fd )
{
    int mqid = fd_mqids[fd].mqid;
    _unregister_fd_mq( fd );

    TH_DEBUG( DEBUG_MSGSFD,
              ( 0, "thmsgs", "sending error indication for fd %d", fd ));
    FdActiveMessage * fam = new FdActiveMessage;

    fam->fd = fd;
    fam->activity = FOR_ERROR;
    fam->dest.set( mqid );

    if ( send( fam, &fam->dest ) == false )
    {
        TH_DEBUG( DEBUG_MSGSFD,
                  ( 0, "thmsgs", "send_error_indication : "
                    "error sending error indication for fd %d", fd ));
        delete fam;
    }
}

void
ThreadMessages :: lookup_thd( void * arg )
{
    ThreadMessages * thm = (ThreadMessages *)arg;
    thm->_lookup_thd();
}

void
ThreadMessages :: _lookup_thd( void )
{
    int mqid = 0;

    while ( 1 )
    {
        union {
            Message * m;
            LookupMqMsg * lmm;
        } m;

        m.m = recv( 1, &mqid, NULL, Threads::WAIT_FOREVER );
        if ( die == true )
            break;
        if ( m.m == NULL )
            continue;

        if ( m.m->type.get() == LookupMqMsg::TYPE )
        {
            int mqid = -1;
            LookupMqReplyMsg * repl = new LookupMqReplyMsg;
            if ( lookup_mq( 0, mqid, m.lmm->mqname ) == false )
                th->printf( "lookup helper: mqname %s "
                            "not found\n",
                            m.lmm->mqname );
            // if lookup_mq fails, we just send back -1
            repl->mqid.set( mqid );
            repl->dest = m.lmm->src;
            if ( send( repl, &repl->dest ) == false )
            {
                th->printf( "lookup helper: couldn't "
                            "send reply\n" );
                delete repl;
            }
        }

        delete m.m;
    }
}
