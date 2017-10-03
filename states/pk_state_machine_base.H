
#ifndef __PK_STATE_MACHINE_BASE_H_
#define __PK_STATE_MACHINE_BASE_H_

class PK_STATE_MACHINE_BASE {
public:
    PK_STATE_MACHINE_BASE( char * _name );
    ~PK_STATE_MACHINE_BASE( void );

    enum transition_return {
        TRANSITION_OK, TRANSITION_EXIT
    };
    transition_return transition( void * m );
    void printhist( void (*printfunc)(char *) );
    void first_call( void );
    int get_current_state( void ) { return current_state; }
    char * current_state_name( void );
    char * name;
protected:
    virtual char * dbg_state_name         ( int    ) = 0;
    virtual char * dbg_input_name         ( int    ) = 0;
    virtual char * dbg_output_name        ( int    ) = 0;
    virtual transition_return _transition ( int    ) = 0;
    virtual int  input_discriminator      ( void * ) = 0;
    virtual void output_generator         ( int    ) = 0;
    virtual void cancel_timer             ( void   ) = 0;
    virtual void call_pre_hooks           ( void   ) = 0;
    virtual void constructor_hook         ( void   ) = 0;
    virtual void destructor_hook          ( void   ) = 0;
    virtual void _first_call              ( void   ) = 0;

    virtual void debug_transition_hook( int input,
                                        int old_state,
                                        int new_state ) = 0;

    static const int INVALID_STATE = -1;
    int current_state;
    int next_state;
    struct logentry {
        int time;
        int prev;
        int input;
        int next;
    };
    static const int numlogentries = 100;
    int logpos;
    logentry logentries[ numlogentries ];
};

#endif /* __PK_STATE_MACHINE_BASE_H_ */
