//__BIGPEMU_SCRIPT_MODULE__
//__BIGPEMU_META_DESC__    "Unlimited lives, health, ammo, in Pitfall."
//__BIGPEMU_META_AUTHOR__  "jguff"

#include "bigpcrt.h"

// Found using BigPEmu System->Load Software screen and highlighting ROM file.
#define PITFALL_HASH             0x10B52759119EF17AULL

// Found using Pro Action Replay like script.
#define PITFALL_LIVES_ADDR       0x00043FF5
#define PITFALL_STONES_ADDR      0x00043FF6
#define PITFALL_BOOMERANGS_ADDR  0x00043FF7
#define PITFALL_BOMBS_ADDR       0x00043FF8
#define PITFALL_LIVES_VALUE      5
#define PITFALL_AMMO_VALUE       5

// Found using Pro Action Replay like script that identifies addresses of memory which always decrease.
// Two locations in memory were found, and they both seem to update in sync, for whatever reason.
// But those two locations didn't seem to impact gameplay.
#define PITFALL_HEALTHA_ADDR     0x000424C1
#define PITFALL_HEALTHB_ADDR     0x000424DD

// Then placed breakpoints on HEALTHA_ADDR and HEALTHB_ADDR.
// Noticed HEALTHB updated, then HEALTHB updated by 68K.
// Tracing backwards, saw values copied into HEALTHA and HEALTHB
// go through 1-2 divide instructions, and source for divides was
// at HEALTHC_ADDR.
//
// Noesis Memory Window analysis, confirmed HEALTH being incremented,
// but at larger increments, so HEALTHC is factor greater then
// HEALTHA and HEALTHB.
//
// Suspect difficulty level selected at startup could be at play here.
// Division factors could vary, based on difficulty level.
// Suspect animated aligator life bar (aligators mouth opens wider
// and wider (not just moving left to right)) could be cause of this too.
// HEALTHA and/or HEALTHB might have a range of values that better matches
// the alligator life bar animation.
//
// And now looking back, see the results of Pro Action Replay like
// script did identify this location as well.  Had looked at all 4
// locations it identified to confirm whether they were increasing
// after each hit, but somehow must have failed to analyze this
// address.  Oh well, infinite health is working now.
#define PITFALL_HEALTHC_ADDR     0x000435E2

// Health meter seems to count from 0 (full life) to around 0x1E.
// 0x1C is still alive.  First enemy encountered (snake) takes 0x02
// off each bite.  So at 29 or 30, player dies.
#define PITFALL_HEALTH_VALUE     0

#define INVALID                  -1

// Boilerplate.
static int32_t sOnLoadEvent = INVALID;
static int32_t sOnEmuFrame = INVALID;
static int32_t sMcFlags = 0;
static const int32_t skMcFlag_Loaded = (1 << 0);

// Setting handles.
static int sLivesSettingHandle = INVALID;
static int sHealthSettingHandle = INVALID;
static int sAmmoSettingHandle = INVALID;

static uint32_t on_sw_loaded(const int eventHandle, void* pEventData)
{
   const uint64_t hash = bigpemu_get_loaded_fnv1a64();

   sMcFlags = 0;

   if (PITFALL_HASH == hash)
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
      int livesSettingValue = INVALID;
      int healthSettingValue = INVALID;
      int ammoSettingValue = INVALID;
      bigpemu_get_setting_value(&livesSettingValue, sLivesSettingHandle);
      bigpemu_get_setting_value(&healthSettingValue, sHealthSettingHandle);
      bigpemu_get_setting_value(&ammoSettingValue, sAmmoSettingHandle);

      if (livesSettingValue > 0)
      {
         bigpemu_jag_write8(PITFALL_LIVES_ADDR, PITFALL_LIVES_VALUE);
      }

      if (healthSettingValue > 0)
      {
         bigpemu_jag_write8(PITFALL_HEALTHC_ADDR, PITFALL_HEALTH_VALUE);
      }

      if (ammoSettingValue > 0)
      {
         bigpemu_jag_write8(PITFALL_STONES_ADDR, PITFALL_AMMO_VALUE);
         bigpemu_jag_write8(PITFALL_BOOMERANGS_ADDR, PITFALL_AMMO_VALUE);
         bigpemu_jag_write8(PITFALL_BOMBS_ADDR, PITFALL_AMMO_VALUE);
      }
   }
   return 0;
}

void bigp_init()
{
   void* pMod = bigpemu_get_module_handle();

   bigpemu_set_module_usage_flags(pMod, BIGPEMU_MODUSAGE_DETERMINISMWARNING);

   const int catHandle = bigpemu_register_setting_category(pMod, "Pitfall");
   sLivesSettingHandle = bigpemu_register_setting_bool(catHandle, "Infinite Lives", 1);
   sHealthSettingHandle = bigpemu_register_setting_bool(catHandle, "Infinite Health", 1);
   sAmmoSettingHandle = bigpemu_register_setting_bool(catHandle, "Infinite Ammo", 1);

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

   sLivesSettingHandle = INVALID;
   sHealthSettingHandle = INVALID;
   sAmmoSettingHandle = INVALID;
}