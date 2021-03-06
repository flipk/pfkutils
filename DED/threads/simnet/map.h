
#ifdef MAP1

#define NODES 6

static const int
topology_map[NODES][NODES] = {
	{  0,  7,  1, -1, -1, -1 },
	{  7,  0,  2,  1,  2, -1 },
	{  1,  2,  0,  4,  3, -1 },
	{ -1,  1,  4,  0,  5,  2 },
	{ -1,  2,  3,  5,  0,  2 },
	{ -1, -1, -1,  2,  2,  0 }
};

#endif /* MAP1 */

#ifdef MAP2

#define NODES 18

static const int
topology_map[NODES][NODES] = {
	/* 0   1   2   3   4   5   6   7   8   9  10  11  12  13  14  15  16, 17
/* 0 */	{  0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,  1, -1, -1 },
/* 1 */	{ -1,  0,  1,  1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
/* 2 */	{ -1,  1,  0,  4,  2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
/* 3 */ { -1,  1,  4,  0, -1, -1,  3,  1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
/* 4 */ { -1, -1,  2, -1,  0,  7,  3, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
/* 5 */ { -1, -1, -1, -1,  7,  0,  3, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
/* 6 */ { -1, -1, -1,  3,  3,  3,  0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
/* 7 */ { -1, -1, -1,  1, -1, -1, -1,  0,  2, -1, -1,  2, -1, -1, -1, -1, -1, -1 },
/* 8 */ { -1, -1, -1, -1, -1, -1, -1,  2,  0,  2, -1, -1,  5,  2,  4, -1, -1, -1 },
/* 9 */ { -1, -1, -1, -1, -1, -1, -1, -1,  2,  0,  2, -1, -1, -1, -1, -1, -1,  4 },
/*10 */ { -1, -1, -1, -1, -1, -1, -1, -1, -1,  2,  0,  2, -1, -1, -1, -1,  4, -1 },
/*11 */ { -1, -1, -1, -1, -1, -1, -1,  2, -1, -1,  2,  0, -1,  1, -1,  3, -1, -1 },
/*12 */ { -1, -1, -1, -1, -1, -1, -1, -1,  5, -1, -1, -1,  0, -1, -1,  7, -1, -1 },
/*13 */ { -1, -1, -1, -1, -1, -1,  3, -1,  2, -1, -1,  1, -1,  0, -1, -1, -1, -1 },
/*14 */ { -1, -1, -1, -1, -1, -1, -1, -1,  4, -1, -1, -1, -1, -1,  0, -1, -1,  1 },
/*15 */ {  1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,  3,  7, -1, -1,  0, -1, -1 },
/*16 */ { -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,  4, -1, -1, -1, -1, -1,  0,  1 },
/*17 */ { -1, -1, -1, -1, -1, -1, -1, -1, -1,  4, -1, -1, -1, -1,  1, -1,  1,  0 }
};

#endif /* MAP2 */

#ifdef MAP3

#define NODES 8

static const int
topology_map[NODES][NODES] = {

	/* A   B   C   D   E   F   G   H */
/* A */	{  0,  1,  2,  3,  3,  2,  1,  3 },
/* B */	{  1,  0,  2, -1, -1, -1, -1,  4 },
/* C */	{  2,  2,  0,  1, -1, -1, -1, -1 },
/* D */	{  3, -1,  1,  0,  3, -1, -1, -1 },
/* E */	{  3, -1, -1,  3,  0,  2, -1, -1 },
/* F */	{  2, -1, -1, -1,  2,  0,  4, -1 },
/* G */	{  1, -1, -1, -1, -1,  4,  0,  1 },
/* H */	{  3,  4, -1, -1, -1, -1,  1,  0 },

};

#endif /* MAP3 */

