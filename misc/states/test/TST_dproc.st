
# this defines the name of the class: "%s_STATE_MACHINE".

machine	 DPROC

# this is the private data stored in the class and used by
# the user-written custom methods.

machine_data
%{
	int slot;
	int mailbox;
	void send_msg( int pid, int mbid, int tag,
		       char * m, int len );
	enum { SOI_INIT, SOI_STOP } stop_or_init;
	int timer_seq;
	int retries;
	static const int MAX_RETRIES = 4;
	bool timer_set;
	int timer_id;
	int last_tag;
	int last_timerid;
%}

# this defines the arguments to the constructor for this class.

machine_cargs %{ int _slot, int _mailbox %}

# this is the code for the constructor

machine_ccode %{ slot = _slot; mailbox = _mailbox; retries = 0; %}

# this is the code for the destructor

machine_dcode %{ %}

starthdr
%{
#include "test_machine.H"
%}

# there must be a method in the class called
# input_discriminator, which takes a MESSAGE_TYPE*
# and returns an enum from this list. note that
# for each message type, the discriminator could
# also populate items in the machine_data section above.
#
# note that we have chosen to implement two versions
# of some input messages (an "ok" and a "fail" version).
# in this example we have chosen to have the input
# discriminator look inside the messages received 
# to differentiate success/fail cases.	note that we
# didn't have to do this!  we could have had only one,
# and have the transition code decide what action to take
# and what next-state to take after looking inside.  (note
# that in order to make that work we would have had to add
# many more "call" directives to look inside those messages
# within the states that needed them.)
#
# we chose this way because the messages are pretty simple,
# and therefore the state definitions in this file can
# describe more of the machine, with the transition functions
# being simpler.

inputs {
	START_REQ
	STOP_REQ
	PCI_EVENT_REG_REPLY
	PCI_EVENT_DEREG_REPLY
	PCI_QUERY_REPLY_ok
	PCI_QUERY_REPLY_fail
	PCI_POWERON_REPLY_ok
	PCI_POWERON_REPLY_fail
	PCI_POWEROFF_REPLY
	PCI_RESET_REPLY_ok
	PCI_RESET_REPLY_fail
	COM_CODELOAD_STARTED_REPLY_ok
	COM_CODELOAD_STARTED_REPLY_fail
	COM_CODELOAD_NOT_NECESSARY
	COM_CODELOAD_COMPLETE_REPLY
	PCI_EVENT_API_INIT
	PCI_EVENT_DEAD
	PCI_EVENT_ALIVE
	EDLSP_ADDLINK_REPLY_ok
	EDLSP_ADDLINK_REPLY_fail
	EDLSP_RMVLINK_REPLY
}

# note that there is an automatically-added implicit input
# which does not need to be specified on this list but which 
# does actually exist.  that is INPUT_TIMEOUT.  since we can put
# guard timers on certain events, this is the input event which
# will occur if a timeout period expires.

# there must be a method in the class called 
# output_generator, which takes one of these enum
# values as an argument, and builds and sends a message
# corresponding to this value.	note that to send these messages,
# the output_generator may use any data from the machine_data
# section above.

outputs {

# the state machine has "crashed" and terminated.
	DEAD

# this means machine is done with initialization and
# is ready to receive a START_REQ
	STARTED

# this means machine has given up and returned to init state.
	STOPPED

# this is in response to a START_REQ; it means the dproc is now
# up and running, ready to be used.
	DPROC_IN_SERVICE

# there are two forms to the out-of-service indication.	 the "fail"
# case is when something abnormal happened, such as a dproc reset
# or removal.  the "normal" is simply a response to a STOP_REQ or
# START_REQ while in the RUNNING state, to indicate that the machine
# is complying with the request to take the dproc out of service.
	DPROC_OUT_SERVICE_fail
	DPROC_OUT_SERVICE_normal

# the following are messages sent to perform the work
# that this state machine does -- bringing boards up and
# starting the slave NIBs.
	PCI_EVENT_REG
	PCI_EVENT_DEREG
	PCI_QUERY
	PCI_POWERON
	PCI_POWEROFF
	PCI_RESET
	COM_CODELOAD_REQ
	EDLSP_ADDLINK_REQ
	EDLSP_RMVLINK_REQ
	START_NIB1
	START_NIB2

# the "normal" cases are when it is due to a software-initiated
# situation; the "fail" cases are due to error circumstances.  the
# difference is that in the "normal" case we are able to send messages
# to shut down things on the NIB, the "fail" case assumes that the
# parent dproc is simply no longer able to support such messages,
# so we should merely reinit the state machine for that device and
# wait for another start req.
	STOP_NIB1_normal
	STOP_NIB1_fail
	STOP_NIB2_normal
	STOP_NIB2_fail
}

# this is the set of states in the machine.  we enumerate them here
# as an extra sanity check -- "state" directives below that do not have
# an entry here are an error, and entries here which do not have a "state"
# directive are also an error.

states {
	INIT
	REMOVELINK
	POWEROFF
	wait_RESTART
	START
	QUERY
	POWERON
	CODELOAD
	EVENT_REG
	wait_CODELOAD_COMPLETE
	wait_API_INIT
	ADDLINK
	RUNNING
	SOFT_RESET1
	SOFT_RESET2
	API_INIT_or_STOP
	REMOVE_WAIT_ADD1
	REMOVE_WAIT_ADD2
}

# a state machine consists of a set of states (periods of rest)
# and a set of work items to be performed during transitions.
# there are two types of work that can be performed.
#  - pre-msgwait : this is a list of work items to be performed
#	whenever a state is entered but before we go to rest
#	waiting for the next message.  it is often convenient
#	to have something occur each time a state is entered.
#	(For instance, in the case where a request is sent and
#	a response is expected, the timeout and retransmit case
#	can be handled in only one state, and with only one piece
#	of code that generates the request. Another example: a
#	state which is a common destination from other places in
#	the state machine, may have a piece of work to perform
#	prior to entry to that state, that otherwise each prior
#	state would have to implement repeatedly.)
# - input : this is a list of work items to be performed when a
#	specific message arrives, for instance to call some work
#	function or proceed to some 'next' state.

# note that it is intended that this file should specify all messages
# which are input to and output from all states.  messages should not be
# sent from anywhere except the output_generator.  this may seem silly,
# since in many cases it might actually be easier to write the code to
# send the messages directly from the call-methods; its even possible to
# have the call methods set the next_state field directly.  however, is
# that as readable?  no. the intent is that a reader can look at this
# file (and this file alone!) and get a complete picture of the sequence
# of states.  note that this also means the values of the call-method
# return enums should be well-chosen meaningful human-readable strings
# that indicate some level of information about what it really means.

state INIT {

	pre {
		output PCI_EVENT_DEREG
		timeout EVENT_REG_TIMEOUT
	}

	input PCI_EVENT_DEREG_REPLY {
		inline %{ retries = 0; %}
		next REMOVELINK
	}

	input TIMEOUT_INPUT {
		inline %{ retries++; %}
		switch retries {
			case MAX_RETRIES {
				output DEAD
				exit
			}
			default {
				next INIT
			}
		}
	}
}

state REMOVELINK {
	pre {
		output EDLSP_RMVLINK_REQ
		timeout RMVLINK_TIMEOUT
	}

	input EDLSP_RMVLINK_REPLY {
		next POWEROFF
	}

	input TIMEOUT_INPUT {
		next REMOVELINK
	}
}

state POWEROFF {
	pre {
		output PCI_POWEROFF
		timeout PCI_POWEROFF_TIMEOUT
	}

	input PCI_POWEROFF_REPLY {
		next START
	}

	input TIMEOUT_INPUT {
		next POWEROFF
	}

	input START_REQ {
		next wait_RESTART
	}
}

state wait_RESTART {
	pre {
		timeout PCI_POWEROFF_TIMEOUT
	}

	input TIMEOUT_INPUT {
		next INIT
	}

	input PCI_POWEROFF_REPLY {
		next QUERY
	}
}

state START {
	pre {
		output STARTED
	}

	input START_REQ {
		next QUERY
	}

	input STOP_REQ {
		next START
	}
}

state QUERY {
	pre {
		output PCI_QUERY
		timeout PCI_QUERY_TIMEOUT
	}

	input PCI_QUERY_REPLY_ok {
		next POWERON
	}

	input PCI_QUERY_REPLY_fail {
		output STOPPED
		next START
	}

	input TIMEOUT_INPUT {
		output STOPPED
		next INIT
	}
}

state POWERON {
	pre {
		output PCI_POWERON
		timeout PCI_POWERON_TIMEOUT
	}

	input PCI_POWERON_REPLY_ok {
		next CODELOAD
	}

	input PCI_POWERON_REPLY_fail {
		output STOPPED
		next START
	}

	input TIMEOUT_INPUT {
		output STOPPED
		next INIT
	}
}

state CODELOAD {
	pre {
		output COM_CODELOAD_REQ
		timeout COM_CODELOAD_REQ_TIMEOUT
	}

	input COM_CODELOAD_STARTED_REPLY_ok {
		next EVENT_REG
	}

	input COM_CODELOAD_STARTED_REPLY_fail {
		output STOPPED
		next INIT
	}

	input COM_CODELOAD_NOT_NECESSARY {
		next ADDLINK
	}

	input TIMEOUT_INPUT {
		output STOPPED
		next INIT
	}
}

state EVENT_REG {
	pre {
		output PCI_EVENT_REG
		timeout EVENT_REG_TIMEOUT
	}

	input PCI_EVENT_REG_REPLY {
		next wait_CODELOAD_COMPLETE
	}

	input TIMEOUT_INPUT {
		output STOPPED
		next INIT
	}
}

state wait_CODELOAD_COMPLETE {
	pre {
		timeout COM_CODELOAD_DONE_TIMEOUT
	}

	input COM_CODELOAD_COMPLETE_REPLY {
		next wait_API_INIT
	}

	input TIMEOUT_INPUT {
		output STOPPED
		next INIT
	}
}

state wait_API_INIT {
	pre {
		timeout API_INIT_TIMEOUT
	}

	input PCI_EVENT_API_INIT {
		next ADDLINK
	}

	input TIMEOUT_INPUT {
		output STOPPED
		next INIT
	}
}

state ADDLINK {
	pre {
		output EDLSP_ADDLINK_REQ
		timeout EDLSP_ADDLINK_TIMEOUT
	}

	input EDLSP_ADDLINK_REPLY_ok {
		next RUNNING
	}

	input EDLSP_ADDLINK_REPLY_fail {
		output STOPPED
		next INIT
	}

	input TIMEOUT_INPUT {
		output STOPPED
		next INIT
	}
}

state RUNNING {
	pre {
		output START_NIB1
		output START_NIB2
		output DPROC_IN_SERVICE
	}

	input PCI_EVENT_API_INIT {
		inline %{ stop_or_init = SOI_INIT; %}
		output DPROC_OUT_SERVICE_fail
		output STOP_NIB1_fail
		output STOP_NIB2_fail
		next API_INIT_or_STOP
	}

	input PCI_EVENT_DEAD {
		output DPROC_OUT_SERVICE_fail
		output STOP_NIB1_fail
		output STOP_NIB2_fail
		next REMOVE_WAIT_ADD1
	}

	input STOP_REQ {
		inline %{ stop_or_init = SOI_STOP; %}
		output DPROC_OUT_SERVICE_normal
		output STOP_NIB1_normal
		output STOP_NIB2_normal
		next API_INIT_or_STOP
	}

	input START_REQ {
		output STOP_NIB1_normal
		output STOP_NIB2_normal
		output DPROC_OUT_SERVICE_normal
		next SOFT_RESET1
	}
}

state SOFT_RESET1 {
	pre {
		output EDLSP_RMVLINK_REQ
		timeout RMVLINK_TIMEOUT
	}

	input EDLSP_RMVLINK_REPLY {
		next SOFT_RESET2
	}

	input TIMEOUT_INPUT {
		output STOPPED
		next INIT
	}
}

state SOFT_RESET2 {
	pre {
		output PCI_RESET
		timeout PCI_POWERON_TIMEOUT
	}

	input PCI_RESET_REPLY_ok {
		# in case the soft reset failed and it
		# actually did a hard reset, jump to CODELOAD
		next CODELOAD
	}

	input PCI_RESET_REPLY_fail {
		output STOPPED
		next INIT
	}

	input TIMEOUT_INPUT {
		output STOPPED
		next INIT
	}
}

state API_INIT_or_STOP {
	pre {
		output EDLSP_RMVLINK_REQ
		timeout RMVLINK_TIMEOUT
	}

	input EDLSP_RMVLINK_REPLY {
		switch stop_or_init {
			case SOI_STOP { next ADDLINK }
			case SOI_INIT { next INIT }
		}
	}

	input TIMEOUT_INPUT {
		output STOPPED
		next INIT
	}
}

state REMOVE_WAIT_ADD1 {
	pre {
		output EDLSP_RMVLINK_REQ
		timeout RMVLINK_TIMEOUT
	}

	input EDLSP_RMVLINK_REPLY {
		next REMOVE_WAIT_ADD2
	}

	input TIMEOUT_INPUT {
		output STOPPED
		next INIT
	}
}

state REMOVE_WAIT_ADD2 {
	pre {
		timeout API_INIT_TIMEOUT
	}

	input PCI_EVENT_ALIVE {
		next CODELOAD
	}

	input TIMEOUT_INPUT {
		output STOPPED
		next INIT
	}
}

endhdr
%{
%}

endimpl
%{

#define TIMEOUT_TAG 0x41004100

void
DPROC_STATE_MACHINE :: send_msg( int pid, int mbid, int tag,
				 char * m, int len )
{
	MESSAGE_TYPE msg;
	msg.destination.flag = 1;
	msg.destination.mbid = mbid;
	msg.destination.subsystem = 0;
	msg.destination.process = pid;
	msg.destination.cpu = 0;
	msg.source.flag = 1;
	msg.source.mbid = mailbox;
	msg.source.subsystem = 0;
	msg.source.process = 0;
	msg.source.cpu = 0;
	msg.tag = tag;
	msg.msg = m;
	msg.length = len;
	msg.router_info = 0;
	e_send( &msg );
}

int
DPROC_STATE_MACHINE :: input_discriminator( void * m )
{
	MESSAGE_TYPE * msg = (MESSAGE_TYPE *)m;
	last_tag = msg->tag;
	switch ( msg->tag )
	{
	case 0x12345678: return START_REQ;
	case 0x12345679: return STOP_REQ;
	case 0x8285:	return PCI_EVENT_REG_REPLY;
	case 0x8286:	return PCI_EVENT_DEREG_REPLY;
	case 0x8282:
		if ( msg->msg[2] == 0 )
			return PCI_QUERY_REPLY_ok;
		else
			return PCI_QUERY_REPLY_fail;
	case 0x8283:
		if ( msg->msg[1] == 0 )
			return PCI_POWERON_REPLY_ok;
		else
			return PCI_POWERON_REPLY_fail;
	case 0x8284:	return PCI_POWEROFF_REPLY;
	case 0x8281:
		if ( msg->msg[2] == 0 )
			return PCI_RESET_REPLY_ok;
		else
			return PCI_RESET_REPLY_fail;
	case 0x1aab:
		if ( msg->msg[2] == 1 )
			return COM_CODELOAD_STARTED_REPLY_ok;
		else if ( msg->msg[2] == 4 )
			return COM_CODELOAD_STARTED_REPLY_fail;
		else
			return COM_CODELOAD_NOT_NECESSARY;
	case 0x1aad:	return COM_CODELOAD_COMPLETE_REPLY;
	case 0x8287:
		switch ( msg->msg[2] )
		{
		case 0: return PCI_EVENT_API_INIT;
		case 1: return PCI_EVENT_DEAD;
		case 3: return PCI_EVENT_ALIVE;
		}
		break;
	case 0x8e25:
		if ( msg->msg[3] == 0 )
			return EDLSP_ADDLINK_REPLY_ok;
		else
			return EDLSP_ADDLINK_REPLY_fail;
	case 0x8e27:	return EDLSP_RMVLINK_REPLY;
	case TIMEOUT_TAG:
		last_timerid = *(int*)(msg->msg);
		return TIMEOUT_INPUT;
	}
	return UNKNOWN_INPUT;
}

void
DPROC_STATE_MACHINE :: output_generator( int ty )
{
	char msg[] = { 0, 0, 0, 0, 0, 0, 0, 0 };
	switch ( ty )
	{
	case STARTED:
//		printf( "dproc %d ready to start\n", slot );
		break;
	case STOPPED:
//		printf( "dproc %d is stopped\n", slot );
		break;
	case DPROC_IN_SERVICE:
//		printf( "dproc %d is in service\n", slot );
		break;
	case DPROC_OUT_SERVICE_fail:
		printf( "dproc %d has fallen out of service!\n", slot );
		break;
	case DPROC_OUT_SERVICE_normal:
//		printf( "dproc %d is out of service\n", slot );
		break;
	case PCI_EVENT_REG:
		msg[0] = slot;
		msg[2] = e_pid();
		msg[4] = mailbox >> 8;
		msg[5] = mailbox & 255;
		/*	  pid, mbid, tag, msg, len */
		send_msg( 0x18, 0x0, 0x05, msg, 8 );
		break;
	case PCI_EVENT_DEREG:
		msg[0] = slot;
		send_msg( 0x18, 0x0, 0x06, msg, 2 );
		break;
	case PCI_QUERY:
		msg[0] = slot;
		send_msg( 0x18, 0x0, 0x02, msg, 2 );
		break;
	case PCI_POWERON:
		msg[0] = slot;
		send_msg( 0x18, 0x0, 0x03, msg, 1 );
		break;
	case PCI_POWEROFF:
		msg[0] = slot;
		send_msg( 0x18, 0x0, 0x04, msg, 1 );
		break;
	case PCI_RESET:
		msg[0] = slot;
		msg[4] = 0x80;
		send_msg( 0x18, 0x0, 0x01, msg, 8 );
		break;
	case COM_CODELOAD_REQ:
		msg[2] = slot;
		msg[3] = 1;
		send_msg( 6, 0xffff, 0x1a2b, msg, 5 );
		break;
	case EDLSP_ADDLINK_REQ:
		msg[0] = 0x11;
		msg[1] = slot;
		send_msg( 0x12, 0xff11, 0x24, msg, 2 );
		break;
	case EDLSP_RMVLINK_REQ:
		msg[0] = 0x11;
		msg[1] = slot;
		send_msg( 0x12, 0xff11, 0x26, msg, 2 );
		break;
	case START_NIB1:
		msg[0] = slot; msg[1] = 1;
		send_msg( 0x90, 0, 3, msg, 2 );
		break;		  
	case START_NIB2:
		msg[0] = slot; msg[1] = 2;
		send_msg( 0x90, 0, 3, msg, 2 );
		break;		  
	case STOP_NIB1_normal:
		msg[0] = slot; msg[1] = 1;
		send_msg( 0x90, 0, 4, msg, 2 );
		break;		  
	case STOP_NIB1_fail:	  
		msg[0] = slot; msg[1] = 1;
		send_msg( 0x90, 0, 5, msg, 2 );
		break;		  
	case STOP_NIB2_normal:	  
		msg[0] = slot; msg[1] = 2;
		send_msg( 0x90, 0, 4, msg, 2 );
		break;		  
	case STOP_NIB2_fail:	  
		msg[0] = slot; msg[1] = 2;
		send_msg( 0x90, 0, 5, msg, 2 );
		break;
	default:
		printf( "unknown message type %d to send!\n", ty );
	}
}

void
DPROC_STATE_MACHINE :: _first_call( void )
{
	timer_set = false;
	timer_seq = 1;
	current_state = START;
}

void
DPROC_STATE_MACHINE :: unknown_message( void * m )
{
	printf( "dproc %d processing unknown message tag %x\n",
		slot, last_tag );
}

void
DPROC_STATE_MACHINE :: unhandled_message( int type )
{
	//
}

void
DPROC_STATE_MACHINE :: debug_transition_hook( int input,
					      int old_state,
					      int new_state )
{
	printf( "dproc %d : %s (%s -> %s)\n",
		slot,
		dbg_input_name( input ),
		dbg_state_name( old_state ),
		dbg_state_name( new_state ));
}

void
DPROC_STATE_MACHINE :: set_timer( int val )
{
	MESSAGE_TYPE msg;
	int seq;
	msg.destination.flag = 1;
	msg.destination.mbid = mailbox;
	msg.destination.subsystem = 0;
	msg.destination.process = 0;
	msg.destination.cpu = 0;
	msg.tag = TIMEOUT_TAG;
	msg.msg = (char*)&seq;
	msg.length = 4;
	msg.router_info = 0;
	seq = timer_seq;
	timer_id = e_set_relative( &msg, val );
	timer_set = true;
}

void
DPROC_STATE_MACHINE :: cancel_timer( void )
{
	if ( timer_set )
	{
		if ( last_tag == TIMEOUT_TAG )
			if ( last_timerid != timer_seq )
			{
				printf( "timer msg with bad seq received\n" );
			}
		timer_set = false;
		e_cancel_timer( timer_id );
		timer_seq++;
	}
}

%}
