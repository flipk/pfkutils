/*
 * This file is licensed under the GPL version 2.
 * Refer to the file LICENSE in this distribution or
 * just search for GPL v2 on the website www.gnu.org.
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
