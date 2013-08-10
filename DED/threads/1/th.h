
#ifndef __TH_H_
#define __TH_H_

#ifdef __cplusplus
extern "C" {
#endif

/* threads */

void  th_init    (void);
int   th_create  (void (*function)(void *), void *arg, int prio, char *name);
int   th_tid     (void);
void  th_loop    (void);
void  th_yield   (void);
void  th_suspend (int tid);
void  th_resume  (int tid);
void  th_kill    (int tid);
void  th_sleep   (int tid);

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

#ifdef __cplusplus
}
#endif

#endif /* __TH_H_ */
