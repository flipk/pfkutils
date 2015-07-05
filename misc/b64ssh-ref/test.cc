
#include "sw.H"
#include "test.H"

int
main(int argc, char ** argv)
{
    int instance;
    app_thread * app;
    net_thread * net;
    sliding_window_tx_thread * sw_tx;
    sliding_window_rx_thread * sw_rx;

    if (argc != 2)
    {
    usage:
        printf( "usage: sw 1 or sw 2\n");
        return 1;
    }

    instance = atoi(argv[1]);
    if (instance < 1 || instance > 2)
        goto usage;

    new PK_Threads(10);

    // channel 1 is app1 -> net -> app2
    // channel 2 is app2 -> net -> app1

    // create the modules under test

    short local_port;
    char * remote_addr;
    short remote_port;

    if (instance == 1)
    {
        sw_tx = new sliding_window_tx_thread(1);
        sw_rx = new sliding_window_rx_thread(2);

        local_port = 7000;
        remote_port = 7001;
        remote_addr = "127.0.0.1";
    }
    else
    {
        sw_tx = new sliding_window_tx_thread(2);
        sw_rx = new sliding_window_rx_thread(1);

        local_port = 7001;
        remote_port = 7000;
        remote_addr = "127.0.0.1";
    }

    // create the app and net test threads

    app = new app_thread(instance);
    net = new net_thread(local_port,remote_addr,remote_port);

    // connect everyone's qids together.

    net->sw_tx_qid = sw_tx->get_my_qid();
    net->sw_rx_qid = sw_rx->get_my_qid();

    app->sw_tx_qid = sw_tx->get_my_qid();

    sw_tx->register_app_net_qids( app->app_rx_qid, net->net_sw_qid );
    sw_rx->register_app_net_qids( app->app_rx_qid, net->net_sw_qid );

    // begin transferring data.

    printf("main: starting all threads\n");

    th->run();
    delete th;

    return 0;
}
