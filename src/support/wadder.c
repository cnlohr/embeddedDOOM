#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <string.h>

#define LONG(x) x

typedef struct
{
    // Should be "IWAD" or "PWAD".
    char		identification[4];		
    int			numlumps;
    int			infotableofs;
    
} wadinfo_t;

typedef struct
{
    int			filepos;
    int			size;
    char		name[8];
    
} filelump_t;

//
// WADFILE I/O related stuff.
//
typedef struct
{
    char	name[8];
    int		handle;
    int		position;
    int		size;
} lumpinfo_t;


lumpinfo_t*	lumpinfo;
int		numlumps;


int main()
{
	FILE * f = fopen( "doom1.wad", "rb" );
	fseek( f, 0, SEEK_END );
	int len = ftell(f);
	if( len <= 0 )
	{
		fprintf( stderr, "Error: Bad WAD.\n" );
	}
	fseek( f, 0, SEEK_SET );
	unsigned char * rawwad = malloc(len);
	fread( rawwad, 1, len, f );
	fclose( f);

	
    // open all the files, load headers, and count lumps
    numlumps = 0;

    // will be realloced as lumps are added
    lumpinfo = malloc(1);	

    wadinfo_t		header;
    lumpinfo_t*		lump_p;
    unsigned		i;
    int			length;
    int			startlump;
    filelump_t*		fileinfo;
    filelump_t		singleinfo;
    int handle = 1;
    // open the file and add to directory

	int filepos = 0;
	startlump = numlumps;
	
	// WAD file
	//read_W (handle, &header, sizeof(header));
	memcpy( &header, &rawwad[filepos], sizeof(header)); filepos +=sizeof(header);
	if (strncmp(header.identification,"IWAD",4))
	{
	    // Homebrew levels?
	    if (strncmp(header.identification,"PWAD",4))
	    {
		fprintf ( stderr, "Wad file  doesn't have IWAD "
			 "or PWAD id\n");
			exit(-1);
	    }
	    
	    // ???modifiedgame = true;		
	}
	header.numlumps = LONG(header.numlumps);
	header.infotableofs = LONG(header.infotableofs);
	printf( "Table: %d %d\n", header.numlumps, header.infotableofs );
	length = header.numlumps*sizeof(filelump_t);
	fileinfo = alloca (length);
	filepos = header.infotableofs;
	//read_W (handle, fileinfo, length);
	memcpy( fileinfo, &rawwad[filepos], length ); filepos += length;
	numlumps += header.numlumps;
    
    // Fill in lumpinfo
    lumpinfo = realloc (lumpinfo, numlumps*sizeof(lumpinfo_t));

    if (!lumpinfo)
	{
		fprintf( stderr, "Couldn't realloc lumpinfo");
		return -1;
	}

    lump_p = &lumpinfo[startlump];
	
    int storehandle = 1; //XXX this looks wrong?
	
    for (i=startlump ; i<numlumps ; i++,lump_p++, fileinfo++)
    {
		lump_p->handle = storehandle;
		lump_p->position = LONG(fileinfo->filepos);
		lump_p->size = LONG(fileinfo->size);
		strncpy (lump_p->name, fileinfo->name, 8);
    }





	FILE * fo = fopen( "rawwad.c_resource", "w" );
	fprintf( fo, "#include \"../w_wad.h\"\n" );
	fprintf( fo, "const int		numlumps = %d;\n", numlumps );
	fprintf( fo, "const unsigned char rawwad[%d] = {", len );
	int byte = 0;
	int dat;
	while( byte < len )
	{
		dat = rawwad[byte];
		if( (byte & 0x1f) == 0 ) fprintf( fo, "\n\t" );
		fprintf( fo, "0x%02x, ", dat );
		byte++;
	}
	fprintf( fo, "};\n\n" );
	fprintf( fo, "const lumpinfo_t	lumpinfo[%d] = {\n", numlumps );
	for( i = 0; i < numlumps; i++ )
	{
		char st[100];
		memcpy( st, lumpinfo[i].name, 8 );
		st[8] = 0;
		fprintf( fo, "	{ \"%s\", %d, %d, },\n", st, lumpinfo[i].position, lumpinfo[i].size );
	}
	fprintf( fo, "};\n" );



	fclose( fo );

	fo = fopen("rawwad.h", "w" );
	fprintf( fo, "#ifndef _RAWWAD_H\n#define _RAWWAD_H\n" );
	fprintf( fo, "extern const unsigned char rawwad[%d];\n#endif\n", byte );
}

