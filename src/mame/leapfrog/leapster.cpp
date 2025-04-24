// license:BSD-3-Clause
// copyright-holders:David Haywood, Alice Shelton
/*
    LeapFrog - Leapster

    educational system from 2003, software is all developed in MXFlash

    hwspecs


    CPU:
      Custom ASIC (ARCTangent 5.1 CPU @ 96MHz)

    Memory:
      Leapster: 2MB onboard RAM, 256 bytes NVRAM.
      Leapster2: 16MB RAM, 128kbytes NVRAM

    Media type:
      Cartridges of 4-16MB with between 2 and 512kb NVRAM

    Graphics:
      4Mb ATI chip.

    Audio:
      Custom

    Screen:
      160x160 CSTN with touchscreen.


    The Leapster 2 also has
        USB 1.1 (client only) + full-sized SD slot.


    many magic numbers in the BIOS ROM match the [strings:VALID_FLAGS] table in
    https://github.com/tsbiberdorf/MqxSrc/blob/master/tools/tad/mqx.tad
    does this mean the System is running on the MQX RTOS?
    https://www.synopsys.com/dw/ipdir.php?ds=os_mqx_software
    indicates it was available for ARC processors

*/

/* Cartridge pinout - for games list see hash/leapster.xml

CARTRIDGE-PINOUT:
-----------------
Look at the Cartridge-Slot:

                   B        A
                  ------------
                  VCC |01| VCC
                   NC |02| VSS
                  D11 |03| D04
                  D03 |04| D12
                  D10 |05| D05
                  VSS |06| D02
                  D13 |07| D09
                   NC |08| NC
                  D06 |09| D01
                  D14 |10| D08
                  ----|--|----
                  D07 |11| VSS
                  D00 |12| D15
                 Byte |13| OE
                   NC |14| A22
                   NC |15| A23
                   CE |16| A16
                  A00 |17| A15
                  A01 |18| A14
                  A02 |19| A13
                  VSS |20| A03
                  A12 |21| A04
                  A11 |22| A05
                  A10 |23| A06
                  A09 |24| A07
                  A08 |25| A17
                  A19 |26| A18
                  A20 |27| A21
                   WE |28| VSS
PIN7 of 24LC02B <---| |29| |---> PIN7 of 24LC02B
                   NC |30| |---> PIN6 of 24LC02B
                  ----------




PCB - Handheld-Console:

               +-----------------------------+
               |                             |
  +------------|                             |------------+
  |            | C A R T R I D G E - S L O T |            |
  |            |                             |            |
  |            +-----------------------------+            |
  |                                                       |
  |ASY 310-10069    +-------------------+                 |
  |                 |                   |                 |
  |                 |                   |                 |
  |LEAPSTER MAIN    |                   |                 |
  |Leap Frog        |                 U3|                 |
  |(c) 2004         +-------------------+     +-----+     |
  |                                           |ISSI |     |
  |                                           |     |     |
  |                      +---------+          |IS42S|     |
  |                      |         |          |16100|     |
   \       +-+           | EPOXY   |          |AT-7T|    /
    \      |A|           |   BLOCK |          |     |   /
     \     +-+           |         |          |   U2|  /
      \                  |       U1|          +-----+ /
       \                 +---------+                 /
        \                                           /
         \                                         /
          \                                       /
           \                                     /
            \                                   /
             \                                 /
              \                               /
               \                             /
                \                           /
                 \-------------------------/


A = 24LC02B / SN0429


ETCHES ON THE BACK OF THE PCB:

"FAB-300-10069-C"

"702800254.01A"
"SW1208 Rev.5"



PCB - Cartridge:
FRONT:

+-------------------------------------+
| LEAPSTER ROM CARTRIDGE              |
|   +--+     +---------+    Leap Frog |
|   |B1|U2   |         |     (c) 2003 |
+-+ +--+     |E P O X Y|            +-+
  |          |       U3|            |
+-+          +---------+            +-+
|                       20232-003-1020|
| ASY 310-10028             REV:00    |
+-+                    +-+          +-+
  |||||||||||||||||||||| ||||||||||||
  +--------------------+ +----------+
 A30                                A01

B1: 24L002B

24L002B:

             +-----+
  (GND)<- A0-|     |-VCC
  (GND)<- A1-|     |-WP
  (GND)<- A2-|     |-SCL
         VSS-|     |-SDA
             +-----+



PCB - LEAPSTER-TV:

+-----------------------------------------------------------------------+
|                                                                       |
|                                                                       |
|           20300+003+0015                                              |
 \           REV:06                                                    /
  |                                                                   |
  |                                                                   |
  |                 +----+     +-------+                              |
   \                |    |     | EPOXY |                             /
    |   +----+      |    |     |       |                            |
    |   |EPOX|      |    |     |       |                            |
    |   |Y   |      |    |     |     U1|    +-+                     |
     \  +----+      |    |     +-------+    | |                    /
      |             |  U2|                  +-+U4                 |
      |             +----+                                        |
      |                     +--------------+   ROADRUNNER CONSOLE |
       \                    | AM29PL160CB  |   Leap Frog (c) 2006/
        |                   | -90SF        |   Asy 310-10378    |
        |                   |            U6|                    |
        |                   +--------------+                    |
         \                                                     /
          |          +-----------------------------+          |
          |          | C A R T R I D G E - S L O T |          |
          |          +-----------------------------+          |
           \                                                 /
            +-----------------------------------------------+

*/

#include "emu.h"

#include "bus/generic/slot.h"
#include "bus/generic/carts.h"
#include "cpu/arcompact/arcompact.h"

#include "emupal.h"
#include "screen.h"
#include "softlist_dev.h"


namespace {

class leapster_state : public driver_device
{
public:
	leapster_state(const machine_config &mconfig, device_type type, const char *tag) :
		driver_device(mconfig, type, tag),
		m_maincpu(*this, "maincpu"),
		m_cart(*this, "cartslot"),
		m_palette(*this, "palette")
		{ }

	void leapster(machine_config &config);

	void init_leapster();

private:
	virtual void machine_start() override ATTR_COLD;
	virtual void machine_reset() override ATTR_COLD;

	uint32_t screen_update_leapster(screen_device &screen, bitmap_rgb32 &bitmap, const rectangle &cliprect);
	DECLARE_DEVICE_IMAGE_LOAD_MEMBER(cart_load);

	TIMER_CALLBACK_MEMBER(leapster_timer_overflow);

	uint32_t leapster_1801000_r();
	uint32_t leapster_1801004_r();
	uint32_t leapster_1801008_r();
	uint32_t leapster_180100c_r();
	uint32_t leapster_1801018_r();
	uint32_t leapster_1809004_r();
	uint32_t leapster_180b000_r();
	uint32_t leapster_180b004_r();
	uint32_t leapster_180b008_r();
	uint32_t leapster_180d514_r();

	uint32_t leapster_180004c_r();

	void leapster_1802070_w(uint32_t data);

	uint32_t leapster_cpu_clock_r();
	void leapster_cpu_clock_w(uint32_t data);

	uint32_t leapster_adc_r(uint32_t offset);
	void leapster_adc_w(uint32_t offset, uint32_t data);
	uint32_t leapster_lcd_r(uint32_t offset);
	void leapster_lcd_w(uint32_t offset, uint32_t data);
	uint32_t leapster_dma_r(uint32_t offset);
	void leapster_dma_w(uint32_t offset, uint32_t data);
	uint32_t leapster_timer_r(uint32_t offset);
	void leapster_timer_w(uint32_t offset, uint32_t data);

	void leapster_aux0047_w(uint32_t data);
	uint32_t leapster_aux0048_r();
	void leapster_aux0048_w(uint32_t data);
	void leapster_aux004b_w(uint32_t data);

	void leapster_aux0010_w(uint32_t data);
	uint32_t leapster_aux0011_r();
	void leapster_aux0011_w(uint32_t data);
	void leapster_aux001a_w(uint32_t data);
	uint32_t leapster_aux001b_r();

	void leapster_aux(address_map &map) ATTR_COLD;
	void leapster_map(address_map &map) ATTR_COLD;

	uint16_t m_1a_data[0x800];
	int m_1a_pointer;

	uint32_t m_timer_ticks[3]{};
	uint32_t m_timer_control[3]{};
	uint32_t m_timer_max[3]{};
	emu_timer *m_overflow_timer[3]{};
	static constexpr int TIMER_IRQS[3] = {0x19, 0x1a, 0x1e};

	uint16_t m_framebuffer_base;
	uint32_t m_display_format = 0;
	uint32_t m_display_stride;

	uint32_t m_dma_src_addr;
	uint32_t m_dma_scanline_count;
	uint32_t m_dma_start_offset;
	uint32_t m_dma_stride; // Counted in words

	required_device<arcompact_device> m_maincpu;
	required_device<generic_slot_device> m_cart;
	required_device<palette_device> m_palette;

	memory_region *m_cart_rom = nullptr;
	uint32_t m_cart_bit = 0x0400'0000;
};


static INPUT_PORTS_START( leapster )
INPUT_PORTS_END

void leapster_state::leapster_aux0010_w(uint32_t data)
{
}

void leapster_state::leapster_aux0011_w(uint32_t data)
{
	// unknown, written with 1a
}

void leapster_state::leapster_aux001a_w(uint32_t data)
{
	// probably not palette, but it does load 0x1000 words of increasing value on startup, so could be?
	m_1a_data[m_1a_pointer & 0x7ff] = data;

	uint8_t r = (data >> 12) & 0x7;
	uint8_t g = (data >> 8) & 0xf;
	uint8_t b = (data >> 4) & 0xf;

	m_palette->set_pen_color(m_1a_pointer & 0x7ff, rgb_t(pal3bit(r), pal4bit(g), pal4bit(b)));
	m_1a_pointer++;
}

uint32_t leapster_state::leapster_aux0011_r()
{
	// unknown, read when 11/1a are being written
	logerror("%s: leapster_aux0011_r\n", machine().describe_context());
	return 0x00000000;
}

uint32_t leapster_state::leapster_aux001b_r()
{
	// unknown, read when 11/1a are being written
	logerror("%s: leapster_aux001b_r\n", machine().describe_context());
	return 0x00000000;
}

void leapster_state::leapster_aux0047_w(uint32_t data)
{
	logerror("%s: leapster_aux0047_w %08x\n", machine().describe_context(), data);
}

uint32_t leapster_state::leapster_aux0048_r()
{
	logerror("%s: leapster_aux0048_r\n", machine().describe_context());
	return 0x00000000;
}

void leapster_state::leapster_aux0048_w(uint32_t data)
{
	logerror("%s: leapster_aux0047_w %08x\n", machine().describe_context(), data);
}

void leapster_state::leapster_aux004b_w(uint32_t data)
{
	logerror("%s: leapster_aux004b_w %08x\n", machine().describe_context(), data);
}

uint32_t leapster_state::leapster_1801000_r()
{
	logerror("%s: leapster_1801000_r\n", machine().describe_context());
	return 0x00000000;
}

uint32_t leapster_state::leapster_1801004_r()
{
	logerror("%s: leapster_1801004_r\n", machine().describe_context());
	return 0x00000000;
}

uint32_t leapster_state::leapster_1801008_r()
{
	logerror("%s: leapster_1801004_r\n", machine().describe_context());
	return 0x00000000;
}

uint32_t leapster_state::leapster_180100c_r()
{
	logerror("%s: leapster_180100c_r\n", machine().describe_context());
	return 0x00000000;
}

uint32_t leapster_state::leapster_1801018_r()
{
	logerror("%s: leapster_1801018_r\n", machine().describe_context());
	return 0x00000000;
}

// Bits: UUUU UCSS SLDU UUUU UCUU UUUU UUUU UUUU
// C: 0 if a cartridge is present, 1 otherwise
// S: Identifies the LCD the leapster was manufactured with? On the original Leapster, 4, 6, and 7 are valid possibilites.
// L: Controls logging level?
// D: If not set, there are many places where execution infinite loops on error rather than panic. Controls debug logging level
// C: If not set, the system will boot to the touch calibration
// U: Unknown
uint32_t leapster_state::leapster_1809004_r()
{
	logerror("%s: leapster_1809004_r (return usually checked against 0x00200000)\n", machine().describe_context());
	return 0x0380'4000 | m_cart_bit;
}

uint32_t leapster_state::leapster_cpu_clock_r()
{
	return 0;
}

// Lower 3 bits are always masked for after read, but upper 29 bits never set?
void leapster_state::leapster_cpu_clock_w(uint32_t data)
{
	m_maincpu->set_unscaled_clock_int(96000000 / (1 << (data & 0x07)));
}

uint32_t leapster_state::leapster_180b000_r()
{
	logerror("%s: leapster_180b000_r\n", machine().describe_context());
	return 0x00000000;
}

uint32_t leapster_state::leapster_180b004_r()
{
	// leapster2 BIOS checks if this is 0
	logerror("%s: leapster_180b004_r\n", machine().describe_context());
	return 0xffffffff;
}

uint32_t leapster_state::leapster_180b008_r()
{
	// checks bit 1 (using BMSK instruction and AND instruction)
	// writes back to same address?
	logerror("%s: leapster_180b008_r\n", machine().describe_context());
	return 0x00000001;
}

// UART Status register (0x0180'd510) bits:
// 0x80 (R): 1 indicates ready for output
// 0x40 (W): 1 causes the current byte in the transfer register to be sent
// 0x20 (R): 0 indicates transfer register contains a received byte
// 0x04 (W): 1 sets IRQ line clear?
// 0x01..0x02 (R): Unknown, used when recieving bytes
// Interrupts are done on IRQ 0x1b when a byte is received or when the other end is ready for another transfer 
uint32_t leapster_state::leapster_180d514_r()
{
	logerror("%s: leapster_180d514_r (return usually checked against 0x0030d400)\n", machine().describe_context());
	// leapster -bios 0 does a BRNE in a loop comparing with 0x80
	return 0x000000A0;
}

// Official name: GIODataIn
uint32_t leapster_state::leapster_180004c_r()
{
	// Prevents Leapster from detecting a change in power status and shutting down
	return 0xffff'ffff;
}

// Seems to be used to commit a change to a hardware voice
// The lower 3 bits are the voice index and the upper 29 are a command
// After this data is received, the audio unit acks it by raising IRQ 0x1d
// The audio driver will wait for this before making any more voice changes
void leapster_state::leapster_1802070_w(uint32_t data)
{
	m_maincpu->set_input_line(29, ASSERT_LINE);
}

// ADC: I/O registers 0x0180'0090 - 0x0180'00ab, Drives IRQ vector 0x10
// Controls communication related to battery power level(?)
// 0x0180'00a8: Seems to be a status register. Bit 9 (0x100) seems to be a ready / received bit. 

uint32_t leapster_state::leapster_adc_r(uint32_t offset)
{
	// The BIOS busy-loops while waiting for the ready bit to be set, so it needs to be emulated
	if (offset == 6) // status
	{
		return 0x100;
	}

	logerror("%s: Unknown ADC I/O read! Addr: %08x\n", machine().describe_context(), 0x0180'0090 + (offset * 4));
	return 0;
}

void leapster_state::leapster_adc_w(uint32_t offset, uint32_t data)
{
	logerror("%s: Unknown ADC I/O write! Addr: %08x, data: %08x\n", machine().describe_context(), 0x0180'0090 + (offset * 4), data);
}

uint32_t leapster_state::leapster_lcd_r(uint32_t offset)
{
	switch (offset)
	{
		case 0: // Written once, never read
			break;
		case 1: // Framebuffer base
			return m_framebuffer_base;
		case 2: // Stride
			return m_display_stride;
		case 3: // Display format
			return m_display_format;
		case 4: // Unknown
			break;
		case 5: // Never written
			break;
		case 6: // Written once, never read
			break;
	}

	logerror("%s: Unknown LCD I/O read! Addr: %08x\n", machine().describe_context(), 0x0180'8084 + (offset * 4));

	return 0;
}

void leapster_state::leapster_lcd_w(uint32_t offset, uint32_t data)
{
	switch (offset)
	{
		case 1:
			m_framebuffer_base = data;
			return;
		case 2: // Stride
			m_display_stride = data;
			return;
		case 3: // Display format
			if (BIT(data, 31) && BIT(data, 0, 30) != 4)
			{
				fatalerror("Unimplemented display mode\n");
			}

			m_display_format = data;
			return;
	}

	logerror("%s: Unknown LCD I/O write! Addr: %08x, data: %08x\n", machine().describe_context(), 0x0180'8084 + (offset * 4), data);
}

uint32_t leapster_state::leapster_dma_r(uint32_t offset)
{
	return 0;
}

void leapster_state::leapster_dma_w(uint32_t offset, uint32_t data)
{
	switch (offset)
	{
		case 0: {
			if (data == 0x1b)
			{
				uint32_t dmaDst = 0x0300'0000 + m_framebuffer_base + m_dma_start_offset;
				uint32_t dmaSrc = m_dma_src_addr;

				for (int i = 0; i < m_dma_scanline_count; i++)
				{
					for (int j = 0; j < m_dma_stride; j++)
					{
						m_maincpu->space().write_dword(dmaDst, m_maincpu->space().read_dword(dmaSrc));
						dmaDst += 4;
						dmaSrc += 4;
					}
				}

				// IRQ 0x12 when DMA is finished
				m_maincpu->set_input_line(18, ASSERT_LINE);
			}

			return;
		}
		case 1:
			m_dma_src_addr = data;
			return;
		case 2:
			m_dma_stride = data;
			return;
		case 3:
			m_dma_scanline_count = data;
			return;
		case 4:
			m_dma_start_offset = data;
			return;
	}
}

uint32_t leapster_state::leapster_timer_r(uint32_t offset)
{
	offset *= 4;

	int index = BIT(offset, 10, 2);

	// Normalize timer 0 address to be in line with timers 1 and 2
	if (index == 0)
	{
		offset -= 0x84;
	}

	switch (BIT(offset, 2, 2))
	{
		case 0: // Tick count
			return m_timer_ticks[index] + m_overflow_timer[index]->elapsed().as_ticks(16'000'000);
		case 1: // Control register
			return m_timer_control[index];
		case 2: // Max ticks
			return m_timer_max[index];
	}

	return 0;
}

void leapster_state::leapster_timer_w(uint32_t offset, uint32_t data)
{
	offset *= 4;

	if (offset == 0x0510)
	{
		printf("%c", data);
		return;
	}

	if (offset == 0x0514)
	{
		// MQX blocks when the output queue fills, so the UART IRQ must have at least bare-bones emulation for transfer
		if (data == 0x44)
		{
			m_maincpu->set_input_line(0x1b, ASSERT_LINE);
		}

		return;
	}

	if (offset & 0x0300)
	{
		return;
	}

	int index = BIT(offset, 10, 2);

	// Normalize timer 0 address to be in line with timers 1 and 2
	if (index == 0)
	{
		offset -= 0x84;
	}

	switch (BIT(offset, 2, 2))
	{
		case 0: { // Tick count
			m_timer_ticks[index] = data;
			uint32_t ticks_until_overflow = data >= m_timer_max[index] ? 0 : m_timer_max[index] - data;
			m_overflow_timer[index]->reset(attotime::from_ticks(ticks_until_overflow, 16'000'000));
			break;
		}
		case 1: // Control register
			m_timer_control[index] = data;
			break;
		case 2: { // Max ticks
			m_timer_max[index] = data;
			m_timer_ticks[index] += m_overflow_timer[index]->elapsed().as_ticks(16'000'000);
			uint32_t ticks_until_overflow = m_timer_ticks[index] >= data ? 0 : data - m_timer_ticks[index];			
			m_overflow_timer[index]->reset(attotime::from_ticks(ticks_until_overflow, 16'000'000));
			break;
		}
	}
}

uint32_t leapster_state::screen_update_leapster(screen_device &screen, bitmap_rgb32 &bitmap, const rectangle &cliprect)
{
	if (!BIT(m_display_format, 31))
	{
		return 0;
	}

	for (int i = 0; i < 160; i++)
	{
		for (int j = 0; j < 80; j++)
		{
			uint8_t byteOne;
			uint8_t byteTwo;
			uint8_t byteThree;

			byteOne = m_maincpu->space().read_byte(0x0300'0000 + m_framebuffer_base + i * m_display_stride + j * 3);
			byteTwo = m_maincpu->space().read_byte(0x0300'0000 + m_framebuffer_base + i * m_display_stride + j * 3 + 1);
			byteThree = m_maincpu->space().read_byte(0x0300'0000 + m_framebuffer_base + i * m_display_stride + j * 3 + 2);

			bitmap.pix(i, j * 2) = pal444(byteOne | ((byteTwo & 0xF0) << 4), 8, 4, 0);
			bitmap.pix(i, j * 2 + 1) = pal444((byteThree << 4) | (byteTwo & 0x0F), 0, 8, 4);
		}
	}

	return 0;
}

DEVICE_IMAGE_LOAD_MEMBER( leapster_state::cart_load )
{
	uint32_t size = m_cart->common_get_size("rom");

	m_cart->rom_alloc(size, GENERIC_ROM32_WIDTH, ENDIANNESS_LITTLE);
	m_cart->common_load_rom(m_cart->get_rom_base(), size, "rom");

	return std::make_pair(std::error_condition(), std::string());
}

void leapster_state::machine_start()
{
	std::string region_tag;
	m_cart_rom = memregion(region_tag.assign(m_cart->tag()).append(GENERIC_ROM_REGION_TAG).c_str());

	if (m_cart_rom)
	{
		m_maincpu->space(AS_PROGRAM).install_rom(0x80000000, 0x807fffff, m_cart_rom->base());
		m_cart_bit = 0;
	}

	for (uint32_t i = 0; i < 3; i++)
	{
		m_overflow_timer[i] = timer_alloc(FUNC(leapster_state::leapster_timer_overflow), this);
		m_overflow_timer[i]->set_param(i);
	}

	save_item(NAME(m_1a_data));
}

void leapster_state::machine_reset()
{
	m_1a_pointer = 0;
	for (int i = 0; i < 0x800; i++)
		m_1a_data[i] = 0;

	for (int i = 0; i < 3; i++)
	{
		m_timer_ticks[i] = 0;
		m_timer_control[i] = 0;
		m_timer_max[i] = 0xffffffff;
		m_overflow_timer[i]->reset(attotime::from_ticks(m_timer_max[i], 16'000'000));
	}

	m_display_format = 0;
	m_display_stride = 0;
	m_framebuffer_base = 0;

	m_dma_src_addr = 0;
	m_dma_scanline_count = 0;
	m_dma_start_offset = 0;
	m_dma_stride = 0;
}

void leapster_state::leapster_map(address_map &map)
{
//  A vector table is copied from 0x00000000 to 0x3c000000, but it is unclear if that is a BIOS mirror
//  or if it should be copying a different table.
	map(0x0000'0000, 0x007f'ffff).mirror(0x40000000).rom().region("maincpu", 0);
//	map(0x4000'0000, 0x407f'ffff).rom().region("maincpu", 0);

	map(0x0180'0090, 0x0180'00ab).rw(FUNC(leapster_state::leapster_adc_r), FUNC(leapster_state::leapster_adc_w));

	map(0x01801000, 0x01801003).r(FUNC(leapster_state::leapster_1801000_r));
	map(0x01801004, 0x01801007).r(FUNC(leapster_state::leapster_1801004_r));
	map(0x01801008, 0x0180100b).r(FUNC(leapster_state::leapster_1801008_r));
	map(0x0180100c, 0x0180100f).r(FUNC(leapster_state::leapster_180100c_r));
	map(0x01801018, 0x0180101b).r(FUNC(leapster_state::leapster_1801018_r));

	map(0x0180'004c, 0x0180'004f).r(FUNC(leapster_state::leapster_180004c_r));

	map(0x0180'2070, 0x0180'2073).w(FUNC(leapster_state::leapster_1802070_w));

	map(0x0180'8084, 0x0180'809b).rw(FUNC(leapster_state::leapster_lcd_r), FUNC(leapster_state::leapster_lcd_w));

	map(0x0180'8800, 0x0180'881b).rw(FUNC(leapster_state::leapster_dma_r), FUNC(leapster_state::leapster_dma_w));

	map(0x01809004, 0x01809007).r(FUNC(leapster_state::leapster_1809004_r));
	map(0x01809008, 0x0180900b).rw(FUNC(leapster_state::leapster_cpu_clock_r), FUNC(leapster_state::leapster_cpu_clock_w));

	map(0x0180b000, 0x0180b003).r(FUNC(leapster_state::leapster_180b000_r));
	map(0x0180b004, 0x0180b007).r(FUNC(leapster_state::leapster_180b004_r));
	map(0x0180b008, 0x0180b00b).r(FUNC(leapster_state::leapster_180b008_r));

	map(0x0180'd000, 0x0180'd8ff).rw(FUNC(leapster_state::leapster_timer_r), FUNC(leapster_state::leapster_timer_w));

	map(0x0180d514, 0x0180d517).r(FUNC(leapster_state::leapster_180d514_r));

	// VRAM, also used for stack prior to MQX boot
	map(0x0300'0000, 0x0300'ffff).ram();

	// The original Leapster BIOS can write past the end of VRAM in its clear screen function (caused by asl by 2 at 0x4007'bc32)
	// This seems to be a bug (pointer arithmetic done on the wrong size?) and doesn't appear in later BIOS
	map(0x0301'0000, 0x0302'ffff).nopw();

	map(0x3c00'0000, 0x3c1f'ffff).ram(); // Main memory
	map(0x3c200000, 0x3fffffff).ram();
	// map(0x80000000, 0x807fffff).bankr("cartrom"); // game ROM pointers are all to the 80xxxxxx region, so I assume it maps here - installed if a cart is present
}


void leapster_state::leapster_aux(address_map &map)
{
	// addresses used here aren't known internal ARC addresses, so are presumed to be Leapster specific
	map(0x000000010, 0x000000010).w(FUNC(leapster_state::leapster_aux0010_w));
	map(0x000000011, 0x000000011).rw(FUNC(leapster_state::leapster_aux0011_r), FUNC(leapster_state::leapster_aux0011_w));
	map(0x00000001a, 0x00000001a).w(FUNC(leapster_state::leapster_aux001a_w));
	map(0x00000001b, 0x00000001b).r(FUNC(leapster_state::leapster_aux001b_r));

	map(0x000000047, 0x000000047).w(FUNC(leapster_state::leapster_aux0047_w));
	map(0x000000048, 0x000000048).rw(FUNC(leapster_state::leapster_aux0048_r), FUNC(leapster_state::leapster_aux0048_w));
	map(0x00000004b, 0x00000004b).w(FUNC(leapster_state::leapster_aux004b_w));
}

TIMER_CALLBACK_MEMBER(leapster_state::leapster_timer_overflow)
{
	m_timer_ticks[param] = 0;
	m_overflow_timer[param]->reset(attotime::from_ticks(m_timer_max[param], 16'000'000));

	if (BIT(m_timer_control[param], 0))
	{
		m_maincpu->set_input_line(TIMER_IRQS[param], ASSERT_LINE);
	}
}

void leapster_state::leapster(machine_config &config)
{
	// Basic machine hardware
	// CPU is ArcTangent-A5 '5.1' (ARCompact core)
	ARCA5(config, m_maincpu, 96000000);
	m_maincpu->set_addrmap(AS_PROGRAM, &leapster_state::leapster_map);
	m_maincpu->set_addrmap(AS_IO, &leapster_state::leapster_aux);
	m_maincpu->set_default_vector_base(0x40000000);

	// Video hardware
	screen_device &screen(SCREEN(config, "screen", SCREEN_TYPE_LCD));
	screen.set_refresh_hz(60);
	screen.set_size(160, 160);
	screen.set_visarea(0, 160-1, 0, 160-1);
	screen.set_screen_update(FUNC(leapster_state::screen_update_leapster));

	PALETTE(config, "palette").set_format(palette_device::xRGB_444, 0x800).set_endianness(ENDIANNESS_BIG);

	// Cartridge
	GENERIC_CARTSLOT(config, "cartslot", generic_plain_slot, "leapster_cart", "bin").set_device_load(FUNC(leapster_state::cart_load));

	// Software lists
	SOFTWARE_LIST(config, "cart_list").set_original("leapster");
}

#define ROM_LOAD_BIOS(bios,name,offset,length,hash) \
		ROMX_LOAD(name, offset, length, hash, ROM_BIOS(bios))

/* There are various build dates and revisions for different parts of the code, the date listed is the newest on in each ROM.
   This is always in the same place relative to the rest of the data.

   V2.1 sets (except TV) are apparently larger because "Learning with Leap" was built in.
*/

ROM_START(leapster)
	ROM_REGION(0x800000, "maincpu", ROMREGION_ERASE00)
	ROM_SYSTEM_BIOS( 0, "uni15", "Universal v1.5" ) // 152-10346 Leapster BaseROM Universal v1.5 - Sep 04 2003 10:46:47
	ROM_LOAD_BIOS( 0, "155-10072-a.bin"   , 0x00000, 0x200000, CRC(af05e5a0) SHA1(d4468d060543ba7e44785041093bc98bcd9afa07) )
	ROM_SYSTEM_BIOS( 1, "us21",  "USA v2.1" )       // 152-11265 Leapster BaseROM US v2.1        - Apr 13 2005 15:34:57
	ROM_LOAD_BIOS( 1, "152-11265_2.1.bin",  0x00000, 0x800000, CRC(9639b3ae) SHA1(002873b782e823c7a8159deed16c78c149f2afab) )
	ROM_SYSTEM_BIOS( 2, "uk21",  "UK v2.1" )        // 152-11452 Leapster BaseROM UK v2.1        - Aug 30 2005 16:01:46
	ROM_LOAD_BIOS( 2, "leapster2_1004.bin", 0x00000, 0x800000, CRC(b466e14d) SHA1(910c234f03e76b7de55b8aa0a0c62fd1daae4910) )
	ROM_SYSTEM_BIOS( 3, "ger21", "German v2.1" )    // 152-11435 Leapster BaseROM German v2.1    - Oct 21 2005 18:53:59
	ROM_LOAD_BIOS( 3, "leapster2_1006.bin", 0x00000, 0x800000, BAD_DUMP CRC(a69ed8ca) SHA1(e6aacba0c39b1465f344c2b07ff1cbd8a395adac) ) // BADADDR xxx-xxxxxxxxxxxxxxxxxxx
	ROM_SYSTEM_BIOS( 4, "sp10",  "Spanish v1.0" )   // 152-11546 Leapster Baserom SP v1.0        - Apr 03 2006 06:26:00
	ROM_LOAD_BIOS( 4, "leapster2_1008.bin", 0x00000, 0x800000, CRC(b43345e7) SHA1(31c27e79568115bf36e5ef668f528e3005054152) )
	ROM_DEFAULT_BIOS( "uni15" )
ROM_END

ROM_START(leapstertv)
	ROM_REGION(0x800000, "maincpu", ROMREGION_ERASE00)
	ROM_SYSTEM_BIOS( 0, "uni2111", "Universal v2.1.11" ) // 152-11594 LeapsterTv Baserom Universal.v2.1.11 - Apr 13 2006 16:36:08
	ROM_LOAD_BIOS( 0, "am29pl160cb-90sf.bin", 0x00000, 0x200000, CRC(194cc724) SHA1(000a79d75c19f2e43532ce0b31f0dca0bed49eab) )
	ROM_DEFAULT_BIOS( "uni2111" )
ROM_END

ROM_START(leapster2)
	ROM_REGION(0x800000, "maincpu", ROMREGION_ERASE00)
	ROM_SYSTEM_BIOS( 0, "2xcip3_m9", "2x CIP3 m9" ) // 152-12659 Leapster 2x CIP3 Baserom m9 - Mar 29 2011 14:13:45
	ROM_LOAD_BIOS( 0, "152-12659_m9.bin",  0x00000, 0x800000, CRC(57bde604) SHA1(4b5eaac1e40bc605eb4cf6d4ad212343334762fd) )
	ROM_SYSTEM_BIOS( 1, "2x_06",     "2x 0.6" )     // 152-12206 Leapster 2x Baserom 0.6     - Feb 02 2009 17:15:38
	ROM_LOAD_BIOS( 1, "152-12206_0.6.bin", 0x00000, 0x800000, CRC(fa94d9a7) SHA1(c5bd84146701dc4a7635b0e37adedb90747adf32) )
	ROM_SYSTEM_BIOS( 2, "connb5",  "Connected B5" )  // 152-12076 Leapster Connected Baserom B5   - Feb 29 2008 18:11:21
	ROM_LOAD_BIOS( 2, "152-12076_b5.bin",   0x00000, 0x800000, CRC(4d223022) SHA1(bdc10ad70aa7641716e16fbea16bd0ef35f6e85e) )
	ROM_DEFAULT_BIOS( "2xcip3_m9" )
ROM_END

ROM_START(leapsterlmx)
	ROM_REGION(0x800000, "maincpu", ROMREGION_ERASE00)
	ROM_SYSTEM_BIOS( 0, "lmax_2_2",    "v2.2" )     // 152-11476 LMAX Baserom v2.2    - Jan 12 2006 11:22:50
	ROM_LOAD_BIOS( 0, "152-11476_v2.2.bin",    0x00000, 0x800000, CRC(e1140475) SHA1(42089165db67005b6a0180e894ff8f36b97a081e) )
	ROM_SYSTEM_BIOS( 1, "lmax_us_2_1", "USA v2.1" ) // 152-11238 LMAX BaseROM US v2.1 - Mar 04 2005 12:01:01
	ROM_LOAD_BIOS( 1, "152-11238_us_v2.1.bin", 0x00000, 0x800000, CRC(80bb4e58) SHA1(7d8b1c23d08ce76a89cff1112957377c6a1d4b63) )
	ROM_DEFAULT_BIOS( "lmax_2_2" )
ROM_END

void leapster_state::init_leapster()
{
}

} // anonymous namespace


CONS( 2003, leapster,    0,        0, leapster, leapster, leapster_state, init_leapster, "LeapFrog", "Leapster",       MACHINE_NO_SOUND | MACHINE_NOT_WORKING )
CONS( 2005, leapstertv,  leapster, 0, leapster, leapster, leapster_state, init_leapster, "LeapFrog", "Leapster TV",    MACHINE_NO_SOUND | MACHINE_NOT_WORKING )
CONS( 2005, leapsterlmx, leapster, 0, leapster, leapster, leapster_state, init_leapster, "LeapFrog", "Leapster L-MAX", MACHINE_NO_SOUND | MACHINE_NOT_WORKING )
CONS( 2009, leapster2,   leapster, 0, leapster, leapster, leapster_state, init_leapster, "LeapFrog", "Leapster 2",     MACHINE_NO_SOUND | MACHINE_NOT_WORKING )
