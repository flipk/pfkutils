
#include "pipe_mgr.H"

// xxx whole file incomplete
struct Pipe_instance {
    Pipe_instance( );
    ~Pipe_instance( void );

};

Pipe_Mgr :: Pipe_Mgr( void )
{
    for ( int i = 0; i < max_pipes; i++ )
        pipes[i] = NULL;
}

Pipe_Mgr :: ~Pipe_Mgr( void )
{
}

int
Pipe_Mgr :: register_proxy( fd_interface * fdi )
{
}

void
Pipe_Mgr :: unregister_proxy( int pipe )
{
}

void
Pipe_Mgr :: handle_pkt( short pipeno, char * buf, int len )
{
}
