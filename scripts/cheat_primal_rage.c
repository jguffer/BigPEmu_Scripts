//__BIGPEMU_SCRIPT_MODULE__
//__BIGPEMU_META_DESC__    "Infinite health, in Primal Rage."
//__BIGPEMU_META_AUTHOR__  "jguff"

#include "bigpcrt.h"

// Found using BigPEmu System->Load Software screen and highlighting ROM file.
#define PRIMAL_RAGE_HASH         0xB658FD728CB60822ULL

// Found the below address using a Pro Action Replay like script that finds addresses
// of always increasing values.  (Decreasing values search came up with 3 hits which
// did not pan out.)
//
// Maybe 40 addresses matched increasing.  Way too many addresses.  Found 2 that were
// reasonably closer to the Time address found the prior day.  One of those seemed like it
// was in sync with player getting hit.  Noesis Fill Memory impacted the displayed life
// bar immediately, but then the next frame the displayed life bar went back to where
// it was before.  Placed write breakpoint on that address, and then saw the 68K code
// that was updating that address, and saw where it was being copied from (actually not
// a straightforward copy), but that source address is what is defined below.
#define PRIMAL_RAGE_HEALTH_ADDR  0x001FF116
#define PRIMAL_RAGE_HEALTH_VALUE 0

// Brain Stem Bar - don't see much value in locating/updating this.
//
// From the manual: "This yellow bar is located directly below the Life Blood Bar and 
// indicates your dino's brain strength.  When the yellow bar is depleted, your dino
// will appear dazed and vulnerable - but not beaten.  You can snap out of the daze
// by moving the D-Pad quickly Left and Right, or when your opponent nails you with
// one of the various moves."

#define INVALID                  -1

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

   if (PRIMAL_RAGE_HASH == hash)
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

      if (healthSettingValue)
      {
         bigpemu_jag_write16(PRIMAL_RAGE_HEALTH_ADDR, PRIMAL_RAGE_HEALTH_VALUE);
      }

   }
   return 0;
}

void bigp_init()
{
   void* pMod = bigpemu_get_module_handle();

   bigpemu_set_module_usage_flags(pMod, BIGPEMU_MODUSAGE_DETERMINISMWARNING);

   const int catHandle = bigpemu_register_setting_category(pMod, "Primal Rage");
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