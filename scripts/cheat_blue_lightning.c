//__BIGPEMU_SCRIPT_MODULE__
//__BIGPEMU_META_DESC__    "All jets unlocked, infinite jets, health and ammo, in Blue Lightning."
//__BIGPEMU_META_AUTHOR__  "jguff"

#include "bigpcrt.h"

// Found using BigPEmu System->Load Software screen and highlighting ROM file.
#define BLIGHTNING__HASH                  0xAC96D5C484853AEAULL

// Found using Pro Action Replay like script.
#define BLIGHTNING_MISSILES_ADDR          0x0005D843
#define BLIGHTNING_SPECIAL_WEAPONS_ADDR   0x0005D849
#define BLIGHTNING_HEALTH_ADDR            0x0005D823
#define BLIGHTNING_MISSILES_VALUE         5
#define BLIGHTNING_SPECIAL_WPNS_VALUE     3
#define BLIGHTNING_HEALTH_VALUE           0

// 0x00 is available.
// 0x01 is not available
// possibly 0xFF if shot down and not replaced.
#define BLIGHTNING_JET_ADDR               0x0005EC72
#define BLIGHTNING_JET_AVAIL_VALUE        0

// 0x0005D742 is likely mission score, 4 bytes. 1 byte for every 2 digits, such that screen display score of 43850 is stored as 0x00043850

// Appears to be no reason to chase down unlocking the third bomb/special-weapon,
// as the third is not really unlocked.  It isn't available on Australia Tour
// Mission 2, is available on Australia Tour Mission 3, isn't available
// again on Australia Tour Mission 4, etc.
//
// Bombs/special weapons... There are three types of bombs/special-weapons:
//    Cluster
//    Napalm
//    SLAMR
//
// There are three slots for the different types of bombs.
//
// Cluster and napalm bombs are unlocked very early in the game.
// Skimming a three hour youtube playthrough of the game, shows SLAMR bombs
// are only available on a handful of missions, and are the only bomb/
// special/weapon for that mission.
//
// Skipping to end of 3 hour youtube playthrough, viewer could be mistaken
// in believing the youtuber just didn't unlock the slamr bombs.  Skimming
// closer over the video, you see slamrs availability is sprinkled through
// handful of missions.
//
// The game never fills all 3 bomb/special weapon slots up.  Two slots
// are filled at maximum.

#define INVALID   -1

// Boilerplate.
static int32_t sOnLoadEvent = INVALID;
static int32_t sOnEmuFrame = INVALID;
static int32_t sMcFlags = 0;
static const int32_t skMcFlag_Loaded = (1 << 0);

// Setting handles.
static int sHealthSettingHandle = INVALID;
static int sMissilesSettingHandle = INVALID;
static int sSpecialWeaponsSettingHandle = INVALID;
static int sJetsSettingHandle = INVALID;

static uint32_t on_sw_loaded(const int eventHandle, void* pEventData)
{
   const uint64_t hash = bigpemu_get_loaded_fnv1a64();

   sMcFlags = 0;

   if (BLIGHTNING__HASH == hash)
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
      int missilesSettingValue = INVALID;
      int specialWeaponsSettingValue = INVALID;
      int jetsSettingValue = INVALID;
      bigpemu_get_setting_value(&healthSettingValue, sHealthSettingHandle);
      bigpemu_get_setting_value(&missilesSettingValue, sMissilesSettingHandle);
      bigpemu_get_setting_value(&specialWeaponsSettingValue, sSpecialWeaponsSettingHandle);
      bigpemu_get_setting_value(&jetsSettingValue, sJetsSettingHandle);

      if (healthSettingValue > 0)
      {
         bigpemu_jag_write8(BLIGHTNING_HEALTH_ADDR, BLIGHTNING_HEALTH_VALUE);
      }
      if (missilesSettingValue > 0)
      {
         bigpemu_jag_write8(BLIGHTNING_MISSILES_ADDR, BLIGHTNING_MISSILES_VALUE);
      }
      if (specialWeaponsSettingValue > 0)
      {
         bigpemu_jag_write8(BLIGHTNING_SPECIAL_WEAPONS_ADDR, BLIGHTNING_SPECIAL_WPNS_VALUE);
      }
      if (sJetsSettingHandle > 0)
      {
         // Unlock all 7 jets referenced in game manual, plus an 8th jet not
         // referenced in game manual.  Believe without this script, the
         // 8th jet gets awarded, if you have gotten all jets unlocked, none of
         // the 7 jets is unavailable (crashed), and player surpasses points
         // amount to be awarded another/replacement jet.
         uint32_t jetAddress = BLIGHTNING_JET_ADDR;
         for (uint32_t jetIdx = 0; jetIdx < 8; ++jetIdx)
         {
            jetAddress += 2;
            bigpemu_jag_write16(jetAddress, BLIGHTNING_JET_AVAIL_VALUE);
         }
         jetAddress += 2;
      }
   }
   return 0;
}

void bigp_init()
{
   void* pMod = bigpemu_get_module_handle();

   bigpemu_set_module_usage_flags(pMod, BIGPEMU_MODUSAGE_DETERMINISMWARNING);

   // Register cheat settings with BigPEmu GUI interface.
   const int catHandle = bigpemu_register_setting_category(pMod, "Blue Lightning");
   sHealthSettingHandle = bigpemu_register_setting_bool(catHandle, "Infinite Health", 1);
   sMissilesSettingHandle = bigpemu_register_setting_bool(catHandle, "Infinite Missiles", 1);
   sSpecialWeaponsSettingHandle = bigpemu_register_setting_bool(catHandle, "Infinite SpecialWeapons", 1);
   sJetsSettingHandle = bigpemu_register_setting_bool(catHandle, "All Jets & Infinite Supply", 1);

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
   sMissilesSettingHandle = INVALID;
   sSpecialWeaponsSettingHandle = INVALID;
   sJetsSettingHandle = INVALID;
}