#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>    // for sqrt (for flat scaling later)

#include "rawwad.h"
#include "rawwad.c"

#include "../info.h"
#include "../scale.h"

// ---------------------------------------------------------------------
// HUD/UI patch rescaling.
#define HUD_RESCALE UI_SCALE

// ---------------------------------------------------------------------
// Separate rescale factors for different asset types
#define PATCH_RESCALE  (1.0f / TEXTURE_SCALE)
#define SPRITE_RESCALE (1.0f / SPRITE_SCALE)
#define FLAT_RESCALE   (1.0f / FLAT_SCALE)    

// ---------------------------------------------------------------------

typedef struct {
    int width;
    int height;
    unsigned char *data;
} picture_t;

static const char * const hud_rescale_prefixes[] = {
	"STF", "STT", "STY", "STK", "STD", "STC", "STB",
	"M_", "WI", "TITLEPIC", "CREDIT", "HELP2", "PFUB", "AMMNUM", "BRDR_",
	NULL
};

static int should_rescale_lump( const char * name )
{
	int i;
	char nm[9] = { 0 };
	memcpy( nm, name, 8 );
	for( i = 0; hud_rescale_prefixes[i]; i++ )
	{
		int plen = strlen( hud_rescale_prefixes[i] );
		if( strncmp( nm, hud_rescale_prefixes[i], plen ) == 0 )
			return 1;
	}
	return 0;
}

// ----- global for flat range and patch name set -----
static int flat_start_lump = -1;
static int flat_end_lump   = -1;

static char **patch_names = NULL;
static int num_patch_names = 0;
// -----

typedef unsigned char rbyte;

// Decode a raw DOOM patch (column-post format) into a raster + opacity mask.
static int decode_patch(const rbyte *raw, int rawlen,
                          int *out_w, int *out_h, int *out_left, int *out_top,
                          rbyte **out_raster, rbyte **out_mask)
{
	if (rawlen < 8) return -1;
	short width      = raw[0] | (raw[1]<<8);
	short height     = raw[2] | (raw[3]<<8);
	short leftoffset = raw[4] | (raw[5]<<8);
	short topoffset  = raw[6] | (raw[7]<<8);

	if (width <= 0 || height <= 0 || width > 4096 || height > 4096) return -2;

	const int *columnofs = (const int *)(raw + 8);

	rbyte *raster = calloc(width*height, 1);
	rbyte *mask   = calloc(width*height, 1);

	int x;
	for (x = 0; x < width; x++)
	{
		int ofs = columnofs[x];
		if (ofs < 0 || ofs >= rawlen) { free(raster); free(mask); return -3; }
		const rbyte *column = raw + ofs;
		while (column[0] != 0xff)
		{
			int topdelta = column[0];
			int length   = column[1];
			const rbyte *data = column + 3;
			int y;
			for (y = 0; y < length; y++)
			{
				int py = topdelta + y;
				if (py >= 0 && py < height)
				{
					raster[py*width + x] = data[y];
					mask[py*width + x]   = 1;
				}
			}
			column += length + 4;
			if (column - raw >= rawlen) break;
		}
	}

	*out_w = width; *out_h = height;
	*out_left = leftoffset; *out_top = topoffset;
	*out_raster = raster; *out_mask = mask;
	return 0;
}

// Nearest-neighbor rescale of raster+mask from (sw,sh) to (dw,dh).
static void rescale_raster(const rbyte *sraster, const rbyte *smask, int sw, int sh,
                            rbyte *draster, rbyte *dmask, int dw, int dh)
{
	int x, y;
	for (y = 0; y < dh; y++)
	{
		int sy = (int)((double)y * sh / dh);
		if (sy >= sh) sy = sh-1;
		for (x = 0; x < dw; x++)
		{
			int sx = (int)((double)x * sw / dw);
			if (sx >= sw) sx = sw-1;
			draster[y*dw+x] = sraster[sy*sw+sx];
			dmask[y*dw+x]   = smask[sy*sw+sx];
		}
	}
}

// Encode raster+mask back into DOOM patch column-post format.
static int encode_patch(const rbyte *raster, const rbyte *mask, int w, int h,
                          int leftoffset, int topoffset,
                          rbyte **out_buf, int *out_len)
{
	int worst = 8 + 4*w + w*(h*5 + 1) + 16;
	rbyte *buf = malloc(worst);
	int *columnofs = malloc(sizeof(int)*w);

	int pos = 8 + 4*w;
	int x;
	for (x = 0; x < w; x++)
	{
		columnofs[x] = pos;
		int y = 0;
		while (y < h)
		{
			if (!mask[y*w+x]) { y++; continue; }
			int start = y;
			int len = 0;
			while (y < h && mask[y*w+x] && len < 254) { y++; len++; }
			buf[pos++] = (rbyte)start;
			buf[pos++] = (rbyte)len;
			buf[pos++] = raster[start*w+x];
			int k;
			for (k = 0; k < len; k++)
				buf[pos++] = raster[(start+k)*w+x];
			buf[pos++] = raster[(start+len-1)*w+x];
		}
		buf[pos++] = 0xff;
	}

	buf[0] = w & 0xff;          buf[1] = (w>>8) & 0xff;
	buf[2] = h & 0xff;          buf[3] = (h>>8) & 0xff;
	buf[4] = leftoffset & 0xff; buf[5] = (leftoffset>>8) & 0xff;
	buf[6] = topoffset & 0xff;  buf[7] = (topoffset>>8) & 0xff;
	for (x = 0; x < w; x++)
	{
		buf[8+x*4+0] = columnofs[x] & 0xff;
		buf[8+x*4+1] = (columnofs[x]>>8) & 0xff;
		buf[8+x*4+2] = (columnofs[x]>>16) & 0xff;
		buf[8+x*4+3] = (columnofs[x]>>24) & 0xff;
	}

	free(columnofs);
	*out_buf = buf;
	*out_len = pos;
	return 0;
}

// decode -> rescale -> encode in one call.
static int rescale_patch(const rbyte *raw, int rawlen, double scale,
                           rbyte **out_buf, int *out_len)
{
	int w, h, left, top;
	rbyte *raster, *mask;
	if (decode_patch(raw, rawlen, &w, &h, &left, &top, &raster, &mask) != 0)
		return -1;

	int dw = (int)(w * scale); if (dw < 1) dw = 1;
	int dh = (int)(h * scale); if (dh < 1) dh = 1;
	int dleft = (int)(left * scale);
	int dtop  = (int)(top * scale);

	printf("Rescaled to %d, %d\n", dw, dh);

	rbyte *draster = malloc(dw*dh);
	rbyte *dmask   = malloc(dw*dh);
	rescale_raster(raster, mask, w, h, draster, dmask, dw, dh);

	int r = encode_patch(draster, dmask, dw, dh, dleft, dtop, out_buf, out_len);

	free(raster); free(mask); free(draster); free(dmask);
	return r;
}

// Scale a raw flat (palette indices, no header) by nearest neighbor.
// The flat is stored as width*height bytes; assume square.
static unsigned char* scale_flat(const unsigned char *src, int src_size, int *out_size)
{
	int y, x;
    int src_dim = (int)sqrt(src_size);   // should be 64 for DOOM flats
    if (src_dim * src_dim != src_size) return NULL; // not a flat
    int dst_dim = (int)(src_dim * FLAT_RESCALE);
    if (dst_dim < 1) dst_dim = 1;
    unsigned char *dst = malloc(dst_dim * dst_dim);
    for (y = 0; y < dst_dim; y++) {
        int sy = (int)((double)y * src_dim / dst_dim);
        for (x = 0; x < dst_dim; x++) {
            int sx = (int)((double)x * src_dim / dst_dim);
            dst[y * dst_dim + x] = src[sy * src_dim + sx];
        }
    }
    *out_size = dst_dim * dst_dim;
    return dst;
}

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
    void		**columndirectory;
    short		patchcount;
    mappatch_t	patches[1];
} maptexture_t;

// ---------------------------------------------------------------------
// Build patch set from TEXTURE1 and PNAMES.
static void build_patch_set(const unsigned char *tex_data, int tex_size)
{
    if (tex_size < 4) return;
    int numtex = *(int*)tex_data;
    int *directory = (int*)(tex_data + 4);
    int i;

    int pnames_idx = -1;
    for (i = 0; i < numlumps; i++) {
        if (strncmp(lumpinfo[i].name, "PNAMES", 6) == 0) {
            pnames_idx = i;
            break;
        }
    }
    if (pnames_idx == -1) {
        fprintf(stderr, "WARNING: PNAMES lump not found, cannot resolve patch names.\n");
        return;
    }
    unsigned char *pnames_data = &rawwad[lumpinfo[pnames_idx].position];
    int num_pnames = *(int*)pnames_data;
    if (num_pnames <= 0) return;

    int total_patches = 0;
    for (i = 0; i < numtex; i++) {
        int offset = directory[i];
        maptexture_t *tex = (maptexture_t*)(tex_data + offset);
        total_patches += tex->patchcount;
    }
    if (total_patches == 0) return;
    patch_names = malloc(total_patches * sizeof(char*));
    num_patch_names = 0;

    for (i = 0; i < numtex; i++) {
        int offset = directory[i];
        maptexture_t *tex = (maptexture_t*)(tex_data + offset);
        int j;
        for (j = 0; j < tex->patchcount; j++) {
            int patch_num = tex->patches[j].patch;
            if (patch_num < 0 || patch_num >= num_pnames) continue;
            char *pname = (char*)pnames_data + 4 + patch_num * 8;
            char *name_copy = malloc(9);
            memcpy(name_copy, pname, 8);
            name_copy[8] = 0;
            patch_names[num_patch_names++] = name_copy;
        }
    }
}

static int is_patch_lump(const char *name)
{
	int i;
    if (!patch_names) return 0;
    for (i = 0; i < num_patch_names; i++) {
        if (strncmp(name, patch_names[i], 8) == 0)
            return 1;
    }
    return 0;
}

// ---------------------------------------------------------------------
// scale_texture1 uses PATCH_RESCALE (not the sprite scale)
static void scale_texture1(unsigned char *tex_data, int *new_size)
{
	int i, j;
    int numtex = *(int*)tex_data;
    int *directory = (int*)(tex_data + 4);
    for (i = 0; i < numtex; i++) {
        int offset = directory[i];
        maptexture_t *tex = (maptexture_t*)(tex_data + offset);
        // Use PATCH_RESCALE for texture dimensions and patch origins
        tex->width = (short)(tex->width * PATCH_RESCALE);
        tex->height = (short)(tex->height * PATCH_RESCALE);
        if (tex->width < 1) tex->width = 1;
        if (tex->height < 1) tex->height = 1;
        for (j = 0; j < tex->patchcount; j++) {
            tex->patches[j].originx = (short)(tex->patches[j].originx * PATCH_RESCALE);
            tex->patches[j].originy = (short)(tex->patches[j].originy * PATCH_RESCALE);
        }
    }
    *new_size = (unsigned char*)directory - tex_data + numtex * 4;
}
// ---------------------------------------------------------------------

static int is_sprite_lump(const char *name)
{
	int i;
    for (i = 0; sprnames[i]; i++) {
        if (strncmp(name, sprnames[i], 4) == 0)
            return 1;
    }
    return 0;
}

// ---------------------------------------------------------------------
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
	
	// Find flat range
	flat_start_lump = -1;
	flat_end_lump = -1;
	for (i = 0; i < numlumps; i++) {
		if (strncmp(lumpinfo[i].name, "F_START", 7) == 0) flat_start_lump = i;
		if (strncmp(lumpinfo[i].name, "F_END", 5) == 0) flat_end_lump = i;
	}

	for( i = 0; i < numlumps; i++ )
		chunkmap[i] = 1;

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
	} else {
		memset( usespritemap, 1, sizeof(usespritemap) );
	}

	// Sprite stripping loop (unchanged)
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

	// Find TEXTURE1
	int * texture1data; 
	int * texdirectory;
	int numtex = 0;
	int texture1datasize;
	for( i = 0 ; i < numlumps; i++ )
	{
		if( strncmp( lumpinfo[i].name, "TEXTURE1", 8 ) == 0 )
		{
			int offset = lumpinfo[i].position;
			texture1datasize = lumpinfo[i].size;
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
		// Build patch name set from original TEXTURE1
		build_patch_set((unsigned char*)texture1data, texture1datasize);

		// Modify TEXTURE1 using PATCH_RESCALE
		int new_tex_size;
		scale_texture1((unsigned char*)texture1data, &new_tex_size);
		
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

	// Force keep TEXTURE1 and PNAMES
	for (i = 0; i < numlumps; i++) {
		if (strncmp(lumpinfo[i].name, "TEXTURE1", 8) == 0 ||
		    strncmp(lumpinfo[i].name, "PNAMES", 6) == 0) {
			chunkmap[i] = 1;
		}
	}

	// Force keep HUD lumps (already done by should_rescale_lump, but ensure they are kept)
	for (i = 0; i < numlumps; i++) {
		if (should_rescale_lump(lumpinfo[i].name)) {
			chunkmap[i] = 1;
		}
	}

	printf( "Loaded list.\n" );

	int couldsave = 0;
	int original_kept_total = 0;
	int numnewchunks = 0;

	for( i = 0; i < numlumps; i++ )
	{
		if ( chunkmap[i] == 0 )
			couldsave += lumpinfo[i].size;
		else if ( chunkmap[i] == -1 )
		{
			numnewchunks++;
		}
		else // chunkmap[i] == 1
		{
			original_kept_total += lumpinfo[i].size;
			numnewchunks++;
		}
	}

	unsigned char *newchunkdata = malloc(original_kept_total);
	if (!newchunkdata) { fprintf(stderr, "malloc failed\n"); return -1; }

	FILE * f_c = fopen( argv[3], "w" );
	FILE * f_h = fopen( argv[4], "w" );

	fprintf( f_h, "#ifndef _RAWWAD_H\n"
	"#define _RAWWAD_H\n"
	"extern const int numlumps;\n"
	"extern const unsigned char rawwad[%d];\n"
	"#endif\n", original_kept_total );
	fclose( f_h );

	int tlump = 0;
	lumpinfo_t newlumpinfo[numnewchunks+1];
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
			int is_hud = should_rescale_lump(lumpinfo[i].name);
			int is_sprite = is_sprite_lump(lumpinfo[i].name);
			int is_patch = is_patch_lump(lumpinfo[i].name);
			int is_texture1 = (strncmp(lumpinfo[i].name, "TEXTURE1", 8) == 0);
			int is_pnames = (strncmp(lumpinfo[i].name, "PNAMES", 6) == 0);
			int is_flat = (i > flat_start_lump && i < flat_end_lump);

			// --- FIXED: scale ALL patch-format lumps (HUD, sprites, wall patches) ---
			int should_scale = (is_hud || is_sprite || is_patch) && !is_texture1 && !is_pnames;
			// Flats are handled separately below.

			if ( should_scale )
			{
				rbyte *rescaled = NULL;
				int rescaled_len = 0;
				double scale;
				if (is_hud)
					scale = HUD_RESCALE;
				else if (is_sprite)
					scale = SPRITE_RESCALE;
				else // is_patch
					scale = PATCH_RESCALE;

				if( rescale_patch( (const rbyte*)&rawwad[lumpinfo[i].position],
				                    lumpinfo[i].size, scale,
				                    &rescaled, &rescaled_len ) == 0 )
				{
					char ct9[9] = { 0 };
					memcpy( ct9, lumpinfo[i].name, 8 );
					printf( "Rescaled %s: %d -> %d bytes\n", ct9, lumpinfo[i].size, rescaled_len );
					newlumpinfo[tlump].size = rescaled_len;
					newlumpinfo[tlump].position = marker;
					tlump++;
					memcpy( newchunkdata + marker, rescaled, rescaled_len );
					marker += rescaled_len;
					free( rescaled );
				}
				else
				{
					char ct9[9] = { 0 };
					memcpy( ct9, lumpinfo[i].name, 8 ); 
					fprintf( stderr, "WARNING: %s failed rescale, copying unmodified\n", ct9 );
					newlumpinfo[tlump].size = lumpinfo[i].size;
					newlumpinfo[tlump].position = marker;
					tlump++;
					memcpy( newchunkdata + marker, &rawwad[lumpinfo[i].position], lumpinfo[i].size );
					marker += lumpinfo[i].size;
				}
			}
			else if (is_texture1)
			{
				// Use modified TEXTURE1 (already scaled with PATCH_RESCALE)
				newlumpinfo[tlump].size = texture1datasize;
				newlumpinfo[tlump].position = marker;
				tlump++;
				memcpy(newchunkdata + marker, texture1data, texture1datasize);
				marker += texture1datasize;
				continue;
			}
			else if (is_flat)
			{
				int new_size;
				unsigned char *scaled = scale_flat(&rawwad[lumpinfo[i].position], lumpinfo[i].size, &new_size);
				if (scaled) {
					char ct9[9] = { 0 };
					memcpy( ct9, lumpinfo[i].name, 8 );
					printf( "Scaled flat %s: %d -> %d bytes\n", ct9, lumpinfo[i].size, new_size );
					newlumpinfo[tlump].size = new_size;
					newlumpinfo[tlump].position = marker;
					tlump++;
					memcpy(newchunkdata + marker, scaled, new_size);
					marker += new_size;
					free(scaled);
					continue;
				} else {
					fprintf(stderr, "WARNING: flat %s scaling failed, copying unmodified\n", lumpinfo[i].name);
				}
			}
			else
			{
				// Copy verbatim (PNAMES, etc.)
				newlumpinfo[tlump].size = lumpinfo[i].size;
				newlumpinfo[tlump].position = marker;
				tlump++;
				memcpy( newchunkdata + marker, &rawwad[lumpinfo[i].position], lumpinfo[i].size );
				marker += lumpinfo[i].size;
			}
   		}
	}

	printf( "\n" );
	printf( "Did save %d\n", couldsave );
	int newtotal = marker;
	printf( "New Total: %d\n", newtotal );
	printf( "Comparing: %d/%d/%d\n", tlump, numnewchunks, numlumps );

	// Write final header and data
	f_h = fopen( argv[4], "w" );
	fprintf( f_h, "#ifndef _RAWWAD_H\n"
	"#define _RAWWAD_H\n"
	"extern const int numlumps;\n"
	"extern const unsigned char rawwad[%d];\n"
	"#endif\n", newtotal );
	fclose( f_h );

	f_c = fopen( argv[3], "w" );
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
		printf( "LUMP %d = %s pos=%d size=%d\n", i, tsr, newlumpinfo[i].position, newlumpinfo[i].size );
		fprintf( f_c, "\t{ \"%s\", %d, %d },\n", tsr, newlumpinfo[i].size?newlumpinfo[i].position:0, newlumpinfo[i].size );
	}
	fprintf( f_c, "};\n" );
	fclose( f_c );

	// Free patch names
	if (patch_names) {
		for (i = 0; i < num_patch_names; i++) free(patch_names[i]);
		free(patch_names);
	}
}
