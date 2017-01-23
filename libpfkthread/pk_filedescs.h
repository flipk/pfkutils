/* -*- Mode:c++; eval:(c-set-style "BSD"); c-basic-offset:4; indent-tabs-mode:nil; tab-width:8 -*- */

/*
This is free and unencumbered software released into the public domain.

Anyone is free to copy, modify, publish, use, compile, sell, or
distribute this software, either in source code form or as a compiled
binary, for any purpose, commercial or non-commercial, and by any
means.

In jurisdictions that recognize copyright laws, the author or authors
of this software dedicate any and all copyright interest in the
software to the public domain. We make this dedication for the benefit
of the public at large and to the detriment of our heirs and
successors. We intend this dedication to be an overt act of
relinquishment in perpetuity of all present and future rights to this
software under copyright law.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.

For more information, please refer to <http://unlicense.org>
*/

#ifndef __PK_FILEDESCS_H__
#define __PK_FILEDESCS_H__

/** specify what kind of activity a file descriptor should be registered for. */
typedef enum {
    PK_FD_None = 0,     /**< no activity; used as an initializer */
    PK_FD_Read = 1,     /**< the filedesc is registered for read */
    PK_FD_Write = 2,    /**< the filedesc is registered for write */
    PK_FD_ReadWrite = 3 /**< the filedesc is registered for read and write */
} PK_FD_RW;

/** message sent when a registered filedesc becomes active 
 * \see PK_Thread::register_fd */
PkMsgIntDef(PK_FD_Activity, 0x5c27, 
            int fd;
            PK_FD_RW rw;
            void * obj;
    );

/** \var PK_FD_Activity::fd
 *       the descriptor that saw activity
 *  \var PK_FD_Activity::rw
 *       the activity that was seen descriptor
 *  \var PK_FD_Activity::obj
 *       the parameter that was registered with the descriptor. */

/** \cond INTERNAL */
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
/** \endcond */

#endif /* __PK_FILEDESCS_H__ */
