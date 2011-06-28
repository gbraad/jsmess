/**********************************************************************

    COMX-35 expansion bus emulation

    Copyright MESS Team.
    Visit http://mamedev.org for licensing and usage restrictions.

**********************************************************************

					GND		 1		A		GND
					NC		 2		B		NC (DS)
					+5V		 3		C		V+
					D0		 4		D		D1
					D2		 5		E		D3
					D4		 6		F		D5
					D6		 7		H		D7
					NC (DP)	 8		J		Q
					_CLEAR	 9		K		_MRD
					TPA		10		L		N0
					N1		11		M		N2
					_RAS	12		N		_INT
					_WAIT	13		P		CLOCK
					SC1		14		R		SC0
					_EF4	15		S		_CASE
					TPB		16		T		_A15
					_MWR	17		U		A14
					MA7		18		V		_A14
					MA5		19		W		MA6
					MA4		20		X		MA3
					MA2		21		Y		_EXTROM
					MA1		22		Z		MA0

**********************************************************************/

#pragma once

#ifndef __COMX35_EXPANSION_BUS__
#define __COMX35_EXPANSION_BUS__

#include "emu.h"



//**************************************************************************
//  CONSTANTS
//**************************************************************************

#define COMX_EXPANSION_BUS_TAG		"comxexp"



//**************************************************************************
//  INTERFACE CONFIGURATION MACROS
//**************************************************************************

#define MCFG_COMX_EXPANSION_BUS_ADD(_cpu_tag, _clock, _config) \
    MCFG_DEVICE_ADD(COMX_EXPANSION_BUS_TAG, COMX_EXPANSION_BUS, _clock) \
    MCFG_DEVICE_CONFIG(_config)


#define COMX_EXPANSION_INTERFACE(_name) \
	const comx_expansion_bus_interface (_name) =


#define MCFG_COMX_EXPANSION_SLOT_ADD(_tag, _slot_intf, _def_slot) \
    MCFG_DEVICE_ADD(_tag, COMX_EXPANSION_SLOT, 0) \
	MCFG_DEVICE_SLOT_INTERFACE(_slot_intf, _def_slot)



//**************************************************************************
//  TYPE DEFINITIONS
//**************************************************************************

// ======================> comx_expansion_slot_device

class comx_expansion_bus_device;

class comx_expansion_slot_device : public device_t,
								   public device_slot_interface
{
public:
	// construction/destruction
	comx_expansion_slot_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock);

	// device-level overrides
	virtual void device_start();

    // inline configuration
    static void static_set_slot(device_t &device, const char *tag);

private:
	// configuration
	comx_expansion_bus_device  *m_bus;
};


// device type definition
extern const device_type COMX_EXPANSION_SLOT;


// ======================> comx_expansion_bus_interface

struct comx_expansion_bus_interface
{
    devcb_write_line	m_out_int_cb;
    devcb_write_line	m_out_ef4_cb;
    devcb_write_line	m_out_wait_cb;
    devcb_write_line	m_out_clear_cb;
    devcb_write_line	m_out_extrom_cb;
};

class device_comx_expansion_card_interface;


// ======================> comx_expansion_bus_device

class comx_expansion_bus_device : public device_t,
								  public comx_expansion_bus_interface
{
public:
	// construction/destruction
	comx_expansion_bus_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock);

	// inline configuration
	void add_card(device_comx_expansion_card_interface *card);

	DECLARE_READ8_MEMBER( mrd_r );
	DECLARE_WRITE8_MEMBER( mwr_w );

	DECLARE_READ8_MEMBER( io_r );
	DECLARE_WRITE8_MEMBER( io_w );

	DECLARE_WRITE_LINE_MEMBER( q_w );

	DECLARE_WRITE_LINE_MEMBER( int_w );
	DECLARE_WRITE_LINE_MEMBER( ef4_w );
	DECLARE_WRITE_LINE_MEMBER( wait_w );
	DECLARE_WRITE_LINE_MEMBER( clear_w );
	DECLARE_WRITE_LINE_MEMBER( extrom_w );

	bool screen_update(screen_device &screen, bitmap_t &bitmap, const rectangle &cliprect);

protected:
	// device-level overrides
	virtual void device_start();
	virtual void device_reset();
	virtual void device_config_complete();

private:
	// internal state
	devcb_resolved_write_line	m_out_int_func;
	devcb_resolved_write_line	m_out_ef4_func;
	devcb_resolved_write_line	m_out_wait_func;
	devcb_resolved_write_line	m_out_clear_func;
	devcb_resolved_write_line	m_out_extrom_func;

	device_comx_expansion_card_interface *m_card;
};


// device type definition
extern const device_type COMX_EXPANSION_BUS;


// ======================> device_comx_expansion_card_interface

// class representing interface-specific live comx_expansion card
class device_comx_expansion_card_interface : public device_interface
{
	friend class comx_expansion_bus_device;

public:
	// construction/destruction
	device_comx_expansion_card_interface(const machine_config &mconfig, device_t &device);
	virtual ~device_comx_expansion_card_interface();

	// interrupts
	virtual void comx_q_w(int state) { };

	// memory access
	virtual UINT8 comx_mrd_r(offs_t offset) { return 0; };
	virtual void comx_mwr_w(offs_t offset, UINT8 data) { };

	// I/O access
	virtual UINT8 comx_io_r(offs_t offset) { return 0; };
	virtual void comx_io_w(offs_t offset, UINT8 data) { };

	// video
	virtual bool comx_screen_update(screen_device &screen, bitmap_t &bitmap, const rectangle &cliprect) { return false; }

public:
	comx_expansion_bus_device  *m_bus;
};

#endif
