
machine	 NIB
machine_data
%{
	int slot;
	int subslot;
	int mailbox;

	void send_msg( int pid, int mbid, int tag,
		       char * m, int len );
	enum { SOI_INIT, SOI_STOP } stop_or_init;
	int timer_seq;
	bool timer_set;
	int timer_id;
	int last_tag;
	int last_timerid;
%}
machine_cargs %{ int _slot, int _subslot, int _mailbox %}
machine_ccode %{ slot = _slot; subslot = _subslot; mailbox = _mailbox; %}
machine_dcode %{ %}

starthdr
%{
#include "test_machine.H"
%}

inputs {
	START_REQ
	STOP_REQ_normal
	STOP_REQ_fail
	EDLSP_NIB_CODELOAD_STARTED_REPLY_ok
	EDLSP_NIB_CODELOAD_STARTED_REPLY_fail
	EDLSP_NIB_CODELOAD_COMPLETE_REPLY
	EDLSP_MMS_ACTIVATE_REPLY_ok
	EDLSP_MMS_ACTIVATE_REPLY_fail
}

outputs {
	DEAD
	STARTED
	STOPPED
	NIB_IN_SERVICE
	NIB_OUT_SERVICE_fail
	NIB_OUT_SERVICE_normal
	EDLSP_NIB_CODELOAD_REQ
	EDLSP_MMS_ACTIVATE_REQ1
	EDLSP_MMS_ACTIVATE_REQ2
}

states {
	INIT
	CODELOAD
	CODELOAD_wait
	ACTIVATE1
	ACTIVATE2
	RUNNING
}

state INIT {
	pre {
		output STOPPED
	}

	input START_REQ {
		output STARTED
		next CODELOAD
	}

	input STOP_REQ_normal {
		next INIT
	}

	input STOP_REQ_fail {
		next INIT
	}
}

state CODELOAD {
	pre {
		output EDLSP_NIB_CODELOAD_REQ
		timeout 1000
	}

	input EDLSP_NIB_CODELOAD_STARTED_REPLY_ok {
		next CODELOAD_wait
	}

	input EDLSP_NIB_CODELOAD_STARTED_REPLY_fail {
		next INIT
	}

	input TIMEOUT_INPUT {
		next INIT
	}
}

state CODELOAD_wait {
	pre {
		timeout 30000
	}

	input EDLSP_NIB_CODELOAD_COMPLETE_REPLY {
		next ACTIVATE1
	}

	input TIMEOUT_INPUT {
		next INIT
	}
}

state ACTIVATE1 {
	pre {
		output EDLSP_MMS_ACTIVATE_REQ1
		timeout 2000
	}

	input EDLSP_MMS_ACTIVATE_REPLY_ok {
		next ACTIVATE2
	}

	input TIMEOUT_INPUT {
		next INIT
	}
}

state ACTIVATE2 {
	pre {
		output EDLSP_MMS_ACTIVATE_REQ2
		timeout 2000
	}

	input EDLSP_MMS_ACTIVATE_REPLY_ok {
		next RUNNING
	}

	input TIMEOUT_INPUT {
		next INIT
	}
}

state RUNNING {
	pre {
		output NIB_IN_SERVICE
	}
	input STOP_REQ_normal {
		output NIB_OUT_SERVICE_normal
		next INIT
	}
	input STOP_REQ_fail {
		output NIB_OUT_SERVICE_fail
		next INIT
	}
}

endhdr
%{
%}

endimpl
%{

#define TIMEOUT_TAG 0x41014101

void
NIB_STATE_MACHINE :: send_msg( int pid, int mbid, int tag,
				 char * m, int len )
{
	MESSAGE_TYPE msg;
	msg.destination.flag = 1;
	msg.destination.mbid = mbid;
	msg.destination.subsystem = 0;
	msg.destination.process = pid;
	msg.destination.cpu = 0x1100 + slot;
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
NIB_STATE_MACHINE :: input_discriminator( void * m )
{
	MESSAGE_TYPE * msg = (MESSAGE_TYPE *)m;
	last_tag = msg->tag;
	switch ( msg->tag )
	{
	case 0x12345678: return START_REQ;
	case 0x12345679: return STOP_REQ_normal;
	case 0x1234567a: return STOP_REQ_fail;
	case 0x8e31:
		if ( msg->msg[3] == 6 )
			return EDLSP_NIB_CODELOAD_STARTED_REPLY_ok;
		if ( msg->msg[3] == 7 )
			return EDLSP_NIB_CODELOAD_COMPLETE_REPLY;
		return EDLSP_NIB_CODELOAD_STARTED_REPLY_fail;
	case 0x8e41:
		if ( msg->msg[3] == 0 )
			return EDLSP_MMS_ACTIVATE_REPLY_ok;
		return EDLSP_MMS_ACTIVATE_REPLY_fail;
	case TIMEOUT_TAG:
		last_timerid = *(int*)(msg->msg);
		return TIMEOUT_INPUT;
	}
	return UNKNOWN_INPUT;
}

void
NIB_STATE_MACHINE :: output_generator( int ty )
{
	char msg[] = { 0, 0, 0, 0, 0, 0, 0, 0 };
	switch ( ty )
	{
	case DEAD:
		printf( "nib %d,%d state machine is dead\n", slot, subslot );
		break;
	case STARTED:
//		printf( "nib %d,%d starting\n", slot, subslot );
		break;
	case STOPPED:
//		printf( "nib %d,%d is stopped, ready to start\n",
//			 slot, subslot );
		break;
	case NIB_IN_SERVICE:
		printf( "nib %d,%d in service\n", slot, subslot );
		break;
	case NIB_OUT_SERVICE_fail:
		printf( "nib %d,%d has fallen out of service!\n",
			 slot, subslot );
		break;
	case NIB_OUT_SERVICE_normal:
//		printf( "nib %d,%d is out of service\n", slot, subslot );
		break;
	case EDLSP_NIB_CODELOAD_REQ:
		msg[0] = slot;
		msg[1] = subslot;
		send_msg( 0x12, 0xff12, 0x8e30, msg, 2 );
		break;
	case EDLSP_MMS_ACTIVATE_REQ1:
	case EDLSP_MMS_ACTIVATE_REQ2:
		msg[0] = slot;
		msg[1] = subslot;
		msg[2] = (ty == EDLSP_MMS_ACTIVATE_REQ1) ? 0 : 1;
		msg[3] = 1;
		msg[4] = 1;
		send_msg( 0x12, 0xff13, 0x8e40, msg, 5 );
		break;
	default:
		printf( "unknown message type %d to send!\n", ty );
	}
}

void
NIB_STATE_MACHINE :: constructor_hook( void )
{
	timer_set = false;
	timer_seq = 1;
}

void
NIB_STATE_MACHINE :: _first_call( void )
{
	current_state = INIT;
}

void
NIB_STATE_MACHINE :: destructor_hook( void )
{
}

void
NIB_STATE_MACHINE :: unknown_message( void )
{
	printf( "nib %d, %d processing unknown message tag %x\n",
		slot, subslot, last_tag );
}

void
NIB_STATE_MACHINE :: debug_transition_hook( int input,
					      int old_state,
					      int new_state )
{
	printf( "nib %d,%d : %s (%s -> %s)\n",
		slot, subslot,
		dbg_input_name( input ),
		dbg_state_name( old_state ),
		dbg_state_name( new_state ));
}

void
NIB_STATE_MACHINE :: set_timer( int val )
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
NIB_STATE_MACHINE :: cancel_timer( void )
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
