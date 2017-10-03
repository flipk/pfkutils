/*
 * This file is licensed under the GPL version 2.
 * Refer to the file LICENSE in this distribution or
 * just search for GPL v2 on the website www.gnu.org.
 */

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "wordentry.h"
#include "machine.h"

#define INITIAL_ID_VALUE 1

MACHINE machine;

void
init_machine( struct args * args )
{
    memset( &machine, 0, sizeof( machine ));
    machine.next_id_value = INITIAL_ID_VALUE;
    machine.bytes_allocated = 0;
    machine.entries = 0;
    machine.args = args;
    machine.line_number = 1;
    machine.name.listname = "name";
    machine.inputs.listname = "inputs";
    machine.outputs.listname = "outputs";
    machine.states.listname = "states";
    machine.calls.listname = "calls";
}

/*

the process of destroying a MACHINE is somewhat complicated.  this is
becase the WENT lists are not very nice structures.  a WENT often has
more than one pointer in it, pointing to lists of WENTs; a given WENT
might be on more than one list.  to avoid the danger of calling 'free'
twice on a WENT, instead we'll just build a freelist as we walk the
lists, and "color" each WENT as we pass it.  when we've walked all
lists, then free everything on the freelist once.

the way that we "color" a WENT is by zeroing its identification number.

 */

static void
destroy_list( WENT *** nextfreeentry, WENT * l )
{
    int i;
    WENT * n;

    for ( ; l; l = n )
    {
        for ( i = 0; i < WENT_NUM_EX; i++ )
        {
            WENT * f = l->ex[i];
            l->ex[i] = NULL;
            destroy_list( nextfreeentry, f );
        }

        if ( l->ident == 0 )
        {
            /* if this one's already on the freelist,
               don't add him again; also don't follow
               his next pointer, because its now on the
               freelist. */
            n = NULL;
        }
        else
        {
            /* this one is not on the freelist; update his
               ident to indicate he is, then grab his next
               pointer, then update his next pointer to be
               on the freelist.  then put him on the freelist
               and get a pointer to his 'next' member. */
            l->ident = 0;
            /* also check for verbatim action */
            if ( l->type == VERBATIM_ACTION && l->u.v != NULL )
            {
                free( l->u.v->data );
                free( l->u.v );
                l->u.v = NULL;
            }
            n = l->next;
            l->next = NULL;
            **nextfreeentry = l;
            *nextfreeentry = &l->next;
        }
    }
}

void
destroy_machine( void )
{
    WENT * l, * n;
    WENT * freelist = NULL;
    WENT ** nextfreeentry = &freelist;

    /* walk all lists, moving WENTs to the freelist. */

    destroy_list( &nextfreeentry, machine.name   .head );
    destroy_list( &nextfreeentry, machine.inputs .head );
    destroy_list( &nextfreeentry, machine.outputs.head );
    destroy_list( &nextfreeentry, machine.states .head );
    destroy_list( &nextfreeentry, machine.calls  .head );

    /* now nuke the freelist. */

    for ( l = freelist; l; l = n )
    {
        n = l->next;
        free( l );
        machine.next_id_value --;
    }

    if ( machine.constructor_args )
    {
        free( machine.constructor_args->data );
        free( machine.constructor_args );
    }
    if ( machine.constructor_code )
    {
        free( machine.constructor_code->data );
        free( machine.constructor_code );
    }
    if ( machine.destructor_code )
    {
        free( machine.destructor_code->data );
        free( machine.destructor_code );
    }
    if ( machine.startv )
    {
        free( machine.startv->data );
        free( machine.startv );
    }
    if ( machine.datav )
    {
        free( machine.datav->data );
        free( machine.datav );
    }
    if ( machine.endhdrv )
    {
        free( machine.endhdrv->data );
        free( machine.endhdrv );
    }
    if ( machine.startimplv )
    {
        free( machine.startimplv->data );
        free( machine.startimplv );
    }
    if ( machine.endimplv )
    {
        free( machine.endimplv->data );
        free( machine.endimplv );
    }

    /* if we've done everything properly with the freelist,
       the next_id_value field should have ended up back where
       it started. */

    if ( machine.next_id_value != INITIAL_ID_VALUE )
    {
        fprintf( stderr, "failure in freeing memory, "
                 "is there a link problem?\n" );
        exit( 1 );
    }

    init_machine( NULL );
}


struct wordentry *
new_wordentry( char * w )
{
    struct wordentry * n;
    int len;
    int wlen;

    wlen = strlen( w ) + 1;
    len = sizeof( struct wordentry ) + wlen;
    n = (struct wordentry *) malloc( len );

    if ( !n )
    {
        fprintf( stderr, "failed to malloc\n" );
        exit( 1 );
    }

    memset( n, 0, len );
    machine.bytes_allocated += len;
    machine.entries ++;

    n->next = NULL;
    n->type = UNKNOWN_ENTRY;
    n->ident = machine.next_id_value++;
    memcpy( n->word, w, wlen );

    return n;
}


void
line_error( char * format, ... )
{
    va_list ap;

    va_start( ap, format );
    fprintf( stderr, "*** line %d: ", machine.line_number );
    vfprintf( stderr, format, ap );
    fprintf( stderr, "\n" );
    va_end( ap );
}

extern char * template_code;
extern char * template_header;
extern char * template_skel;

static int process_var( char * );
static void emit_actions( int tabs, WENT * w );
static void emit_action ( int tabs, WENT * w );

void
dump_machine( enum dump_type ty )
{
    char * tmpl = NULL;

    switch ( ty )
    {
    case DUMP_HEADER:    tmpl = template_header;     break;
    case DUMP_CODE:      tmpl = template_code;       break;
    case DUMP_SKELETON:  tmpl = template_skel;       break;
    }

    for ( ; *tmpl; tmpl++ )
    {
        if ( tmpl[0] == '@' && tmpl[1] == '@' )
        {
            tmpl += process_var( tmpl );
            continue;
        }

        putchar( *tmpl );
    }

}

#define IFSTR(x)  ret=sizeof(x)+2; if (strncmp(cp, "@@" x "@@", ret+1)==0)

static int
process_var( char *cp )
{
    int ret;
    WENT * w;
    char str[32];

    IFSTR("inputfilename")
        {
            printf( "%s", machine.args->inputfile );
            return ret;
        }
    IFSTR("headerfile")
        {
            printf( "%s", machine.args->headerfile );
            return ret;
        }
    IFSTR("machine")
        {
            printf( "%s", machine.name.head->word );
            return ret;
        }
    IFSTR("statenamelist")
        {
            for ( w = machine.states.head; w; w=w->next )
                printf( "\tcase %s:\n\t\treturn \"%s\";\n",
                        w->word, w->word );
            return ret;
        }
    IFSTR("inputnamelist")
        {
            for ( w = machine.inputs.head; w; w=w->next )
                printf( "\tcase %s:\n"
                        "\t\treturn \"%s\";\n",
                        w->word, w->word );
            return ret;
        }
    IFSTR("outputnamelist")
        {
            for ( w = machine.outputs.head; w; w=w->next )
                printf( "\tcase %s:\n"
                        "\t\treturn \"%s\";\n",
                        w->word, w->word );
            return ret;
        }
    IFSTR("stateprelist")
        {
            for ( w = machine.states.head; w; w=w->next )
                if ( w->ex[0] )
                {
                    printf( "\tcase %s:\n", w->word );
                    emit_actions( 2, w->ex[0]->ex[0] );
                    printf( "\t\tbreak;\n" );
                }
            return ret;
        }
    IFSTR("validtransitions")
        {
            for ( w = machine.states.head; w; w=w->next )
            {
                WENT * w2;
                if ( w->ex[1] )
                {
                    printf( "\tcase %s:\n", w->word );
                    printf( "\t\tswitch ( it )\n\t\t{\n" );
                    for ( w2 = w->ex[1]; w2; w2=w2->next )
                    {
                        printf( "\t\tcase %s:\n", w2->ex[0]->word );
                    }
                    printf( "\t\t\tret = true;\n" );
                    printf( "\t\t\tbreak;\n" );
                    printf( "\t\t}\n" );
                    printf( "\t\tbreak;\n" );
                }
            }
            return ret;
        }
    IFSTR("statetransitions")
        {
            for ( w = machine.states.head; w; w=w->next )
            {
                WENT * w2;
                printf( "\tcase %s:\n", w->word );
                if ( w->ex[0] )
                    emit_actions( 2, w->ex[0]->ex[1] );
                if ( w->ex[1] )
                {
                    printf( "\t\tswitch ( it )\n\t\t{\n" );
                    for ( w2 = w->ex[1]; w2; w2=w2->next )
                    {
                        printf( "\t\tcase %s:\n", w2->ex[0]->word );
                        emit_actions( 3, w2->ex[1] );
                        printf( "\t\t\tbreak;\n" );
                    }
                    printf( "\t\t}\n" );
                }
                printf( "\t\tbreak;\n" );
            }
            return ret;
        }
    IFSTR("starthdr")
        {
            if ( machine.startv && machine.startv->data )
                printf( "%s\n", machine.startv->data );
            return ret;
        }
    IFSTR("data")
        {
            if ( machine.datav && machine.datav->data )
                printf( "%s\n", machine.datav->data );
            return ret;
        }
    IFSTR("inputlist")
        {
            for ( w = machine.inputs.head; w; w=w->next )
                printf( "\t\t%s = %d%c\n",
                        w->word, w->ident, w->next ? ',' : ' ' );
            return ret;
        }
    IFSTR("outputlist")
        {
            for ( w = machine.outputs.head; w; w=w->next )
                printf( "\t\t%s = %d%c\n",
                        w->word, w->ident, w->next ? ',' : ' ' );
            return ret;
        }
    IFSTR("statelist")
        {
            for ( w = machine.states.head; w; w=w->next )
                printf( "\t\t%s = %d%c\n",
                        w->word, w->ident, 
                        w->next ? ',' : ' ' );
            return ret;
        }
    IFSTR("callenums")
        {
            for ( w = machine.calls.head; w; w=w->next )
            {
                if ( w->ex[0] )
                {
                    WENT * w2;
                    printf( "\tenum call_%s_retvals {\n", w->word );
                    for ( w2 = w->ex[0]; w2; w2=w2->next )
                        if ( w2->type != CALL_DEFRESULT )
                            printf( "\t\t%s = %d%c\n",
                                    w2->word, w2->ident, 
                                    w2->next ? ',' : ' ' );
                    else
                            printf( "\t\tDEFAULT_%s_ret = %d%c\n",
                                    w->word, w2->ident, 
                                    w2->next ? ',' : ' ' );
                    printf( "\t};\n" );
                }
            }
            return ret;
        }
    IFSTR("constructor_args")
        {
            char * c;
            if ( machine.constructor_args && machine.constructor_args->data )
            {
                for ( c = machine.constructor_args->data; *c; c++ )
                    if ( *c != ' ' && *c != '\t' &&
                         *c != '\n' && *c != '\r' )
                        break;
                if ( *c )
                    printf( ",%s", machine.constructor_args->data );
            }
            return ret;
        }
    IFSTR("constructor_code")
        {
            if ( machine.constructor_code && machine.constructor_code->data )
                printf( "%s\n", machine.constructor_code->data );
            return ret;
        }
    IFSTR("destructor_code")
        {
            if ( machine.destructor_code && machine.destructor_code->data )
                printf( "%s\n", machine.destructor_code->data );
            return ret;
        }
    IFSTR("startimpl")
        {
            if ( machine.startimplv && machine.startimplv->data )
                printf( "%s\n", machine.startimplv->data );
            return ret;
        }
    IFSTR("endimpl")
        {
            if ( machine.endimplv && machine.endimplv->data )
                printf( "%s\n", machine.endimplv->data );
            return ret;
        }
    IFSTR("endhdr")
        {
            if ( machine.endhdrv && machine.endhdrv->data )
                printf( "%s\n", machine.endhdrv->data );
            return ret;
        }
    IFSTR("callprotos")
        {
            for ( w = machine.calls.head; w; w=w->next )
                if ( w->ex[0] )
                    printf( "\tenum call_%s_retvals call_%s( void );\n",
                            w->word, w->word );
                else
                    printf( "\tvoid call_%s( void );\n",
                            w->word );
            return ret;
        }
    IFSTR("callstubs")
        {
            char sm[64];
            sprintf( sm, "%s_STATE_MACHINE", machine.name.head->word );
            for ( w = machine.calls.head; w; w=w->next )
            {
                WENT * w2;
                if ( w->ex[0] )
                    printf( "enum %s::call_%s_retvals\n"
                            "%s :: call_%s( void )\n{\n",
                            sm, w->word, sm, w->word );
                else
                    printf( "void\n"
                            "%s :: call_%s( void )\n{\n",
                            sm, w->word );
                for ( w2 = w->ex[0]; w2; w2=w2->next )
                    if ( w2->type != CALL_DEFRESULT )
                        printf( "\t// return %s;\n", w2->word );
                    else
                        printf( "\t// return DEFAULT_%s_ret;\n", w->word );
                printf( "}\n\n" );
            }
            return ret;
        }

    memcpy( str, cp, 32 );
    str[31] = 0;
    fprintf( stderr, "unhandled macro var at:\n%s\n", str );

    exit( 1 );
}

static char *
maketabs( int t )
{
    switch ( t )
    {
    case 1:  return "\t";
    case 2:  return "\t\t";
    case 3:  return "\t\t\t";
    case 4:  return "\t\t\t\t";
    case 5:  return "\t\t\t\t\t";
    }
    line_error( "error, maketabs can't do %d tabs", t );
    exit( 1 );
}

void
emit_actions( int tabs, WENT * w )
{
    for ( ; w; w=w->next )
        emit_action( tabs, w );
}

void
emit_action( int tabs, WENT * w )
{
    char * t = maketabs( tabs );
    WENT * w2;

    switch ( w->type )
    {
    case OUTPUT_ACTION:
        printf( "%soutput_generator( %s );\n",
                t, w->ex[0]->word );
        break;
    case CALL_ACTION:
        if ( !w->ex[0] )
            printf( "%scall_%s();\n", t, w->word );
        else
        {
            printf( "%sswitch ( call_%s() )\n%s{\n", t, w->word, t );
            for ( w2 = w->ex[0]; w2; w2=w2->next )
            {
                if ( w2->type == CALL_RESULT )
                    printf( "%scase %s:\n", t, w2->word );
                else if ( w2->type == CALL_DEFRESULT )
                    printf( "%sdefault:\n", t );
                emit_actions( tabs+1, w2->ex[0] );
                printf( "%s\tbreak;\n", t );
            }
            printf( "%s}\n", t );
        }
        break;
    case SWITCH_ACTION:
        printf( "%sswitch ( %s )\n%s{\n", t, w->word, t );
        for ( w2 = w->ex[0]; w2; w2=w2->next )
        {
            if ( w2->type == CALL_RESULT )
                printf( "%scase %s:\n", t, w2->word );
            else if ( w2->type == CALL_DEFRESULT )
                printf( "%sdefault:\n", t );
            emit_actions( tabs+1, w2->ex[0] );
            printf( "%s\tbreak;\n", t );
        }
        printf( "%s}\n", t );
        break;
    case TIMEOUTV_ACTION:
        printf( "%sset_timer( %d );\n", t, w->u.timeoutval );
        break;
    case TIMEOUTS_ACTION:
        printf( "%sset_timer( %s );\n", t, w->word );
        break;
    case NEXT_ACTION:
        printf( "%snext_state = %s;\n", t, w->ex[0]->word );
        break;
    case VERBATIM_ACTION:
        printf( "%s%s\n", t, w->u.v->data );
        break;
    case EXIT_ACTION:
        printf( "%sreturn TRANSITION_EXIT;\n", t );
        break;

/* the following are all valid types for w->type, but are 
   not valid 'actions' per se.  (in-)validate them. */

    case UNKNOWN_ENTRY:
    case MACHINE_NAME:
    case INPUT_NAME:
    case INPUT_TIMEOUT:
    case INPUT_UNKNOWN:
    case OUTPUT_NAME:
    case STATE_NAME:
    case PREPOST_ACTION:
    case STATEINPUT_DEF:
    case CALL_RESULT:
        line_error( "action %d located", w->type );
        break;

    case CALL_NAME:
    case CALL_DEFRESULT:
        /* here only to satisfy compiler. */
        break;
    }
}
