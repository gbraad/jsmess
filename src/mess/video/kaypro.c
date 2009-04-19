
#include "driver.h"
#include "includes/kaypro.h"

static UINT8 mc6845_cursor[16];				// cursor shape
static UINT8 mc6845_reg[32];				/* registers */
static UINT8 mc6845_ind;				/* register index */
static const UINT8 mc6845_mask[32]={0xff,0xff,0xff,0x0f,0x7f,0x1f,0x7f,0x7f,3,0x1f,0x7f,0x1f,0x3f,0xff,0x3f,0xff,0,0};

static const device_config *mc6845;
static const UINT8 *FNT;
static UINT8 chr,gfx,fg,bg;
static UINT16 mem,x;
static UINT8 framecnt=0;
static UINT8 speed,flash;
static UINT16 cursor;
static UINT16 mc6845_video_address;

/***********************************************************

	Video

************************************************************/

PALETTE_INIT( kaypro )
{
	palette_set_color(machine,0,RGB_BLACK); /* black */
	palette_set_color(machine,1,MAKE_RGB(0, 220, 0)); /* green */	
}

VIDEO_UPDATE( kayproii )
{
/* The display consists of 80 columns and 24 rows. Each row is allocated 128 bytes of ram,
	but only the first 80 are used. The total video ram therefore is 0x0c00 bytes.
	There is one video attribute: bit 7 causes blinking. The first half of the
	character generator is blank, with the visible characters in the 2nd half.
	During the "off" period of blanking, the first half is used. Only 5 pixels are
	connected from the rom to the shift register, the remaining pixels are held high.
	A high pixel is black and a low pixel is green. */

	static UINT8 framecnt=0;
	UINT8 y,ra,chr,gfx;
	UINT16 sy=0,ma=0,x;

	framecnt++;

	for (y = 0; y < 24; y++)
	{
		for (ra = 0; ra < 10; ra++)
		{
			UINT16  *p = BITMAP_ADDR16(bitmap, sy++, 0);

			for (x = ma; x < ma + 80; x++)
			{
				if (ra < 8)
				{
					chr = videoram[x]^0x80;

					/* Take care of flashing characters */
					if ((chr < 0x80) && (framecnt & 0x08))
						chr |= 0x80;

					/* get pattern of pixels for that character scanline */
					gfx = FNT[(chr<<3) | ra ];
				}
				else
					gfx = 0xff;

				/* Display a scanline of a character (7 pixels) */
				*p = 0; p++;
				*p = ( gfx & 0x10 ) ? 0 : 1; p++;
				*p = ( gfx & 0x08 ) ? 0 : 1; p++;
				*p = ( gfx & 0x04 ) ? 0 : 1; p++;
				*p = ( gfx & 0x02 ) ? 0 : 1; p++;
				*p = ( gfx & 0x01 ) ? 0 : 1; p++;
				*p = 0; p++;
			}
		}
		ma+=128;
	}
	return 0;
}

VIDEO_UPDATE( omni2 )
{
	static UINT8 framecnt=0;
	UINT8 y,ra,chr,gfx;
	UINT16 sy=0,ma=0,x;

	framecnt++;

	for (y = 0; y < 24; y++)
	{
		for (ra = 0; ra < 10; ra++)
		{
			UINT16  *p = BITMAP_ADDR16(bitmap, sy++, 0);

			for (x = ma; x < ma + 80; x++)
			{
				if (ra < 8)
				{
					chr = videoram[x];

					/* Take care of flashing characters */
					if ((chr > 0x7f) && (framecnt & 0x08))
						chr |= 0x80;

					/* get pattern of pixels for that character scanline */
					gfx = FNT[(chr<<3) | ra ];
				}
				else
					gfx = 0xff;

				/* Display a scanline of a character (7 pixels) */
				*p = ( gfx & 0x40 ) ? 0 : 1; p++;
				*p = ( gfx & 0x20 ) ? 0 : 1; p++;
				*p = ( gfx & 0x10 ) ? 0 : 1; p++;
				*p = ( gfx & 0x08 ) ? 0 : 1; p++;
				*p = ( gfx & 0x04 ) ? 0 : 1; p++;
				*p = ( gfx & 0x02 ) ? 0 : 1; p++;
				*p = ( gfx & 0x01 ) ? 0 : 1; p++;
			}
		}
		ma+=128;
	}
	return 0;
}

VIDEO_UPDATE( kaypro2x )
{
	framecnt++;
	speed = mc6845_reg[10]&0x20, flash = mc6845_reg[10]&0x40;				// cursor modes
	cursor = (mc6845_reg[14]<<8) | mc6845_reg[15];					// get cursor position
	mc6845_update(mc6845, bitmap, cliprect);
	return 0;
}

/* bit 6 of kaypro2x_system_port selects alternate characters (A12 on character generator rom) */


MC6845_UPDATE_ROW( kaypro2x_update_row )
{
	UINT16  *p = BITMAP_ADDR16(bitmap, y, 0);

	fg = 1;
	bg=0;

	for (x = 0; x < x_count; x++)				// for each character
	{
		UINT8 inv=0;
		//		if (x == cursor_x) inv=0xff;	/* uncomment when mame fixed */
		mem = (ma + x) & 0xfff;
		chr = videoram[mem];

		/* get colour or b&w */
//		col = 5;					/* green */
//		if ((options & 0x60) == 0x60) col = 15;		/* b&w */
//		if (!(options & 0x40)) col = colorram[mem];			// read a byte of colour

		/* if inverse mode, replace any pcgram chrs with inverse chrs */
//		if ((!super80v_rom_pcg) && (chr & 0x80))			// is it a high chr in inverse mode
//		{
//			inv ^= 0xff;						// invert the chr
//			chr &= 0x7f;						// and drop bit 7
//		}

		/* process cursor - remove when mame fixed */
		if ((((!flash) && (!speed)) ||
			((flash) && (speed) && (framecnt & 0x10)) ||
			((flash) && (!speed) && (framecnt & 8))) &&
			(mem == cursor))
				inv ^= mc6845_cursor[ra];

		/* get pattern of pixels for that character scanline */
		gfx = FNT[(chr<<4) | ra ];

		/* Display a scanline of a character (7 pixels) */
		*p = ( gfx & 0x80 ) ? fg : bg; p++;
		*p = ( gfx & 0x40 ) ? fg : bg; p++;
		*p = ( gfx & 0x20 ) ? fg : bg; p++;
		*p = ( gfx & 0x10 ) ? fg : bg; p++;
		*p = ( gfx & 0x08 ) ? fg : bg; p++;
		*p = ( gfx & 0x04 ) ? fg : bg; p++;
		*p = ( gfx & 0x02 ) ? fg : bg; p++;
		*p = ( gfx & 0x01 ) ? fg : bg; p++;
	}
}

/************************************* MC6845 SUPPORT ROUTINES ***************************************/

/* The 6845 can produce a variety of cursor shapes - all are emulated here - remove when mame fixed */
static void mc6845_cursor_configure(void)
{
	UINT8 i,curs_type=0,r9,r10,r11;

	/* curs_type holds the general cursor shape to be created
		0 = no cursor
		1 = partial cursor (only shows on a block of scan lines)
		2 = full cursor
		3 = two-part cursor (has a part at the top and bottom with the middle blank) */

	for ( i = 0; i < ARRAY_LENGTH(mc6845_cursor); i++) mc6845_cursor[i] = 0;		// prepare cursor by erasing old one

	r9  = mc6845_reg[9];					// number of scan lines - 1
	r10 = mc6845_reg[10] & 0x1f;				// cursor start line = last 5 bits
	r11 = mc6845_reg[11]+1;					// cursor end line incremented to suit for-loops below

	/* decide the curs_type by examining the registers */
	if (r10 < r11) curs_type=1;				// start less than end, show start to end
	else
	if (r10 == r11) curs_type=2;				// if equal, show full cursor
	else curs_type=3;					// if start greater than end, it's a two-part cursor

	if ((r11 - 1) > r9) curs_type=2;			// if end greater than scan-lines, show full cursor
	if (r10 > r9) curs_type=0;				// if start greater than scan-lines, then no cursor
	if (r11 > 16) r11=16;					// truncate 5-bit register to fit our 4-bit hardware

	/* create the new cursor */
	if (curs_type > 1) for (i = 0;i < ARRAY_LENGTH(mc6845_cursor);i++) mc6845_cursor[i]=0xff; // turn on full cursor

	if (curs_type == 1) for (i = r10;i < r11;i++) mc6845_cursor[i]=0xff; // for each line that should show, turn on that scan line

	if (curs_type == 3) for (i = r11; i < r10;i++) mc6845_cursor[i]=0; // now take a bite out of the middle
}

/* Resize the screen within the limits of the hardware. Expand the image to fill the screen area */
static void mc6845_screen_configure(running_machine *machine)
{
	rectangle visarea;

	UINT16 width = mc6845_reg[1]*7-1;							// width in pixels
	UINT16 height = mc6845_reg[6]*(mc6845_reg[9]+1)-1;					// height in pixels
	UINT16 bytes = mc6845_reg[1]*mc6845_reg[6]-1;						// video ram needed -1

	/* Resize the screen */
	visarea.min_x = 0;
	visarea.max_x = width-1;
	visarea.min_y = 0;
	visarea.max_y = height-1;
	if ((width < 610) && (height < 460) && (bytes < 0x1000))	/* bounds checking to prevent an assert or violation */
		video_screen_set_visarea(machine->primary_screen, 0, width, 0, height);
}


/**************************** I/O PORTS *****************************************************************/

WRITE8_HANDLER( kaypro2x_index_w )
{
	if (data < 32) mc6845_ind = data; else mc6845_ind = 32;		/* make sure if you try using an invalid register your write will go nowhere */
	mc6845_address_w( mc6845, 0, data );
}

WRITE8_HANDLER( kaypro2x_data_w )
{
	if (mc6845_ind < 16)
		mc6845_reg[mc6845_ind] = data & mc6845_mask[mc6845_ind];	/* save data in register */
	else
		mc6845_reg[mc6845_ind] = data;

	if ((mc6845_ind == 1) || (mc6845_ind == 6) || (mc6845_ind == 9))
		mc6845_screen_configure(space->machine);			/* adjust screen size */

	if ((mc6845_ind > 8) && (mc6845_ind < 12))
		mc6845_cursor_configure();		/* adjust cursor shape - remove when mame fixed */

	if ((mc6845_ind > 17) && (mc6845_ind < 20))
		mc6845_video_address = mc6845_reg[19] | (mc6845_reg[18] << 8);	/* internal ULA address */

	if (mc6845_ind == 31)
		kaypro_videoram_w(space, mc6845_video_address, data);

	mc6845_register_w( mc6845, 0, data );
}

READ8_HANDLER( kaypro_videoram_r )
{
	return videoram[offset];
}

WRITE8_HANDLER( kaypro_videoram_w )
{
	videoram[offset] = data;
}

VIDEO_START( kaypro )
{
	mc6845 = devtag_get_device(machine, "crtc");
	FNT = memory_region(machine, "gfx1");
}
