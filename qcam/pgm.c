
/*
    This file is part of the "pkutils" tools written by Phil Knaack
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

#include "headers.h"

void
pgm_readhdr(qs,f)
	struct qcam_softc *qs;
	FILE *f;
{
	char inl[40], *tp;

	if (fgets(inl, 40, f) == NULL)
		return;

	if (fgets(inl, 40, f) == NULL)
		return;

	tp = inl;
	strsep(&tp, " ");
	qs->x_size = atoi(inl);
	qs->y_size = atoi(tp);
	qs->init_req = 1;

	fgets(inl, 40, f);
}

void
pgm_read(qs,f)
	struct qcam_softc *qs;
	FILE *f;
{
	fread(qs->buffer, qs->x_size, qs->y_size, f);
}

void
pgm_writehdr(qs, f)
	struct qcam_softc *qs;
	FILE *f;
{
	fprintf(f, "P5\n");
	fprintf(f, "%d %d\n", qs->x_size, qs->y_size);
	fprintf(f, "63\n");
}

void
pgm_write(qs, f)
	struct qcam_softc *qs;
	FILE *f;
{
	fwrite(qs->buffer, qs->x_size, qs->y_size, f);
}
