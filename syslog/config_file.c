/*
    This file is part of the "pfkutils" tools written by Phil Knaack
    (pfk@pfk.org).
    Copyright (C) 2008  Phillip F Knaack

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "regex.h"
#include "rules.h"
#include "config_file.h"
#include "strip_chars.h"

/* see sample.ini */

int line_number;
int syslogd_port_number = 514;
FILE * raw_output_file = NULL;

#if DEBUG_LOG
FILE * debug_log_file;
#endif

struct rule_file * rule_files;
struct rule_file * rule_files_tail;
int next_rule_id = 1;
struct rule * rules;
struct rule * rules_tail;

void
syntax_error(void)
{
    fprintf(stderr,"config file syntax line %d\n",line_number);
    exit(1);
}

static const char * config_file_expr_string = "\
^(\\[config])$|\
^(\\[ignore])$|\
^(\\[type])$|\
^(\\[default])$|\
^stdout=([^ \t]+)|\
^raw-output=([^ \t]+)|\
^output=([^ \t]+)|\
^pattern=(.*)$|\
^port=([0-9]+)$|\
^([ \t]*#.*)$|\
^([ \t]+|)$|\
^(.*)$\
";

enum config_file_expr_type {
    ARG_DUMMY = 0,
    ARG_CONFIG, ARG_IGNORE, ARG_TYPE, ARG_DEFAULT,
    ARG_STDOUT, ARG_RAW_OUTPUT, ARG_OUTPUT, ARG_PATTERN, ARG_PORT,
    ARG_COMMENT, ARG_BLANKLINE, ARG_ERR,
    ARG_LARGEST
};

void
parse_config_file(char *config_file)
{
    FILE * f;
    char line[MAX_LINE_LEN];
    char match[MAX_LINE_LEN];
    int match_len;
    enum {
        STATE_TOPLEVEL,
        STATE_CONFIG,
        STATE_IGNORE,
        STATE_TYPE,
        STATE_DEFAULT
    } state;
    int file_specified;
    int default_specified;
    regex_t expr;
    int i, regerr, arg_type;
    regmatch_t matches[ ARG_LARGEST ];
    struct rule_file * rfp;
    struct rule * rp;
    int rule_count = 0;
    int file_count = 0;
    int enable_stdout;

#if DEBUG_LOG
    debug_log_file = fopen( "debug_log.txt", "w" );
#endif

    f = fopen( config_file, "r" );
    if (!f)
    {
        fprintf(stderr, "unable to open configuration file '%s'\n",
                config_file);
        return;
    }

    regerr = regcomp( &expr, config_file_expr_string, REG_EXTENDED );
    if ( regerr != 0 )
    {
        regerror(regerr, &expr, line, sizeof(line));
        fprintf(stderr, "compiling config file expr: %s\n", line);
        exit(1);
    }

    line_number = 0;
    file_specified = 0;
    default_specified = 0;
    state = STATE_TOPLEVEL;

    while (fgets(line,sizeof(line)-1,f))
    {
        line[sizeof(line)-1] = 0;
        strip_chars(line, strlen(line)+1);
        line_number++;
        regerr = regexec( &expr, line, ARG_LARGEST, matches, 0 );
        if (regerr != 0)
        {
            regerror(regerr, &expr, line, sizeof(line));
            fprintf(stderr, "pattern-matching line %d: %s\n",
                    line_number, line);
            exit(1);
        }
        arg_type = 0;
        for (i=1; i<ARG_LARGEST; i++)
        {
            if ( (int)matches[i].rm_so != -1 )
            {
                arg_type = i;
                break;
            }
        }

        match_len = matches[arg_type].rm_eo - matches[arg_type].rm_so;
        memcpy(match, line + matches[arg_type].rm_so, match_len);
        match[match_len]=0;

        switch (arg_type)
        {
        case ARG_CONFIG:
            state = STATE_CONFIG;
            break;

        case ARG_IGNORE:
            state = STATE_IGNORE;
            file_specified = 0;
            break;

        case ARG_TYPE:
            state = STATE_TYPE;
            file_specified = 0;
            break;

        case ARG_DEFAULT:
            state = STATE_DEFAULT;
            file_specified = 0;
            break;

        case ARG_STDOUT:
            if (state != STATE_TYPE && state != STATE_DEFAULT)
            {
                fprintf(stderr, "stdout should only be "
                        "in [type] or [default] sections\n");
                exit(1);
            }
            if (!file_specified)
            {
                fprintf(stderr, "stdout spec must be after output spec\n");
                exit(1);
            }
            if (strcmp(match,"on")==0)
                enable_stdout = 1;
            else if (strcmp(match,"off")==0)
                enable_stdout = 0;
            else
                syntax_error();
            rule_files_tail->enable_stdout = enable_stdout;
            break;

        case ARG_RAW_OUTPUT:
            if (state != STATE_CONFIG)
            {
                fprintf(stderr, "line %d: "
                        "raw-output= should be in "
                        "[config] section\n",
                        line_number);
                exit(1);
            }
            raw_output_file = fopen(match,"a");
            break;

        case ARG_OUTPUT:
            if (state != STATE_TYPE && state != STATE_DEFAULT)
            {
                fprintf(stderr,
                        "output must be in type or default sections\n");
                exit(1);
            }
            if (file_specified)
            {
                fprintf(stderr, "line %d: "
                        "file already specified in this section\n",
                        line_number);
                exit(1);
            }
            file_specified = 1;
            rfp = (struct rule_file *)malloc(sizeof(struct rule_file));
            rfp->next = NULL;
            rfp->file = fopen(match,"a");
            rfp->enable_stdout = 1;
            if (!rfp->file)
            {
                fprintf(stderr, "line %d: "
                        "unable to open output file '%s': %s\n",
                        line_number, match, strerror(errno));
                exit(1);
            }
            if (rule_files_tail)
            {
                rule_files_tail->next = rfp;
                rule_files_tail = rfp;
            }
            else
                rule_files = rule_files_tail = rfp;
            file_count++;

            if (state == STATE_DEFAULT)
            {
                if (default_specified)
                {
                    fprintf(stderr, "line %d: "
                            "already specified a default section!\n",
                            line_number);
                    exit(1);
                }
                default_specified = 1;
                rp = (struct rule *)malloc(sizeof(struct rule));
                rp->next = NULL;
                rp->rule_id = next_rule_id++;
#if DEBUG_LOG
                fprintf(debug_log_file, "default rule number is %d\n", rp->rule_id);
#endif
                rp->action = ACTION_DEFAULT_STORE;
                rp->file = rule_files_tail;
                memset(&rp->expr, 0, sizeof(rp->expr));
                if (rules_tail)
                {
                    rules_tail->next = rp;
                    rules_tail = rp;
                }
                else
                    rules = rules_tail = rp;
                rule_count++;
            }

            break;

        case ARG_PATTERN:
            if (default_specified)
            {
                fprintf(stderr, "line %d: "
                        "cannot have patterns after default output file\n",
                        line_number);
                exit(1);
            }
            rp = (struct rule *)malloc(sizeof(struct rule));
            switch (state)
            {
            case STATE_TOPLEVEL:
            case STATE_DEFAULT:
                syntax_error();
                break;

            case STATE_IGNORE:
                if (file_specified)
                {
                    fprintf(stderr, "line %d: "
                            "[ignore] section cannot have output file\n",
                            line_number);
                    exit(1);
                }
                rp->action = ACTION_COMPARE_IGNORE;
                break;

            case STATE_TYPE:
                if (!file_specified)
                {
                    fprintf(stderr, "line %d: "
                            "[type] section must have output file\n",
                            line_number);
                    exit(1);
                }
                rp->action = ACTION_COMPARE_STORE;
                break;

            default:
                // nothing? perhaps error?
                break;
            }
            rp->next = NULL;
            rp->rule_id = next_rule_id++;

#if DEBUG_LOG
            fprintf(debug_log_file, "rule %d (%d:%s): %s\n",
             rp->rule_id,
             rp->action,
             rp->action==ACTION_COMPARE_IGNORE ? "ignore" : "store",
             match);
#endif

            rp->file = rule_files_tail;
            regerr = regcomp(&rp->expr, match, REG_EXTENDED);
            if (regerr != 0)
            {
                regerror(regerr, &expr, line, sizeof(line));
                fprintf(stderr, "line %d: "
                        "error compiling regex: %s\n",
                        line_number, line);
                exit(1);
            }
            if (rules_tail)
            {
                rules_tail->next = rp;
                rules_tail = rp;
            }
            else
                rules = rules_tail = rp;
            rule_count++;
            break;

        case ARG_PORT:
            if (state != STATE_CONFIG)
            {
                fprintf(stderr, "line %d: "
                        "port= statement should only be "
                        "in [config] section\n",
                        line_number);
                exit(1);
            }
            syslogd_port_number = atoi(match);
            break;

        case ARG_COMMENT:
        case ARG_BLANKLINE:
            break;

        case ARG_ERR:
            syntax_error();
            break;

        default:
            fprintf(stderr, "no matches found!\n");
        }
    }
    regfree( &expr );
    fclose(f);
#if DEBUG_LOG
    fflush(debug_log_file);
#endif
    if (!default_specified)
    {
        fprintf(stderr, "no default output file specified!\n");
        exit(1);
    }
    if (!rules)
    {
        fprintf(stderr, "no rules specified!\n");
        exit(1);
    }
    fprintf(stderr, "parsed %d rules for %d files\n",
            rule_count, file_count);
}
