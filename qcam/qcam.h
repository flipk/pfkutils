extern int qcam_debug;

struct qcam_softc {
    u_char          *buffer;             /* frame buffer */
    u_char          *oldbuffer;          /* the previous scan buffer */
    u_int           bufsize;             /* x_size times y_size */
    u_int           flags;
    u_int           iobase;
    int             unit;                /* device */
    void            (*scanner)(struct qcam_softc *);
    int             init_req;            /* initialization required */
    int             x_size;              /* pixels */
    int             y_size;              /* pixels */
    int             x_origin;            /* ?? units */
    int             y_origin;            /* ?? units */
    int             zoom;                /* 0=none, 1=1.5x, 2=2x */
    int             bpp;                 /* 4 or 6 */
    u_char          xferparms;           /* calcualted transfer params */
    u_char          contrast;
    u_char          brightness;
    u_char          whitebalance;
};

typedef struct qcam_softc * QSC;

QSC   qcam_open           __P((u_int));
QSC   qcam_new            __P((void));
void  qcam_scan           __P((QSC));
int   qcam_adjustpic      __P((QSC));
void  qcam_close          __P((QSC));
void  qcam_setbrcn        __P((QSC, int));
void  qcam_setsize        __P((QSC, int));
int   qcam_detect_motion  __P((QSC, int, int));

#define QC_S80x60   1
#define QC_S160x120 2
#define QC_S320x240 3

