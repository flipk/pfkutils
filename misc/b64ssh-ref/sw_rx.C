
#include "sw.H"
#include "sw_int.H"

// this thread can send:
//    AppData to APP
//    NetworkQuench to RXNET
//    NetworkUnquench to RXNET
//    NetworkAck to RXNET

// this thread can receive:
//    AppQuench from APP
//    AppUnquench from APP
//    NetworkData from RXNET

// this thread can send to itself:
//    (none)

sliding_window_rx_thread :: sliding_window_rx_thread( int _channel )
{
    channel = _channel;
    app_qid = -1;
    char buf[64];
    sprintf(buf, "sw rx %d", channel);
    my_qid = msg_create(buf);
    set_name(buf);
    app_quenched = false;
    net_quenched = false;
    next_app_seqno = 1;
    resume();
}

sliding_window_rx_thread :: ~sliding_window_rx_thread( void )
{
    msg_destroy(my_qid);
}

void
sliding_window_rx_thread :: entry( void )
{
    union {
        pk_msg_int * m;
        AppQuench * quench;
        AppUnquench * unquench;
        NetworkData * netdata;
    } m;

    printf("%s: created %d; connected to app %d net %d\n",
           get_name(), my_qid, app_qid, net_qid);

    while (1)
    {
        m.m = msg_recv(1, &my_qid, NULL, -1 );
        if (!m.m)
            continue;

        switch (m.m->type)
        {
        case AppQuench::TYPE:
            handle_app_quench(m.quench);
            break;
        case AppUnquench::TYPE:
            handle_app_unquench(m.unquench);
            break;
        case NetworkData::TYPE:
            handle_network_data(m.netdata);
            break;
        default:
            printf("rx thread : unknown message %#x received!\n",m.m->type);
            break;
        }

        delete m.m;
    }
}

void
sliding_window_rx_thread :: send_ack( unsigned char seqno )
{
    NetworkAck * ack = new NetworkAck;
    ack->src_q = my_qid;
    ack->dest_q = net_qid;
    ack->channel = channel;
    (void) msg_send( net_qid, ack );
}

void
sliding_window_rx_thread :: send_net_quench( void )
{
    NetworkQuench * q = new NetworkQuench;
    q->src_q = my_qid;
    q->dest_q = net_qid;
    q->channel = channel;
    (void) msg_send( net_qid, q );
    net_quenched = true;
}

void
sliding_window_rx_thread :: send_net_unquench( void )
{
    NetworkUnquench * unq = new NetworkUnquench;
    unq->src_q = my_qid;
    unq->dest_q = net_qid;
    unq->channel = channel;
    (void) msg_send( net_qid, unq );
    net_quenched = false;
}

void
sliding_window_rx_thread :: handle_app_quench( AppQuench * q )
{
    send_net_quench();
    app_quenched = true;
}

void
sliding_window_rx_thread :: handle_app_unquench( AppUnquench * q )
{
    send_net_unquench();
    app_quenched = false;
}

void
sliding_window_rx_thread :: handle_network_data( NetworkData * data )
{
    bool valid = false, old = false;

    // if this seqno was very recently already sent to the app,
    // we don't have to queue it again.  determine if data->seqno
    // is within 2*SLIDING_WINDOW_SIZE of current next_app_seqno.

    do {
        // let 'a' denote the min range, i.e. next_app_seqno - 2*window_size
        // let 'b' denote the data->seqno received
        // let 'c' denote current next_app_seqno
        // let 'd' denote current next_app_seqno + window_size

        // four valid possibilities:
        // 1:   0..................a...b...c...d........MAX
        // 2:   0...b...c...d.......................a...MAX
        // 3:   0...c...d.......................a...b...MAX
        // 4:   0...d.......................a...b...c...MAX

        int a = next_app_seqno - (2*SLIDING_WINDOW_SIZE);
        int b = data->seqno;
        int c = next_app_seqno;
        int d = next_app_seqno + SLIDING_WINDOW_SIZE;

        if (a < d)
        {
            // case 1
            if (b < a || b > d)
                // leave valid==false
                break;
            valid = true;
            if (b <= c)
                old = true;
            break;
        }
        // case 2,3, or 4
        if (b > d && b < a)
            // leave valid==false
            break;
        if (c < d)
        {
            // case 2 or 3
            if (b < d)
            {
                // case 2
                valid = true;
                if (b < c)
                    old = true;
            }
            else
            {
                if (b > a)
                {
                    // case 3
                    valid = true;
                    old = true;
                    break;
                }
                // leave valid==false
            }
            break;
        }
        // case 4
        if (b < d)
            valid = true;
        else
        {
            if (b > c)
                valid = true;
            else
            {
                if (b > a)
                {
                    valid = true;
                    old = true;
                    break;
                }
            }
        }

    } while(0);

    if (!valid)
    {
        delete data->data;
        return;
    }

    // valid sequence number so ack it.
    send_ack( data->seqno );

    if (old)
    {
        delete data->data;
        return;
    }

    DataBufferEntry * ent = new DataBufferEntry;
    ent->data = data->data;
    ent->seqno = data->seqno;
    ent->timer_id = -1; // not used by receiver
    rx_queue.add( ent );

    bool found_one = false;
    do {
        ent = rx_queue.find( next_app_seqno );
        if (ent)
        {
            next_app_seqno ++;
            found_one = true;
            rx_queue.remove(ent);
            AppData * appdata = new AppData;
            appdata->src_q = my_qid;
            appdata->dest_q = app_qid;
            appdata->data = ent->data;
            (void) msg_send( app_qid, appdata );
            delete ent;
        }
    } while (found_one);
}
