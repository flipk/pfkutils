
#include "pipe_mgr.H"

// xxx whole file incomplete

Pipe_Mgr :: Pipe_Mgr( void )
{
    for ( int i = 0; i < max_pipes; i++ )
        pipes[i] = NULL;
}

Pipe_Mgr :: ~Pipe_Mgr( void )
{
}

void
Pipe_Mgr :: handle_pkt( short pipeno, char * buf, int len )
{
}
