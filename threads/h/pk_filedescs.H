/*
    This file is part of the "pkutils" tools written by Phil Knaack
    (pknaack1@netscape.net).
    Copyright (C) 2008  Phillip F Knaack

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifndef __PK_FILEDESCS_H__
#define __PK_FILEDESCS_H__

typedef enum {
    PK_FD_None = 0,
    PK_FD_Read = 1,
    PK_FD_Write = 2,
    PK_FD_ReadWrite = 3 // note this is (PK_FD_Read | PK_FD_Write)
} PK_FD_RW;

// this message is sent to the qid registered for a fd
PkMsgIntDef(PK_FD_Activity, 0x5c27, 
            int fd;
            PK_FD_RW rw;
            void * obj;
    );

class PK_File_Descriptor_List;
class PK_File_Descriptor_Thread;

class PK_File_Descriptor_Manager {
    friend class PK_File_Descriptor_Thread;
    PK_File_Descriptor_Thread * thread;
    pthread_mutex_t   mutex;
    PK_File_Descriptor_List * descs;
    void   _lock( void ) { pthread_mutex_lock  ( &mutex ); }
    void _unlock( void ) { pthread_mutex_unlock( &mutex ); }
    int wakeup_pipe[2];
public:
    PK_File_Descriptor_Manager(void);
    ~PK_File_Descriptor_Manager(void);
    void stop( void ); // used by PK_Threads to kill thread
    //
    int /*pkfdid*/ register_fd( int fd, PK_FD_RW rw, int qid, void *obj );
    // this returns the void *obj if the registration was found
    void * unregister_fd( int pkfdid );
};

extern PK_File_Descriptor_Manager * PK_File_Descriptors_global;

#endif /* __PK_FILEDESCS_H__ */
