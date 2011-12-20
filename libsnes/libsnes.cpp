#include "libsnes.hpp"
#include "ameteor.hpp"
#include "ameteor/cartmem.hpp"
#include <stdio.h>
#include <assert.h>

const char* snes_library_id(void) { return "Meteor/libsnes"; }

unsigned snes_library_revision_major(void) { return 1; }
unsigned snes_library_revision_minor(void) { return 3; }

snes_video_refresh_t psnes_refresh;
snes_audio_sample_t psnes_sample;
snes_input_poll_t psnes_poll;
snes_input_state_t psnes_input;
static snes_environment_t psnes_environment;

void snes_set_video_refresh(snes_video_refresh_t video_refresh) { psnes_refresh = video_refresh; }
void snes_set_audio_sample(snes_audio_sample_t audio_sample) { psnes_sample = audio_sample; }
void snes_set_input_poll(snes_input_poll_t input_poll) { psnes_poll = input_poll; }
void snes_set_input_state(snes_input_state_t input_state) { psnes_input = input_state; }
void snes_set_environment(snes_environment_t environment) { psnes_environment = environment; }

void snes_set_controller_port_device(bool, unsigned) {}

void snes_set_cartridge_basename(const char *) {}

void snes_init(void)
{
	assert(psnes_environment);

	unsigned pitch = 240 * 2;
	psnes_environment(SNES_ENVIRONMENT_SET_PITCH, &pitch);
	snes_geometry geom = { 240, 160, 240, 160 };
	psnes_environment(SNES_ENVIRONMENT_SET_GEOMETRY, &geom);

	snes_system_timing timing = { 16777216.0 / 280896.0, 44100.0 };
	psnes_environment(SNES_ENVIRONMENT_SET_TIMING, &timing);

	AMeteor::CartMem::ClearMemory();
}

void snes_power(void) { AMeteor::Reset(AMeteor::UNIT_ALL ^ AMeteor::UNIT_MEMORY_BIOS); }
void snes_reset(void) { snes_power(); }
void snes_term(void) { snes_power(); }

static bool first_run = true;
void snes_run(void)
{
	if (first_run)
	{
		AMeteor::_memory.LoadCartInferred();
		first_run = false;
	}

	psnes_poll();
	AMeteor::Run(10000000);
}

static unsigned serialize_size;
unsigned snes_serialize_size(void)
{
	if (first_run)
	{
		AMeteor::_memory.LoadCartInferred();
		first_run = false;
	}

	AMeteor::omemstream stream(0);
	AMeteor::SaveState(stream);
	serialize_size = stream.size();
	return serialize_size;
}

bool snes_serialize(uint8_t *data, unsigned size)
{
	if (!serialize_size)
		snes_serialize_size();

	if (size != serialize_size)
		return false;

	AMeteor::omemstream stream(data);
	AMeteor::SaveState(stream);
	assert(size == stream.size());

	return true;
}

bool snes_unserialize(const uint8_t *data, unsigned size)
{
	if (!serialize_size)
		snes_serialize_size();

	if (size != serialize_size)
		return false;

	AMeteor::imemstream stream(data);
	AMeteor::LoadState(stream);

	return true;
}

void snes_cheat_reset(void) {}
void snes_cheat_set(unsigned, bool, const char *) {}

bool snes_load_cartridge_normal(
		const char *, const uint8_t *data, unsigned size
		)
{
	if (!AMeteor::_memory.LoadRom(data, size))
		return false;

	return true;
}

bool snes_load_cartridge_bsx_slotted(
		const char *, const uint8_t *, unsigned,
		const char *, const uint8_t *, unsigned
		)
{
	return false;
}

bool snes_load_cartridge_bsx(
		const char *, const uint8_t *, unsigned,
		const char *, const uint8_t *, unsigned
		)
{
	return false;
}

bool snes_load_cartridge_sufami_turbo(
		const char *, const uint8_t *, unsigned,
		const char *, const uint8_t *, unsigned,
		const char *, const uint8_t *, unsigned
		)
{
	return false;
}

bool snes_load_cartridge_super_game_boy(
		const char *, const uint8_t *, unsigned,
		const char *, const uint8_t *, unsigned
		)
{
	return false;
}

void snes_unload_cartridge(void) {}

bool snes_get_region(void) { return false; }

uint8_t* snes_get_memory_data(unsigned id)
{
	if (id != SNES_MEMORY_CARTRIDGE_RAM)
		return 0;

	return AMeteor::CartMem::MemoryBuffer();
}

unsigned snes_get_memory_size(unsigned id)
{
	if (id != SNES_MEMORY_CARTRIDGE_RAM)
		return 0;

	switch (AMeteor::_memory.GetCartType())
	{
		case AMeteor::Memory::CTYPE_UNKNOWN:
		case AMeteor::Memory::CTYPE_FLASH128:
			return 0x20000;
		case AMeteor::Memory::CTYPE_EEPROM512:
			return 512;
		case AMeteor::Memory::CTYPE_EEPROM8192:
			return 8192;
		case AMeteor::Memory::CTYPE_FLASH64:
			return 0x10000;
		case AMeteor::Memory::CTYPE_SRAM:
			return 0x8000;

		default:
			return 0;
	}
}
