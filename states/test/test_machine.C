
#include "test_machine.H"
#include "pk_state_machine_base.H"
#include "TEST_dproc.H"
#include "TEST_nib.H"

static void dproctest_process( void );

extern "C" void dproctest( void );

void
dproctest( void )
{
	struct proc_code pc;

	pc.type = 2;
	pc.entry = dproctest_process;

	e_create_process( "dproc", &pc, 0x90, 5, 64, 65536 );
}

/* 0x1000 */

enum board_type { DPROC, NIB };

#define NUMDEVS 36
#define NUMDPROCS 12

static struct {
	board_type type;
	int slot;
	int subslot;
} devices[] = {
	{ DPROC,  1, 0 }, { DPROC,  2, 0 },
	{ DPROC,  3, 0 }, { DPROC,  4, 0 },
	{ DPROC,  5, 0 }, { DPROC,  6, 0 },
	{ DPROC, 11, 0 }, { DPROC, 12, 0 },
	{ DPROC, 13, 0 }, { DPROC, 14, 0 },
	{ DPROC, 15, 0 }, { DPROC, 16, 0 },
	{ NIB,  1, 1 }, { NIB,  1, 2 }, { NIB,  2, 1 }, { NIB,  2, 2 },
	{ NIB,  3, 1 }, { NIB,  3, 2 }, { NIB,  4, 1 }, { NIB,  4, 2 },
	{ NIB,  5, 1 }, { NIB,  5, 2 }, { NIB,  6, 1 }, { NIB,  6, 2 },
	{ NIB, 11, 1 }, { NIB, 11, 2 }, { NIB, 12, 1 }, { NIB, 12, 2 },
	{ NIB, 13, 1 }, { NIB, 13, 2 }, { NIB, 14, 1 }, { NIB, 14, 2 },
	{ NIB, 15, 1 }, { NIB, 15, 2 }, { NIB, 16, 1 }, { NIB, 16, 2 }
};

int
find_ss( MESSAGE_TYPE * m, int * dev, int * slot, int * subslot )
{
	if ( m->length != 2 )
		return 0;
	for ( *dev = 0; *dev < NUMDEVS; (*dev)++ )
		if ( devices[*dev].slot == m->msg[0] &&
		     devices[*dev].subslot == m->msg[1] )
			return 1;
	return 0;
}

int
handle_mb0( MESSAGE_TYPE * m, PK_STATE_MACHINE_BASE ** devs )
{
	int dev, slot, subslot;

	switch ( m->tag )
	{
	case 0:
		printf( "usage:\n"
			"  tag 0 : help\n"
			"  tag 1 : kill process\n"
			"  tag 2 : print history of a device\n"
			"  tag 3 : start a device\n"
			"  tag 4 : stop a device\n"
			"  tag 5 : kill a device\n"
			"  tag 6 : print stats\n"
			"  tag 7 : start all devices\n"
			"  tag 8 : stop all devices\n" );
		break;

	case 1:
		return -1;

	case 2:
		if ( find_ss( m, &dev, &slot, &subslot ) == 1 )
			devs[dev]->printhist();
		break;

	case 3:
		if ( find_ss( m, &dev, &slot, &subslot ) == 1 )
		{
			m->tag = 0x12345678;
			devs[dev]->transition( m );
		}
		break;

	case 4:
		if ( find_ss( m, &dev, &slot, &subslot ) == 1 )
		{
			m->tag = 0x12345679;
			devs[dev]->transition( m );
		}
		break;

	case 5:
		if ( find_ss( m, &dev, &slot, &subslot ) == 1 )
		{
			m->tag = 0x1234567a;
			devs[dev]->transition( m );
		}
		break;

	case 6:
		for ( dev = 0; dev < NUMDEVS; dev++ )
			printf( "%s : %s\n",
				devs[dev]->name,
				devs[dev]->current_state_name() );
		break;

	case 7:
	case 8:
		if ( m->tag == 7 )
			m->tag = 0x12345678;
		else
			m->tag = 0x12345679;
		for ( dev = 0; dev < NUMDPROCS; dev++ )
			if ( devs[dev] )
				devs[dev]->transition( m );
		break;
	}

	return 0;
}

void
dproctest_process( void )
{
	MESSAGE_TYPE msg;
	char body[ MAXMSG ];
	PK_STATE_MACHINE_BASE * devs [NUMDEVS];
	int dev;
	int mb;

	e_init_process( 0x90 );

	e_create_mb( "ctl", 0, 1, 0 );

	for ( dev = 0; dev < NUMDEVS; dev++ )
	{
		char name[16];
		int slot = devices[dev].slot;
		int subslot = devices[dev].subslot;

		mb = 0x1000 + dev;

		if ( devices[dev].type == DPROC )
		{
			sprintf( name, "dproc%d", slot );
			devs[dev] = new DPROC_STATE_MACHINE( name, slot, mb );
		}
		else
		{
			sprintf( name, "nib%d.%d", slot, subslot );
			devs[dev] = new NIB_STATE_MACHINE( name, slot, subslot, mb );
		}

		devs[dev]->first_call();

		e_create_mb( devs[dev]->name, mb, 1, 0 );
	}

	while ( 1 )
	{
		msg.msg = body;
		e_receive( &msg, MAXMSG, -1, &mb );
		if ( mb == 0 )
		{
			if ( handle_mb0( &msg, devs ) < 0 )
				break;
		}
		else
		{
			dev = mb - 0x1000;
			devs[dev]->transition( &msg );
		}
	}

	for ( dev = 0; dev < NUMDEVS; dev++ )
		delete devs[dev];

	printf( "exiting\n" );

	e_delete_process( 0 );
}
