/* Copyright (c) 1997 Silicon Graphics, Inc. */

/* SCCSid "$SunId$ SGI" */

/*
 * Header file for holodeck programs
 *
 *	9/29/97	GWLarson
 */

#include "standard.h"
#include "color.h"

#ifndef HDMAX
#define HDMAX		128	/* maximum active holodeck sections */
#endif
#ifndef int2
#define int2	short
#endif
#ifndef int4
#define int4	long
#endif

#define DCINF	(unsigned)((1L<<16)-1)	/* special value for infinity */
#define DCLIN	(unsigned)(1L<<11)	/* linear depth limit */

typedef struct {
	BYTE 	r[2][2];	/* ray direction index */
	COLR 	v;		/* value */
	unsigned int2	d;	/* depth code */
} RAYVAL;		/* ray value (from second wall) */

/*
 * Walls are ordered:		X0	X1	X2	WN
 *				0	?	?	0
 *				1	?	?	1
 *				?	0	?	2
 *				?	1	?	3
 *				?	?	0	4
 *				?	?	1	5
 *
 * Grid on wall WN corresponds to X(WN/2+1)%3 and X(WN/2+2)%3, resp.
 */

typedef struct {
	short	w;		/* wall number */
	short	i[2];		/* index on wall grid */
} GCOORD;		/* grid coordinates (two for beam) */

typedef struct {
	unsigned int4	nrd;	/* number of beam rays bundled on disk */
	long	fo;		/* position in file */
} BEAMI;		/* beam index */

typedef struct {
	unsigned int4	nrm;	/* number of beam rays bundled in memory */
	unsigned long	tick;	/* clock tick for LRU replacement */
} BEAM;			/* followed by nrm RAYVAL's */

#define hdbray(bp)	((RAYVAL *)((bp)+1))
#define hdbsiz(nr)	(sizeof(BEAM)+(nr)*sizeof(RAYVAL))

typedef struct {
	FVECT	orig;		/* prism origin (first) */
	FVECT	xv[3];		/* side vectors (second) */
	int2	grid[3];	/* grid resolution (third) */
} HDGRID;		/* holodeck section grid (must match HOLO struct) */

typedef struct holo {
	FVECT	orig;		/* prism origin (first) */
	FVECT	xv[3];		/* side vectors (second) */
	int2	grid[3];	/* grid resolution (third) */
	int	fd;		/* file descriptor */
	short	dirty;		/* beam index needs update to file */
	double	tlin;		/* linear range for depth encoding */
	FVECT	wn[3];		/* wall normals (derived) */
	double	wg[3];		/* wall grid multipliers (derived) */
	double	wo[6];		/* wall offsets (derived) */
	int	wi[6];		/* wall super-indices (derived) */
	char	*priv;		/* pointer to private client data */
	BEAM	**bl;		/* beam pointers (memory cache) */
	BEAMI	bi[1];		/* beam index (extends struct) */
} HOLO;			/* holodeck section */

#define nbeams(hp)	(2*((hp)->wi[5]-1))
#define biglob(hp)	((hp)->bi)
#define blglob(hp)	(*(hp)->bl)

#define bnrays(hp,i)	((hp)->bl[i]!=NULL ? (hp)->bl[i]->nrm : (hp)->bi[i].nrd)

#define hdflush(hp)	(hdfreebeam(hp,0) && hdsync(hp))

extern HOLO	*hdinit(), *hdalloc();
extern BEAM	*hdgetbeam();
extern RAYVAL	*hdnewrays();
extern long	hdmemuse(), hdfiluse();
extern double	hdray(), hdinter();
extern unsigned	hdcode();

extern int	hdcachesize;		/* target cache size (bytes) */
extern unsigned long	hdclock;	/* holodeck system clock */
extern HOLO	*hdlist[HDMAX+1];	/* holodeck pointers (NULL term.) */

extern float	hd_depthmap[];		/* depth conversion map */

#define hddepth(hp,dc)	( (dc) >= DCINF ? FHUGE : \
				(hp)->tlin * ( (dc) >= DCLIN ? \
					hd_depthmap[(dc)-DCLIN] : \
					((dc)+.5)/DCLIN ) )

#define HOLOFMT		"Holodeck"	/* file format identifier */
#define HOLOVERS	0		/* file format version number */
#define HOLOMAGIC	(327+HOLOVERS)	/* file magic number */

/*
 * A holodeck file consists of an information header terminated by a
 * blank line, with "FORMAT=Holodeck" somewhere in it.
 * The first integer after the information header is the
 * above magic number, which includes the file format version number.
 * The first longword after the magic number is the position
 * of the SECOND holodeck section, or 0 if there is only one.
 * This longword is immediately followed by the first holodeck
 * section header and directory.
 * Similarly, every holodeck section in the file is preceeded by
 * a pointer to the following section, or 0 for the final section.
 * Since holodeck files consist of directly written C data structures, 
 * they are not generally portable between different machine architectures.
 * In particular, different floating point formats or bit/byte ordering
 * will make the data unusable.  This is unfortunate, and may be changed
 * in future versions, but we thought this would be best for paging speed
 * in our initial implementation.
 */