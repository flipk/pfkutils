
#define _GNU_SOURCE 
#define _FILE_OFFSET_BITS 64

#include <stdio.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include "m.h"

#if defined(sparc)
#include "curses.h"
#else
#include "/usr/include/curses.h"
#endif

#ifndef MAP_FAILED
#define MAP_FAILED -1
#endif

enum mark_op { SET_MARK, GOTO_MARK };
enum pointer_type { BIG_ENDIAN_PTR, LITTLE_ENDIAN_PTR };

static int         file_descriptor;
static char *      file_name;
static off_t       file_size;
static off_t       file_position;
static int         pos_width;
static int         cursor_line;
static int         charsperline;
static int         insert_mode;
static int         readonly;

static enum { ed_HEX, ed_TEXT } ed;
static enum { disp_HEX, disp_TEXTW, disp_TEXTL } disp;
static enum { ESC_NORMAL, ESC_GOT1B, ESC_GOT5B,
              ESC_GOT35,  ESC_GOT36 } esc_seq_state;

#define MAX_MARKS 26
static off_t mark_locations[ MAX_MARKS ];

static int    get_pos_width( void );
static void   update_screen( void );
static void   handle_character( int c );
static void   do_help( void );
static void   do_help_tty( void );
static int    ishex( int c );
static int    hex_to_dig( int c );
static void   go_offset( void );
static void   do_search( void );
static void   do_mark( enum mark_op op );
static void   do_rel_move( void );
static void   formula_initialize( void );
static void   deref_pointer( enum pointer_type );
static int    get_file_byte( off_t pos );
static void   get_file_data( off_t pos, unsigned char *data, int len );
static void   put_file_byte( off_t pos, int data );

#if 0
static FILE * debug_fd = NULL;
#define DEBUG(x) if ( debug_fd != NULL ) fprintf x
#define OPEN_TTY "/dev/pts/1"
#else
#define DEBUG(x)
#endif

int
he_main( int argc, char ** argv )
{
    struct stat sb;
    int i, c;
    int open_options;

    if ( argc != 2 )
    {
        do_help_tty();
        return 1;
    }

#ifdef OPEN_TTY
    debug_fd = fopen( OPEN_TTY, "w" );
    setlinebuf( debug_fd );
#endif

    formula_initialize();

    for ( i = 0; i < MAX_MARKS; i++ )
        mark_locations[ i ] = -1;

    readonly = 0;
    file_name = argv[1];
    open_options = 0;
#ifdef O_LARGEFILE
    open_options |= O_LARGEFILE;
#endif
    file_descriptor = open( file_name, open_options | O_RDWR );
    if ( file_descriptor < 0 )
    {
        file_descriptor = open( file_name, open_options | O_RDONLY );
        if ( file_descriptor < 0 )
        {
            perror( "open" );
            return 1;
        }
        readonly = 1;
    }

    initscr();
    move( 0, 0 );
    raw();
    noecho();

    fstat( file_descriptor, &sb );

    file_size = sb.st_size;
    pos_width = get_pos_width();
    file_position = 0;
    cursor_line = 0;
    ed = ed_HEX;
    disp = disp_HEX;
    insert_mode = FALSE;
    esc_seq_state = ESC_NORMAL;

    do {

        update_screen();
        c = getch();
        handle_character( c );

    } while ( c != 'q' );

    clear();
    refresh();
    endwin();

    return 0;
}

int
get_pos_width( void )
{
    off_t v = file_size;
    int width = 0;
    while (v != 0)
    {
        width++;
        v /= 16;
    }
    return width;
}

int
get_file_byte( off_t pos )
{
    unsigned char c;
    lseek( file_descriptor, pos, SEEK_SET );
    if ( read( file_descriptor, &c, 1 ) != 1 )
        return 0;
    return c;
}

void
get_file_data( off_t pos, unsigned char *data, int len )
{
    int rlen;

    if ( pos < 0 )
    {
        lseek( file_descriptor, 0, SEEK_SET );
        /* tricky!  recall that 'pos' is negative! */
        rlen = len + pos;
        data -= pos;
    }
    else
    {
#ifdef __CYGWIN__
        /* ??? wtf? */
        _lseek64( file_descriptor, pos, SEEK_SET );
#else
        lseek64( file_descriptor, pos, SEEK_SET );
#endif
        rlen = len;
    }
    read( file_descriptor, data, rlen );
}

void
put_file_byte( off_t pos, int data )
{
    unsigned char c = data;
    lseek( file_descriptor, pos, SEEK_SET );
    write( file_descriptor, &c, 1 );
}

void
update_screen( void )
{
    int curs_x=0, curs_y=0;

    erase();
    move( 0, 0 );

    if ( disp == disp_TEXTL )
    {
        unsigned char * buff;
        off_t buffstart;
        int buffsize, middle, * newls, i, n;

        charsperline = COLS-16;
        buffsize = charsperline * (LINES-1) * 5;
        middle = (buffsize/2);
        buffstart = file_position - middle;

        buff = (unsigned char *)malloc( buffsize );
        newls = (int*)malloc( sizeof( int ) * LINES );

        DEBUG(( debug_fd, "buffsize = %d  charsperline = %d\n",
                buffsize, charsperline ));

        for ( i = 0; i < LINES; i++ )
            newls[i] = -1;

        if ( buffstart >= 0 )
        {
            get_file_data( buffstart, buff, buffsize );
        }
        else
        {
            int unused = -buffstart;
            memset( buff, 0, unused );
            get_file_data( 0, buff+unused, buffsize - unused );
        }

        for ( n = cursor_line, i = (middle-1); n >= 0 && i >= 0; i-- )
            if ( buff[i] == 10 )
                newls[n--] = i;
            else if ( i == -buffstart )
                newls[n--] = i-1;

        DEBUG(( debug_fd, "bailing out with i %d n %d bs %d mid %d\n",
                i, n, buffstart, middle ));
        if ( i == -1 && buffstart == -middle && n >= 0 )
        {
            DEBUG(( debug_fd, "special case\n" ));
            newls[n--] = middle-1;
        }

        for ( n = (cursor_line+1), i = middle;
              n < LINES && i < buffsize; i++ )
        {
            if ( buff[i] == 10 )
                newls[n++] = i;
        }

        for ( n = 0; n < (LINES-1); n++ )
        {
            DEBUG(( debug_fd, "line %d is at offset %d\n", n, newls[n] ));

            if ( newls[n] != -1 )
            {
                int pos = newls[n] + 1;
                off_t file_pos = file_position + pos - middle;

                printw( "%*s: ", pos_width,
                        m_dump_number( file_pos, 16 ));

                for ( i = 0; i < charsperline; i++, file_pos++ )
                {
                    int c;
                    if ( file_pos >= file_size )
                        break;
                    c = buff[pos+i];
                    if (( pos + i ) == middle )
                        getyx( stdscr, curs_y, curs_x );
                    if ( c == 10 )
                        break;
                    if (isprint(c))
                        printw( "%c", c );
                    else
                        printw( "." );
                }
            }

            printw( "\n" );
        }

        free( buff );
        free( newls );
    }
    else
    {
        off_t start_file_pos, end_file_pos, pos;
        unsigned char * file_buf;
        int readlen, readpos;

        if ( disp == disp_HEX )
        {
/* 
 * 1-10 chars for the number (pos_width)
 *  3 for colon and spaces
 *  1 space after every 4 byte group
 *  1 more space after every 8 byte group
 *  3 more spaces before text
 *  2 asterisks around text
 *  2 more spaces before end of line
 *  thus there are remaining :
 *  3.375 for each byte
 *
 *    (COLS - (12 + pos_width)) / 3.375     *note that 3.375 = 27 / 8
 */
            charsperline = (COLS - (12 + pos_width)) * 8 / 27;
            charsperline &= ~3;
        }
        else if ( disp == disp_TEXTW )
            charsperline = COLS-16;

        readlen = (LINES-1) * charsperline;

        start_file_pos =
            (file_position - (file_position % charsperline)) -
            (cursor_line * charsperline);

        end_file_pos = start_file_pos + readlen;

        file_buf = (unsigned char *)malloc( readlen );

        get_file_data( start_file_pos, file_buf, readlen );

        for ( readpos = 0; readpos < readlen; readpos += charsperline )
        {
            int byte;
            pos = start_file_pos + readpos;

            if (( pos < 0 ) || ( pos >= file_size ))
            {
                printw( "\n" );
                continue;
            }

            printw( "%*s: ", pos_width, m_dump_number( pos, 16 ));

            for ( byte = 0; byte < charsperline; byte++ )
            {
                if ( disp == disp_HEX  )
                {
                    if (( byte & 7 ) == 0 )
                        printw( " " );
                    if (( byte & 3 ) == 0 )
                        printw( " " );
                    if ( (pos+byte) == file_position  &&  ed == ed_HEX )
                        getyx( stdscr, curs_y, curs_x );
                    if ((pos+byte) >= file_size)
                        printw( "  " );
                    else
                        printw( "%02x", file_buf[ readpos + byte ] );
                }
                else if ( disp == disp_TEXTW )
                {
                    for ( byte = 0; byte < charsperline; byte++ )
                    {
                        int c = file_buf[ readpos + byte ];
                        if ( (pos+byte) == file_position )
                            getyx( stdscr, curs_y, curs_x );
                        if ( (pos+byte) >= file_size )
                            printw( " " );
                        else if ( isprint(c))
                            printw( "%c", c );
                        else
                            printw( "." );
                    }
                }
            }

            if ( disp == disp_HEX  )
            {
                printw( "     *" );

                for ( byte=0; byte < charsperline; byte++ )
                {
                    int c;
                    if ((pos+byte) >= file_size)
                    {
                        printw( " " );
                        continue;
                    }
                    if ( (pos+byte) == file_position  &&  ed == ed_TEXT )
                        getyx( stdscr, curs_y, curs_x );
                    c = file_buf[ readpos + byte ];
                    if (isprint(c))
                        printw( "%c", c );
                    else
                        printw( "." );
                }

                printw( "*\n" );
            }
            else
                printw( "\n" );
        }

        free( file_buf );
    }

    move( LINES-1, 0 );
    printw( "\"%s\"", file_name );

    move( LINES-1, COLS-47 );
    if ( insert_mode == TRUE )
        printw( "[insert] " );
    else if ( readonly )
        printw( "[rdonly] " );
    else
        printw( "         " );

    printw( "help:'?' " );
    printw( "sz:%10s ", m_dump_number( file_size, 16 ));
    printw( "pos:%10s", m_dump_number( file_position, 16 ));

    move( curs_y, curs_x );

    refresh();
}

static unsigned char insert_mode_backup[ 512 ];
static int insert_mode_backup_size;

void
handle_character( int c )
{
    off_t before;

    if ( insert_mode == TRUE )
    {
        if ( c == 0x1b )
        {
            insert_mode = FALSE;
            return;
        }

        if ( ed == ed_TEXT )
        {
            if ( c == 8 )  /* backspace */
            {
                if ( insert_mode_backup_size > 0 )
                {
                    put_file_byte(
                        --file_position,
                        insert_mode_backup[ --insert_mode_backup_size ] );
                }
                return;
            }

            if ( c == 22 )   /* control-V */
                c = getch();

            insert_mode_backup[ insert_mode_backup_size++ ] = 
                get_file_byte( file_position );

            put_file_byte( file_position++, c );

            if ( file_position > file_size )
                file_position = file_size;
        }

        if ( ed == ed_HEX )
        {
            int d1, d2;

            if ( c == 8 )  /* backspace */
            {
                if ( insert_mode_backup_size > 0 )
                {
                    put_file_byte(
                        --file_position,
                        insert_mode_backup[ --insert_mode_backup_size ] );
                }

                return;
            }

            if ( !ishex( c ))
                return;

            d1 = c;
            printw( "%c", c );
            refresh();
            c = getch();

            if ( !ishex( c ))
                return;

            d2 = c;
            printw( "%c", c );
            refresh();
            c = ( hex_to_dig( d1 ) << 4 ) + hex_to_dig( d2 );

            insert_mode_backup[ insert_mode_backup_size++ ] = 
                get_file_byte( file_position );

            put_file_byte( file_position++, c );

            if ( file_position > file_size )
                file_position = file_size;
        }

        if (( file_position & 15 ) == 0 )
        {
            if ( cursor_line < ( LINES-2 ))
                cursor_line++;
        }

        return;
    }

    if ( c == 0x1b )
    {
        esc_seq_state = ESC_GOT1B;
        return;
    }

    if ( esc_seq_state == ESC_GOT1B  && c == 0x5b )
    {
        esc_seq_state = ESC_GOT5B;
        return;
    }

    if ( esc_seq_state == ESC_GOT5B )
    {
        esc_seq_state = ESC_NORMAL;
        switch ( c ) 
        {
            /* found arrow key?  translate to the motion key. */
        case 'A':  c = 'k'; break;
        case 'B':  c = 'j'; break;
        case 'C':  c = 'l'; break;
        case 'D':  c = 'h'; break;

            /* found home/end ? move the cursor. */
        case 'H':
            file_position = 0;
            cursor_line = 0;
            return;
        case 'F':
            file_position = (file_size>0) ? file_size-1 : 0;
            cursor_line = LINES-2;
            return;

            /* found page-up/page-down? wait for final
               character, then translate to motion key. */
        case '5':  esc_seq_state = ESC_GOT35; return;
        case '6':  esc_seq_state = ESC_GOT36; return;

        default:
            /* unknown esc sequence, ignore it. */
            return;
        }
    }

    if ( esc_seq_state == ESC_GOT35 || esc_seq_state == ESC_GOT36 )
    {
        if ( c != 0x7e )
        {
            /* unknown esc sequence, ignore it. */
            esc_seq_state = ESC_NORMAL;
            return;
        }
        /* now we're sure we found page-up or page-down,
           translate to motion key. */
        if ( esc_seq_state == ESC_GOT35 )
            c = 0x15; /* control-U */
        else
            c = 0x4; /* control-D */

        esc_seq_state = ESC_NORMAL;
    }

    switch ( c )
    {
    case '?':
        do_help();
        break;

    case 'i':
        if ( readonly || disp != disp_HEX )
            return;

        insert_mode = TRUE;
        insert_mode_backup_size = 0;
        return;

    case 't':
        switch ( disp )
        {
        case disp_HEX:    disp = disp_TEXTW;  break;
        case disp_TEXTW:  disp = disp_TEXTL;  break;
        case disp_TEXTL:  disp = disp_HEX;    break;
        }
        break;

    case 'g':
        go_offset();
        break;

    case '/':
        do_search();
        break;

    case 'm':
        do_mark( SET_MARK );
        break;

    case '\'':
        do_mark( GOTO_MARK );
        break;

    case 'M':
        do_rel_move();
        break;

    case 'p':
        deref_pointer( LITTLE_ENDIAN_PTR );
        break;

    case 'P':
        deref_pointer( BIG_ENDIAN_PTR );
        break;

    case 'j':
        before = file_position;
        if ( disp == disp_TEXTL )
        {
            off_t pos = file_position;
            while ( pos < file_size )
                if ( get_file_byte( pos++ ) == 10 )
                {
                    file_position = pos;
                    break;
                }
        }
        else
        {
            file_position += charsperline;
        }
        if ( before != file_position )
            if ( cursor_line < ( LINES-2 ))
                cursor_line++;
        break;

    case 'k':
        before = file_position;
        if ( disp == disp_TEXTL )
        {
            off_t pos = file_position-1;
            while ( pos > 0 )
                if ( get_file_byte( --pos ) == 10 )
                {
                    file_position = pos+1;
                    break;
                }
            if ( pos == 0 )
                file_position = 0;
        }
        else
        {
            file_position -= charsperline;
        }
        if ( before != file_position )
            if ( cursor_line > 0 )
                cursor_line--;
        break;

    case 'h':
        file_position--;
        if (( file_position % charsperline ) == ( charsperline-1 ) &&
            ( cursor_line > 0 ))
            cursor_line--;
        break;

    case 'l':
        file_position++;
        if (( file_position % charsperline ) == 0 &&
            ( cursor_line < ( LINES-2 )))
            cursor_line++;
        break;

    case 9:  /* tab */
        if ( ed == ed_HEX )
            ed = ed_TEXT;
        else
            ed = ed_HEX;
        break;

    case 4:  /* control-D */
        if ( cursor_line != ( LINES-2 ))
        {
            file_position += (charsperline * ((LINES-2) - cursor_line));
            cursor_line = LINES-2;
        }
        else
        {
            file_position += (charsperline * (LINES-2));
        }
        break;

    case 21: /* control-U */
        if ( cursor_line != 0 )
        {
            file_position -= ( charsperline * cursor_line );
            cursor_line = 0;
        }
        else
        {
            file_position -= ( charsperline * ( LINES-2 ));
        }
        break;

    case 12: /* control-L */
        touchwin( stdscr );
        break;
    }

    if ( file_position > ( file_size - 1 ))
        file_position = file_size - 1;

    if ( file_position < 0 )
        file_position = 0;
}

#define MAKEWIN2( win, lines, cols ) \
    win = newwin( lines+4, cols+8, (LINES/2)-7, (COLS/2)-(cols/2)-4 ); \
    werase( win ); \
    touchwin( win ); \
    wrefresh( win )

#define MAKEWIN1( win, lines, cols ) \
    win = newwin( lines+2, cols+4, (LINES/2)-6, (COLS/2)-(cols/2)-2 ); \
    werase( win ); \
    box( win, '*', '*' ); \
    touchwin( win ); \
    wrefresh( win )

#define MAKEWIN( win, lines, cols )  \
    MAKEWIN2( win##2, lines, cols ); \
    MAKEWIN1( win##1, lines, cols ); \
    win = newwin( lines,   cols,   (LINES/2)-5, (COLS/2)-(cols/2)   ); \
    werase( win ); \
    wmove( win, 0, 0 )

void
go_offset( void )
{
    WINDOW * go_window2, * go_window1, * go_window;
    M_INT64 offset;
    int c, done;
    enum { num_DEC, num_HEX } num;

    MAKEWIN(  go_window,  4, 42 );

    wprintw( go_window,
             "Enter offset in hex:  \n"
             "Enter offset in dec:  \n"
             "\n"
             "[tab: switch] [esc: cancel] [ret: ok]" );

    offset = 0;
    num = num_HEX;

    while ( 1 )
    {
        if ( num == num_DEC )
        {
            wmove( go_window, 0, 22 );
            wclrtoeol( go_window );
            wmove( go_window, 1, 22 );
            wclrtoeol( go_window );
            wprintw( go_window, "%10s", m_dump_number( offset, 10 ));
            wrefresh( go_window );
        }
        else
        {
            wmove( go_window, 1, 22 );
            wclrtoeol( go_window );
            wmove( go_window, 0, 22 );
            wclrtoeol( go_window );
            wprintw( go_window, "%10s", m_dump_number( offset, 16 ));
            wrefresh( go_window );
        }

        done = 0;

        switch ( c = getch() )
        {
        case 9:   /* tab */
            if ( num == num_HEX )
                num = num_DEC;
            else
                num = num_HEX;
            break;

        case 0x1b:   /* esc */
            offset = -1;
            done = 1;
            break;

        case 8:  case 0x7f:  /* backspace */
            if ( num == num_HEX )
                offset /= 16;
            else
                offset /= 10;
            break;

        case 13:  case 10:   /* return */
            done = 1;
            break;
        }

        if ( done == 1 )
            break;

        if ( ishex( c ))
        {
            if ( num == num_DEC )
            {
                if ( isdigit( c ))
                {
                    offset *= 10;
                    offset += hex_to_dig( c );
                }
            }
            else
            {
                offset *= 16;
                offset += hex_to_dig( c );
            }
        
        }
    }

    if ( offset != -1 )
    {
        file_position = offset;

        if ( file_position > file_size )
            file_position = file_size;

        cursor_line = LINES/3;
    }

    delwin( go_window  );
    delwin( go_window1 );
    delwin( go_window2 );
    touchwin( stdscr );
    refresh();
}

void
do_search( void )
{
    static unsigned char search_string[ 80 ];
    static int search_length = 0;

    WINDOW * srch_win2, * srch_win1, * srch_win;
    enum { srch_HEX, srch_TEXT } srch;
    int i, c, done, halfhex;

    MAKEWIN( srch_win, 5, 60 );

    wprintw( srch_win, 
             "Enter text search string:\n\n"
             "Enter hex search string:\n\n"
             "   [tab: switch] [^u: erase] [esc: cancel] [ret: search]" );

    wmove( srch_win, 1, 0 );
    wrefresh( srch_win );

    srch = srch_TEXT;
    halfhex = -1;

    while ( 1 )
    {
        if ( srch == srch_TEXT )
        {
            wmove( srch_win, 3, 0 );
            wclrtoeol( srch_win );
            wmove( srch_win, 1, 0 );
            wclrtoeol( srch_win );

            for (i = 0; i < search_length; i++)
                if (isprint(search_string[i]))
                    wprintw( srch_win, "%c", search_string[i]);
                else
                    wprintw( srch_win, "." );

            wrefresh( srch_win );
        }
        else
        {
            wmove( srch_win, 1, 0 );
            wclrtoeol( srch_win );
            wmove( srch_win, 3, 0 );
            wclrtoeol( srch_win );

            for (i = 0; i < search_length; i++)
                wprintw( srch_win, "%02x", search_string[ i ]);

            if ( halfhex != -1 )
                wprintw( srch_win, "%c", halfhex );

            wrefresh( srch_win );
        }

        done = 0;

        switch ( c = getch() )
        {
        case 8:   /* backspace */
            if ( srch == srch_TEXT )
            {
                if ( search_length > 0 )
                    search_length--;
            }
            else
            {
                if ( halfhex != -1 )
                {
                    halfhex = -1;
                }
                else
                {
                    if ( search_length > 0 )
                    {
                        search_length--;
                        halfhex = (search_string[ search_length ] & 0xf0) >> 4;
                        halfhex = "0123456789abcdef"[ halfhex ];
                    }
                }
            }
            break;

        case 9:   /* tab */
            if ( srch == srch_TEXT )
                srch = srch_HEX;
            else
                srch = srch_TEXT;

            halfhex = -1;
            break;

        case 21:  /* control-U */
            search_length = 0;
            break;

        case 0x1b:  /* esc */
            search_length = -1;
            done = 1;
            break;

        case 10:   case 13:   /* ret */
            done = 1;
            break;
        }

        if ( done == 1 )
            break;

        if ( srch == srch_HEX )
        {
            if ( ishex( c ))
            {
                if ( halfhex == -1 )
                {
                    halfhex = c;
                    continue;
                }
                c = hex_to_dig( c );
                c += (hex_to_dig(halfhex) << 4);
                halfhex = -1;
                search_string[ search_length++ ] = c;
                continue;
            }
        }
        else
        {
            if ( isprint( c ))
            {
                search_string[ search_length++ ] = c;
                continue;
            }
        }
    }

    if ( search_length > 0 )
    {
        off_t pos, npos, searched;
#define SBUFSZ 65536
        static unsigned char buf[ SBUFSZ ];
        unsigned char machine[ 80 ];  /* search state machine */
        int cur;            /* current state */
        int i, interrupt;
        time_t last_display_time, current_time;

        /*
         * first build the state machine (look for substrings
         * within the search pattern).  at each position,
         * find the largest number of chars from the beginning
         * of the pattern which will match up to but not including
         * the current char.
         */

        for ( cur = 0; cur < search_length; cur++ )
        {
            if ( cur == 0  ||  cur == 1 )
                machine[cur] = 0;
            else
            {
                machine[cur] = 0;
                for ( i = cur-1; i > 0; i-- )
                {
                    if ( memcmp( search_string,
                                 search_string + cur - i,
                                 i ) == 0 )
                    {
                        machine[cur] = i;
                        break;
                    }
                }
            }
        }

#ifdef OPEN_TTY
        for ( i = 0; i < search_length; i++ )
            DEBUG(( debug_fd, "search state machine pos %d -> %d\n",
                    i, machine[i] ));
#endif

        /* now begin the search */
        cur = 0;
        npos = file_position + 1;
        searched = 0;
        time( &last_display_time );
        interrupt = 0;

        do {

            int cc;

            pos = npos;
            lseek( file_descriptor, pos, SEEK_SET );
            cc = read( file_descriptor, buf, SBUFSZ );
            if ( cc <= 0 )
                break;

            searched += cc;
            npos = pos + cc;

            for ( i = 0; i < cc; i++ )
            {
                if ( search_string[cur] == buf[i] )
                {
                    if ( ++cur == search_length )
                        break;
                }
                else
                    do {

                        cur = machine[cur];
                        if ( search_string[cur] == buf[i] )
                        {
                            cur++;
                            break;
                        }

                    } while ( cur != 0 );
            }

            time( &current_time );
            if ( last_display_time != current_time )
            {
                struct timeval tv;
                fd_set rfds;
                last_display_time = current_time;
                werase( srch_win );
                wmove( srch_win, 1, 10 );
                wprintw( srch_win, "Searched 0x%s so far",
                         m_dump_number( searched, 16 ));
                wmove( srch_win, 2, 10 );
                wprintw( srch_win, "Press any key to interrupt...." );
                wmove( srch_win, 3, 10 );
                wrefresh( srch_win );
                tv.tv_sec = 0;
                tv.tv_usec = 0;
                FD_ZERO( &rfds );
                FD_SET( 0, &rfds );
                if ( select( 1, &rfds, 0, 0, &tv ) == 1 )
                    interrupt = 1;
            }

        } while ( cur != search_length  &&
                  npos != file_size  &&
                  interrupt == 0 );

        if ( cur == search_length )
        {
            file_position = pos + i - search_length + 1;
            cursor_line = LINES/3;
        }
        else
        {
            werase( srch_win );
            wmove( srch_win, 1, 10 );
            wprintw( srch_win, "No match found." );
            wmove( srch_win, 2, 10 );
            wprintw( srch_win, "Press any key to continue...." );
            wmove( srch_win, 3, 10 );
            wrefresh( srch_win );
            getch();
        }
    }

    delwin( srch_win  );
    delwin( srch_win1 );
    delwin( srch_win2 );
    touchwin( stdscr );
    refresh();
}

static const char help_text[] = 
"        hexedit  1.0\n"
"        ------------\n"
"\n"
"key    function              key    function\n"
"---    --------              ---    --------\n"
" h     left                   g     go to offset\n"
" l     right                  /     search for string\n"
" j     down                  tab    switch edit hex or text\n"
" k     up                     i     insert mode\n"
"^d     down page             esc    stop insert mode \n"
"^u     up page               ^v     ins next char verbatim\n"
" q     quit                   ?     help\n"
" m     set mark <letter>      '     got to mark <letter>\n"
" M     move by formulae      ^l     redraw screen\n"
" p     deref little-end ptr   P     deref big endian ptr\n"
" t     toggle text/hex mode\n";

static const char copyright[] = 
"\n"
"hexedit is written by phillip f knaack. you are free to use and\n"
"modify this program as you see fit. the only thing you can't do\n"
"with this program is claim you wrote it, and if you changed it\n"
"you must add your name here (so that if it breaks, people don't\n"
"blame me).\n";

static const char usage[] = 
"\n"
"usage: he <file>\n";

void
do_help_tty( void )
{
    printf( "%s", usage );
    printf( "%s", copyright );
}

void
do_help( void )
{
    WINDOW * help_window;

    help_window = newwin( LINES, COLS, 0, 0 );

    werase( help_window );
    touchwin( help_window );
    wprintw( help_window, "%s", help_text );
    wprintw( help_window, "%s", copyright );
    wprintw( help_window, "press any key to exit help\n" );
    wrefresh( help_window );
    getch();

    delwin( help_window );
    touchwin( stdscr );
    refresh();
}

void
deref_pointer( enum pointer_type pt )
{
    int result = 0, pos;
    unsigned char p[4];

    pos = file_position & ~3;
    get_file_data( pos, p, 4 );

    switch ( pt )
    {
    case LITTLE_ENDIAN_PTR:
        result =
            (p[0] << 24) +
            (p[1] << 16) +
            (p[2] <<  8) +
            (p[3] <<  0);
        break;

    case BIG_ENDIAN_PTR:
        result =
            (p[0] <<  0) +
            (p[1] <<  8) +
            (p[2] << 16) +
            (p[3] << 24);
        break;
    }

    file_position = result;
    cursor_line = LINES/3;
}

int
ishex( int c )
{
    if (( c >= '0' ) && ( c <= '9' ))
        return 1;
    if (( c >= 'a' ) && ( c <= 'f' ))
        return 1;
    if (( c >= 'A' ) && ( c <= 'F' ))
        return 1;

    return 0;
}

int
hex_to_dig( int c )
{
    if (( c >= 'A' ) && ( c <= 'F' ))
        return c - 'A' + 10;

    if (( c >= 'a' ) && ( c <= 'f' ))
        return c - 'a' + 10;

    if (( c >= '0' ) && ( c <= '9' ))
        return c - '0';

    return 0;
}

static void
do_mark( enum mark_op op )
{
    WINDOW * mark_win2, * mark_win1, * mark_win;
    int i, c, waitforkey = 1;

    MAKEWIN( mark_win, 12, 59 );

    wprintw( mark_win,
             "                  select mark to set : [ ]\n"
             "              select mark to jump to : [ ]\n"
             "\n"
             "\n"
             "\n"
             "\n"
             "\n"
             "\n"
             "\n"
             "                                   [all #s in hex]\n"
             "\n"
             "        [tab: switch] [esc: cancel] [a-z: proceed]" );

    for ( i = 0; i < MAX_MARKS; i++ )
    {
        wmove( mark_win, (3 + (i/4)), (1 + (15*(i%4))) );
        if ( mark_locations[ i ] == -1 )
            wprintw( mark_win, "%c: <unset>", 'a' + i );
        else
            wprintw( mark_win, "%c: %9s", 'a' + i,
                     m_dump_number( mark_locations[ i ], 16 ));
    }

    while ( 1 )
    {
        if ( op == SET_MARK )
            wmove( mark_win, 0, 40 );
        else /* op == GOTO_MARK */
            wmove( mark_win, 1, 40 );

        wrefresh( mark_win );

        switch ( c = getch() )
        {
        case 9: /* tab */
            if ( op == SET_MARK )
                op = GOTO_MARK;
            else
                op = SET_MARK;
            break;

        case 0x1b: /* esc */
            goto out;

        case 12:  /* control-L */
            touchwin( mark_win );
            wrefresh( mark_win );
            break;
        }
        if ( c >= 'a' && c <= 'z' )
        {
            c -= 'a';
            break;
        }
        if ( c >= 'A' && c <= 'Z' )
        {
            c -= 'A';
            break;
        }
    }

    werase( mark_win );
    wmove( mark_win, 6, 5 );

    if ( op == GOTO_MARK )
    {
        if ( mark_locations[c] == -1 )
        {
            wprintw( mark_win,
                     "mark %c is not set now", c + 'a' );
        }
        else
        {
            file_position = mark_locations[c];
            cursor_line = LINES/3;
            waitforkey = 0;
        }
    }
    else  /* op == SET_MARK */
    {
        char * now = "";
        if ( mark_locations[c] != -1 )
        {
            wprintw( mark_win,
                     "mark %c was set at offset %08x\n     ",
                     c + 'a', mark_locations[c] );
            now = "now ";
        }
        mark_locations[c] = file_position;
        wprintw( mark_win,
                 "mark %c %sset at offset %08x",
                 c + 'a', now, mark_locations[c] );
    }

    if ( waitforkey )
    {
        wmove( mark_win, 11, 5 );
        wprintw( mark_win, "press any key to continue..." );
        wrefresh( mark_win );
        getch();
    }

 out:
    delwin( mark_win );
    delwin( mark_win1 );
    delwin( mark_win2 );
    touchwin( stdscr );
    refresh();
}

static char formula[49];
static int  formlen;

static void
formula_initialize( void )
{
    formula[0] = '$';
    formula[1] = '0';
    formula[2] = ' ';
    formula[3] = 0;
    formlen = 3;
}

static void
do_rel_move( void )
{
    WINDOW * move_win2, * move_win1, * move_win;
    int maxformlen = 49;
    char formula_calc[180];
    int formlen_calc, c, i;
    M_INT64 result;
    enum m_math_retvals mathret;

    MAKEWIN( move_win, 12, 56 );

 reedit:
    werase( move_win );
    wprintw( move_win,
             " enter a formulae to calculate next cursor position.\n"
             "\n"
             " [                                                 ]\n"
             "    [enter: calculate] [^u: erase] [esc: cancel]\n"
             "\n"
             " calculator is stack-based, with same format as 'm'\n"
             " program. use arg '$<letter>' to replace with address\n"
             " of a mark. use '$0' [digit, not letter] to represent\n"
             " the current cursor position. example: \n"
             "    $0 $a - 40h * $a +\n"
             " (means: current pos minus mark 'a' value, times 40h,\n"
             "  and plus mark 'a' value; result is new position" );

    while ( 1 )
    {
        wmove( move_win, 2, 2 );
        for ( i = 0; i < maxformlen; i++ )
        {
            if ( i < formlen )
                wprintw( move_win, "%c", formula[i] );
            else
                wprintw( move_win, " " );
        }

        wmove( move_win, 2, 2+formlen );
        wrefresh( move_win );

        switch ( c = getch() )
        {
        case 13: case 10: /* enter */
            goto calculate;

        case 21: /* control-U */
            formula_initialize();
            break;

        case 27: /* esc */
            goto out;

        case 8: /* backspace */
            if ( formlen > 0 )
                formula[--formlen] = 0;
            break;
        }

        if ( c >= ' ' && c <= 'z' )
        {
            if ( formlen < maxformlen )
            {
                formula[formlen++] = c;
                formula[formlen] = 0;
            }
        }
    }

 calculate:
    memcpy( formula_calc, formula, formlen+1 );
    formlen_calc = formlen;
    wmove( move_win, 3, 0 );
    wclrtobot( move_win );
    wmove( move_win, 5, 0 );

    /* perform $ substitutions */
    for ( i = 0; i < formlen_calc; i++ )
    {
        if ( formula_calc[i] == '$' )
        {
            int which = formula_calc[i+1];
            M_INT64 whichval;
            if ( which == '0' )
                whichval = file_position;
            else if ( which >= 'a' && which <= 'z' )
            {
                whichval = mark_locations[which - 'a'];
                if ( whichval == -1 )
                {
                    /* mark not set now */
                    wprintw( move_win,
                             "  mark %c is not set now!\n"
                             "  re-edit (y or n) ? ", which );
                reedit_ask:
                    wrefresh( move_win );
                    while ( c = getch())
                    {
                        if ( c == 'y' )
                            goto reedit;
                        if ( c == 'n' )
                            goto out;
                    }
                }
            }
            else
            {
                wprintw( move_win,
                         "  char %c is not a valid mark char\n"
                         "  re-edit (y or n) ? ", which );
                goto reedit_ask;
            }
            bcopy( formula_calc+i+2,
                   formula_calc+i+11,
                   formlen_calc - i - 1 );
            sprintf( formula_calc+i, "%10sh", m_dump_number( whichval, 16 ));
            formula_calc[i+11] = ' ';
            formlen_calc += 9;
        }
    }

    formula_calc[formlen_calc] = 0;
    werase( move_win );
    wmove( move_win, 0, 0 );
    wprintw( move_win, "  formula: %s\n\n", formula_calc );
    wrefresh( move_win );

    /* spawn off m_do_math */
    {
        char * arg = formula_calc;
        mathret = m_do_math( 1, &arg, &result, NULL );
    }

    if ( mathret != M_MATH_OK )
    {
        /* handle math error */
        wprintw( move_win, "  error %d: %s\n\n",
                 mathret, (char*)(int)result );
        wprintw( move_win, "  re-edit (y or n) ? " );
        goto reedit_ask;
    }

    wprintw( move_win,
             "  result: %10s (hex)\n",
             m_dump_number( result, 16 ));
    wprintw( move_win,
             "          %10s (dec)\n\n",
             m_dump_number( result, 10 ));
    wprintw( move_win, "  do you want to go there (y or n) ? " );
    wrefresh( move_win );
    while ( c = getch())
    {
        if ( c == 'n' )
            goto out;
        if ( c == 'y' )
            break;
    }

    file_position = result;
    cursor_line = LINES/3;

 out:
    delwin( move_win2 );
    delwin( move_win1 );
    delwin( move_win  );
    touchwin( stdscr );
    refresh();
}
