//__BIGPEMU_SCRIPT_MODULE__
//__BIGPEMU_META_DESC__    "Infinite lives and health, unlock powers and levels, in Rayman."
//__BIGPEMU_META_AUTHOR__  "jguff"

#include "bigpcrt.h"

// Found using BigPEmu System->Load Software screen and highlighting ROM file.
#define RAYMAN_HASH           0x4F7E323A69447A71ULL

#define RAYMAN_LIVES_ADDR     0x00006C60
#define RAYMAN_LIVES_VALUE    8

// Each sphere of health seems to be represented by 0x10.
// Couldn't find this using Pro Action Replay counter value script, cause
// values were 30,20,10, instead of 3,2,1, or 2,1,0.
// Instead found num lives memory, using Pro Action Replay style
// script, and then used Noesis Memory Window to inspect changes in
// memory nearby as collected blue spheres and finally got hit by enemies.
#define RAYMAN_HEALTH_ADDR    0x00006C62
#define RAYMAN_BLUE_SPHERES   0x00006C61
#define RAYMAN_HEALTH_VALUE   48

#define INVALID               -1

// Boilerplate.
static int32_t sOnLoadEvent = INVALID;
static int32_t sOnEmuFrame = INVALID;
static int32_t sMcFlags = 0;
static const int32_t skMcFlag_Loaded = (1 << 0);

static int sLivesSettingHandle = INVALID;
static int sHealthSettingHandle = INVALID;
static int sLevelsSettingHandle = INVALID;
static int sPowersSettingHandle = INVALID;

static uint32_t on_sw_loaded(const int eventHandle, void* pEventData)
{
   const uint64_t hash = bigpemu_get_loaded_fnv1a64();

   sMcFlags = 0;

   if (RAYMAN_HASH == hash)
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
      int levelsSettingValue = INVALID;
      int powersSettingValue = INVALID;
      bigpemu_get_setting_value(&livesSettingValue, sLivesSettingHandle);
      bigpemu_get_setting_value(&healthSettingValue, sHealthSettingHandle);
      bigpemu_get_setting_value(&levelsSettingValue, sLevelsSettingHandle);
      bigpemu_get_setting_value(&powersSettingValue, sPowersSettingHandle);

      if (livesSettingValue > 0)
      {
         // Possibly need to disable infinite lives to allow saving game progress.
         bigpemu_jag_write8(RAYMAN_LIVES_ADDR, RAYMAN_LIVES_VALUE);
      }

      if (healthSettingValue > 0)
      {
         // (128 decimal = 0x80 = should have been 8 health spheres, but
         // instead displayed like a hollow circle of pink that was likely
         // intentional by developer)
         //
         // (96 decimal = 0x60 - should have been 6 health spheres, but
         // displayed as 2 health spheres, which i really don't understand
         //
         // Setting to 48 decimal = 0x30 = 3 health spheres = same as how game
         // normally starts up as.
         bigpemu_jag_write8(RAYMAN_HEALTH_ADDR, RAYMAN_HEALTH_VALUE);
      }

      if (levelsSettingValue > 0)
      {
         // All levels unlocked.
         // Sourced from https://emucheats.emulation64.com/rayman-world/
         bigpemu_jag_write8(0x00006C2B, 0xFF);
         bigpemu_jag_write8(0x00006C31, 0xFF);
         bigpemu_jag_write8(0x00006C37, 0xFF);
         bigpemu_jag_write8(0x00006C3B, 0xFF);
         bigpemu_jag_write8(0x00006C43, 0x0F);
      }
      if (powersSettingValue > 0)
      {
         // Helicoptor and throw punch powers.
         // Sourced from https://emucheats.emulation64.com/rayman-world/, probably derived from 1,3,5,7,9 joypad cheat.
         bigpemu_jag_write8(0x00006C42, 0x23);
      }
   }
   return 0;
}

void bigp_init()
{
   void* pMod = bigpemu_get_module_handle();

   bigpemu_set_module_usage_flags(pMod, BIGPEMU_MODUSAGE_DETERMINISMWARNING);

   // Register cheat settings with BigPEmu GUI interface.
   const int catHandle = bigpemu_register_setting_category(pMod, "Rayman");
   sLivesSettingHandle = bigpemu_register_setting_bool(catHandle, "Infinite Lives", 1);
   sHealthSettingHandle = bigpemu_register_setting_bool(catHandle, "Invinite Health", 1);
   sLevelsSettingHandle = bigpemu_register_setting_bool(catHandle, "Unlock All Levels", 1);
   sPowersSettingHandle = bigpemu_register_setting_bool(catHandle, "Unlock Helicoptor and Throw Punch", 1);

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
   sLevelsSettingHandle = INVALID;
   sPowersSettingHandle = INVALID;
}