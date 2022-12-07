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
// $Log:$
//
// DESCRIPTION:
//		Handles WAD file header, directory, lump I/O.
//
//-----------------------------------------------------------------------------

//static const char
//rcsid[] = "$Id: w_wad.c,v 1.5 1997/02/03 16:47:57 b1 Exp $";

//Added elsewhere

#ifdef GENERATE_BAKED
#include "support/rawwad.h"
#else
#include "support/rawwad_use.h"
#endif

#ifdef NORMALUNIX
#include <ctype.h>
#include <sys/types.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <alloca.h>
#define O_BINARY				0
#endif

#include "doomtype.h"
#include "m_swap.h"
#include "i_system.h"
#include "z_zone.h"

#ifdef __GNUG__
#pragma implementation "w_wad.h"
#endif
#include "w_wad.h"



int filelength (int handle) 
{ 
		return sizeof( rawwad );
}



//
// GLOBALS
//

// Location of each lump on disk.
//lumpinfo_t*				lumpinfo;				
//int						numlumps;
//void**						lumpcache;


#define strcmpi		strcasecmp

void strupr (char* s)
{
	while (*s) { *s = toupper(*s); s++; }
}


void
ExtractFileBase
( char*				path,
  char*				dest )
{
	char*		src;
	int				length;

	src = path + strlen(path) - 1;
	
	// back up until a \ or the start
	while (src != path
		   && *(src-1) != '\\'
		   && *(src-1) != '/')
	{
		src--;
	}
	
	// copy up to eight characters
	memset (dest,0,8);
	length = 0;
	
	while (*src && *src != '.')
	{
		if (++length == 9)
			I_Error ("Filename base of %s >8 chars",path);

		*dest++ = toupper((int)*src++);
	}
}





//
// LUMP BASED ROUTINES.
//

//
// W_AddFile
// All files are optional, but at least one file must be
//  found (PWAD, if all required lumps are present).
// Files with a .wad extension are wadlink files
//  with multiple lumps.
// Other files are single lumps with the base filename
//  for the lump name.
//
// If filename starts with a tilde, the file is handled
//  specially to allow map reloads.
// But: the reload feature is a fragile hack...

int						reloadlump;
char*						reloadname;


void W_AddFile (char *filename)
{
		return;

}




//
// W_Reload
// Flushes any of the reloadable lumps in memory
//  and reloads the directory.
//
void W_Reload (void)
{
		//STUB.
}

//
// W_InitMultipleFiles
// Pass a null terminated list of files to use.
// All files are optional, but at least one file
//  must be found.
// Files with a .wad extension are idlink files
//  with multiple lumps.
// Other files are single lumps with the base filename
//  for the lump name.
// Lump names can appear multiple times.
// The name searcher looks backwards, so a later file
//  does override all earlier ones.
//
void W_InitMultipleFiles (char** filenames)
{		
		return;
}




//
// W_InitFile
// Just initialize from a single file.
//
void W_InitFile (char* filename)
{
	char*		names[2];

	names[0] = filename;
	names[1] = NULL;
	W_InitMultipleFiles (names);
}



//
// W_NumLumps
//
int W_NumLumps (void)
{
	return numlumps;
}



//
// W_CheckNumForName
// Returns -1 if name not found.
//

int W_CheckNumForName (char* name)
{
	union {
		char	s[9];
		int		x[2];
	} name8;
	
	int				v1;
	int				v2;
	lumpinfo_t*		lump_p;

	// make the name into two integers for easy compares
	strncpy (name8.s,name,8);

	// in case the name was a fill 8 chars
	name8.s[8] = 0;

	// case insensitive
	strupr (name8.s);				

	v1 = name8.x[0];
	v2 = name8.x[1];


	// scan backwards so patch lump files take precedence
	lump_p = lumpinfo + numlumps;

	while (lump_p-- != lumpinfo)
	{
		if ( *(int *)lump_p->name == v1
			 && *(int *)&lump_p->name[4] == v2)
		{
#ifdef GENERATE_BAKED
			printf( "\nACCESS_LUMP 0 %d\n", lump_p - lumpinfo );
#endif
			return lump_p - lumpinfo;
		}
	}

	printf( "WARNING: FILE NOT FOUND: %s\n", name );

	// TFB. Not found.
	return -1;
}




//
// W_GetNumForName
// Calls W_CheckNumForName, but bombs out if not found.
//
int W_GetNumForName (char* name)
{
	int		i;

	i = W_CheckNumForName (name);

	if (i == -1)
	  I_Error ("W_GetNumForName: %s not found!", name);
  
	return i;
}


//
// W_LumpLength
// Returns the buffer size needed to load the given lump.
//
int W_LumpLength (int lump)
{
#ifdef GENERATE_BAKED
	printf( "\nACCESS_LUMP 1 %d\n", lump );
#endif
	if (lump >= numlumps)
		I_Error ("W_LumpLength: %i >= numlumps",lump);

	return lumpinfo[lump].size;
}



//
// W_ReadLump
// Loads the lump into the given buffer,
//  which must be >= W_LumpLength().
//
void W_ReadLump( int lump, void* dest )
{
	int				c;
	lumpinfo_t*		l;
	int				handle;

#ifdef GENERATE_BAKED
	printf( "\nACCESS_LUMP 2 %d\n", lump );
#endif
	if (lump >= numlumps)
		I_Error ("W_ReadLump: %i >= numlumps",lump);

	l = lumpinfo+lump;
	memcpy( dest, &rawwad[l->position], l->size  );
}




//
// W_CacheLumpNum
//
const void*
W_CacheLumpNum
( int				lump,
  int				tag )
{
	byte*		ptr;

#ifdef GENERATE_BAKED
	printf( "\nACCESS_LUMP 3 %d\n", lump );
#endif
	if ((unsigned)lump >= numlumps)
				I_Error ("W_CacheLumpNum: %i >= numlumps",lump);

	return &rawwad[lumpinfo[lump].position];

/*	if (!lumpcache[lump])
	{
		// read the lump in
		
		//printf ("cache miss on lump %i\n",lump);
		ptr = Z_Malloc (W_LumpLength (lump), tag, &lumpcache[lump]);
				W_ReadLump (lump, lumpcache[lump]);
	}
	else
	{
				//printf ("cache hit on lump %i\n",lump);
				Z_ChangeTag (lumpcache[lump],tag);
	}
		
	return lumpcache[lump];*/
}


void*
W_CacheLumpNum_Old
( int				lump,
  int				tag )
{
	byte*		ptr;

#ifdef GENERATE_BAKED
	printf( "\nACCESS_LUMP 4 %d\n", lump );
#endif

	if ((unsigned)lump >= numlumps)
		I_Error ("W_CacheLumpNum: %i >= numlumps",lump);

	ptr = Z_Malloc (W_LumpLength (lump), tag, 0 );

	W_ReadLump (lump, ptr);

	return ptr;
}


//
// W_CacheLumpName
//
void*
W_CacheLumpName
( char*				name,
  int				tag )
{
	return W_CacheLumpNum (W_GetNumForName(name), tag);
}



