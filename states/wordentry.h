
typedef struct {
    int usedlen;
    int alloclen;
    char * data;
} VERBATIM;

typedef struct wordentry {
    struct wordentry * next;
    enum wordentry_type {
        UNKNOWN_ENTRY    = 1,

        /* only one of these exists ever, on machine.name */
        MACHINE_NAME     = 2,

        /* a linked list thru 'next' off of machine.inputs;
           these are also linked to from STATEINPUT_DEF ex[0] entries */
        INPUT_NAME       = 3,

        /* the final two entries on the machine.inputs list */
        INPUT_TIMEOUT    = 4,
        INPUT_UNKNOWN    = 5,

        /* a linked list thru 'next' off machine.outputs */
        OUTPUT_NAME      = 6,

        /* a linked list thru 'next' off machine.states; 
           also, ex[0] is used to point to a PREPOST_ACTION if a pre
           block exists for this state.  ex[1] points to a linked
           list of STATEINPUT_DEF entries to define the inputs
           handled by this state. */
        STATE_NAME       = 7,

        /* ex[0] points to a linked list of actions for PRE, 
           ex[1] points to a linked list of actions for POST */
        PREPOST_ACTION   = 8,

        /* a possible input to a state; ex[0] points to the INPUT_NAME
           entry on the machine.inputs list, and ex[1] points to a linked
           list of actions to perform for this input. */
        STATEINPUT_DEF   = 9,

        /* all object types ending in "_ACTION" are valid entries on
           an action list.  follow 'next' for the linked list of actions. */

        /* an action to output a message. ex[0] points to the OUTPUT_NAME
           on machine.outputs list. */
        OUTPUT_ACTION    = 10,

        /* a "call".  ex[0] is a linked list of CALL_RESULT, one
           for each possible return value from the call.   also, there
           is a linked list of all CALL_ACTION objects in the
           state machine, starting from machine.calls and proceeding
           thru the ex[1] pointer. */
        CALL_ACTION      = 11,

        /* a call result.  ex[0] points to an action list. */
        CALL_RESULT      = 12,

        /* a call "default" result.  ex[0] points to an action list. */
        CALL_DEFRESULT   = 13,

        /* a timeout action.  used in pre blocks to set a timeout
           before going to rest.  this version has an integer value
           stored in the u.timeoutval field. */
        TIMEOUTV_ACTION  = 14,

        /* a timeout action.  this version has a string name (presumably
           a macro or enum) in the 'word' field. */
        TIMEOUTS_ACTION  = 15,

        /* a next action.  this indicates what the next state should be,
           and ex[0] points to the STATE_NAME indicating the destination
           state. */
        NEXT_ACTION      = 16,

        /* a verbatim action, also known as an inline action. 
           raw C code is stored in the u.v field to be printf'd
           into the output stream. */
        VERBATIM_ACTION  = 17,

        /* a switch-statement. very similar to a call action except
           that we don't need to keep it on a separate list.  we don't
           emit prototypes for the calls or enums for the types.
           ex[0] is a linked list of CALL_RESULTS. */
        SWITCH_ACTION    = 18,

        /* exit action; this indicates that the state machine should
           terminate.  */
        EXIT_ACTION
    } type;
#define WENT_NUM_EX 2
    struct wordentry * ex[ WENT_NUM_EX ];
    union {
        int timeoutval;
        VERBATIM * v;
    } u;
    int ident;
    char word[1];
} WENT;

typedef struct {
    char * listname;
    WENT * head;
    WENT * tail;
} WENTLIST;
