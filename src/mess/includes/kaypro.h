
/* These are needed here otherwise the "extern const" gives compile errors */
#include "video/mc6845.h"
#include "machine/z80pio.h"
#include "machine/z80sio.h"
#include "machine/wd17xx.h"


/* machine.c */

extern UINT8 kaypro2x_system_port;
extern const z80pio_interface kayproii_pio_g_intf;
extern const z80pio_interface kayproii_pio_s_intf;
extern const z80sio_interface kaypro_sio_intf;
extern const wd17xx_interface kaypro_wd1793_interface;

READ8_DEVICE_HANDLER( kayproii_pio_r );
READ8_DEVICE_HANDLER( kaypro_sio_r );
READ8_DEVICE_HANDLER( kaypro2x_sio_r );

READ8_HANDLER( kaypro2x_system_port_r );

WRITE8_DEVICE_HANDLER( kayproii_pio_w );
WRITE8_DEVICE_HANDLER( kaypro_sio_w );
WRITE8_DEVICE_HANDLER( kaypro2x_sio_w );

WRITE8_HANDLER( kaypro_baud_a_w );
WRITE8_HANDLER( kayproii_baud_b_w );
WRITE8_HANDLER( kaypro2x_baud_a_w );
WRITE8_HANDLER( kaypro2x_system_port_w );

void kayproii_floppy_getinfo(const mess_device_class *devclass, UINT32 state, union devinfo *info);
void kaypro2x_floppy_getinfo(const mess_device_class *devclass, UINT32 state, union devinfo *info);

MACHINE_RESET( kayproii );
MACHINE_RESET( kaypro2x );

/* video.c */

MC6845_UPDATE_ROW( kaypro2x_update_row );
PALETTE_INIT( kaypro );
VIDEO_START( kaypro );
VIDEO_UPDATE( kayproii );
VIDEO_UPDATE( omni2 );
VIDEO_UPDATE( kaypro2x );

READ8_HANDLER( kaypro_videoram_r );
WRITE8_HANDLER( kaypro_videoram_w );
WRITE8_HANDLER( kaypro2x_index_w );
WRITE8_HANDLER( kaypro2x_data_w );
