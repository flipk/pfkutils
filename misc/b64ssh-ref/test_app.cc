
#include "sw.H"
#include "test.H"

app_thread :: app_thread( int _instance )
{
    instance = _instance;
    set_name("app %d", instance);
    app_rx_qid = msg_create( get_name() );
    resume();
}

app_thread :: ~app_thread( void )
{
    msg_destroy( app_rx_qid );
}

// virtual
void
app_thread :: entry( void )
{
    printf( "%s: created %d; connected to %d\n",
            get_name(),
            app_rx_qid, sw_tx_qid );

    union {
        pk_msg_int * m;
        // 
    } m;

    while (1)
    {
        m.m = msg_recv( 1, &app_rx_qid, NULL, -1 );
        if (!m.m)
            continue;

        switch (m.m->type)
        {
            //
        default:
            printf("%s: unknown msg type %#x received!\n", m.m->type);
            break;
        }

        delete m.m;
    }
}
