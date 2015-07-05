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

#define MAX_LINE_LEN 10000

struct rule_file {
    struct rule_file * next;
    FILE * file;
    int enable_stdout;
};

extern struct rule_file * rule_files;
extern struct rule_file * rule_files_tail;

typedef enum {
    ACTION_COMPARE_IGNORE,
    ACTION_COMPARE_STORE,
    ACTION_DEFAULT_STORE
} rule_action;

struct rule {
    struct rule *next;
    int rule_id;
    rule_action action;
    regex_t expr;
    struct rule_file * file;
};

extern int next_rule_id;
extern struct rule * rules;
extern struct rule * rules_tail;
