// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id:$
//
// Copyright (C) 1993-1996 by id Software, Inc.
//
// This source is available for distribution and/or modification
// only under the terms of the DOOM Source Code License as
// published by id Software. All rights reserved.
//
// The source is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// FITNESS FOR A PARTICULAR PURPOSE. See the DOOM Source Code License
// for more details.
//
// DESCRIPTION:
//	Refresh/render internal state variables (global).
//
//-----------------------------------------------------------------------------


#ifndef __R_STATE__
#define __R_STATE__

// Need data structure definitions.
#include "d_player.h"
#include "r_data.h"



#ifdef __GNUG__
#pragma interface
#endif



//
// Refresh internal data structures,
//  for rendering.
//

// needed for texture pegging
#ifdef GENERATE_BAKED
extern fixed_t*		textureheight;
#else
extern const fixed_t		textureheight[];
#endif

// needed for pre rendering (fracs)
#ifdef GENERATE_BAKED
extern fixed_t*		spritewidth;
extern fixed_t*		spriteoffset;
extern fixed_t*		spritetopoffset;
#else
extern const fixed_t	spritewidth[];	
extern const fixed_t	spriteoffset[];
extern const fixed_t	spritetopoffset[];
#endif

#ifdef GENERATE_BAKED
extern lighttable_t*	colormaps;
extern int colormapsize;
#else
extern const lighttable_t	colormaps[];
#endif
extern int		viewwidth;
extern int		scaledviewwidth;
extern int		viewheight;

extern int		firstflat;

// for global animation
extern int*		flattranslation;	
extern int*		texturetranslation;	


// Sprite....
#ifdef GENERATE_BAKED
extern int		firstspritelump;
extern int		lastspritelump;
#else
extern const int		firstspritelump;
extern const int		lastspritelump;
#endif


//
// Lookup tables for map data.
//

extern int		numsprites;

extern spritedef_t*	sprites;
extern side_t*		sides;
extern sector_t*	sectors;
extern subsector_t*	subsectors;
extern seg_t*		segs;
extern line_t*		lines;

extern int		numsides;
extern int		numlines;
extern int		numnodes;
extern int		numsubsectors;
extern int		numsectors;
extern int		numsegs;
extern int		numvertexes;

#ifdef GENERATE_BAKED
extern vertex_t*	vertexes;
extern node_t*		nodes;
#else
extern const vertex_t*	vertexes;
extern const node_t*		nodes;
#endif


//
// POV data.
//
extern fixed_t		viewx;
extern fixed_t		viewy;
extern fixed_t		viewz;

extern angle_t		viewangle;
extern player_t*	viewplayer;


// ?
extern angle_t		clipangle;

#ifdef GENERATE_BAKED
extern int		viewangletox[FINEANGLES/2];
extern angle_t		xtoviewangle[SCREENWIDTH+1];
#else
extern const int		viewangletox[FINEANGLES/2];
extern const angle_t		xtoviewangle[SCREENWIDTH+1];
#endif
//extern fixed_t		finetangent[FINEANGLES/2];

extern fixed_t		rw_distance;
extern angle_t		rw_normalangle;



// angle to line origin
extern int		rw_angle1;

// Segs count?
extern int		sscount;

extern visplane_t*	floorplane;
extern visplane_t*	ceilingplane;


#endif
//-----------------------------------------------------------------------------
//
// $Log:$
//
//-----------------------------------------------------------------------------
