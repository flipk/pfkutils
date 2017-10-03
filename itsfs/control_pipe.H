/*
 * This file is licensed under the GPL version 2.
 * Refer to the file LICENSE in this distribution or
 * just search for GPL v2 on the website www.gnu.org.
 */

#ifndef __CONTROL_PIPE_H_
#define __CONTROL_PIPE_H_

#if 0
enum {
    CONTROL_PIPE_EXIT,   // impossible
};

union CONTROL_PIPE_REQ {
    struct {
        uchar type;   // CONTROL_PIPE_EXIT
    } exit;
};

union CONTROL_PIPE_REPL {
    struct {
        uchar type;   // CONTROL_PIPE_EXIT
    } exit;
};
#endif

class Control_Pipe {
    uchar * data;
    int     datalen;
public:
    Control_Pipe( void );
    ~Control_Pipe( void );
    int write  ( uchar * buf, int  length );
    int read   ( uchar * buf, int &length );
    int len    ( void ) { return datalen; }
};

#endif /* __CONTROL_PIPE_H_ */
