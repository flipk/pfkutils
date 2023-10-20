
#include "main.H"
#include "client.H"
#include "clients.H"

Clients :: Clients( void )
{
    first = NULL;
    last = NULL;
    num_clients = 0;
    sem = global_th->seminit( 1 );
}

Clients :: ~Clients( void )
{
    global_th->semdelete( sem );
}

#ifdef CLIENTS_DEBUG
void
Clients :: dump( void )
{
    global_th->stdio();
    printf( "Clients::num_clients = %d\n", num_clients );
    Client * x;
    for ( x = first; x != NULL; x = x->next )
        printf( "x = %#x\n", x );
    global_th->unstdio();
}
#endif

void
Clients :: add_client( Client * c )
{
    lock();
    c->next = NULL;
    num_clients++;

    if ( first == NULL )
    {
        first = last = c;
    }
    else
    {
        last->next = c;
        last = c;
    }

#ifdef CLIENTS_DEBUG
    dump();
#endif
    unlock();
}

void
Clients :: del_client( Client * c )
{
    Client *x, *px;

    lock();
    for ( px = NULL, x = first; x != NULL; px = x, x = x->next )
        if ( x == c )
            break;

    if ( x == c )
    {
        num_clients--;

        if ( px == NULL )
        {
            first = x->next;
            if ( first == NULL )
                last = NULL;
        }
        else
        {
            px->next = x->next;
            if ( x == last )
                last = px;
        }
    }
    else
        c = NULL;

#ifdef CLIENTS_DEBUG
    dump();
#endif
    unlock();

    if ( c == NULL )
    {
        global_th->stdio();
        printf( "Clients::del_client : error : client not found!\n" );
        global_th->unstdio();
    }
}

char **
Clients :: get_usernames( void )
{
    char ** ret;
    int i;
    Client * x;

    lock();
    ret = new char*[ num_clients + 1 ];
    x = first;
    for ( i = 0; i < num_clients; i++ )
    {
        ret[i] = x->username;
        x = x->next;
    }
    unlock();

    ret[i] = NULL;
    return ret;
}

void
Clients :: status_function( void )
{
    if ( global_th->take( sem, Thread::NO_WAIT ) == false )
    {
        printf( "Clients :: status_function : couldn't take "
                "semaphore, try again\n" );
        return;
    }

    Client * x;

    for ( x = first; x != NULL; x = x->next )
    {
        char * a = (char*)&x->addr;
        printf( "Client on fd %d : username %s from %d.%d.%d.%d\n",
                x->fd, x->username,
                a[0], a[1], a[2], a[3] );
    }

    unlock();
}
