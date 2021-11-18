
/*
 * this file contains an implementation of the "approximate string
 * matching" bestmatch algorithm as described in the class of
 * computer science 311, as taught at Iowa State University by
 * Mr Fernandez-Baca in spring semester 1997.
 * 
 * implemented by Phillip F Knaack, copyright 1997, 1998.
 * do with this as you like, but just don't claim you wrote it.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef __P
#define __P(x) x
#endif

struct bestmatch {
    int match_start;
    int match_end;
    int approx;
};

static void bestmatch_cost ( char *, char *, struct bestmatch * );
static void print_matched_string ( struct bestmatch *, char *, int );

static void
usage( void )
{
    fprintf( stderr,
             "\n\n"
             "Approximate GREP: perform approximate string matching.\n"
             "\n"
             "usage: agrep string file [file...]\n"
             "\n\n" );
    exit( 1 );
}

int
agrep_main( int argc, char ** argv )
{
    int onefile = 0;
    int level;
    char *pattern = argv[1];
    int terminal;

    int pargc;
    char ** pargv;
    
    terminal = isatty(1);

    pargc = argc;
    pargv = argv;

    if (argc < 3)
        usage();

    pattern = argv[1];
    level = atoi(argv[2]);

    if (argc > 4)
        onefile = 1;

    for (level=0; level < (strlen(pattern)-1); level++)
    {
        argc = pargc - 2;
        argv = pargv + 2;

        while (argc)
        {
            FILE *f;
            char inputline[300];
            struct bestmatch bm;

            f = fopen(argv[0], "re");

            if (!f)
            {
                fprintf(stderr, "cannot open %s\n", argv[0]);
                goto nextfile;
            }

            while (fgets(inputline, 300, f))
            {
                char *tmp;
                tmp = strdup(inputline);
                bestmatch_cost(tmp, pattern, &bm);
                free(tmp);
                if (bm.approx <= level)
                {
                    if (onefile)
                        printf("%s: ", argv[0]);
                    print_matched_string(&bm,
                                         inputline,
                                         terminal);
                }
            }
            fclose(f);

        nextfile:
            argc--;
            argv++;
        }
    }
    return 0;
}

static void
toupper_case( char *str )
{
    while (*str)
    {
        *str = toupper(*str);
        str++;
    }
}

/* 
 * this algorithm constructs a two-dimensional array
 * of size m+1 by n+1, where m is the strlen of the text,
 * and n is the strlen of the pattern to search for in the text.
 *
 * a pointer is started at the upper left.  a move to the right
 * indicates moving forward in the text by one character.  a move
 * downward indicates moving forward in the pattern.  at each 
 * element, the value stored there indicates the number of 
 * "corrections" which must be made to cause the pattern to match
 * the text.
 *
 * "corrections" come in three flavors.  first, substituting a letter.
 * second, inserting a "blank" into the text to change the lineup of
 * the text and pattern. and third, by inserting a "blank" into the
 * pattern to change the lineup with the text.
 *
 * using this matrix, all possible combinations of this are calculated,
 * by using the following rule:
 *  the value of an element of the matrix is the smallest of the
 *  following three cases.
 *      1) if the character in the text at that position matches 
 *         the character in the pattern, add zero to the value to the
 *         upper left.  if they do not match, add 1 to the value to the
 *         upper left. this corresponds to either a match
 *         or a substitution.
 *      2) the value is 1 plus the value in the element above.
 *         this corresponds to adding a space in the pattern.
 *      3) the value is 1 plus the value in the element to the left,
 *         or equal to the element to the left if this is the rightmost
 *         element in this row.
 *         this corresponds to adding a space in the text.
 *
 * when all elements of the matrix are calculated, the value in the 
 * bottom right element is the number of corrections which must be made
 * for the best possible match between the string and the pattern.
 *
 * also, an extension to this algorithm is that at each element we store
 * the case (1, 2, or 3) which we used to obtain that value.  then, the
 * "path" can be traced backwards from the bottom right element to the
 * first character of the text which is matched by the pattern.
 * 
 * 
 *
 * this function returns an integer identifying the start index of
 * the matched pattern in the text.
 */

static void
bestmatch_cost( char * text,
                char * pattern,
                struct bestmatch *results )
{
    enum camefrom { INIT=0, CASE1, CASE2, CASE3 };
    struct answer {
        int value;
        enum camefrom camefrom;
    } *answer;
    int m,n,i,j;
    int end_match = 0;

    n = strlen(pattern);
    if ((m = strlen(text)) < n)
    {
        results->match_start = results->match_end = 0;
        results->approx = n;
        return;
    }

    toupper_case( text );
    toupper_case( pattern );

#define ARRAYSIZE ((n+1) * (m+1) * sizeof(struct answer))
#define IND(x,y) ((x)+(n+1)*(y))

    answer = (struct answer *) malloc( ARRAYSIZE );

    memset( answer, 0, ARRAYSIZE );

    /* 
     * initialize the top row
     */

    for (i=0; i <= n; i++)
        answer[IND(i,0)].value = i;

    /*
     * iterate over the matrix, filling it in as we go
     */

    for (j=1; j <= m; j++)
    {
        for (i=1; i <= n; i++)
        {
            int case1, case2, case3, r;
            enum camefrom w;

            /*
             * calculate the three cases
             */

            if (pattern[i-1] == text[j-1])
                case1 = answer[IND(i-1,j-1)].value;
            else
                case1 = answer[IND(i-1,j-1)].value + 1;

            case2 = answer[IND(i-1,j)].value + 1;
        
            /*
             * this was special: not noted in class,
             * case3 does NOT cost more than the neighbor
             * above in the matrix if we're at the end of
             * the pattern.
             */

            if (i == n)
                case3 = answer[IND(i,j-1)].value;
            else
                case3 = answer[IND(i,j-1)].value + 1;

            /*
             * now, find the smallest of the three cases;
             * the order here is very special, because if
             * there is a tie between case1 and some other
             * case, we default to case1. this helps the
             * special case where the last character of the
             * pattern doesn't match in the string. if this
             * order wasn't assured, we wouldn't be able to
             * reliably find the end of the match in that
             * case.
             */

            if (case2 < case3)
            {
                if (case2 < case1)
                {
                    r = case2; w = CASE2;
                } else {
                    r = case1; w = CASE1;
                }
            } else {
                if (case3 < case1)
                {
                    r = case3; w = CASE3;
                } else {
                    r = case1; w = CASE1;
                }
            }

            /*
             * we store both the cost of this match,
             * and what case we used to get here. this
             * makes it possible to back-track later
             * to find the beginning of the match.
             */

            answer[IND(i,j)].value = r;
            answer[IND(i,j)].camefrom = w;
        }
    }

/*
 *  DISPLAY THE MATRIX FOR DEBUGGING PURPOSES
 *  displays in the form
 *  
 *  (index_of_pattern, index_of_text) = cost_of_match, where_came from
 */

#ifdef DEBUG
    for (j=0; j <= m; j++)
    {
        for (i=0; i <= n; i++)
            printf("(%2d,%2d)=%d,%d ",
                   i, j,
                   answer[IND(i,j)].value,
                   answer[IND(i,j)].camefrom);
        printf("\n");
    }
#endif

    i = n;
    j = m;

    /*
     * start at the end of the matrix, and start back-tracking
     * to find the actual path taken. we mark the end of the match
     * as we pass it, and we know the beginning when we've reached
     * the beginning of the pattern (the i==1 case)
     */
    
    while (i != 1)
    {
        /* if we're at the end of the pattern, remember it */
        if (i == n)
            end_match = j;

        switch (answer[IND(i,j)].camefrom)
        {
        case CASE1:
            i--; j--;
            break;
        case CASE2:
            i--;
            break;
        case CASE3:
            j--;
            break;
            /*
             * this is the case where we reach one of the (x,0)
             * cases. we just start going left till we hit the 
             * beginning of the pattern.
             */
        case INIT:
            i--;
            break;
        }
    }

    /* the "-1"s are because this function addresses the
       start of the string at 1. but in real life, they
       start at 0. */

    results->match_start = j - 1;
    results->match_end   = end_match - 1;
    results->approx  = answer[IND(n,m)].value;

    free( answer );
}

static void
print_matched_string( struct bestmatch * bm, char * line, int terminal )
{
    int i;

    for (i=0; i < bm->match_start; i++)
        putchar(line[i]);
    if (terminal)
        printf("%c[7m", 27);
    for (; i <= bm->match_end; i++)
        putchar(line[i]);
    if (terminal)
        printf("%c[m", 27);
    for (; line[i] != 0; i++)
        putchar(line[i]);
}
