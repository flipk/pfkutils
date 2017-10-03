/*
 * This file is licensed under the GPL version 2.
 * Refer to the file LICENSE in this distribution or
 * just search for GPL v2 on the website www.gnu.org.
 */

typedef struct {
    int usedlen;
    int alloclen;
    char * data;
} VERBATIM;

typedef struct wordentry {
    struct wordentry * next;
    enum wordentry_type {
        UNKNOWN_ENTRY,

        /* only one of these exists ever, on machine.name */
        MACHINE_NAME,

        /* a linked list thru 'next' off of machine.inputs;
           these are also linked to from STATEINPUT_DEF ex[0] entries */
        INPUT_NAME,

        /* the final two entries on the machine.inputs list */
        INPUT_TIMEOUT,
        INPUT_UNKNOWN,

        /* a linked list thru 'next' off machine.outputs */
        OUTPUT_NAME,

        /* a linked list thru 'next' off machine.states; 
           also, ex[0] is used to point to a PREPOST_ACTION if a pre
           block exists for this state.  ex[1] points to a linked
           list of STATEINPUT_DEF entries to define the inputs
           handled by this state. */
        STATE_NAME,

        /* ex[0] points to a linked list of actions for PRE, 
           ex[1] points to a linked list of actions for POST */
        PREPOST_ACTION,

        /* a possible input to a state; ex[0] points to the INPUT_NAME
           entry on the machine.inputs list, and ex[1] points to a linked
           list of actions to perform for this input. */
        STATEINPUT_DEF,

        /* all object types ending in "_ACTION" are valid entries on
           an action list.  follow 'next' for the linked list of actions. */

        /* an action to output a message. ex[0] points to the OUTPUT_NAME
           on machine.outputs list. */
        OUTPUT_ACTION,

        /* a "call".  these are on a linked list from machine.calls.
           ex[0] lists all the possible results from the call, if any,
           using CALL_RESULT objects.  these CALL_RESULT objects do not
           have any ex[0] or ex[1] links, they simply list the 
           possible return values. */
        CALL_NAME,

        /* an instance of a call.  this exists on an action list. ex[1]
           points to the CALL_NAME with the same name.  for a given call,
           there is only one CALL_NAME while there may be many CALL_ACTIONS
           which invoke it.  also, ex[0] is a linked list of CALL_RESULTs,
           but these CALL_RESULTS are different from the ones linked off
           of a CALL_NAME. see CALL_RESULT below. */
        CALL_ACTION,

        /* a call result.  for each possible result, there is a CALL_RESULT
           object hanging off of CALL_NAME->ex[0], which has no ex pointers.
           there is also another CALL_RESULT hanging off of a CALL_ACTION,
           which has ex[0] pointing to an action list and ex[1] pointing back
           to the corresponding CALL_RESULT on the CALL_NAME list. */
        CALL_RESULT,

        /* a call "default" result.  same comments apply to this one as to
           CALL_RESULT. */
        CALL_DEFRESULT,

        /* a timeout action.  used in pre blocks to set a timeout
           before going to rest.  this version has an integer value
           stored in the u.timeoutval field. */
        TIMEOUTV_ACTION,

        /* a timeout action.  this version has a string name (presumably
           a macro or enum) in the 'word' field. */
        TIMEOUTS_ACTION,

        /* a next action.  this indicates what the next state should be,
           and ex[0] points to the STATE_NAME indicating the destination
           state. */
        NEXT_ACTION,

        /* a verbatim action, also known as an inline action. 
           raw C code is stored in the u.v field to be printf'd
           into the output stream. */
        VERBATIM_ACTION,

        /* a switch-statement. very similar to a call action except
           that we don't need to keep it on a separate list.  we don't
           emit prototypes for the calls or enums for the types.
           ex[0] is a linked list of CALL_RESULTS. */
        SWITCH_ACTION,

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
