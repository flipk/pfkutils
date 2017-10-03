
struct motion_location {
	int detected;
	struct {
		int x, y;
	} top, bottom;
};

int	qcam_detect_motion	__P((struct qcam_softc *, int, int));
int	qcam_detect_motion2	__P((struct qcam_softc *, 
				     struct motion_location *, 
				     int, int));
