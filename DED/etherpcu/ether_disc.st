
# this defines the name of the class: "%s_STATE_MACHINE".

machine  ETHER_DISC

# this is the private data stored in the class and used by
# the user-written custom methods.

machine_data
%{
	mproc_info * info;
	int seqno;
	timer_ident timer_id;
%}

# this defines the arguments to the constructor for this class.

machine_cargs %{ mproc_info * _info %}

# this is the code for the constructor

machine_ccode
%{
	info = _info;
	timer_id.value = -1UL;
	seqno = 1;
%}

# this is the code for the destructor

machine_dcode
%{
	if ( timer_id.value != -1UL )
		timer_cancel( timer_id );
%}

starthdr
%{

#include <stdio.h>
#include "timer.H"
#include "mproc.H"

#define BCI_S_PAUSE  1
#define BCI_A_RETRY 20    /* should be random */

%}

inputs {
	in_BCI_S
#	in_BCI_F
	in_BCI_A
#	in_C1_ASSERT
#	in_C1_RELEASE
}

outputs {
	out_BCI_S
#	out_BCI_F
	out_BCI_A
#	out_C1_ASSERT
#	out_C1_RELEASE
#
}

states {
	MPROC_INIT1
	MPROC_INIT2
	MPROC_INIT3
	MPROC_INIT_AB_RETRY
}

state MPROC_INIT1 {
	pre {
		timeout BCI_S_PAUSE
		output out_BCI_S
		inline %{
			printf( PCUMPF "output first BCI_S\n", PCUMP );
		%}
	}
	input in_BCI_S {
		inline %{
			printf( PCUMPF "input foreign BCI_S! "
				"aborting for retry\n", PCUMP );
		%}
		output out_BCI_A
		next MPROC_INIT_AB_RETRY
	}
	input in_BCI_A {
		inline %{
			printf( PCUMPF "input BCI_A! "
				"aborting for retry\n", PCUMP );
		%}
		output out_BCI_A
		next MPROC_INIT_AB_RETRY
	}
	input TIMEOUT_INPUT {
		next MPROC_INIT2
	}
}

state MPROC_INIT_AB_RETRY {
	pre {
		timeout BCI_A_RETRY
	}
	input in_BCI_A {
		inline %{
			printf( PCUMPF "input BCI_A in retry state\n", PCUMP );
		%}
		next MPROC_INIT_AB_RETRY
	}
	input TIMEOUT_INPUT {
		next MPROC_INIT1
	}
}

state MPROC_INIT2 {
	pre {
		timeout BCI_S_PAUSE
		output out_BCI_S
		inline %{
			printf( PCUMPF "output second BCI_S\n", PCUMP );
		%}
	}
	input in_BCI_S {
		inline %{
			printf( PCUMPF "input foreign BCI_S! "
				"aborting for retry\n", PCUMP );
		%}
		output out_BCI_A
		next MPROC_INIT_AB_RETRY
	}
	input in_BCI_A {
		inline %{
			printf( PCUMPF "input BCI_A! "
				"aborting for retry\n", PCUMP );
		%}
		output out_BCI_A
		next MPROC_INIT_AB_RETRY
	}
	input TIMEOUT_INPUT {
		next MPROC_INIT3
	}
}

state MPROC_INIT3 {
	pre {
		timeout BCI_S_PAUSE
		output out_BCI_S
		inline %{
			printf( PCUMPF "output third BCI_S\n", PCUMP );
		%}
	}
	input in_BCI_S {
		inline %{
			printf( PCUMPF "input foreign BCI_S! "
				"aborting for retry\n", PCUMP );
		%}
		output out_BCI_A
		next MPROC_INIT_AB_RETRY
	}
	input in_BCI_A {
		inline %{
			printf( PCUMPF "input BCI_A! "
				"aborting for retry\n", PCUMP );
		%}
		output out_BCI_A
		next MPROC_INIT_AB_RETRY
	}
	input TIMEOUT_INPUT {
		exit
	}
}

endhdr
%{
%}

endimpl
%{
%}
