/*
 * This file is licensed under the GPL version 2.
 * Refer to the file LICENSE in this distribution or
 * just search for GPL v2 on the website www.gnu.org.
 */

#include <sys/param.h>
#include <sys/types.h>
#include <sys/time.h>
#include <machine/bus.h>
#include <machine/sysarch.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <err.h>

#include "qcam.h"
#include "pgm.h"
#include "motion.h"
#include "ttydisp.h"
#include "xwindisp.h"
