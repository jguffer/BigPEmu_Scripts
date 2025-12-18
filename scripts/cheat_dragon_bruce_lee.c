//__BIGPEMU_SCRIPT_MODULE__
//__BIGPEMU_META_DESC__       "Infinite health and constant chi, in Double Dragon."
//__BIGPEMU_META_AUTHOR__     "jguff"

#include "bigpcrt.h"

// Found using BigPEmu System->Load Software screen and highlighting ROM file.
#define DRAGON_BRUCE_HASH        0x1D72B9ED4D0094F2ULL

// Found the below addresses using Pro Action Replay like script that finds addresses
// of always decreasing values.  A and B always had the same values.  C and D always had
// same values.  A and B were always higher (likely a factor) of C and D.
// Using Noesis Fill Memory feature, noted writing to A impacted player health bar, but
// writing to B, C, and D did not impact player health bar.
//     A) 0x4102
//     B) 0x4b52
//     C) 0x4194
//     D) 0x4198
#define DRAGON_BRUCE_HEALTH_ADDR    0x00004102

#define DRAGON_BRUCE_CHI_ADDR       0x0000413A
#define DRAGON_BRUCE_BAR_VALUE      0x0200

// NTSC
#define DRAGON_FRAMES_PER_SECOND    60

#define DRAGON_NUM_SECONDS          5
#define DRAGON_DEFAULT_NUM_SECONDS  30
#define DRAGON_MAX_NUM_SECONDS      60

#define INVALID                     -1

// Boilerplate.
static int32_t sOnLoadEvent = INVALID;
static int32_t sOnEmuFrame = INVALID;
static int32_t sMcFlags = 0;
static const int32_t skMcFlag_Loaded = (1 << 0);

static uint64_t lastUpdateFrameCount = 0;

// Setting handles.
static int sHealthSettingHandle = INVALID;
static int sChiSettingHandle = INVALID;

static uint32_t on_sw_loaded(const int eventHandle, void* pEventData)
{
   const uint64_t hash = bigpemu_get_loaded_fnv1a64();

   sMcFlags = 0;

   if (DRAGON_BRUCE_HASH == hash)
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
      int chiSettingValue = INVALID;
      bigpemu_get_setting_value(&healthSettingValue, sHealthSettingHandle);
      bigpemu_get_setting_value(&chiSettingValue, sChiSettingHandle);

      if (healthSettingValue > 0)
      {
         // Only max out health every n seconds.
         //
         // Updating every frame, causes an infinite loop at end-of-round,
         // since end-of-round decrements health down to zero as part
         // of score tally.
         //
         // N seconds is a workaround to allow the vote tally to fully
         // decrement health, but still allow health to continually get
         // maxed out.  Player just needs to make sure they don't get
         // their health bar completely drained within n number of seconds.
         const const uint64_t newFrameCount = bigpemu_jag_get_frame_count();
         if (newFrameCount > (lastUpdateFrameCount + (DRAGON_FRAMES_PER_SECOND * DRAGON_NUM_SECONDS)))
         {
            lastUpdateFrameCount = newFrameCount;

            bigpemu_jag_write16(DRAGON_BRUCE_HEALTH_ADDR, DRAGON_BRUCE_BAR_VALUE);
         }
      }
      if (chiSettingValue > 0)
      {
         // With chi >= 50%, press '2' or '5', to enter Fighter Mode.
         // With chi >= 75%, press '3' or '6', to enter Nunchaku Mode.
         // Press '1' or '4', to return to starting Mantis Mode.
         // Player should really read the manual for this game, or
         // player will likely miss out on major features of game.
         bigpemu_jag_write16(DRAGON_BRUCE_CHI_ADDR, DRAGON_BRUCE_BAR_VALUE);
      }
   }
   return 0;
}

void bigp_init()
{
   void* pMod = bigpemu_get_module_handle();

   bigpemu_set_module_usage_flags(pMod, BIGPEMU_MODUSAGE_DETERMINISMWARNING);

   // Register cheat settings with BigPEmu GUI interface.
   const int catHandle = bigpemu_register_setting_category(pMod, "Dragon Bruce Lee");
   sHealthSettingHandle = bigpemu_register_setting_bool(catHandle, "Infinite Health", 1);
   sChiSettingHandle = bigpemu_register_setting_bool(catHandle, "Infinite Chi", 1);

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
   sChiSettingHandle = INVALID;
}