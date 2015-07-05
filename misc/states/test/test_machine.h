
#ifndef __TEST_MACHINE_H_
#define __TEST_MACHINE_H_

struct proc_code {
	int type;
	void (*entry)();
};

typedef struct {
	short flag; /* 1 == physical */
	short mbid;
	char subsystem;
	char process;
	short cpu;
} ADDRESS;

typedef struct {
	ADDRESS source;
	ADDRESS destination;
	int tag;
	char * msg;
	short length;
	short router_info;
	char _msg[0];
} MESSAGE_TYPE;

#define MAXMSG 576
#define MAXMSGCOMBINED (576+sizeof(MESSAGE_TYPE))
#define MSG_HDR void

extern "C" {
	void printf( char *, ... );
	int e_pid( void );
	int e_set_relative( MESSAGE_TYPE *, int len );
	void e_cancel_timer( int timerid );
	void e_send( MESSAGE_TYPE * );
	void e_create_process( char *, struct proc_code *,
			       int, int, int, int );
	void e_init_process( int );
	void e_create_mb( char *, int, int, int );
	void sprintf( char *, ... );
	int e_receive( MESSAGE_TYPE *, int, int, int * );
	void e_delete_process( int );
};

#define EVENT_REG_TIMEOUT 1000
#define RMVLINK_TIMEOUT   3000
#define PCI_POWEROFF_TIMEOUT 10000
#define PCI_QUERY_TIMEOUT  5000
#define PCI_POWERON_TIMEOUT 60000
#define COM_CODELOAD_DONE_TIMEOUT 45000
#define COM_CODELOAD_REQ_TIMEOUT 3000
#define API_INIT_TIMEOUT 45000
#define EDLSP_ADDLINK_TIMEOUT 30000

#endif /* __TEST_MACHINE_H_ */
