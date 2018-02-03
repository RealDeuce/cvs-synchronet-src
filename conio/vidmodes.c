/* $Id: vidmodes.c,v 1.21 2018/02/01 21:11:30 deuce Exp $ */

#include <stdlib.h>
#include <stdbool.h>

/* xpdev: */
#include <gen_defs.h>	/* FREE_AND_NULL */

#include "vidmodes.h"

struct video_params vparams[49] = {
	/* BW 40x25 */
	{BW40, GREYSCALE_PALETTE, 40, 25, 14, 15, 16, 8, 1},
	/* CO 40x25 */
	{C40, COLOUR_PALETTE, 40, 25, 14, 15, 16, 8, 1},
	/* BW 80x25 */
	{BW80, GREYSCALE_PALETTE, 80, 25, 14, 15, 16, 8, 1},
	/* CO 80x25 */
	{C80, COLOUR_PALETTE, 80, 25, 14, 15, 16, 8, 1},
	/* MONO */
	{MONO, 0, 80, 25, 14, 15, 16, 8, 1},
	/* CO 40x14 */
	{C40X14, COLOUR_PALETTE, 40, 14, 14, 15, 16, 8, 1},
	/* CO 40x21 */
	{C40X21, COLOUR_PALETTE, 40, 21, 14, 15, 16, 8, 1},
	/* CO 40x28 */
	{C40X28, COLOUR_PALETTE, 40, 28, 12, 13, 14, 8, 1},
	/* CO 40x43 */
	{C40X43, COLOUR_PALETTE, 40, 43, 7, 7, 8, 8, 1},
	/* CO 40x50 */
	{C40X50, COLOUR_PALETTE, 40, 50, 7, 7, 8, 8, 1},
	/* CO 40x60 */
	{C40X60, COLOUR_PALETTE, 40, 60, 7, 7, 8, 8, 1},
	/* CO 80x14 */
	{C80X14, COLOUR_PALETTE, 80, 14, 14, 15, 16, 8, 1},
	/* CO 80x21 */
	{C80X21, COLOUR_PALETTE, 80, 21, 14, 15, 16, 8, 1},
	/* CO 80x28 */
	{C80X28, COLOUR_PALETTE, 80, 28, 12, 13, 14, 8, 1},
	/* CO 80x43 */
	{C80X43, COLOUR_PALETTE, 80, 43, 7, 7, 8, 8, 1},
	/* CO 80x50 */
	{C80X50, COLOUR_PALETTE, 80, 50, 7, 7, 8, 8, 1},
	/* CO 80x60 */
	{C80X60, COLOUR_PALETTE, 80, 60, 7, 7, 8, 8, 1},
	/* B 40x14 */
	{BW40X14, GREYSCALE_PALETTE, 40, 14, 14, 15, 16, 8, 1},
	/* BW 40x21 */
	{BW40X21, GREYSCALE_PALETTE, 40, 21, 14, 15, 16, 8, 1},
	/* BW 40x28 */
	{BW40X28, GREYSCALE_PALETTE, 40, 28, 12, 13, 14, 8, 1},
	/* BW 40x43 */
	{BW40X43, GREYSCALE_PALETTE, 40, 43, 7, 7, 8, 8, 1},
	/* BW 40x50 */
	{BW40X50, GREYSCALE_PALETTE, 40, 50, 7, 7, 8, 8, 1},
	/* BW 40x60 */
	{BW40X60, GREYSCALE_PALETTE, 40, 60, 7, 7, 8, 8, 1},
	/* BW 80x14 */
	{BW80X14, GREYSCALE_PALETTE, 80, 14, 14, 15, 16, 8, 1},
	/* BW 80x21 */
	{BW80X21, GREYSCALE_PALETTE, 80, 21, 14, 15, 16, 8, 1},
	/* BW 80x28 */
	{BW80X28, GREYSCALE_PALETTE, 80, 28, 12, 13, 14, 8, 1},
	/* BW 80x43 */
	{BW80X43, GREYSCALE_PALETTE, 80, 43, 7, 7, 8, 8, 1},
	/* BW 80x50 */
	{BW80X50, GREYSCALE_PALETTE, 80, 50, 7, 7, 8, 8, 1},
	/* BW 80x60 */
	{BW80X60, GREYSCALE_PALETTE, 80, 60, 7, 7, 8, 8, 1},
	/* MONO 80x14 */
	{MONO14, MONO_PALETTE, 80, 14, 14, 15, 16, 8, 1},
	/* MONO 80x21 */
	{MONO21, MONO_PALETTE, 80, 21, 14, 15, 16, 8, 1},
	/* MONO 80x28 */
	{MONO28, MONO_PALETTE, 80, 28, 12, 13, 14, 8, 1},
	/* MONO 80x43 */
	{MONO43, MONO_PALETTE, 80, 43, 7, 7, 8, 8, 1},
	/* MONO 80x50 */
	{MONO50, MONO_PALETTE, 80, 50, 7, 7, 8, 8, 1},
	/* MONO 80x60 */
	{MONO60, MONO_PALETTE, 80, 60, 7, 7, 8, 8, 1},
	/* Magical C4350 Mode */
	{C4350, COLOUR_PALETTE, 80, 50, 7, 7, 8, 8, 1},
	/* Commodore 64 40x25 mode */
	{C64_40X25, C64_PALETTE, 40, 25, 0, 7, 8, 8, 1},
	/* Commodore 128 40x25 mode */
	{C128_40X25, COLOUR_PALETTE, 40, 25, 0, 7, 8, 8, 1},
	/* Commodore 128 80x25 mode */
	{C128_80X25, COLOUR_PALETTE, 80, 25, 0, 7, 8, 8, 2},
	/* Atari 800 40x24 mode */
	{ATARI_40X24, ATARI_PALETTE, 40, 24, 0, 7, 8, 8, 1},
	/* Atari 800 XEP80 80x25 mode */
	{ATARI_80X25, GREYSCALE_PALETTE, 80, 25, 0, 15, 16, 8, 1},
	/* VESA 21x132 mode */
	{VESA_132X21, COLOUR_PALETTE, 132, 21, 14, 15, 16, 8, 1},
	/* VESA 25x132 mode */
	{VESA_132X25, COLOUR_PALETTE, 132, 25, 14, 15, 16, 8, 1},
	/* VESA 28x132 mode */
	{VESA_132X28, COLOUR_PALETTE, 132, 28, 12, 13, 14, 8, 1},
	/* VESA 30x132 mode */
	{VESA_132X30, COLOUR_PALETTE, 132, 30, 14, 15, 16, 8, 1},
	/* VESA 34x132 mode */
	{VESA_132X34, COLOUR_PALETTE, 132, 34, 12, 13, 14, 8, 1},
	/* VESA 43x132 mode */
	{VESA_132X43, COLOUR_PALETTE, 132, 43, 7, 7, 8, 8, 1},
	/* VESA 50x132 mode */
	{VESA_132X50, COLOUR_PALETTE, 132, 50, 7, 7, 8, 8, 1},
	/* VESA 60x132 mode */
	{VESA_132X60, COLOUR_PALETTE, 132, 60, 7, 7, 8, 8, 1},
};

uint32_t palettes[5][16] = {
	/* Mono */
	{ 0x00, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07,
	  0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07
	},
	/* Black and White */
	{ 0x00, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07,
	  0x08, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f
	},
	/* Colour */
	{ 0x00, 0x04, 0x02, 0x06, 0x01, 0x05, 0x03, 0x07, 
	  0x08, 0x0c, 0x0a, 0x0e, 0x09, 0x0d, 0x0b, 0x0f
	},
	/* C64 */
	{ 0x100, 0x101, 0x102, 0x103, 0x104, 0x105, 0x106, 0x107, 
	  0x108, 0x109, 0x10a, 0x10b, 0x10c, 0x10d, 0x10e, 0x10f
	},
	/* Atari */
	{ 0x110, 0x111, 0x111, 0x111, 0x111, 0x111, 0x111, 0x111,
	  0x111, 0x111, 0x111, 0x111, 0x111, 0x111, 0x111, 0x111
	},
};

struct dac_colors dac_default[TOTAL_DAC_SIZE] = {
	/* "System" colours */
	{0, 0, 0},       // Black
	{168, 0, 0},     // Red
	{0, 168, 0},     // Green
	{168, 84, 0},    // Brown
	{0, 0, 168},     // Blue
	{168, 0, 168},   // Magenta
	{0, 168, 168},   // Cyan
	{168, 168, 168}, // Light Gray

	{84, 84, 84},    // Dark Gray
	{255, 84, 84},   // Light Red
	{84, 255, 84},   // Light Green
	{255, 255, 84},  // Yellow
	{84, 84, 255},   // Light Blue
	{255, 84, 255},  // Light Magenta
	{84, 255, 255},  // Light Cyan
	{255, 255, 255}, // White

	/* XTerm 256 colour palette */
	{0, 0, 0}, {0, 0, 95}, {0, 0, 135}, {0, 0, 175},
	{0, 0, 215}, {0, 0, 255}, {0, 95, 0}, {0, 95, 95},
	{0, 95, 135}, {0, 95, 175}, {0, 95, 215}, {0, 95, 255},
	{0, 135, 0}, {0, 135, 95}, {0, 135, 135}, {0, 135, 175},
	{0, 135, 215}, {0, 135, 255}, {0, 175, 0}, {0, 175, 95},
	{0, 175, 135}, {0, 175, 175}, {0, 175, 215}, {0, 175, 255},
	{0, 215, 0}, {0, 215, 95}, {0, 215, 135}, {0, 215, 175},
	{0, 215, 215}, {0, 215, 255}, {0, 255, 0}, {0, 255, 95},
	{0, 255, 135}, {0, 255, 175}, {0, 255, 215}, {0, 255, 255},
	{95, 0, 0}, {95, 0, 95}, {95, 0, 135}, {95, 0, 175},
	{95, 0, 215}, {95, 0, 255}, {95, 95, 0}, {95, 95, 95},
	{95, 95, 135}, {95, 95, 175}, {95, 95, 215}, {95, 95, 255},
	{95, 135, 0}, {95, 135, 95}, {95, 135, 135}, {95, 135, 175},
	{95, 135, 215}, {95, 135, 255}, {95, 175, 0}, {95, 175, 95},
	{95, 175, 135}, {95, 175, 175}, {95, 175, 215}, {95, 175, 255},
	{95, 215, 0}, {95, 215, 95}, {95, 215, 135}, {95, 215, 175},
	{95, 215, 215}, {95, 215, 255}, {95, 255, 0}, {95, 255, 95},
	{95, 255, 135}, {95, 255, 175}, {95, 255, 215}, {95, 255, 255},
	{135, 0, 0}, {135, 0, 95}, {135, 0, 135}, {135, 0, 175},
	{135, 0, 215}, {135, 0, 255}, {135, 95, 0}, {135, 95, 95},
	{135, 95, 135}, {135, 95, 175}, {135, 95, 215}, {135, 95, 255},
	{135, 135, 0}, {135, 135, 95}, {135, 135, 135}, {135, 135, 175},
	{135, 135, 215}, {135, 135, 255}, {135, 175, 0}, {135, 175, 95},
	{135, 175, 135}, {135, 175, 175}, {135, 175, 215}, {135, 175, 255},
	{135, 215, 0}, {135, 215, 95}, {135, 215, 135}, {135, 215, 175},
	{135, 215, 215}, {135, 215, 255}, {135, 255, 0}, {135, 255, 95},
	{135, 255, 135}, {135, 255, 175}, {135, 255, 215}, {135, 255, 255},
	{175, 0, 0}, {175, 0, 95}, {175, 0, 135}, {175, 0, 175},
	{175, 0, 215}, {175, 0, 255}, {175, 95, 0}, {175, 95, 95},
	{175, 95, 135}, {175, 95, 175}, {175, 95, 215}, {175, 95, 255},
	{175, 135, 0}, {175, 135, 95}, {175, 135, 135}, {175, 135, 175},
	{175, 135, 215}, {175, 135, 255}, {175, 175, 0}, {175, 175, 95},
	{175, 175, 135}, {175, 175, 175}, {175, 175, 215}, {175, 175, 255},
	{175, 215, 0}, {175, 215, 95}, {175, 215, 135}, {175, 215, 175},
	{175, 215, 215}, {175, 215, 255}, {175, 255, 0}, {175, 255, 95},
	{175, 255, 135}, {175, 255, 175}, {175, 255, 215}, {175, 255, 255},
	{215, 0, 0}, {215, 0, 95}, {215, 0, 135}, {215, 0, 175},
	{215, 0, 215}, {215, 0, 255}, {215, 95, 0}, {215, 95, 95},
	{215, 95, 135}, {215, 95, 175}, {215, 95, 215}, {215, 95, 255},
	{215, 135, 0}, {215, 135, 95}, {215, 135, 135}, {215, 135, 175},
	{215, 135, 215}, {215, 135, 255}, {215, 175, 0}, {215, 175, 95},
	{215, 175, 135}, {215, 175, 175}, {215, 175, 215}, {215, 175, 255},
	{215, 215, 0}, {215, 215, 95}, {215, 215, 135}, {215, 215, 175},
	{215, 215, 215}, {215, 215, 255}, {215, 255, 0}, {215, 255, 95},
	{215, 255, 135}, {215, 255, 175}, {215, 255, 215}, {215, 255, 255},
	{255, 0, 0}, {255, 0, 95}, {255, 0, 135}, {255, 0, 175},
	{255, 0, 215}, {255, 0, 255}, {255, 95, 0}, {255, 95, 95},
	{255, 95, 135}, {255, 95, 175}, {255, 95, 215}, {255, 95, 255},
	{255, 135, 0}, {255, 135, 95}, {255, 135, 135}, {255, 135, 175},
	{255, 135, 215}, {255, 135, 255}, {255, 175, 0}, {255, 175, 95},
	{255, 175, 135}, {255, 175, 175}, {255, 175, 215}, {255, 175, 255},
	{255, 215, 0}, {255, 215, 95}, {255, 215, 135}, {255, 215, 175},
	{255, 215, 215}, {255, 215, 255}, {255, 255, 0}, {255, 255, 95},
	{255, 255, 135}, {255, 255, 175}, {255, 255, 215}, {255, 255, 255},
	{8, 8, 8}, {18, 18, 18}, {28, 28, 28}, {38, 38, 38},
	{48, 48, 48}, {58, 58, 58}, {68, 68, 68}, {78, 78, 78},
	{88, 88, 88}, {98, 98, 98}, {108, 108, 108}, {118, 118, 118},
	{128, 128, 128}, {138, 138, 138}, {148, 148, 148}, {158, 158, 158},
	{168, 168, 168}, {178, 178, 178}, {188, 188, 188}, {198, 198, 198},
	{208, 208, 208}, {218, 218, 218}, {228, 228, 228}, {238, 238, 238}, 

	/* C64 colours */
	/* Black, White, Red, Cyan, Purple, Green, Blue, Yellow */
	/* Orange, Brown, Lt Red, Dk Grey, Grey, Lt Green, Lt Blue, Lt Grey */
	/* Taken from CTerm source */
	{0x00, 0x00, 0x00},	// Black
	{0xFD, 0xFE, 0xFC},	// White
	{0xBE, 0x1A, 0x24},	// Red
	{0x30, 0xE6, 0xC6},	// Cyan
	{0xB4, 0x1A, 0xE2},	// Purple
	{0x1F, 0xD2, 0x1E},	// Green
	{0x21, 0x1B, 0xAE},	// Blue
	{0xDF, 0xF6, 0x0A},	// Yellow
	{0xB8, 0x41, 0x04},	// Orange
	{0x6A, 0x33, 0x04},	// Brown
	{0xFE, 0x4A, 0x57},	// Light Red
	{0x42, 0x45, 0x40},	// Dark Grey
	{0x70, 0x74, 0x6F},	// Grey
	{0x59, 0xFE, 0x59},	// Light Green
	{0x5F, 0x53, 0xFE},	// Light Blue
	{0xA4, 0xA7, 0xA2},	// Light Grey
	/* Atari Colours */
	/* BG, FG */
	{0, 81, 129}, {96, 183, 231},
};

int find_vmode(int mode)
{
    unsigned i;

	for (i = 0; i < NUMMODES; i++)
		if (vparams[i].mode == mode)
			return i;

	return -1;
}

struct vstat_vmem *get_vmem(struct video_stats *vs)
{
	vs->vmem->refcount++;
	return vs->vmem;
}

void release_vmem(struct vstat_vmem *vm)
{
	if (vm == NULL)
		return;
	vm->refcount--;
	if (vm->refcount == 0) {
		FREE_AND_NULL(vm->vmem);
		FREE_AND_NULL(vm->fgvmem);
		FREE_AND_NULL(vm->bgvmem);
		FREE_AND_NULL(vm);
	}
}

static struct vstat_vmem *new_vmem(int cols, int rows, bool palette)
{
	struct vstat_vmem *ret = malloc(sizeof(struct vstat_vmem));

	if (ret == NULL)
		return ret;
	ret->refcount = 1;
	ret->vmem = (unsigned short *)malloc(cols*rows*sizeof(unsigned short));
	if (ret->vmem == NULL) {
		free(ret);
		return NULL;
	}
	if (palette) {
		ret->fgvmem = malloc(cols*rows*sizeof(ret->fgvmem[0]));
		if (ret->fgvmem == NULL) {
			free(ret->vmem);
			free(ret);
			return NULL;
		}
		ret->bgvmem = malloc(cols*rows*sizeof(ret->bgvmem[0]));
		if (ret->bgvmem == NULL) {
			free(ret->fgvmem);
			free(ret->vmem);
			free(ret);
			return NULL;
		}
	}
	else {
		ret->fgvmem = NULL;
		ret->bgvmem = NULL;
	}
	return ret;
}

int load_vmode(struct video_stats *vs, int mode)
{
	int i;

	i=find_vmode(mode);
	if(i==-1)
		return(-1);
	release_vmem(vs->vmem);
	vs->vmem=new_vmem(vparams[i].cols, vparams[i].rows, vs->flags & VIDMODES_FLAG_PALETTE_VMEM);
	if (vs->vmem == NULL)
		return -1;
	vs->rows=vparams[i].rows;
	vs->cols=vparams[i].cols;
	vs->curs_start=vparams[i].curs_start;
	vs->curs_end=vparams[i].curs_end;
	vs->default_curs_start=vparams[i].curs_start;
	vs->default_curs_end=vparams[i].curs_end;
	vs->curs_blink=1;
	vs->curs_visible=1;
	vs->bright_background=0;
	vs->no_bright=0;
	vs->bright_altcharset=0;
	vs->no_blink=0;
	vs->blink_altcharset=0;
	if(vs->curs_row < 0)
		vs->curs_row=0;
	if(vs->curs_row >= vparams[i].rows)
		vs->curs_row=vparams[i].rows-1;
	if(vs->curs_col < 0)
		vs->curs_col=0;
	if(vs->curs_col >= vparams[i].cols)
		vs->curs_col=vparams[i].cols-1;
	memcpy(vs->palette, palettes[vparams[i].palette], sizeof(vs->palette));
	vs->charheight=vparams[i].charheight;
	vs->charwidth=vparams[i].charwidth;
	vs->mode=mode;
	vs->vmultiplier=vparams[i].vmultiplier;
	return(0);
}
