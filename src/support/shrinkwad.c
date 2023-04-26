#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "rawwad.h"
#include "rawwad.c"

#include "../info.h"

const char * const sprnames[NUMSPRITES+1] = {
    "TROO","SHTG","PUNG","PISG","PISF","SHTF","SHT2","CHGG","CHGF","MISG",
    "MISF","SAWG","PLSG","PLSF","BFGG","BFGF","BLUD","PUFF","BAL1","BAL2",
    "PLSS","PLSE","MISL","BFS1","BFE1","BFE2","TFOG","IFOG","PLAY","POSS",
    "SPOS","VILE","FIRE","FATB","FBXP","SKEL","MANF","FATT","CPOS","SARG",
    "HEAD","BAL7","BOSS","BOS2","SKUL","SPID","BSPI","APLS","APBX","CYBR",
    "PAIN","SSWV","KEEN","BBRN","BOSF","ARM1","ARM2","BAR1","BEXP","FCAN",
    "BON1","BON2","BKEY","RKEY","YKEY","BSKU","RSKU","YSKU","STIM","MEDI",
    "SOUL","PINV","PSTR","PINS","MEGA","SUIT","PMAP","PVIS","CLIP","AMMO",
    "ROCK","BROK","CELL","CELP","SHEL","SBOX","BPAK","BFUG","MGUN","CSAW",
    "LAUN","PLAS","SHOT","SGN2","COLU","SMT2","GOR1","POL2","POL5","POL4",
    "POL3","POL1","POL6","GOR2","GOR3","GOR4","GOR5","SMIT","COL1","COL2",
    "COL3","COL4","CAND","CBRA","COL6","TRE1","TRE2","ELEC","CEYE","FSKU",
    "COL5","TBLU","TGRN","TRED","SMBT","SMGT","SMRT","HDB1","HDB2","HDB3",
    "HDB4","HDB5","HDB6","POB1","POB2","BRS1","TLMP","TLP2", 0
};

char usespritemap[NUMSPRITES];

//
// Texture definition.
// A DOOM wall texture is a list of patches
// which are to be combined in a predefined order.
//

typedef struct
{
    short	originx;
    short	originy;
    short	patch;
    short	stepdir;
    short	colormap;
} mappatch_t;
typedef struct
{
    char		name[8];
    char		masked;	
    short		width;
    short		height;
    void		**columndirectory;	// OBSOLETE
    short		patchcount;
    mappatch_t	patches[1];
} maptexture_t;


void copy8( char * out, const char * in )
{
	int i;
	for( i = 0; i < 8; i++ )
	{
		char c = in[i];
		out[i] = c;
		if( !c ) break;
	}
}

int main( int argc, char ** argv )
{
	if( argc < 5 )
	{
		fprintf( stderr, "Error: Usage: ./shrinkwad [choice file] [texture file, or '0' to ignore] [.c file] [.h file]\n" );
		return -9;
	}
	char *line = NULL;
	size_t len = 0;
	ssize_t drd;
	int i;
	int chunkmap[numlumps];
	
	
#if 0
	memset( chunkmap, 0, numlumps * sizeof( int ) );
	FILE * flumpaccess = fopen( "../lumpaccess.txt", "r" );

	while ((drd = getline(&line, &len, flumpaccess)) != -1)
	{
		char header[1024];
		int source, chunkno;
		sscanf( line, "%1023s %d %d\n", header, &source, &chunkno );
		if( chunkno >= numlumps || chunkno < 0 )
		{
			fprintf( stderr, "Chunkno out of range. (part 1, %d)\n", chunkno );
			return -9;
		}
		if( chunkno > highestchunk ) highestchunk = chunkno;
		chunkmap[chunkno] = 1;
	}
	fclose( flumpaccess );
#else
	for( i = 0; i < numlumps; i++ )
		chunkmap[i] = 1;
#endif


	int usingspritemap = 0;

	if( argv[2][0] != '0' )
	{
		memset( usespritemap, 0, sizeof(  usespritemap ) );
		FILE * fAccessSprites = fopen( argv[2], "r" );
		if( ! fAccessSprites )
		{
			fprintf( stderr, "Error: can't open %s\n", argv[2] );
			return -9;
		}
		while( !feof( fAccessSprites ) && !ferror( fAccessSprites ) )
		{
			char cstart[128];
			int dummy1;
			int spriteno;
			char csend[16];
			int ct = fscanf( fAccessSprites, "%127s %d %d %15s\n", cstart, &dummy1, &spriteno, csend );
			if( ct != 4 )
			{
				fprintf( stderr, "Error: invalid sprite on line %d\n", ct );
				return -113;
			}
			if( spriteno >= NUMSPRITES )
			{
				fprintf( stderr, "Error: Invalid sprite (too big) %d\n", spriteno );
				return -112;
			}
			if( dummy1 != 1 )
			{
				printf( "%s marked for use\n", sprnames[spriteno] );
				usespritemap[spriteno] = 1;
			}
		}
		fclose( fAccessSprites );
		usingspritemap = 1;
	}
	else
	{
		memset( usespritemap, 1, sizeof(usespritemap) );
	}


	
	int j;
	for( j = 0; j < NUMSPRITES; j++ )
	{
		printf( "%s: %d\n", sprnames[j], usespritemap[j] );
		if( usespritemap[j] == 0 )
		{
			printf( "Stripping %s\n", sprnames[j] );

			for( i = 0 ; i < numlumps; i++ )
			{
				if( strncmp( lumpinfo[i].name, sprnames[j], 4 ) == 0 )
				{
					chunkmap[i] = -1;
					char ct9[9] = { 0 };
					memcpy( ct9, lumpinfo[i].name, 8 );
					printf( "  %s\n", ct9 );
				}
			}
		}
	}

	// XXX TRICKY: Find "TEXTURE1" as that actually contains our texture list.
	int * texture1data; 
	int * texdirectory;
	int numtex = 0;
	int texture1datasize;
	for( i = 0 ; i < numlumps; i++ )
	{
		if( strncmp( lumpinfo[i].name, "TEXTURE1", 8 ) == 0 )
		{
			int offset = lumpinfo[i].position;
			texture1datasize = lumpinfo[i].position;
			texture1data = malloc( texture1datasize+1 );
			memcpy( texture1data, &rawwad[offset], texture1datasize );
			((unsigned char*)texture1data)[texture1datasize] = 0;
		}
	}
	if( !texture1data )
	{
		fprintf( stderr, "ERROR: Need 'TEXTURE1' lump in wad.\n" );
		return -5;
	}
	{
		maptexture_t * mtexture;
		numtex = *texture1data;
		printf( "Num Textures: %d\n", numtex );
		texdirectory = texture1data+1;
		int * directory = texdirectory;
		
		for( i = 0; i < numtex; i++, directory++ )
		{
			int offset = (int)(*directory);
			mtexture = (maptexture_t *) ( (unsigned char *)texture1data + offset);
			char sname[9] = { 0 };
			memcpy( sname, mtexture->name, 8 );
			printf( "%s(%d) ", sname, mtexture->patchcount * sizeof(mappatch_t) );
		}
	}
	
	FILE * fneverstrip = fopen( argv[1], "r" );
	printf( "Open %s status: %p\n", argv[1], fneverstrip );
	while ((drd = getline(&line, &len, fneverstrip)) != -1)
	{
		char header[1024];
		if( sscanf( line, "%1023s\n", header ) != 1 ) continue;
		if( strlen( header ) < 1 ) continue;
		int chunkno = -1;
		int mtocheck = 8;
		char * star = strchr( header+1, '*' );
		if( star )
		{
			mtocheck = star - (header+1);
			printf( "Wildcard to %d chars\n", mtocheck );
		}

		for( i = 0 ; i < numlumps; i++ )
		{
			if( strncmp( lumpinfo[i].name, header+1, mtocheck ) == 0 )
			{
				chunkno = i;

				if( header[0] == '+' )
					chunkmap[chunkno] = 1;
				else if( header[0] == '-' )
					chunkmap[chunkno] = 0;
				else if( header [0] == '0' )
					chunkmap[chunkno] = -1;
				else
					fprintf( stderr, "UNKNOWN STRIPCHOICE %s\n", header );


				if( chunkmap[chunkno] <= 0 && strncmp( header+1, "E1M", 3 ) == 0 )
				{
					//Apply selection to everything in this map.
					int k;
					printf( "Section applying for %s (%d)\n", header+1, chunkmap[chunkno] );
					for( k = 1; k <= 10; k++ )
					{
						chunkmap[chunkno+k] = chunkmap[chunkno];
					}
				}

			}
		}

		if( chunkno >= numlumps || chunkno < 0 )
		{
			fprintf( stderr, "WARNING: Chunkno out of range #2. (%d) (%s)\n", chunkno, header+1 );
		}
	}

/*
	if( usespritemap )
	{
		// If using the sprite map, then need to zero out
		//0VERTEXES
		//0NODES
		int mtocheck = 8;
		int chunkno = -1;
		for( i = 0 ; i < numlumps; i++ )
		{
			if( strncmp( lumpinfo[i].name, "VERTEXES", mtocheck ) == 0 )
			{
				chunkno = i;
				chunkmap[chunkno] = 0;
			}
			if( strncmp( lumpinfo[i].name, "NODES", mtocheck ) == 0 )
			{
				chunkno = i;
				chunkmap[chunkno] = 0;
			}
		}
	}
*/
	printf( "Loaded list.\n" );

	int couldsave = 0;
	int newtotal = 0;
	int numnewchunks = 0;

	for( i = 0; i < numlumps; i++ )
	{
		if( chunkmap[i] == 0 )
		{
			//printf( "%d %s\n", chunkmap[i], lumpinfo[i].name );
			couldsave += lumpinfo[i].size;
		}
		else if( chunkmap[i] == -1 )
		{
			numnewchunks++;
		}
		else
		{
			newtotal += lumpinfo[i].size;
			numnewchunks++;
		}
	}

	FILE * f_c = fopen( argv[3], "w" );
	FILE * f_h = fopen( argv[4], "w" );

	fprintf( f_h, "#ifndef _RAWWAD_H\n"
	"#define _RAWWAD_H\n"
	"extern const int numlumps;\n"
	"extern const unsigned char rawwad[%d];\n"
	"#endif\n", newtotal );
	fclose( f_h );

	int tlump = 0;
	lumpinfo_t        newlumpinfo[numnewchunks+1];
	unsigned char  * newchunkdata = malloc( newtotal );
	int marker = 0;

	printf( "Stripping: " );
	for( i = 0; i < numlumps; i++ )
	{
		if( chunkmap[i] == -1 )
		{
			copy8( newlumpinfo[tlump].name, lumpinfo[i].name );
			newlumpinfo[tlump].size = 0;
			newlumpinfo[tlump].position = marker;
			tlump++;
   		}
		else if( chunkmap[i] == 0 )
		{
			char stp[9] = { 0 };
			copy8( stp, lumpinfo[i].name );
			printf( "%s(%d) ", stp, lumpinfo[i].size );
		}
		else if( chunkmap[i] == 1 )
		{
			copy8( newlumpinfo[tlump].name, lumpinfo[i].name );
			newlumpinfo[tlump].size = lumpinfo[i].size;
			newlumpinfo[tlump].position = marker;
			tlump++;
			memcpy( newchunkdata + marker, &rawwad[lumpinfo[i].position], lumpinfo[i].size );
			marker += lumpinfo[i].size;
   		}
	}

	printf( "\n" );
	printf( "Including: " );
	for( i = 0; i < numlumps; i++ )
	{
		if( chunkmap[i] != 0 )
		{
			char stp[9] = { 0 };
			copy8( stp, lumpinfo[i].name );
			printf( "%s(%d) ", stp, (chunkmap[i]<0)?0:lumpinfo[i].size );
		}
	}

	printf( "\n" );
	printf( "Did save %d\n", couldsave );
	printf( "New Total: %d\n", newtotal );

	printf( "Comparing: %d/%d/%d\n", tlump, numnewchunks, numlumps );

	fprintf( f_c, "#include \"../w_wad.h\"\n"
"const int               numlumps = %d;\n", numnewchunks );
	fprintf( f_c, "const unsigned char rawwad[%d] = {", newtotal );
	for( i = 0; i < newtotal; i++ )
	{
		if( i & 0x1f )
			fprintf( f_c, "0x%02x, ",newchunkdata[i] ); 
		else
			fprintf( f_c, "0x%02x,\n\t",newchunkdata[i] ); 
	}
	fprintf( f_c, "};\n\nconst lumpinfo_t        lumpinfo[%d] = {\n", numnewchunks );
	for( i = 0; i < numnewchunks; i++ )
	{
		char tsr[9];
		copy8( tsr, newlumpinfo[i].name );
		tsr[8] = 0;
		printf( "LUMP %d = %s %d %d\n", i, tsr, newlumpinfo[i].size?newlumpinfo[i].position:0, newlumpinfo[i].size );
		fprintf( f_c, "\t{ \"%s\", %d, %d },\n", tsr, newlumpinfo[i].size?newlumpinfo[i].position:0, newlumpinfo[i].size );
	}
	fprintf( f_c, "};\n" );
	fclose( f_c );
}


