// license:BSD-3-Clause
// copyright-holders:David Haywood, Alice Shelton

// The Zevio SoC was developed by Koto Laboratory, the same company behind the Wonderswan
// is it related to Ponto-1 in epoch_tv_globe.cpp, as Koto is credited there too
//   Also used in the V.Flash / V.Smile Pro
#include "emu.h"

#include "cpu/arm7/arm7.h"

#include "screen.h"
#include "speaker.h"
#include <video/poly.h>

#define ZEVIO_DUMP_GPU  0
#define ZEVIO_DUMP_APU  0
#define ZEVIO_DUMP_INTC 1

#if ZEVIO_DUMP_GPU == 1
	#define ZEVIO_GPU_DEBUG(...) printf(__VA_ARGS__)
#else
	#define ZEVIO_GPU_DEBUG(...) snprintf(NULL, 0, __VA_ARGS__);
#endif

#if ZEVIO_DUMP_APU == 1
	#define ZEVIO_APU_DEBUG(...) printf(__VA_ARGS__)
#else
	#define ZEVIO_APU_DEBUG(...) snprintf(NULL, 0, __VA_ARGS__);;
#endif

#if ZEVIO_DUMP_INTC == 1
	#define ZEVIO_INTC_DEBUG(...) printf(__VA_ARGS__)
#else
	#define ZEVIO_INTC_DEBUG(...) snprintf(NULL, 0, __VA_ARGS__);;
#endif

namespace {

struct zevio_vertex {
	float x, y;
	float u, v;
	rgb_t diffuse;
};

class zevio_state;

class zevio_renderer : public poly_manager<float, zevio_vertex, 2>
{
	friend class zevio_state;

public:
	zevio_renderer(zevio_state &state);
	void draw_verts(uint8_t vertexType, int count, zevio_vertex *verts);

private:
	zevio_state &m_state;

	void render_callback(int32_t y, extent_t const &extent, zevio_vertex const &object, int threadid);
};

class zevio_state : public driver_device
{
	friend class zevio_renderer;

public:
	zevio_state(const machine_config &mconfig, device_type type, const char *tag)
		: driver_device(mconfig, type, tag)
		, m_maincpu(*this, "maincpu")
		, m_screen(*this, "screen")
	{ }

	void zevio(machine_config &config);

protected:
	virtual void machine_start() override ATTR_COLD;
	virtual void machine_reset() override ATTR_COLD;

private:
	void check_interrupts(void)
	{
//		printf("Ints: %08X\n", m_pendingInterrupts);

		if (m_pendingInterrupts & m_interruptMasks[0])
		{
//			printf("Asserting IRQ\n");
			m_maincpu->set_input_line(arm7_cpu_device::ARM7_IRQ_LINE, HOLD_LINE);
		}
		else
		{
			m_maincpu->set_input_line(arm7_cpu_device::ARM7_IRQ_LINE, CLEAR_LINE);
		}

		if (m_pendingInterrupts & m_interruptMasks[1])
		{
//			printf("Asserting FIQ\n");
			m_maincpu->set_input_line(arm7_cpu_device::ARM7_FIRQ_LINE, HOLD_LINE);
		}
		else
		{
			m_maincpu->set_input_line(arm7_cpu_device::ARM7_FIRQ_LINE, CLEAR_LINE);
		}
	}

	// Zevio passes some of its parameters to the GPU using a 1.6.9 half float format
	static float conv_zevio_half(uint16_t zf)
	{
	    if(zf == 0) {
	        return 0.0f;
	    }

	    int exponent = ((zf >> 9) & 0x3F) - 31;
	    int mantissa = zf & 0x1FF;

	    float f = 1 + (mantissa / 512.0f);

	    f = ldexpf(f, exponent);

	    if(zf & 0x8000) {
	        f = -f;
	    }

	    return f;
	}

	required_device<cpu_device> m_maincpu;
	required_device<screen_device> m_screen;

	std::unique_ptr<zevio_renderer> m_renderer;

	uint32_t m_commandStreamPtr;
	uint32_t m_pendingInterrupts;
	uint32_t m_interruptMasks[2];

	uint32_t m_vramOffset;
	uint32_t m_paletteOffset;
	uint32_t m_textureOffset;
	uint16_t m_textureWidth;
	uint16_t m_textureHeight;
	uint8_t m_textureBPP;
	bool m_textureIs4444;

	uint32_t screen_update(screen_device &screen, bitmap_rgb32 &bitmap, const rectangle &cliprect);

	INTERRUPT_GEN_MEMBER(vblank);

	void process_verts(uint8_t type);

	void za8000000(uint32_t val);
	uint32_t za8000014();
	void zGPUCommandStream_w(uint32_t val);

	uint32_t z900b0014_r();

	void zAPUWrite(offs_t offset, uint32_t val);

	uint32_t zb8000004_r();
	uint32_t zb8000024_r();

	uint32_t z900X00XX_r(offs_t offset);

	uint32_t irq_controller_r(offs_t offset);
	void irq_controller_w(offs_t offset, uint32_t val);

	void arm_map(address_map &map) ATTR_COLD;
};

zevio_renderer::zevio_renderer(zevio_state &state)
	: poly_manager(state.machine())
	, m_state(state)
{ }

void zevio_renderer::draw_verts(uint8_t vertexType, int count, zevio_vertex *verts)
{
	vertex_t v[4];

	// Diffuse-only
	if(!(vertexType & 0x01))
	{
		return;
	}

	for(int i = 0; i < 4; i++)
	{
		v[i].x = verts[i].x;
		v[i].y = verts[i].y;
		v[i].p[0] = verts[i].u;
		v[i].p[1] = verts[i].v;
	}

	rectangle const &clipRect = rectangle(0, 320, 0, 240);

	render_triangle_strip<2>(clipRect, render_delegate(&zevio_renderer::render_callback, this), 4, v);

	wait();
}

void zevio_renderer::render_callback(int32_t y, extent_t const &extent, zevio_vertex const &object, int threadid)
{
//	printf("Y: %i, (%i, %i)\n", y, extent.startx, extent.stopx);
//	uint16_t widthReal = m_state.m_textureWidth

	uint32_t paletteBase = m_state.m_vramOffset + m_state.m_paletteOffset;
	uint32_t textureBase = m_state.m_vramOffset + m_state.m_textureOffset;

	float currU = extent.param[0].start;
	float currV = extent.param[1].start;

	for(int i = extent.startx; i < extent.stopx; i++)
	{
		auto texelY = (int) (currV * m_state.m_textureHeight);
		texelY = std::clamp(texelY, 0, m_state.m_textureHeight - 1);

		auto texelX = (int) (currU * m_state.m_textureWidth);
		texelX = std::clamp(texelX, 0, m_state.m_textureWidth - 1);

		uint8_t sourceIndex = m_state.m_maincpu->space().read_byte(textureBase + texelY * 2048 + texelX);
		uint16_t color = m_state.m_maincpu->space().read_word(paletteBase + sourceIndex * 2);

		if(m_state.m_textureIs4444)
		{
			auto fragmentColor = argbexpand<4, 4, 4, 4>(color, 12, 0, 4, 8);
			auto backbufferColor = pal555(m_state.m_maincpu->space().read_word(m_state.m_vramOffset + y * 2048 + i * 2), 0, 5, 10);

			fragmentColor = alpha_blend_r32(backbufferColor, fragmentColor, fragmentColor.a());

			color = ((fragmentColor.b() >> 3) << 10) | ((fragmentColor.g() >> 3) << 5) | (fragmentColor.r() >> 3);
		}
		else if(color & 0x8000)
		{
			color = m_state.m_maincpu->space().read_word(m_state.m_vramOffset + y * 2048 + i);
		}

		m_state.m_maincpu->space().write_word(m_state.m_vramOffset + (y * 2048) + (i * 2), color);

		currU += extent.param[0].dpdx;
		currV += extent.param[1].dpdx;
	}
}

uint32_t zevio_state::screen_update(screen_device &screen, bitmap_rgb32 &bitmap, const rectangle &cliprect)
{
	for(int i = 0; i < bitmap.height(); i++)
	{
		for(int j = 0; j < bitmap.width(); j++)
		{
			uint16_t rawPixel = m_maincpu->space().read_word(m_vramOffset + i * 2048 + j * 2);
			bitmap.pix(i, j) = pal555(rawPixel, 0, 5, 10);
		}
	}

//	bitmap.rowpixels()
	return 0;
}

void zevio_state::machine_start()
{
	m_renderer = std::make_unique<zevio_renderer>(*this);
}

void zevio_state::machine_reset()
{
	m_pendingInterrupts = 0;
	m_interruptMasks[0] = 0;
	m_interruptMasks[1] = 0;

	m_vramOffset = 0x3000'0000;
}

void zevio_state::process_verts(uint8_t type)
{
	ZEVIO_GPU_DEBUG("Verts with type %02hhX\n", type);

	zevio_vertex verts[4];

	if((type & 0x01) == 0)
	{
		type |= 0x04;
	}

	for(int i = 0; i < 4; i++)
	{
		ZEVIO_GPU_DEBUG("Vert %i:\n", i);

		verts[i].x = conv_zevio_half(m_maincpu->space().read_word(m_commandStreamPtr + 4)) + 160.0f;
		verts[i].y = conv_zevio_half(m_maincpu->space().read_word(m_commandStreamPtr + 6)) + 120.0f;

		ZEVIO_GPU_DEBUG("  w(?): %f\n  x: %f\n  y: %f\n", conv_zevio_half(m_maincpu->space().read_word(m_commandStreamPtr)),
											     verts[i].x, verts[i].y);

		m_commandStreamPtr += 8;

		if(type & 0x01)
		{
			verts[i].u = conv_zevio_half(m_maincpu->space().read_word(m_commandStreamPtr));
			verts[i].v = conv_zevio_half(m_maincpu->space().read_word(m_commandStreamPtr + 2));

			ZEVIO_GPU_DEBUG("  u: %f\n  v: %f\n", verts[i].u, verts[i].v);

			m_commandStreamPtr += 4;
		}

		if(type & 0x04)
		{
			uint16_t diffuse = m_maincpu->space().read_word(m_commandStreamPtr);

			verts[i].diffuse = diffuse;

			ZEVIO_GPU_DEBUG("  diffuse: %04hX\n", m_maincpu->space().read_word(m_commandStreamPtr));
			m_commandStreamPtr += 4;
		}

		if(type & 0x08)
		{
			ZEVIO_GPU_DEBUG("  ???: %04hX\n", m_maincpu->space().read_word(m_commandStreamPtr));
			m_commandStreamPtr += 4;
		}
	}

	m_renderer->draw_verts(type, 4, verts);
}

// Stat interrupt?
uint32_t zevio_state::zb8000004_r()
{
	return 2;
}

uint32_t zevio_state::z900b0014_r()
{
	return machine().rand();
}

uint32_t zevio_state::z900X00XX_r(offs_t offset)
{
//	printf("Off: %X\n", offset);

	switch(offset >> 4)
	{
		case 0:
			return 1;
		case 1:
			return 2;
	}

	return 0;
}

void zevio_state::za8000000(uint32_t val)
{
	if(val != 1)
	{
		return;
	}

	m_pendingInterrupts |= 0x100;
	check_interrupts();

	while(1)
	{
		uint32_t command = m_maincpu->space().read_dword(m_commandStreamPtr);

		m_commandStreamPtr += 4;

		// For some of these the number of additional 16-bit arguments is the lower byte
		//   of the command. Unfortunately that's not a consistent rule

		switch(command >> 24)
		{
			case 0x08: // Stream jump
				m_commandStreamPtr = m_maincpu->space().read_dword(m_commandStreamPtr);
				break;
			case 0x10:
				ZEVIO_GPU_DEBUG("0x10: %08X\n", m_maincpu->space().read_dword(m_commandStreamPtr));
				m_commandStreamPtr += 4;

				break;
			case 0x14:
				ZEVIO_GPU_DEBUG("Terminal!\n");
				return;
			case 0x1C:
				ZEVIO_GPU_DEBUG("0x1C: %06X\n", command & 0xFFFFFF);
				break;
			case 0x1E:
				ZEVIO_GPU_DEBUG("0x1E: %06X\n", command & 0xFFFFFF);
				break;
			case 0x20:
				ZEVIO_GPU_DEBUG("0x20, %02hhX, ", (command >> 16) & 0xFF);

				for(int i = 0; i < (command & 0x0F); i++) {
					ZEVIO_GPU_DEBUG("0x%04hX, ", m_maincpu->space().read_word(m_commandStreamPtr));
					m_commandStreamPtr += 2;
				}

				m_commandStreamPtr = (m_commandStreamPtr + 2) & (~3);

				ZEVIO_GPU_DEBUG("\n");
				break;
			case 0x81:
				ZEVIO_GPU_DEBUG("0x81: %03X, %03X\n", BIT(command, 0, 12), BIT(command, 12, 12));
				break;
			case 0x82:
				ZEVIO_GPU_DEBUG("0x82: %03X, %03X\n", BIT(command, 0, 12), BIT(command, 12, 12));
				break;
			case 0x83:
				ZEVIO_GPU_DEBUG("0x83: %03X, %03X\n", BIT(command, 0, 12), BIT(command, 12, 12));
				break;
			case 0x84:
				ZEVIO_GPU_DEBUG("0x84: %06X\n", BIT(command, 0, 24));
				break;
			case 0x85:
				ZEVIO_GPU_DEBUG("0x85: (%i, %i)\n", BIT(command, 0, 12), BIT(command, 12, 12));
				break;
			case 0x86: {
				uint16_t width = 8 << BIT(command, 0, 3);
				uint16_t height = 8 << BIT(command, 3, 3);
				uint8_t bpp = 16 >> BIT(command, 9, 2);
				uint8_t textureFormat = BIT(command, 8);

				m_textureWidth = width;
				m_textureHeight = height;
				m_textureBPP = bpp;
				m_textureIs4444 = textureFormat == 1;

				// Bit 18 is unconditionally set if texture format is ARGB4444
				//   Enables transparency maybe?

				ZEVIO_GPU_DEBUG("Set Texture Params (0x86): Raw: %08X\n", command);

				command &= ~(0x8600'073F);

				ZEVIO_GPU_DEBUG("  Size %ix%i\n  BPP: %i\n  Format: %i\n  Unk: %08X\n",
					   width, height, bpp, textureFormat, command);
				break;
			}
			case 0x87:
				m_textureOffset = BIT(command, 12, 12) * 2048 + BIT(command, 0, 12) * 2;

				ZEVIO_GPU_DEBUG("Set Texture (0x87): (%i, %i)\n", (command & 0xFFF) * 2, (command >> 12) & 0xFFF);
				break;
			case 0x88:
				ZEVIO_GPU_DEBUG("0x88: %06X\n", command & 0xFFFFFF);
				break;
			case 0x89:
				ZEVIO_GPU_DEBUG("0x89\n");
				break;
			case 0x8A:
				ZEVIO_GPU_DEBUG("0x8A: %06X\n", command & 0xFFFFFF);
				break;
			case 0x8B:
				ZEVIO_GPU_DEBUG("0x8B: %06X\n", command & 0xFFFFFF);
				break;
			case 0x8C:
				ZEVIO_GPU_DEBUG("0x8C: %06X\n", command & 0xFFFFFF);
				break;
			case 0x8D:
				ZEVIO_GPU_DEBUG("0x8D: %06X\n", command & 0xFFFFFF);
				break;
			case 0x8F:
				m_paletteOffset = BIT(command, 12, 12) * 2048 + BIT(command, 0, 12) * 2;

				ZEVIO_GPU_DEBUG("Set Palette (0x8F): (%i, %i)\n", (command & 0xFFF) * 2, (command >> 12) & 0xFFF);
				break;
			case 0x96: // Flush?????
				break;
			case 0xC0: // Vertices
				process_verts((command >> 16) & 0xFF);
				m_commandStreamPtr += 4;

				break;
			case 0xD0: { // DMA
				uint32_t unk = m_maincpu->space().read_dword(m_commandStreamPtr);
				uint16_t vramColumn = m_maincpu->space().read_word(m_commandStreamPtr);
				uint16_t vramRow = m_maincpu->space().read_word(m_commandStreamPtr + 2);
				uint16_t rows = m_maincpu->space().read_word(m_commandStreamPtr + 6);
				uint16_t stride = m_maincpu->space().read_word(m_commandStreamPtr + 4);
				uint32_t transferCount = m_maincpu->space().read_dword(m_commandStreamPtr + 8);
				uint32_t source = m_maincpu->space().read_dword(m_commandStreamPtr + 12);
				// One more dword, which is always 0x95000000. This might be a separate flush command?
				m_commandStreamPtr += 20;

				// Scanlines in VRAM always have a stride of 2048 bytes

				ZEVIO_GPU_DEBUG("DMA: %08X:\n  VRAM location: (%i, %i) (%08X)\n  Rows: %i\n  Stride: %i\n  Data Transfer: %06X\n"
					   "  Source: %08X\n", command, vramColumn * 2, vramRow, unk, rows, stride * 2, transferCount, source);

				for(int i = 0; i < rows; i++)
				{
					for(int j = 0; j < stride; j++)
					{
						uint16_t srcWord = m_maincpu->space().read_word(source + (i * stride + j) * 2);
						m_maincpu->space().write_word(m_vramOffset + (vramRow + i) * 2048 + (vramColumn + j) * 2, srcWord);
					}
				}

//				for(uint32_t i = 0; i < transferCount; i++) {
//					m_maincpu->space().write_dword(vramRow * 2048 + vramColumn * 2)
//				}

				break;
			}
			case 0xD3: { // Color blit
				uint16_t xLow = m_maincpu->space().read_word(m_commandStreamPtr);
				uint16_t yLow = m_maincpu->space().read_word(m_commandStreamPtr + 2);
				uint16_t xHigh = m_maincpu->space().read_word(m_commandStreamPtr + 4);
				uint16_t yHigh = m_maincpu->space().read_word(m_commandStreamPtr + 6);
				uint16_t color = m_maincpu->space().read_word(m_commandStreamPtr + 8);
				m_commandStreamPtr += 12;

				ZEVIO_GPU_DEBUG("Color blit: Set (%i, %i)::(%i, %i) to %04hX\n", xLow, yLow, xHigh, yHigh, color);
				break;
			}
			default:
				ZEVIO_GPU_DEBUG("Unknown command %08X\n", command);
				return;
		}
	}
}

// IRQ 3 checks the second byte to be 0xFD and bit 3 to be set before
//   giving the GPU its next command stream. This value lines up with the second byte
//   of the stream end marker (0x1400'FD01)
uint32_t zevio_state::za8000014()
{
//	printf("Reading X14\n");
	return 0x0000'FD08;
}

void zevio_state::zGPUCommandStream_w(uint32_t val)
{
	m_commandStreamPtr = val;
}

void zevio_state::zAPUWrite(offs_t offset, uint32_t val)
{
	ZEVIO_APU_DEBUG("APU: %08X: Write %08X to %08X\n", m_maincpu->pc(), val, 0xB000'0000 + (offset << 2));
}

uint32_t zevio_state::zb8000024_r()
{
	return machine().rand();
}

// 0x0XX and 0x1XX seem to have identical register sets, with 0x0XX affecting IRQs
//   and 0x1XX affecting FIQs
//  00: Current pending interrupts
//  04: Written to clear an interrupt?
//  08: IRQ source mask?
//  0C: ???

uint32_t zevio_state::irq_controller_r(offs_t offset)
{
//	printf("%08X: Read %08X\n", m_maincpu->pc(), offset << 2);

	offset <<= 2;

	if(offset < 0x200)
	{
		if((offset & 0xFF) == 0x00)
		{
			return m_pendingInterrupts & m_interruptMasks[offset >> 8];
		}
	}

	return 0;
}

// V.Flash BIOS refers to functions that set 08 and 0C as "Set Int" and "Set Int Disable" respectively
//   08 doesn't *seem* to be write-only (a debug function in DBZ Scouter reads it, though whatever log
//   function it was seemingly intended for was stripped out), so I'm not sure why the hardware registers
//   are set up this way instead of just using a single R/W mask register.

void zevio_state::irq_controller_w(offs_t offset, uint32_t val)
{
	offset <<= 2;

	if(offset < 0x200)
	{
		if((offset & 0xFF) == 0x04)
		{
			m_pendingInterrupts &= ~val;
			check_interrupts();
		}
		else if((offset & 0xFF) == 0x08)
		{
			m_interruptMasks[offset >> 8] |= val;
			check_interrupts();
		}
		else if((offset & 0xFF) == 0x0C)
		{
			m_interruptMasks[offset >> 8] &= ~val;
			check_interrupts();
		}
	}

	ZEVIO_INTC_DEBUG("%08X: Write %08X to %X\n", m_maincpu->pc(), val, offset);
}

INTERRUPT_GEN_MEMBER(zevio_state::vblank)
{
	m_pendingInterrupts |= 0x200000;
	check_interrupts();
}

static INPUT_PORTS_START( zevio )
INPUT_PORTS_END

void zevio_state::arm_map(address_map &map)
{
	map(0x00000000, 0x007fffff).rom().region("maincpu", 0);

	map(0x1000'0000, 0x10ff'ffff).ram();

	map(0x3000'0000, 0x3fff'ffff).ram();

	map(0x900a0f04, 0x900a0f07).nopw();
	map(0x900b0014, 0x900b0017).r(FUNC(zevio_state::z900b0014_r));

	map(0x900D'0018, 0x900D'001B).select(0xC0).r(FUNC(zevio_state::z900X00XX_r));

	map(0xa800'0000, 0xa800'0003).w(FUNC(zevio_state::za8000000));
	map(0xa800'0014, 0xa800'0017).r(FUNC(zevio_state::za8000014));
	map(0xa800'0028, 0xa800'002b).w(FUNC(zevio_state::zGPUCommandStream_w));

	map(0xb000'0000, 0xb000'ffff).w(FUNC(zevio_state::zAPUWrite));

	map(0xb800'0000, 0xb800'0003).r(FUNC(zevio_state::zb8000004_r));
	map(0xb800'0004, 0xb800'0007).r(FUNC(zevio_state::zb8000004_r));
	map(0xb800'0024, 0xb800'0027).r(FUNC(zevio_state::zb8000024_r));

	map(0xb8000800, 0xb8000fff).ram();

	map(0xdc00'0000, 0xdc00'0fff).rw(FUNC(zevio_state::irq_controller_r), FUNC(zevio_state::irq_controller_w));
}


void zevio_state::zevio(machine_config &config)
{
	ARM9(config, m_maincpu, 72000000); // unknown ARM core, unknown frequency
	m_maincpu->set_addrmap(AS_PROGRAM, &zevio_state::arm_map);

	SCREEN(config, m_screen, SCREEN_TYPE_RASTER);
	m_screen->set_refresh_hz(60);
	m_screen->set_size(320, 262);
	m_screen->set_visarea(0, 320-1, 0, 240-1);
	m_screen->set_screen_update(FUNC(zevio_state::screen_update));

	m_maincpu->set_vblank_int("screen", FUNC(zevio_state::vblank));

	SPEAKER(config, "lspeaker").front_left();
	SPEAKER(config, "rspeaker").front_right();
}


// ドラゴンボールＺ スカウターバトル体感かめはめ波 おらとおめえとスカウター
ROM_START( dbzscout )
	ROM_REGION( 0x800000, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD( "mr27t6402l.ic6", 0x000000, 0x800000, CRC(9cb896d6) SHA1(4185ee4593c2ef3b637f6004d1f80dadd4530902) )
ROM_END

ROM_START( dbzonep )
	ROM_REGION( 0x800000, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD( "mr27t6402l.u1", 0x000000, 0x800000, CRC(57f7c319) SHA1(65118a9c61defc75cefe5e45062c0a4788e2a26c) )
	// original dump had the first 0x100 bytes repeated again at the end, why?

	ROM_REGION( 0x800, "eeprom", ROMREGION_ERASEFF )
	ROM_LOAD( "s24cs16a.u6", 0x000000, 0x800, CRC(a1724ea8) SHA1(93a6f73e30f47b6a0c83f62dfd9d8236473518a8) )
ROM_END

} // anonymous namespace

CONS( 2007, dbzscout,     0,              0,      zevio, zevio, zevio_state, empty_init, "Bandai / Koto", "Dragon Ball Z: Scouter Battle Taikan Kamehameha: Ora to Omee to Scouter (Japan)", MACHINE_NO_SOUND | MACHINE_NOT_WORKING )
CONS( 2008, dbzonep,      0,              0,      zevio, zevio, zevio_state, empty_init, "Bandai / Koto", "Dragon Ball Z x One Piece: Battle Taikan Gum-Gum no Kamehameha: Omee no Koe de Ora o Yobu (Japan)", MACHINE_NO_SOUND | MACHINE_NOT_WORKING )
