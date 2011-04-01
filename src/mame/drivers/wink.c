/*
    Wink    -   (c) 1985 Midcoin

    driver by HIGHWAYMAN, Nicola Salmoria, Pierpaolo Prazzoli

    TODO:
    - better interrupts?
    - finish sound
    - fix protection properly
    - better handling of nvram? it loses the default values
    - I need a better comparison screenshot to be sure about the colors.
*/

#include "emu.h"
#include "cpu/z80/z80.h"
#include "sound/ay8910.h"
#include "machine/nvram.h"


class wink_state : public driver_device
{
public:
	wink_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config) { }

	UINT8 *m_videoram;
	tilemap_t *m_bg_tilemap;
	UINT8 m_sound_flag;
	UINT8 m_tile_bank;
};


static TILE_GET_INFO( get_bg_tile_info )
{
	wink_state *state = machine.driver_data<wink_state>();
	UINT8 *videoram = state->m_videoram;
	int code = videoram[tile_index];
	code |= 0x200 * state->m_tile_bank;

	// the 2 parts of the screen use different tile banking
	if(tile_index < 0x360)
	{
		code |= 0x100;
	}

	SET_TILE_INFO(0, code, 0, 0);
}

static VIDEO_START( wink )
{
	wink_state *state = machine.driver_data<wink_state>();
	state->m_bg_tilemap = tilemap_create(machine, get_bg_tile_info, tilemap_scan_rows, 8, 8, 32, 32);
}

static SCREEN_UPDATE( wink )
{
	wink_state *state = screen->machine().driver_data<wink_state>();
	tilemap_draw(bitmap, cliprect, state->m_bg_tilemap, 0, 0);
	return 0;
}

static WRITE8_HANDLER( bgram_w )
{
	wink_state *state = space->machine().driver_data<wink_state>();
	UINT8 *videoram = state->m_videoram;
	videoram[offset] = data;
	tilemap_mark_tile_dirty(state->m_bg_tilemap, offset);
}

static WRITE8_HANDLER( player_mux_w )
{
	//player_mux = data & 1;
	//no mux / cocktail mode in the real pcb? strange...
}

static WRITE8_HANDLER( tile_banking_w )
{
	wink_state *state = space->machine().driver_data<wink_state>();
	state->m_tile_bank = data & 1;
	tilemap_mark_all_tiles_dirty(state->m_bg_tilemap);
}

static WRITE8_HANDLER( wink_coin_counter_w )
{
	coin_counter_w(space->machine(), offset,data & 1);
}

static READ8_HANDLER( analog_port_r )
{
	return input_port_read(space->machine(), /* player_mux ? "DIAL2" : */ "DIAL1");
}

static READ8_HANDLER( player_inputs_r )
{
	return input_port_read(space->machine(), /* player_mux ? "INPUTS2" : */ "INPUTS1");
}

static WRITE8_HANDLER( sound_irq_w )
{
	cputag_set_input_line(space->machine(), "audiocpu", 0, HOLD_LINE);
	//sync with sound cpu (but it still loses some soundlatches...)
	//space->machine().scheduler().synchronize();
}

static ADDRESS_MAP_START( wink_map, AS_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x7fff) AM_ROM
	AM_RANGE(0x8000, 0x87ff) AM_RAM
	AM_RANGE(0x9000, 0x97ff) AM_RAM	AM_SHARE("nvram")
	AM_RANGE(0xa000, 0xa3ff) AM_RAM_WRITE(bgram_w) AM_BASE_MEMBER(wink_state, m_videoram)
ADDRESS_MAP_END


static READ8_HANDLER( prot_r )
{
	//take a0-a7 and do some math using the variable created from the upper address-lines,
	//put the result onto the databus.

/*
math

take 2 bytes:

byte1 = a8,a9,a10,a11,a12,a13,a14,a15
byte2 = a0,a2,a4,a6,a1,a3,a5,a7

add the 2 bytes together so that if the final value overflows past 255 it wraps to 0
the 8bit result is placed on the databus.

*/
	return 0x20; //hack to pass the jump calculated using this value
}

static WRITE8_HANDLER( prot_w )
{
	//take a9-a15 and stuff them in a variable for later use.
}

static ADDRESS_MAP_START( wink_io, AS_IO, 8 )
	ADDRESS_MAP_GLOBAL_MASK(0xff)
	AM_RANGE(0x00, 0x1f) AM_RAM_WRITE(paletteram_xxxxBBBBRRRRGGGG_le_w) AM_BASE_GENERIC(paletteram) //0x10-0x1f is likely to be something else
//  AM_RANGE(0x20, 0x20) AM_WRITENOP                //??? seems unused..
	AM_RANGE(0x21, 0x21) AM_WRITE(player_mux_w)		//??? no mux on the pcb.
	AM_RANGE(0x22, 0x22) AM_WRITE(tile_banking_w)
//  AM_RANGE(0x23, 0x23) AM_WRITENOP                //?
//  AM_RANGE(0x24, 0x24) AM_WRITENOP                //cab Knocker like in q-bert!
	AM_RANGE(0x25, 0x27) AM_WRITE(wink_coin_counter_w)
	AM_RANGE(0x40, 0x40) AM_WRITE(soundlatch_w)
	AM_RANGE(0x60, 0x60) AM_WRITE(sound_irq_w)
	AM_RANGE(0x80, 0x80) AM_READ(analog_port_r)
	AM_RANGE(0xa0, 0xa0) AM_READ(player_inputs_r)
	AM_RANGE(0xa4, 0xa4) AM_READ_PORT("DSW1")	//dipswitch bank2
	AM_RANGE(0xa8, 0xa8) AM_READ_PORT("DSW2")	//dipswitch bank1
//  AM_RANGE(0xac, 0xac) AM_WRITENOP            //protection - loads video xor unit (written only once at startup)
	AM_RANGE(0xb0, 0xb0) AM_READ_PORT("DSW3")	//unused inputs
	AM_RANGE(0xb4, 0xb4) AM_READ_PORT("DSW4")	//dipswitch bank3
	AM_RANGE(0xc0, 0xdf) AM_WRITE(prot_w)		//load load protection-buffer from upper address bus
	AM_RANGE(0xc3, 0xc3) AM_READNOP				//watchdog?
	AM_RANGE(0xe0, 0xff) AM_READ(prot_r)		//load math unit from buffer & lower address-bus
ADDRESS_MAP_END

static ADDRESS_MAP_START( wink_sound_map, AS_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x1fff) AM_ROM
	AM_RANGE(0x4000, 0x43ff) AM_RAM
	AM_RANGE(0x8000, 0x8000) AM_READ(soundlatch_r)
ADDRESS_MAP_END

static ADDRESS_MAP_START( wink_sound_io, AS_IO, 8 )
	ADDRESS_MAP_GLOBAL_MASK(0xff)
	AM_RANGE(0x00, 0x00) AM_DEVREADWRITE("aysnd", ay8910_r, ay8910_data_w)
	AM_RANGE(0x80, 0x80) AM_DEVWRITE("aysnd", ay8910_address_w)
ADDRESS_MAP_END

static INPUT_PORTS_START( wink )
	PORT_START("DIAL1")
	PORT_BIT( 0xff, 0x00, IPT_DIAL ) PORT_SENSITIVITY(50) PORT_KEYDELTA(3) PORT_REVERSE

	PORT_START("INPUTS1")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON2 )	// right curve
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON1 )	// left curve
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON3 )	// slam

	PORT_START("DSW1")
	PORT_DIPNAME( 0x01, 0x01, "1" )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START("DSW2")
	PORT_DIPNAME( 0x01, 0x01, "2" )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START("DSW3")
	PORT_DIPNAME( 0x01, 0x01, "3" )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START("DSW4")
	PORT_DIPNAME( 0x01, 0x01, "Summary" )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, "4" )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_SERVICE( 0x10, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
INPUT_PORTS_END

static const gfx_layout charlayout =
{
	8,8,	/* 8*8 characters */
	RGN_FRAC(1,3),
	3,
	{ RGN_FRAC(0,3), RGN_FRAC(1,3), RGN_FRAC(2,3) },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8 /* every char takes 8 consecutive bytes */
};

static GFXDECODE_START( wink )
	GFXDECODE_ENTRY( "gfx1", 0, charlayout, 0, 4 )
GFXDECODE_END

static READ8_DEVICE_HANDLER( sound_r )
{
	wink_state *state = device->machine().driver_data<wink_state>();
	return state->m_sound_flag;
}

static const ay8910_interface ay8912_interface =
{
	AY8910_LEGACY_OUTPUT,
	AY8910_DEFAULT_LOADS,
	DEVCB_HANDLER(sound_r),
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL
};

//AY portA is fed by an input clock at 15625 Hz
static INTERRUPT_GEN( wink_sound )
{
	wink_state *state = device->machine().driver_data<wink_state>();
	state->m_sound_flag ^= 0x80;
}

static MACHINE_RESET( wink )
{
	wink_state *state = machine.driver_data<wink_state>();
	state->m_sound_flag = 0;
}

static MACHINE_CONFIG_START( wink, wink_state )
	/* basic machine hardware */
	MCFG_CPU_ADD("maincpu", Z80, 12000000 / 4)
	MCFG_CPU_PROGRAM_MAP(wink_map)
	MCFG_CPU_IO_MAP(wink_io)
	MCFG_CPU_VBLANK_INT("screen", nmi_line_pulse)

	MCFG_CPU_ADD("audiocpu", Z80, 12000000 / 8)
	MCFG_CPU_PROGRAM_MAP(wink_sound_map)
	MCFG_CPU_IO_MAP(wink_sound_io)
	MCFG_CPU_PERIODIC_INT(wink_sound, 15625)

	MCFG_NVRAM_ADD_1FILL("nvram")
	MCFG_MACHINE_RESET(wink)

	/* video hardware */
	MCFG_SCREEN_ADD("screen", RASTER)
	MCFG_SCREEN_REFRESH_RATE(60)
	MCFG_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(0))
	MCFG_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MCFG_SCREEN_SIZE(32*8, 32*8)
	MCFG_SCREEN_VISIBLE_AREA(0*8, 32*8-1, 0*8, 32*8-1)
	MCFG_SCREEN_UPDATE(wink)

	MCFG_GFXDECODE(wink)
	MCFG_PALETTE_LENGTH(16)

	MCFG_VIDEO_START(wink)

	/* sound hardware */
	MCFG_SPEAKER_STANDARD_MONO("mono")
	MCFG_SOUND_ADD("aysnd", AY8912, 12000000 / 8)
	MCFG_SOUND_CONFIG(ay8912_interface)
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.0)
MACHINE_CONFIG_END

/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( wink )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "midcoin-wink00.rom", 0x0000, 0x4000, CRC(044f82d6) SHA1(4269333578c4fb14891b937c683aa5b105a193e7) )
	ROM_LOAD( "midcoin-wink01.rom", 0x4000, 0x4000, CRC(acb0a392) SHA1(428c24845a27b8021823a4a930071b3b47108f01) )

	ROM_REGION( 0x10000, "audiocpu", 0 )
	ROM_LOAD( "midcoin-wink05.rom", 0x0000, 0x2000, CRC(c6c9d9cf) SHA1(99984905282c2310058d1ce93aec68d8a920b2c0) )

	ROM_REGION( 0x6000, "gfx1", 0 )
	ROM_LOAD( "midcoin-wink02.rom", 0x0000, 0x2000, CRC(d1cd9d06) SHA1(3b3ce61a0516cc94663f6d3aff3fea46aceb771f) )
	ROM_LOAD( "midcoin-wink03.rom", 0x2000, 0x2000, CRC(2346f50c) SHA1(a8535fcde0e9782ea61ad18443186fd5a6ebdc7d) )
	ROM_LOAD( "midcoin-wink04.rom", 0x4000, 0x2000, CRC(06dd229b) SHA1(9057cf10e9ec4119297c2d40b26f0ce0c1d7b86a) )
ROM_END

ROM_START( winka )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "wink0.bin",    0x0000, 0x4000, CRC(554d86e5) SHA1(bf2de874a62d9137f79063d6ca1906b1ed0c87e6) )
	ROM_LOAD( "wink1.bin",    0x4000, 0x4000, CRC(9d8ad539) SHA1(77246df8195f7e3f3b06edc08d344801bf62e1ba) )

	ROM_REGION( 0x10000, "audiocpu", 0 )
	ROM_LOAD( "wink5.bin",    0x0000, 0x2000, CRC(c6c9d9cf) SHA1(99984905282c2310058d1ce93aec68d8a920b2c0) )

	ROM_REGION( 0x6000, "gfx1", 0 )
	ROM_LOAD( "wink2.bin",    0x0000, 0x2000, CRC(d1cd9d06) SHA1(3b3ce61a0516cc94663f6d3aff3fea46aceb771f) )
	ROM_LOAD( "wink3.bin",    0x2000, 0x2000, CRC(2346f50c) SHA1(a8535fcde0e9782ea61ad18443186fd5a6ebdc7d) )
	ROM_LOAD( "wink4.bin",    0x4000, 0x2000, CRC(06dd229b) SHA1(9057cf10e9ec4119297c2d40b26f0ce0c1d7b86a) )
ROM_END

static DRIVER_INIT( wink )
{
	UINT32 i;
	UINT8 *ROM = machine.region("maincpu")->base();
	UINT8 *buffer = auto_alloc_array(machine, UINT8, 0x8000);

	// protection module reverse engineered by HIGHWAYMAN

	memcpy(buffer,ROM,0x8000);

	for (i = 0x0000; i <= 0x1fff; i++)
		ROM[i] = buffer[BITSWAP16(i,15,14,13, 11,12, 7, 9, 8,10, 6, 4, 5, 1, 2, 3, 0)];

	for (i = 0x2000; i <= 0x3fff; i++)
		ROM[i] = buffer[BITSWAP16(i,15,14,13, 10, 7,12, 9, 8,11, 6, 3, 1, 5, 2, 4, 0)];

	for (i = 0x4000; i <= 0x5fff; i++)
		ROM[i] = buffer[BITSWAP16(i,15,14,13,  7,10,11, 9, 8,12, 6, 1, 3, 4, 2, 5, 0)];

	for (i = 0x6000; i <= 0x7fff; i++)
		ROM[i] = buffer[BITSWAP16(i,15,14,13, 11,12, 7, 9, 8,10, 6, 4, 5, 1, 2, 3, 0)];

	auto_free(machine, buffer);

	for (i = 0; i < 0x8000; i++)
		ROM[i] += BITSWAP8(i & 0xff, 7,5,3,1,6,4,2,0);
}

GAME( 1985, wink,  0,    wink, wink, wink, ROT0, "Midcoin", "Wink (set 1)", GAME_IMPERFECT_SOUND | GAME_UNEMULATED_PROTECTION )
GAME( 1985, winka, wink, wink, wink, wink, ROT0, "Midcoin", "Wink (set 2)", GAME_IMPERFECT_SOUND | GAME_UNEMULATED_PROTECTION )
