/*****************************************************************************
 *
 * includes/lviv.h
 *
 ****************************************************************************/

#ifndef LVIV_H_
#define LVIV_H_

#include "devices/snapquik.h"


/*----------- defined in machine/lviv.c -----------*/

extern unsigned char * lviv_video_ram;
 READ8_HANDLER ( lviv_io_r );
WRITE8_HANDLER ( lviv_io_w );
MACHINE_RESET( lviv );
SNAPSHOT_LOAD( lviv );


/*----------- defined in video/lviv.c -----------*/

extern VIDEO_START( lviv );
extern VIDEO_UPDATE( lviv );
extern const unsigned char lviv_palette[8*3];
extern unsigned short lviv_colortable[1][4];
extern PALETTE_INIT( lviv );
extern void lviv_update_palette (UINT8);


#endif /* LVIV_H_ */
