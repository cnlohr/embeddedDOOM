#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "rawwad.h"
#include "rawwad.c"


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
	if( argc < 4 )
	{
		fprintf( stderr, "Error: Usage: ./shrinkwad [choice file] [.c file] [.h file]\n" );
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
					printf( "Section applying for %s\n", header+1 );
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
	printf( "Did save %d\n", couldsave );
	printf( "New Total: %d\n", newtotal );

	FILE * f_c = fopen( argv[2], "w" );
	FILE * f_h = fopen( argv[3], "w" );

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
			printf( "%s ", stp );
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
			printf( "%s ", stp );
		}
	}

	printf( "\n" );

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
		fprintf( f_c, "\t{ \"%s\", %d, %d },\n", tsr, newlumpinfo[i].size?newlumpinfo[i].position:0, newlumpinfo[i].size );
	}
	fprintf( f_c, "};\n" );
	fclose( f_c );
}


