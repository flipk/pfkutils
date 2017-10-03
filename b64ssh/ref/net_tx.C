
#include "net.H"
#include "net_int.H"

// this thread can send:
//   NetworkData
//   NetworkQuench/NetworkUnquench
//   NetworkAck

// this thread can receive:
//   on data qid:
//     NetworkData
//   on cm qid:
//     Connect*
//     Auth*
//   on user qid:
//     UserData

// this thread can send to itself:


net_tx_thread :: net_tx_thread( void )
{
    my_data_qid = msg_create( "net tx data" );
    my_cm_qid   = msg_create( "net tx cm" );
    my_user_qid = msg_create( "net tx user" );
}

net_tx_thread :: ~net_tx_thread( void )
{
    msg_destroy( my_data_qid );
    msg_destroy( my_cm_qid );
    msg_destroy( my_user_qid );
}

void
net_tx_thread :: entry( void )
{
    union {
        pk_msg_int * m;
    } m;

    while (1)
    {
        int qids[3] = { my_cm_qid, my_data_qid, my_user_qid };
        int rcv_qid;
        m.m = msg_recv( 3, qids, &rcv_qid, -1 );
        if (!m.m)
            continue;

        if (rcv_qid == my_cm_qid)
        {
            switch (m.m->type)
            {
            case AuthChallengeReq::TYPE:
                break;
            case AuthChallengeResp::TYPE:
                break;
            case ConnectReq::TYPE:
                break;
            case ConnectAck::TYPE:
                break;
            case ConnectRefused::TYPE:
                break;
            case ConnectClose::TYPE:
                break;
            case ConnectCloseAck::TYPE:
                break;
            default:
                printf("unknown message %#x on cm_qid!\n",
                       m.m->type);
                break;
            }
        }
        else if (rcv_qid == my_data_qid)
        {
            switch (m.m->type)
            {
            case NetworkData::TYPE:
            case NetworkQuench::TYPE:
            case NetworkUnquench::TYPE:
            case NetworkAck::TYPE:
                // forward to a waiting sw instance;
                // lookup which instance by channel#.
                break;
            default:
                printf("unknown message %#x on data_qid!\n",
                       m.m->type);
                break;
            }
        }
        else if (rcv_qid == my_user_qid)
        {
            switch (m.m->type)
            {
            case UserData::TYPE:
                break;
            default:
                printf("unknown message %#x on user_qid!\n",
                       m.m->type);
                break;
            }
        }

        if (m.m)
            // some of the above handlers may forward
            // the message to another qid; in this case
            // it is more efficient to send the same ptr
            // rather than new/copy/delete.
            delete m.m;
    }

}
