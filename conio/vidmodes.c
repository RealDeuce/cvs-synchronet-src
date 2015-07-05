/* $Id: vidmodes.c,v 1.18 2015/07/05 09:07:34 deuce Exp $ */

#include <stdlib.h>

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

unsigned char palettes[5][16] = {
	/* Mono */
	{ 0x00, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07,
	  0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07
	},
	/* Black and White */
	{ 0x00, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07,
	  0x08, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f
	},
	/* Colour */
	{ 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 
	  0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f
	},
	/* C64 */
	{ 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 
	  0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f
	},
	/* Atari */
	{ 0x20, 0x21, 0x21, 0x21, 0x21, 0x21, 0x21, 0x21,
	  0x21, 0x21, 0x21, 0x21, 0x21, 0x21, 0x21, 0x21
	},
};

struct dac_colors dac_default[TOTAL_DAC_SIZE] = {
	{0, 0, 0},    {0, 0, 168},   {0, 168, 0},   {0, 168, 168},
	{168, 0, 0},   {168, 0, 168},  {168, 84, 0},  {168, 168, 168},
	{84, 84, 84}, {84, 84, 255}, {84, 255, 84}, {84, 255, 255},
	{255, 84, 84}, {255, 84, 255}, {255, 255, 84}, {255, 255, 255},
	/* C64 colours */
	/* Black, White, Red, Cyan, Purple, Green, Blue, Yellow */
	/* Orange, Brown, Lt Red, Dk Grey, Grey, Lt Green, Lt Blue, Lt Grey */
	{0x00, 0x00, 0x00}, {0xff, 0xff, 0xff}, {0x68, 0x37, 0x2b}, 
	{0x70, 0xa4, 0xb2}, {0x6f, 0x3d, 0x86}, {0x58, 0x8d, 0x43},
	{0x35, 0x29, 0x79}, {0xb8, 0xc7, 0x6f}, {0x6f, 0x4f, 0x25},
	{0x43, 0x39, 0x00}, {0x9a, 0x67, 0x59}, {0x44, 0x44, 0x44},
	{0x6c, 0x6c, 0x6c}, {0x9a, 0xd2, 0x84}, {0x6c, 0x5e, 0xb5},
	{0x95, 0x95, 0x95},
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
		FREE_AND_NULL(vm);
	}
}

static struct vstat_vmem *new_vmem(int cols, int rows)
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
	return ret;
}

int load_vmode(struct video_stats *vs, int mode)
{
	int i;

	i=find_vmode(mode);
	if(i==-1)
		return(-1);
	release_vmem(vs->vmem);
	vs->vmem=new_vmem(vparams[i].cols, vparams[i].rows);
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
	memcpy(vs->dac_colors, dac_default, sizeof(dac_default));
	vs->charheight=vparams[i].charheight;
	vs->charwidth=vparams[i].charwidth;
	vs->mode=mode;
	vs->vmultiplier=vparams[i].vmultiplier;
	return(0);
}
