//__BIGPEMU_SCRIPT_MODULE__
//__BIGPEMU_META_DESC__    "Infinite health, in Ultra Vortek."
//__BIGPEMU_META_AUTHOR__  "jguff"

#include "bigpcrt.h"

// Found using BigPEmu System->Load Software screen and highlighting ROM file.
#define ULTRA_VORTEK_HASH           0x13773A13560CA917ULL

// Found the below address using a Pro Action Replay like script that finds addresses
// of always decreasing values.
#define ULTRA_VORTEK_HEALTH_ADDR    0x00005776
#define ULTRA_VORTEK_HEALTH_VALUE   0x19

// The below address also updates at same time and same value as above.  Using
// Noesis Fill Memory functionality, appeared it controlled the Heads Up Display
// (HUD) display of health.  But likely gets overwritten every frame by the
// value from above.
//     0x0000577A

#define INVALID	-1

// Boilerplate.
static int32_t sOnLoadEvent = INVALID;
static int32_t sOnEmuFrame = INVALID;
static int32_t sMcFlags = 0;
static const int32_t skMcFlag_Loaded = (1 << 0);

// Setting handles.
static int sHealthSettingHandle = INVALID;

static uint32_t on_sw_loaded(const int eventHandle, void* pEventData)
{
   const uint64_t hash = bigpemu_get_loaded_fnv1a64();

   sMcFlags = 0;

   if (ULTRA_VORTEK_HASH == hash)
   {
      sMcFlags |= skMcFlag_Loaded;
   }
   return 0;
}

static uint32_t on_emu_frame(const int eventHandle, void* pEventData)
{
   if (sMcFlags & skMcFlag_Loaded)
   {
      // Get setting values.  Doing this once per frame, allows player
      // to modify setting during game session.
      int healthSettingValue = INVALID;
      bigpemu_get_setting_value(&healthSettingValue, sHealthSettingHandle);

      if (healthSettingValue > 0)
      {
         bigpemu_jag_write16(ULTRA_VORTEK_HEALTH_ADDR, ULTRA_VORTEK_HEALTH_VALUE);
      }
   }
   return 0;
}

void bigp_init()
{
   void* pMod = bigpemu_get_module_handle();

   bigpemu_set_module_usage_flags(pMod, BIGPEMU_MODUSAGE_DETERMINISMWARNING);

   const int catHandle = bigpemu_register_setting_category(pMod, "Ultra Vortek");
   sHealthSettingHandle = bigpemu_register_setting_bool(catHandle, "Infinite Health", 1);

   sOnLoadEvent = bigpemu_register_event_sw_loaded(pMod, on_sw_loaded);
   sOnEmuFrame = bigpemu_register_event_emu_thread_frame(pMod, on_emu_frame);
}

void bigp_shutdown()
{
   void* pMod = bigpemu_get_module_handle();
   bigpemu_unregister_event(pMod, sOnLoadEvent);
   sOnLoadEvent = INVALID;
   bigpemu_unregister_event(pMod, sOnEmuFrame);
   sOnEmuFrame = INVALID;

   sHealthSettingHandle = INVALID;
}