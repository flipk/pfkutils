
#include "sw.H"

// this thread can send:
//    AppQuench to APP
//    AppUnquench to APP
//    NetworkData to TXNET

// this thread can receive:
//    AppData from APP
//    NetworkQuench from TXNET
//    NetworkUnquench from TXNET
//    NetworkAck from TXNET

// this thread can send to itself:
//    TxTimerExpire
//    TxFreeTimer

sliding_window_tx_thread :: sliding_window_tx_thread( int _channel )
{
    channel = _channel;
    net_qid = -1;
    app_qid = -1;
    char buf[64];
    sprintf(buf, "sw tx %d", channel);
    my_qid = msg_create(buf);
    set_name(buf);
    next_seqno = 1;
    net_quenched = false;
    resume();
}

sliding_window_tx_thread :: ~sliding_window_tx_thread( void )
{
    DataBufferEntry * ent, * nent;

    for (ent = tx_queue.get_head(); ent; ent = nent)
    {
        nent = tx_queue.get_next(ent);
        delete ent->data;
        delete ent;
    }

    for (ent = delayed_free_queue.get_head(); ent; ent = nent)
    {
        nent = delayed_free_queue.get_next(ent);
        delete ent->data;
        delete ent;
    }

    msg_destroy(my_qid);
}

void
sliding_window_tx_thread :: entry( void )
{
    union {
        pk_msg_int * m;
        AppData * swtx;
        NetworkQuench * quench;
        NetworkUnquench * unquench;
        NetworkAck * ack;
        TxTimerExpire * txtimer;
    } m;

    printf("%s: created %d; connected to app %d net %d\n",
           get_name(), my_qid, app_qid, net_qid );

    handle_free_timer(); // start free timer interval

    while (1)
    {
        m.m = msg_recv(1, &my_qid, NULL, -1 );
        if (!m.m)
            continue;

        switch (m.m->type)
        {
        case AppData::TYPE:
            handle_tx_data(m.swtx);
            break;
        case NetworkQuench::TYPE:
            handle_net_quench(m.quench);
            break;
        case NetworkUnquench::TYPE:
            handle_net_unquench(m.unquench);
            break;
        case NetworkAck::TYPE:
            handle_net_ack(m.ack);
            break;
        case TxTimerExpire::TYPE:
            handle_tx_timer(m.txtimer);
            break;
        case TxFreeTimer::TYPE:
            handle_free_timer();
            break;
        default:
            printf("tx_thread : unknown message type %#x!\n", m.m->type);
            break;
        }

        delete m.m;
    }
}

void
sliding_window_tx_thread :: start_tx_timer( DataBufferEntry * ent )
{
    TxTimerExpire * timer_msg = new TxTimerExpire;
    timer_msg->src_q = my_qid;
    timer_msg->dest_q = my_qid;
    timer_msg->data = ent;

    // start tx timer
    ent->timer_id = timer_create( tps()/2, my_qid, timer_msg );
}

void
sliding_window_tx_thread :: send_to_net( DataBufferEntry * ent )
{
    if (net_quenched)
        return;

    NetworkData * netdata = new NetworkData;
    netdata->src_q = my_qid;
    netdata->dest_q = net_qid;
    netdata->channel = channel;
    netdata->seqno = ent->seqno;
    netdata->data = ent->data;
    (void) msg_send( net_qid, netdata );
}

void
sliding_window_tx_thread :: send_app_quench( void )
{
    AppQuench * quench = new AppQuench;
    quench->src_q = my_qid;
    quench->dest_q = app_qid;
    quench->channel = channel;
    (void) msg_send( app_qid, quench );
    app_quenched = true;
}

void
sliding_window_tx_thread :: send_app_unquench( void )
{
    AppUnquench * unquench = new AppUnquench;
    unquench->src_q = my_qid;
    unquench->dest_q = app_qid;
    unquench->channel = channel;
    (void) msg_send( app_qid, unquench );
    app_quenched = false;
}

void
sliding_window_tx_thread :: handle_tx_data( AppData * swtx )
{
    DataBufferEntry * ent = new DataBufferEntry;
    ent->delayed_free_age = 0;
    ent->data = swtx->data;
    ent->seqno = next_seqno++;
    start_tx_timer( ent );
    tx_queue.add( ent );
    send_to_net(ent);
    if (tx_queue.get_cnt() >= SLIDING_WINDOW_SIZE /* && !app_quenched */)
        send_app_quench();
    if (app_quenched && tx_queue.get_cnt() < SLIDING_WINDOW_SIZE)
        send_app_unquench();
}

void
sliding_window_tx_thread :: handle_tx_timer( TxTimerExpire * txtimer )
{
    DataBufferEntry * ent = txtimer->data;
    if (tx_queue.onthislist(ent))
    {
        start_tx_timer(ent);
        send_to_net(ent);
        if (app_quenched && tx_queue.get_cnt() < SLIDING_WINDOW_SIZE)
            send_app_unquench();
    }
}

void
sliding_window_tx_thread :: handle_net_ack( NetworkAck * ack )
{
    DataBufferEntry * ent = tx_queue.find( ack->seqno );
    if (ent)
    {
        tx_queue.remove(ent);
        timer_cancel(ent->timer_id);
        delayed_free_queue.add(ent);
    }
}

void
sliding_window_tx_thread :: handle_net_quench( NetworkQuench * quench )
{
    send_app_quench();
    net_quenched = true;
}

void
sliding_window_tx_thread :: handle_net_unquench( NetworkUnquench * unquench )
{
    send_app_unquench();
    net_quenched = false;
}

void
sliding_window_tx_thread :: handle_free_timer( void )
{
    DataBufferEntry * ent, * nent;

    for (ent = delayed_free_queue.get_head();
         ent != NULL;
         ent = nent)
    {
        nent = delayed_free_queue.get_next(ent);
        if (ent->delayed_free_age > 0)
        {
            delayed_free_queue.remove(ent);
            delete ent->data;
            delete ent;
        }
        else
        {
            ent->delayed_free_age ++;
        }
    }

    (void) timer_create( tps()/2, my_qid, new TxFreeTimer );
}
