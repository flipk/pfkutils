/*
 * This file is licensed under the GPL version 2.
 * Refer to the file LICENSE in this distribution or
 * just search for GPL v2 on the website www.gnu.org.
 */

void    pgm_readhdr    __P((struct qcam_softc *, FILE *));
void    pgm_read       __P((struct qcam_softc *, FILE *));
void    pgm_writehdr   __P((struct qcam_softc *, FILE *));
void	pgm_write      __P((struct qcam_softc *, FILE *));
