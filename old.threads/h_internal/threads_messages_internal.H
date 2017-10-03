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

// a thread may wait on a number of messageQueues

struct waiter {
    int tid;
    int whichwoke;  // which mqid got a message? -1 until filled in
};

// a message queue is a linked list of messages

struct messageQueue {
    Message * head;
    Message * tail;
    int nummsgs;
    waiter * wait;
    char * mqname;
    messageQueue( char * _mqname ) {
        head = tail = NULL;
        nummsgs = 0;
        wait = NULL;
        int mqname_l = strlen( _mqname ) + 1;
        mqname = new char[ mqname_l ];
        memcpy( mqname, _mqname, mqname_l );
    }
    ~messageQueue( void ) {
        while ( head != NULL )
        {
            Message * nxt = head->next;
            delete head;
            head = nxt;
        }
        delete mqname;
    }
    void enqueue( Message * m ) {
        m->next = NULL;
        if ( tail )
        {
            tail->next = m;
            tail = m;
        }
        else
        {
            head = tail = m;
        }
        nummsgs++;
    }
    Message * dequeue( void ) {
        Message * ret = NULL;
        if ( nummsgs > 0 )
        {
            nummsgs--;
            ret = head;
            head = ret->next;
            if ( head == NULL )
                tail = NULL;
        }
        return ret;
    }
};

class LookupMqMsg : public Message {
public:
    static const unsigned int TYPE = MagicNumbers_Messages_LookupReq;
    LookupMqMsg( char * mqname_input )
        : Message( sizeof( LookupMqMsg ), TYPE ) {
        strcpy( mqname, mqname_input );
        size.set( size.get() + strlen( mqname_input ) + 1 );
    }
    char mqname[1];
    void * operator new( size_t s, char * ptr ) {
        char * ret = new char[s + strlen(ptr) + 1];
        return (void*)ret;
    }
};

class LookupMqReplyMsg : public Message {
public:
    static const unsigned int TYPE = MagicNumbers_Messages_LookupReply;
    LookupMqReplyMsg( void )
        : Message( sizeof( LookupMqReplyMsg ), TYPE ) { }
    UINT32_t mqid;
};
