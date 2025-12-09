//__BIGPEMU_SCRIPT_MODULE__
//__BIGPEMU_META_DESC__		"Infinite health, in Kasumi Ninja."
//__BIGPEMU_META_AUTHOR__	"jguff"

#include "bigpcrt.h"

// Found using BigPEmu System->Load Software screen and highlighting ROM file.
#define KASUMI_NINJA_HASH		0x392906008D83777EULL

// Found the below address using a Pro Action Replay like script that finds addresses
// of always decreasing values.
#define KASUMI_NINJA_ENERGY_ADDR	0x000060B6

// Boilerplate.
static int32_t sOnLoadEvent = -1;
static int32_t sOnEmuFrame = -1;
static int32_t sMcFlags = 0;
static const int32_t skMcFlag_Loaded = (1 << 0);

// Setting handles.
static int sHealthSettingHandle = -1;

static uint32_t on_sw_loaded(const int eventHandle, void *pEventData)
{
	const uint64_t hash = bigpemu_get_loaded_fnv1a64();
	
	sMcFlags = 0;
	
	if (KASUMI_NINJA_HASH == hash)
	{
		sMcFlags |= skMcFlag_Loaded;
	}
	return 0;
}

static uint32_t on_emu_frame(const int eventHandle, void *pEventData)
{
	if (sMcFlags & skMcFlag_Loaded)
	{
		// Get setting values.  Doing this once per frame, allows player
		// to modify setting during game session.
		int healthSettingValue = -1;
		bigpemu_get_setting_value(&healthSettingValue, sHealthSettingHandle);

		if (healthSettingValue > 0)
		{
			// Full energy.
			bigpemu_jag_write16(KASUMI_NINJA_ENERGY_ADDR, 0xFFFF);
		}
	}
	return 0;
}

void bigp_init()
{
	void *pMod = bigpemu_get_module_handle();
	
	bigpemu_set_module_usage_flags(pMod, BIGPEMU_MODUSAGE_DETERMINISMWARNING);
	
	// Register cheat settings with BigPEmu GUI interface.
	const int catHandle = bigpemu_register_setting_category(pMod, "Kasumi Ninja");
	sHealthSettingHandle = bigpemu_register_setting_bool(catHandle, "Infinite Health", 1);

	sOnLoadEvent = bigpemu_register_event_sw_loaded(pMod, on_sw_loaded);
	sOnEmuFrame = bigpemu_register_event_emu_thread_frame(pMod, on_emu_frame);
}

void bigp_shutdown()
{
	void *pMod = bigpemu_get_module_handle();
	bigpemu_unregister_event(pMod, sOnLoadEvent);
	sOnLoadEvent = -1;
	bigpemu_unregister_event(pMod, sOnEmuFrame);
	sOnEmuFrame = -1;

	sHealthSettingHandle = -1;

}
