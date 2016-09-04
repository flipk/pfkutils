
starthdr
%{
#ifndef __ETHER_DISC_H_
#define __ETHER_DISC_H_
#include "ether_disc_helper.H"
%}

endhdr
%{
#endif /* __ETHER_DISC_H_ */
%}

# this defines the name of the class: "%s_STATE_MACHINE".
machine  ETHER_DISC

# this is the private data stored in the class and used by
# the user-written custom methods.

machine_data
%{
#define ETHER_PRIVATE_DATA
#include "ether_disc_helper.H"
#undef  ETHER_PRIVATE_DATA
%}

# this defines the arguments to the constructor for this class.
machine_cargs %{ MPROC_Thread * _mproc %}

# this is the code for the constructor
machine_ccode %{
	timer_set = false;
	timer_sequence = 1;
	mac_received = -1;
	mac_other_mproc = -1;
	mproc = _mproc;
	has_link = false;
%}

# this is the code for the destructor
machine_dcode %{  cancel_timer();  %}

inputs {
	BECOME_SECONDARY
	FINISH_BCAST
	ASSERTED_C1
	I_CLAIM
	I_CLAIM_ACK
	I_CLAIM_NMM
	I_HEARTBEAT
	I_HEARTBEAT_ACK
}

outputs {
	O_CLAIM
	O_CLAIM_ACK
	O_CLAIM_NMM
	O_HEARTBEAT
	O_HEARTBEAT_ACK
	RESTART_AUDIT
	ADD_LINK
	REMOVE_LINK
	PRIMARY_FAILED
}

states {

# below this line is the primary mproc's state machine

	ST1_WAITFOR_C1
	ST4_RESTART_SLEEP
	ST5_NO_LINK_FOUND
	ST6_WAITFOR_F
	ST7_WAITFOR_CLAIMACK
	ST8_FOUND_LINK

# below this line is the secondary's claim state machine

	ST9_WAITFOR_CLAIM
	ST10_GOT_CLAIM
	ST11_INVALID_CLAIM
	ST12_NOW_CLAIMED
}

state ST1_WAITFOR_C1 {
	pre {
		output REMOVE_LINK
		timeout ST1_MAC_CLEAN_INTERVAL
	}
	input TIMEOUT_INPUT {
		call purge_mac_list
		next ST1_WAITFOR_C1
	}
	input BECOME_SECONDARY {
		next ST9_WAITFOR_CLAIM
	}
	input ASSERTED_C1 {
		call mac_list_count {
			case ZERO_MACS {
				next ST5_NO_LINK_FOUND
			}
			case ONE_MAC {
				next ST6_WAITFOR_F
			}
			case TWO_OR_MORE_MACS {
				next ST4_RESTART_SLEEP
			}
		}
	}
}

state ST4_RESTART_SLEEP {
	pre {
		timeout ST4_RANDOM_VALUE
	}
	input TIMEOUT_INPUT {
		inline %{ 
iml_print( "ethernet-discovery state machine resetting secondary MPROC now" );
		%}
		output RESTART_AUDIT
		next ST1_WAITFOR_C1
	}
}

state ST5_NO_LINK_FOUND {
# stay here in this state.
	pre {
		inline %{
ThreadShortCuts::printf( "no ethernet link to other MPROC was found!\n" );
ThreadShortCuts::sleep(ThreadShortCuts::tps());
kill(0,6);
		%}
	}
}

state ST6_WAITFOR_F {
	pre {
		timeout ST6_TIMEOUT_VALUE
	}
	input TIMEOUT_INPUT {
		inline %{
iml_print( "timeout waiting for F packet, restarting secondary audit" );
		%}
		next ST4_RESTART_SLEEP
	}
	input FINISH_BCAST {
		call st6_same_mac {
			case FIRST_TRY {
				inline %{
iml_print( "ethernet-discovery succeeded in first pass" );
iml_print( "resetting secondary MPROC for second pass." );
				%}
				next ST4_RESTART_SLEEP
			}
			case SECOND_TRY {
				next ST7_WAITFOR_CLAIMACK
			}
		}
	}
}

state ST7_WAITFOR_CLAIMACK {
	pre {
		inline %{
iml_print( "ethernet-discovery found MPROC in second pass, sending CLAIM" );
		%}
		output O_CLAIM
		timeout ST7_TIMEOUT_VALUE
	}
	input TIMEOUT_INPUT {
		inline %{
iml_print( "No CLAIM-ACK was received! Restarting secondary audit." );
		%}
		next ST4_RESTART_SLEEP
	}
	input I_CLAIM_NMM {
		inline %{
iml_print( "The secondary I claimed says I'm not his master." );
iml_print( "  Restarting secondary audit." );
		%}
		next ST4_RESTART_SLEEP
	}
	input I_CLAIM_ACK {
		inline %{
iml_print( "The secondary has acknowledged my CLAIM." );
iml_print( "Bringing Inter-MPROC ethernet link in service." );
		%}
		output ADD_LINK
		inline %{ last_heartbeat_ack_time = tickGet(); %}
		next ST8_FOUND_LINK
	}
}

state ST8_FOUND_LINK {
	pre {
		timeout ST8_HEARTBEAT_INTERVAL
	}
	input I_HEARTBEAT_ACK {
		inline %{
last_heartbeat_ack_time = tickGet();
iml_print( "got heartbeat ack" );
		%}
		next ST8_FOUND_LINK
	}
	input TIMEOUT_INPUT {
		output O_HEARTBEAT
		call st8_last_hb_ack_ok {
			case ACK_OK {
				next ST8_FOUND_LINK
			}
			case NO_ACK {
				inline %{
iml_print( "Secondary MPROC has failed an ethernet heartbeat audit!" );
				%}
				output RESTART_AUDIT
				next ST1_WAITFOR_C1
			}
		}
		next ST8_FOUND_LINK
	}
}


state ST9_WAITFOR_CLAIM {
	pre {
		timeout ST9_CLAIM_TIMEOUT_VALUE
		output REMOVE_LINK
	}
	input TIMEOUT_INPUT {
		# we should have gotten a claim by now.
		inline %{
iml_print( "should have received an ethernet CLAIM request by now" );
iml_print( "  inter-mproc ethernet link not available this session." );
		%}
		next ST11_INVALID_CLAIM
	}
	input I_CLAIM {
		inline %{
iml_print( "a primary MPROC has sent me a CLAIM request." );
mac_other_mproc = mac_received;
		%}
		next ST10_GOT_CLAIM
	}
}

state ST10_GOT_CLAIM {
	pre {
		timeout ST10_TIMEOUT_VALUE
	}
	input TIMEOUT_INPUT {
		output O_CLAIM_ACK
		inline %{
iml_print( "Successful CLAIM/ACK negotiation with primary MPROC." );
iml_print( "Bringing Inter-MPROC ethernet link in service." );
		%}
		output ADD_LINK
		next ST12_NOW_CLAIMED
	}
	input I_CLAIM {
		inline %{
iml_print( "more than one primary MPROC is trying to claim me" );
iml_print( "  as its secondary!  sending Not-My-Master response..." );
		%}
		next ST11_INVALID_CLAIM
	}
}

state ST11_INVALID_CLAIM {
	pre {
		output O_CLAIM_NMM
	}
	input I_CLAIM {
		inline %{
iml_print( "more than one primary MPROC is trying to claim me" );
iml_print( "  as its secondary!  sending Not-My-Master response..." );
		%}
		next ST11_INVALID_CLAIM
	}
}

state ST12_NOW_CLAIMED {
	pre {
		timeout ST12_HEARTBEAT_TIMEOUT
	}
	input TIMEOUT_INPUT {
		inline %{
iml_print( "The primary has failed a heartbeat audit! Taking over..." );
		%}
		output PRIMARY_FAILED
	}
	input I_HEARTBEAT {
		inline %{
iml_print( "got heartbeat" );
		%}
		output O_HEARTBEAT_ACK
		next ST12_NOW_CLAIMED
	}
}

endimpl
%{
#include "ether_disc_helper.C"
%}
