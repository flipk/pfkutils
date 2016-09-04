/* threads */

void  th_init    (void);
int   th_create  (void (*)(void *), void *, int, char *);
int   th_tid     (void);
void  th_loop    (void);
void  th_yield   (void);
void  th_suspend (int);
void  th_resume  (int);
void  th_kill    (int);
void  th_sleep   (int);

int   th_read    (int fd, char *buf, int size);
int   th_write   (int fd, char *buf, int size);
int   th_select  (int nrfds, int *rfds,
		  int nwfds, int *wfds, 
		  int nofds, int *ofds, int ticks);

/*
 * in the ofds array in th_select, if a file descriptor was
 * selected for write, the following bit will be OR'd in with 
 * the file descriptor number.
 */
#define SELECT_FOR_WRITE 0x10000
#define NUM_PRIOS 64

extern int numthreads;
extern int thread_ticks;

/* pools */

struct mpool_data {
	struct mpool_data *next;
	int magic;
};

typedef struct mpool {
	int poolmagic;
	void *start;
	void *end;
	int totalsize;
	int size;
	int free;
	unsigned int *bitmap;
	struct mpool_data *head;
} mpool;

typedef struct mpoolset {
	int numpools;
	int poolsetmagic;
	int maxbufsize;
	struct mpool *pools[1];
} mpoolset;

mpool * pool_init             (int size, int numelements);
mpool * _pool_init            (int size, int numelements, void *addr);

int     pool_init_estimate    (int size, int numelements);
void    pool_destroy          (mpool * cookie);
void *  pool_alloc            (mpool * cookie);
void    pool_free             (mpool * cookie, void * dat);

void    pool_dump             (mpool * cookie);

mpoolset * poolset_init    (int numpools, int *quantities, int *sizes);
void       poolset_destroy (mpoolset * cookie);
void *     poolset_alloc   (mpoolset * cookie, int size);
void       poolset_free    (mpoolset * cookie, void *dat);
void       poolset_dump    (mpoolset * cookie);

/* ring buffers */

typedef struct {
	char *buf;
	int start;
	int end;
	int size;
} ringbuf;

ringbuf * ringbuf_create  (int size);
void      ringbuf_destroy (ringbuf *buf);
void      ringbuf_add     (ringbuf *buf, char *dat, int size);
int       ringbuf_remove  (ringbuf *buf, char *dat, int size);
int       ringbuf_empty   (ringbuf *buf);
int       ringbuf_cursize (ringbuf *buf);

/* messages */

void       msg_init(void);
int        msg_register(int qid, mpoolset *pool);
mpoolset * msg_deregister(int qid);
int        msg_send(int qid, char *buf, int siz);
int        msg_recv(int qid, char *buf, int siz, int ms);
int        msg_recvlist(int *qids, int numqs, int *qidret,
			char *buf, int siz, int ms);

/* used internally by the MSG*SIZE macros below */
int        msg_sizes(int);

/*
 * return values
 */

#define MSG_OK           0
#define MSG_Q_EXISTS    -1
#define MSG_Q_NOT_EXIST -2
#define MSG_TIMEOUT     -3
#define MSG_Q_FULL      -4
#define MSG_2BIG        -5
#define MSG_NO_MEM      -6
#define MSG_BAD_ARG     -7
#define MSG_INTR        -8

/*
 * this is the size of the internal control struct
 * used to mark the existence of a message q.
 * there must be one buffer of this size free 
 * in the pool passed to msg_register.
 */

#define MSGQ_SIZE (msg_sizes(0))

/*
 * one of these is allocated from the pool supplied to 
 * msg_register for each message that is placed on the
 * queue.  the pool must have as many buffers of this 
 * size as it has buffers for messages themselves.
 */

#define MSG_STRUCT_SIZE (msg_sizes(1))

/* timers */

/* 
 * timer_cancel returns -3 if it couldn't cancel the timer, otherwise 0.
 * timer_sleep returns -1 if someone resumed us during sleep.  they all
 * return -2 if no mem was available for timers.
 */

#define TIMER_OK      0
#define TIMER_INTR   -1
#define TIMER_NO_MEM -2
#define TIMER_FAIL   -3

void   timer_init(void);
int    timer_create(int ms, int qid, char *msg, int len, mpoolset *pools);
int    timer_cancel(int timerid);
int    timer_sleep(int ms);
