/* -*- Mode:c++; eval:(c-set-style "BSD"); c-basic-offset:4; indent-tabs-mode:nil; tab-width:8 -*- */

/*
    This file is part of the "pfkutils" tools written by Phil Knaack
    (pknaack1@netscape.net).
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

/** \file protos.h
 * \brief prototypes of various functions in the pfkbak tool.
 * \author Phillip F Knaack
 */

#ifndef __PFKBAK_PROTOS_H__
#define __PFKBAK_PROTOS_H__

/** @name utility function prototypes */
//@{
bool   pfkbak_get_info    ( PfkBackupDbInfo * info );
UINT32 pfkbak_find_backup ( char * bakname );
void   pfkbak_md5_buffer  ( UCHAR * buf, int buflen, UCHAR * md5 );
void   pfkbak_sprint_md5  ( UCHAR * md5hash, char * string );
#define MD5_STRING_LEN 34
//@}

/** @name command-line triggered operations */
//@{

void pfkbak_create_file   ( char * filename );
void pfkbak_list_backups  ( void );
void pfkbak_create_backup ( char * bakname, char * root_dir, char * comment );
void pfkbak_delete_backup ( UINT32 baknum );
void pfkbak_update_backup ( UINT32 baknum );
void pfkbak_delete_gens   ( UINT32 baknum, int argc, char ** argv );
void pfkbak_delete_gen    ( UINT32 baknum,
                            UINT32 gen_num_s, UINT32 gen_num_e );
void pfkbak_list_files    ( UINT32 baknum, UINT32 gen_num );
void pfkbak_extract       ( UINT32 baknum, UINT32 gen_num,
                            int argc, char ** argv );
void pfkbak_extract_list  ( UINT32 baknum, UINT32 gen_num,
                            char * list_file );

//@}

#endif /* __PFKBAK_PROTOS_H__ */
