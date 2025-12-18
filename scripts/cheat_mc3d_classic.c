//__BIGPEMU_SCRIPT_MODULE__
//__BIGPEMU_META_DESC__    "Infinite missiles, city health and launcher health, in Classic mode of Missile Command 3D."
//__BIGPEMU_META_AUTHOR__  "jguff"

#include "bigpcrt.h"

// Found using BigPEmu System->Load Software screen and highlighting ROM file.
#define MC3D_HASH	                              0x8A324BEEC06140ABULL

// Found using Pro Action Replay like descending counter value search script.
#define MC3D_CLASSIC_LEFT_MISSILE_ADDR          0x00011B57
#define MC3D_CLASSIC_CENTER_MISSILE_ADDR        0x00011B5B
#define MC3D_CLASSIC_RIGHT_MISSILE_ADDR         0x00011B5F
#define MC3D_CLASSIC_DEFAULT_NUM_MISSILES       10
#define MC3D_MAX_NUM_MISSILES                   99

// Found via memory inspection, using Noesis Memory Window, near the
// above addresses.
#define MC3D_CLASSIC_NUM_CITIES_REMAIN_ADDR     0x00011AEF
#define MC3D_CLASSIC_CITIES_BIT_STATUS_ADDR     0x00011AEB
#define MC3D_CLASSIC_LAUNCHERS_BIT_STATUS_ADDR  0x00011B53
#define MC3D_CLASSIC_NUM_CITIES                 6
#define MC3D_CLASSIC_CITIES_BIT_STATUS_VALUE    0x3F
#define MC3D_CLASSIC_LAUNCHERS_BIT_STATUS_VALUE 0x07

#define INVALID                                 -1

// Boilerplate.
static int32_t sOnLoadEvent = INVALID;
static int32_t sOnEmuFrame = INVALID;
static int32_t sMcFlags = 0;
static const int32_t skMcFlag_Loaded = (1 << 0);

// Setting handles.
static int sMissilesSettingHandle = INVALID;
static int sCitiesSettingHandle = INVALID;
static int sLaunchersSettingHandle = INVALID;

static uint32_t on_sw_loaded(const int eventHandle, void* pEventData)
{
   const uint64_t hash = bigpemu_get_loaded_fnv1a64();

   sMcFlags = 0;

   if (MC3D_HASH == hash)
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
      int missilesSettingValue = INVALID;
      int citiesSettingValue = INVALID;
      int launchersSettingValue = INVALID;
      bigpemu_get_setting_value(&missilesSettingValue, sMissilesSettingHandle);
      bigpemu_get_setting_value(&citiesSettingValue, sCitiesSettingHandle);
      bigpemu_get_setting_value(&launchersSettingValue, sLaunchersSettingHandle);

      if (missilesSettingValue > 0)
      {
         // If set number of missiles every frame, an infinite loop is entered
         // at end of level/wave, when tallying up missiles to add to score.
         // So only increase the number of missiles, when each silo contains the
         // default/start number of missiles per silo.
         const uint8_t numLeftMissiles = bigpemu_jag_read8(MC3D_CLASSIC_LEFT_MISSILE_ADDR);
         const uint8_t numCenterMissiles = bigpemu_jag_read8(MC3D_CLASSIC_CENTER_MISSILE_ADDR);
         const uint8_t numRightMissiles = bigpemu_jag_read8(MC3D_CLASSIC_RIGHT_MISSILE_ADDR);
         if ((MC3D_CLASSIC_DEFAULT_NUM_MISSILES == numLeftMissiles) &&
             (MC3D_CLASSIC_DEFAULT_NUM_MISSILES == numCenterMissiles) &&
             (MC3D_CLASSIC_DEFAULT_NUM_MISSILES == numRightMissiles))
         {
            // MC3D_MAX_NUM_MISSILES makes the missile launchers look a little weird,
            // but oh well...
            bigpemu_jag_write8(MC3D_CLASSIC_LEFT_MISSILE_ADDR, MC3D_MAX_NUM_MISSILES);
            bigpemu_jag_write8(MC3D_CLASSIC_CENTER_MISSILE_ADDR, MC3D_MAX_NUM_MISSILES);
            bigpemu_jag_write8(MC3D_CLASSIC_RIGHT_MISSILE_ADDR, MC3D_MAX_NUM_MISSILES);
         }
      }

      if (citiesSettingValue > 0)
      {
         bigpemu_jag_write8(MC3D_CLASSIC_CITIES_BIT_STATUS_ADDR, MC3D_CLASSIC_CITIES_BIT_STATUS_VALUE);
      }

      if (launchersSettingValue > 0)
      {
         // If one of the launchers' status bit has been cleared.
         uint8_t launchersStatusBits = bigpemu_jag_read8(MC3D_CLASSIC_LAUNCHERS_BIT_STATUS_ADDR);
         if (launchersStatusBits < MC3D_CLASSIC_LAUNCHERS_BIT_STATUS_VALUE)
         {
            // Set the bits for all launchers.
            bigpemu_jag_write8(MC3D_CLASSIC_LAUNCHERS_BIT_STATUS_ADDR, MC3D_CLASSIC_LAUNCHERS_BIT_STATUS_VALUE);

            // Re-max out missile counts, as the game logic zero'd out num missiles
            // for the launcher, which was hit by enemy fire.  Just be lazy and do for
            // all launchers.
            bigpemu_jag_write8(MC3D_CLASSIC_LEFT_MISSILE_ADDR, MC3D_MAX_NUM_MISSILES);
            bigpemu_jag_write8(MC3D_CLASSIC_CENTER_MISSILE_ADDR, MC3D_MAX_NUM_MISSILES);
            bigpemu_jag_write8(MC3D_CLASSIC_RIGHT_MISSILE_ADDR, MC3D_MAX_NUM_MISSILES);
         }
      }
   }
   return 0;
}

void bigp_init()
{
   void* pMod = bigpemu_get_module_handle();

   bigpemu_set_module_usage_flags(pMod, BIGPEMU_MODUSAGE_DETERMINISMWARNING);

   const int catHandle = bigpemu_register_setting_category(pMod, "MC3D - Classic");
   sMissilesSettingHandle = bigpemu_register_setting_bool(catHandle, "Infinite Missiles", 1);
   sCitiesSettingHandle = bigpemu_register_setting_bool(catHandle, "Invincible Cities", 1);
   sLaunchersSettingHandle = bigpemu_register_setting_bool(catHandle, "Invincible Launchers", 1);

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

   sMissilesSettingHandle = INVALID;
   sCitiesSettingHandle = INVALID;
   sLaunchersSettingHandle = INVALID;
}