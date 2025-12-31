//__BIGPEMU_SCRIPT_MODULE__
//__BIGPEMU_META_DESC__    "Infinite health without infinite loop possibility, in Fight For Life."
//__BIGPEMU_META_AUTHOR__  "jguff"

#include "bigpcrt.h"

// Found using BigPEmu System->Load Software screen and highlighting ROM file.
#define FIGHT4LIFE_HASH                0xC56B6F168A80E5EEULL

// Found using a Pro Action Replay like script that finds addresses of always decreasing values.
// Script came back with 3 results. Inspection from Noesis Memory Window, showed all three were
// getting updated each time player hit and updated with same value.  Unsure why 3 copies.
//
// The following instruction from source code, of some version of the game, indicates
// that health is represented in one byte (but with 3 leading 0 bytes):
//    move.l   #255,energy_p1
// That matched what saw in memory at start of match.
#define FIGHT4LIFE_HEALTHA_ADDR        0x000F77FF
#define FIGHT4LIFE_HEALTHB_ADDR        0x000F7807
#define FIGHT4LIFE_HEALTHC_ADDR        0x001D890B
#define FIGHT4LIFE_HEALTH_VALUE        255

// Found using a Pro Action Replay like script that finds addresses of always decreasing values.
#define FIGHT4LIFE_VOLUME_ADDR   0x0016EBCC

// Found counting bytes backwards from volume addr, per FYBMAIN.S
#define FIGHT4LIFE_ELECTR_ADDR   0x0016EB37

// NTSC
#define FIGHT4LIFE_FRAMES_PER_SECOND   60

#define FIGHT4LIFE_DEFAULT_NUM_SECONDS 30
#define FIGHT4LIFE_MAX_NUM_SECONDS     60

#define INVALID                        -1

// Boilerplate.
static int32_t sOnLoadEvent = INVALID;
static int32_t sOnEmuFrame = INVALID;
static int32_t sMcFlags = 0;
static const int32_t skMcFlag_Loaded = (1 << 0);

static uint64_t lastUpdateFrameCount = 0;

// Setting handles.
static int sHealthSettingHandle = INVALID;

static uint32_t on_sw_loaded(const int eventHandle, void* pEventData)
{
   const uint64_t hash = bigpemu_get_loaded_fnv1a64();

   sMcFlags = 0;

   if (FIGHT4LIFE_HASH == hash)
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
      bigpemu_get_setting_value(&healthSettingValue, sHealthSettingHandle );

      if (healthSettingValue > 0)
      {
         // Just write to all 3 locations.  Unsure if all 3 need
         // to be written to.
         //
         // BTW: Do not let your player get electrocuted, or else
         // you are stuck in infinite loop of being electrocuted.
         // Apparently the electrocution never stops, until player's
         // health goes down to zero, and this script prevents
         // players health ever going much below 100%.
         //
         // Another side effect, is: when you get hit, your health
         // bar doesn't display.  Not important in this case.
         //
         // Limited testing: Verified that can transition from
         // first round to second round.  Probably can transition
         // from one match to next match, but didn't try.
         // Did not see a score system,, so likely shouldn't be
         // a problem.
         // (This approach will likely fail with fighting games
         // that tally/decrement remaining life into total score
         // at the end of match/round.  Failure would be infinite
         // loop.)

         // Only max out health, if not being electrocuted
         uint8_t electrocuting = bigpemu_jag_read8(FIGHT4LIFE_ELECTR_ADDR);
         if (0 == electrocuting)
         {
            bigpemu_jag_write8(FIGHT4LIFE_HEALTHA_ADDR, FIGHT4LIFE_HEALTH_VALUE);
            bigpemu_jag_write8(FIGHT4LIFE_HEALTHB_ADDR, FIGHT4LIFE_HEALTH_VALUE);
            bigpemu_jag_write8(FIGHT4LIFE_HEALTHC_ADDR, FIGHT4LIFE_HEALTH_VALUE);
         }
      }
   }
   return 0;
}

void bigp_init()
{
   void* pMod = bigpemu_get_module_handle();

   bigpemu_set_module_usage_flags(pMod, BIGPEMU_MODUSAGE_DETERMINISMWARNING);

   // Register cheat settings with BigPEmu GUI interface.
   const int catHandle = bigpemu_register_setting_category(pMod, "Fight For Life");
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