/* RCSid $Id$ */
/*
 * xraster.h - header file for X routines using images.
 */

typedef struct {
	int	width, height;			/* image size */
	int	ncolors;			/* number of colors */
	union {
		unsigned short	*m;			/* monochrome */
		unsigned char	*bz;			/* color */
	}	data;				/* storage on our side */
	Pixmap	pm;				/* storage on server side */
	Color	*cdefs;				/* color definitions */
	int	*pmap;				/* inverse pixel mapping */
	int	*pixels;			/* allocated table entries */
}	XRASTER;

extern int	*map_rcolors();

extern Pixmap	make_rpixmap();
