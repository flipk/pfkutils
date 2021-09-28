#ifndef COST_THRESHOLD
#define COST_THRESHOLD 10
#endif

/*
 * to find the ideal pfk arch, we want to find a subdir that best
 * matches the arch we're actually on. when automated patches update
 * the linux kernel, a patch level increments but the major does not.
 *
 * this code works by using approximate string matching (described in
 * more detail below) to locate several candidates which are
 * "closest".  next, we extract "place values" for the numbers in the
 * strings.  e.g. the string "Linux-5.10.6-200.fc33.x86_64" contains
 * the following place values: 5, 10, 6, 200, 33, 86, 64.  (In this
 * case the x86_64 is not relevant, but since they're so far to the
 * right they don't play much of a role; if you have a pfkdir with
 * lots of 'x86'_64 and also lots of 'i686', hopefully the approximate
 * matching code percolates the matching CPU types to the top anyway.
 *
 * we then construct a single 64-bit integer using these place values
 * by bitshifting them in. we figure out how many bits the first place
 * requires (i.e. '5' requires 3 bits, '10' requires 4, and '200' requires
 * 8 bits) and then shift the values together.
 *
 * note that we have to measure the bitwidths of every place value of
 * every candidate, and pick the max bitwidth of that place value in
 * all the candidates, before we shift any of them, because the nth
 * place value of every candidate must be bitshifted to the same
 * position.
 *
 * now, each entry can be numerically compared. the largest pvalue
 * that doesn't go over the desired arch's pvalue wins.
 *
 * ======================================================================
 *
 * this file contains an implementation of the "approximate string
 * matching" bestmatch algorithm as described in the class of Computer
 * Science 311, as taught at Iowa State University by Mr
 * Fernandez-Baca in spring semester 1997.
 *
 * implemented by Phillip F Knaack, copyright 1997-2021 as homework
 * for that class.  do with this as you like, but just don't claim you
 * wrote it.
 *
 * this algorithm constructs a two-dimensional array of size m+1 by
 * n+1, where m is the strlen of the text, and n is the strlen of the
 * pattern to search for in the text.
 *
 * a pointer is started at the upper left.  a move to the right
 * indicates moving forward in the text by one character.  a move
 * downward indicates moving forward in the pattern.  at each element,
 * the value stored there indicates the number of "corrections" which
 * must be made to cause the pattern to match the text.
 *
 * "corrections" come in three flavors.  first, substituting a letter.
 * second, inserting a "blank" into the text to change the lineup of
 * the text and pattern. and third, by inserting a "blank" into the
 * pattern to change the lineup with the text.
 *
 * using this matrix, all possible combinations of this are
 * calculated, by using the following rule:
 *   the value of an element of the matrix is the smallest of the
 *   following three cases.
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
 * bottom right element is the number of corrections which must be
 * made for the best possible match between the string and the
 * pattern.
 *
 * also, an extension to this algorithm is that at each element we
 * store the case (1, 2, or 3) which we used to obtain that value.
 * then, the "path" can be traced backwards from the bottom right
 * element to the first character of the text which is matched by the
 * pattern.
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>

struct bestmatch {
    int match_start;
    int match_end;
    int cost; // how many edits had to be made?
};

static void
bestmatch_cost( const char * text, const char * pattern,
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
        results->cost = n;
        return;
    }

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
                    r = case2;
                    w = CASE2;
                }
                else
                {
                    r = case1;
                    w = CASE1;
                }
            }
            else
            {
                if (case3 < case1)
                {
                    r = case3;
                    w = CASE3;
                }
                else
                {
                    r = case1;
                    w = CASE1;
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
    results->cost        = answer[IND(n,m)].value;

    free( answer );
}

#define MAX_PVS 10

struct candidate {
    struct candidate * next;
    char               name      [512];
    int                cost;
    uint64_t           pvs       [MAX_PVS];
    int                bitwidths [MAX_PVS];
    int                num_pvs;
    uint64_t           pvalue;
};

static int
calculate_width(int val)
{
    int ret = 0;
    while (val != 0)
    {
        val >>= 1;
        ret++;
    }
    return ret;
}

static void
extract_pvs(struct candidate *c)
{
    int in_digit = 0;
    const char * cp = c->name;

    c->num_pvs = 0;
    while (1)
    {
        // the loop is oriented this way on purpose, to
        // process the tailing nul in the not-digit code.
        // easiest way i could think of to finish every pv.
        if (isdigit(*cp))
        {
            int d = (*cp - '0');
            if (in_digit == 0)
                // starting a new place
                c->pvs[c->num_pvs] = d;
            else
                // continuing a place already started.
                c->pvs[c->num_pvs] = (c->pvs[c->num_pvs] * 10) + d;
            in_digit = 1;
        }
        else
        {
            if (in_digit)
            {
                // finishing a place
                c->bitwidths[c->num_pvs] =
                    calculate_width(c->pvs[c->num_pvs]);
                if (++c->num_pvs >= MAX_PVS)
                {
                    fprintf(stderr, "OUT OF PVS\n");
                    break;
                }
            }
            in_digit = 0;
        }
        if (*cp == 0)
            break;
        cp++;
    }
}

static void
normalize_bitwidths(struct candidate *cands)
{
    int bitwidths[MAX_PVS];
    int num_pvs = 0;
    int ind;
    struct candidate *c;

    memset(bitwidths, 0, sizeof(bitwidths));

    // find the largest bitwidth for each place
    // across all candidates.
    for (c = cands; c; c = c->next)
    {
        if (num_pvs < c->num_pvs)
            num_pvs = c->num_pvs;
        for (ind = 0; ind < c->num_pvs; ind++)
            if (bitwidths[ind] < c->bitwidths[ind])
                bitwidths[ind] = c->bitwidths[ind];
    }

    // now go back and calculate pvalues for each
    // candidate using the max-bitwidths we just
    // finished measuring.
    for (c = cands; c; c = c->next)
    {
        uint64_t pvalue = 0;
        int bitpos = 64;
        for (ind = 0; ind < num_pvs; ind++)
        {
            bitpos -= bitwidths[ind];
            pvalue |= (c->pvs[ind] << bitpos);
        }
        c->pvalue = pvalue;
    }
}

int
main(int argc, char ** argv)
{
    int ret = 1;
    struct candidate arch;
    struct candidate * c, * best_c, * nc;
    struct candidate * cands, ** pnext_cands;
    const char * dirpath;

    if (argc != 3)
    {
        printf("usage: find-pfk-pf <dir> <pfkarch>\n");
        return ret;
    }

    dirpath = argv[1];
    if (chdir(dirpath) < 0)
    {
        int e = errno;
        fprintf(stderr, "chdir '%s': %d: %s\n",
                dirpath, e, strerror(e));
        return 1;
    }

    strncpy(arch.name, argv[2], sizeof(arch.name));
    arch.name[sizeof(arch.name)-1] = 0;
    extract_pvs(&arch);

    cands = NULL;
    pnext_cands = &cands;

    DIR * d = opendir(".");
    if (d)
    {
        struct dirent * de;
        struct stat sb;
        struct bestmatch bm;

        while ((de = readdir(d)) != NULL)
        {
            const char * pfent = de->d_name;

            if (strcmp(pfent, ".") == 0  ||
                strcmp(pfent, "..") == 0)
            {
                continue;
            }

            if (stat(pfent, &sb) < 0)
            {
                int e = errno;
                fprintf(stderr, "stat '%s/%s': %d: %s\n",
                        dirpath, pfent, e, strerror(e));
                continue;
            }

            // note we used stat, not lstat, so this returns
            // "DIR" for a symlink to a dir, which is what
            // we want -- many pfk arches are symlinks to dirs.
            if (!S_ISDIR(sb.st_mode))
            {
                // not a dir, skipping
                continue;
            }

            bestmatch_cost( pfent, arch.name, &bm );
            if (bm.cost < COST_THRESHOLD)
            {
                c = malloc(sizeof(struct candidate));
                c->next = NULL;
                c->cost = bm.cost;
                strncpy(c->name, pfent, sizeof(c->name));
                c->name[sizeof(c->name)-1] = 0;
                c->pvalue = 0;
                extract_pvs(c);
                *pnext_cands = c;
                pnext_cands = &c->next;
            }
        }
        closedir(d);

        // the input arch has to be among the normalized
        // values, so put the linked list in arch's 'next'
        // and pass it.
        arch.next = cands;
        normalize_bitwidths(&arch);

        best_c = NULL;

        for (c = cands; c; c = c->next)
        {
            if (c->pvalue > arch.pvalue)
            {
                // too new! reject
            }
            else if (best_c == NULL)
            {
                // first best
                best_c = c;
            }
            else if (c->pvalue > best_c->pvalue)
            {
                // new best
                best_c = c;
            }
        }
        if (best_c)
            printf("%s\n", best_c->name);
        ret = 0;

        // release memory. technically don't have to if we're
        // just about to exit(2) anyway, but i'm pedantic.
        for (c = cands; c; c = nc)
        {
            nc = c->next;
            free(c);
        }
    }
    else
    {
        int e = errno;
        fprintf(stderr, "opendir '%s': %d: %s\n",
                dirpath, e, strerror(e));
    }

    return ret;
}
