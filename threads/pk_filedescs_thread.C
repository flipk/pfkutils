/*
 * This file is licensed under the GPL version 2.
 * Refer to the file LICENSE in this distribution or
 * just search for GPL v2 on the website www.gnu.org.
 */

#include "pk_threads.H"
#include "pk_filedescs_internal.H"

#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <signal.h>
#include <unistd.h>
#include <sys/select.h>

PK_File_Descriptor_Thread :: PK_File_Descriptor_Thread(
    PK_File_Descriptor_Manager * _mgr )
{
    mgr = _mgr;
    set_name( (char*)"PK_FD" );
    resume();
}

PK_File_Descriptor_Thread :: ~PK_File_Descriptor_Thread( void )
{
    // ?
}

void
PK_File_Descriptor_Thread :: stop( void )
{
    char c = 2;
    write(mgr->wakeup_pipe[1], &c, 1);
}

void
PK_File_Descriptor_Thread :: entry( void )
{
    int mgmt_pipe = mgr->wakeup_pipe[0];

    while (1)
    {
        fd_set rfds, wfds;
        FD_ZERO( &rfds );
        FD_ZERO( &wfds );
        PK_File_Descriptor * fd, * nfd;

        mgr->_lock();
        int max = mgmt_pipe;
        FD_SET(mgmt_pipe, &rfds);
        for (fd = mgr->descs->get_head();
             fd;
             fd = mgr->descs->get_next(fd))
        {
            if (fd->fd > max)
                max = fd->fd;
            if (fd->rw & PK_FD_Read)
                FD_SET(fd->fd, &rfds);
            if (fd->rw & PK_FD_Write)
                FD_SET(fd->fd, &wfds);
        }
        mgr->_unlock();

        int cc = select( max+1, &rfds, &wfds, NULL, NULL );

        if (FD_ISSET(mgmt_pipe, &rfds))
        {
            char c;
            read(mgmt_pipe,&c,1);
            if (c == 2)
                // we've been instructed to die by the stop() method
                break;
        }
        if ( cc > 0 )
        {
            mgr->_lock();
            for (fd = mgr->descs->get_head(); fd; fd = nfd)
            {
                nfd = mgr->descs->get_next(fd);
                int /*PK_FD_RW*/ rw = PK_FD_None;

                if (fd->rw & PK_FD_Read)
                    if (FD_ISSET(fd->fd, &rfds))
                        rw |= PK_FD_Read;
                if (fd->rw & PK_FD_Write)
                    if (FD_ISSET(fd->fd, &wfds))
                        rw |= PK_FD_Write;

                if (rw != PK_FD_None)
                {
                    PK_FD_Activity * m = new PK_FD_Activity;
                    m->fd = fd->fd;
                    m->rw = (PK_FD_RW)rw;
                    m->obj = fd->obj;
                    m->src_q = (unsigned int)-1;
                    m->dest_q = fd->qid;

                    if (msg_send(fd->qid, m) == false)
                    {
                        fprintf(stderr,
                                "failure sending PK_FD_Activity msg!\n");
                        delete m;
                    }

                    mgr->descs->remove(fd);
                    delete fd;
                }
            }

            mgr->_unlock();
        }
    }
}
