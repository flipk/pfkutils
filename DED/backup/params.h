/* -*- Mode:c++; eval:(c-set-style "BSD"); c-basic-offset:4; indent-tabs-mode:nil; tab-width:8 -*- */

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

/** \file params.h
 * \brief defines and enums dealing with configuration and command line options
 *        for the pfkbak tool.
 * \author Phillip F Knaack
 */

#ifndef __PFKBAK_PARAMS_H__
#define __PFKBAK_PARAMS_H__

enum backop {
    BAK_NONE,
    BAK_CREATE_FILE,
    BAK_LIST_BACKUPS,
    BAK_CREATE_BACKUP,
    BAK_DELETE_BACKUP,
    BAK_UPDATE_BACKUP,
    BAK_DELETE_GENS,
    BAK_LIST_FILES,
    BAK_EXTRACT,
    BAK_EXTRACT_LIST
};

enum verblevel {
    VERB_QUIET,    // display nothing.
    VERB_1,        // display progress chart.
    VERB_2         // display filenames as it goes.
};

extern backop               pfkbak_op;
extern verblevel            pfkbak_verb;
extern char               * pfkbak_file;
extern char               * pfkbak_data_file;
extern char               * pfkbak_name;
extern int                  pfkbak_gen;
extern Btree              * pfkbak_meta;
extern FileBlockInterface * pfkbak_data;

#endif /* __PFKBAK_PARAMS_H__ */
