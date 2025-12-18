//__BIGPEMU_SCRIPT_MODULE__
//__BIGPEMU_META_DESC__    "Infinite city health, launcher health, missiles, special weapons, in 3d mode of Missile Command 3D."
//__BIGPEMU_META_AUTHOR__  "jguff"

#include "bigpcrt.h"

// Found using BigPEmu System->Load Software screen and highlighting ROM file.
#define MC3D_HASH                         0x8A324BEEC06140ABULL

// These addresses are shared by virtual mode.
// !!DO NOT ENABLE THIS SCRIPT WHILE VIRTUAL MODE'S SCRIPT IS ENABLED.
// !!BOTH ENABLED SIMULTANEOUSLY, RESULTS IN INFINITE LOOP AT END OF LEVEL SCORE TALLY.
#define MC3D_LEFT_MISSILE_ADDR            0x0005989B
#define MC3D_CENTER_MISSILE_ADDR          0x00059927
#define MC3D_RIGHT_MISSILE_ADDR           0x000599B3
#define MC3D_DEFAULT_NUM_MISSILES         20
#define MC3D_MAX_NUM_MISSILES             99

// Found using Pro Action Relay like descending counter value search script.
#define MC3D_3D_BOMB_ADDR                 0x00025587
#define MC3D_3D_BOMB_VALUE                2

// Found via logically poking memory near the bomb address.
#define MC3D_3D_CASCADE_ADDR              0x0002558F
#define MC3D_3D_CASCADE_VALUE             2

// Located using Proaction Replay like script to find decreasing values, when in Virtual mode.
// Accidentally happened to have virtual mode and 3d mode BigPEmu cheat scripts enabled simultaneously
// and noticed that caused cities in 3d mode to become indestructible.
// After that updated this script for indestructible cities, with only this script enabled.
#define MC3D_3D_CITY1_HEALTH_ADDR         0x00059513
#define MC3D_3D_CITY2_HEALTH_ADDR         0x000595A7
#define MC3D_3D_CITY3_HEALTH_ADDR         0x0005963B
#define MC3D_3D_CITY4_HEALTH_ADDR         0x000596CF
#define MC3D_3D_CITY5_HEALTH_ADDR         0x00059763
#define MC3D_3D_CITY6_HEALTH_ADDR         0x000597F7
#define MC3D_3D_LAUNCHER1_HEALTH_ADDR     0x00059867
#define MC3D_3D_LAUNCHER2_HEALTH_ADDR     0x000598F3
#define MC3D_3D_LAUNCHER3_HEALTH_ADDR     0x0005997F
#define MC3D_3D_CITY_HEALTH_VALUE         0x2E
#define MC3D_3D_LAUNCHER_EXISTS_VALUE     0x50
#define MC3D_3D_LAUNCHER_DESTROYED_VALUE  0x54

#define INVALID                           -1

// Boilerplate.
static int32_t sOnLoadEvent = INVALID;
static int32_t sOnEmuFrame = INVALID;
static int32_t sMcFlags = 0;
static const int32_t skMcFlag_Loaded = (1 << 0);

// Setting handles.
static int sMissilesSettingHandle = INVALID;
static int sSpecialSettingHandle = INVALID;
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
      int specialSettingValue = INVALID;
      int citiesSettingValue = INVALID;
      int launchersSettingValue = INVALID;
      bigpemu_get_setting_value(&missilesSettingValue, sMissilesSettingHandle);
      bigpemu_get_setting_value(&specialSettingValue, sSpecialSettingHandle);
      bigpemu_get_setting_value(&citiesSettingValue, sCitiesSettingHandle);
      bigpemu_get_setting_value(&launchersSettingValue, sLaunchersSettingHandle);

      uint8_t numLeftMissiles = bigpemu_jag_read8(MC3D_LEFT_MISSILE_ADDR);
      uint8_t numCenterMissiles = bigpemu_jag_read8(MC3D_CENTER_MISSILE_ADDR);
      uint8_t numRightMissiles = bigpemu_jag_read8(MC3D_RIGHT_MISSILE_ADDR);
      if (missilesSettingValue > 0)
      {
         // If you set number of missiles every frame, an infinite loop is entered
         // at end of level/wave, when tallying up missiles to add to score.
         // So only increase the number of missiles, when each silo contains the
         // default/start number of missiles per silo.
         //
         // If you need more then 3*MC3D_MAX_NUM_MISSILES missiles, spend them such
         // that get back to MC3D_DEFAULT_NUM_MISSILES in each silo, then they will
         // replenish to MC3D_MAX_NUM_MISSILES each.
         //
         // Also, likely don't want level to end with MC3D_DEFAULT_NUM_MISSILES
         // remaining in each silo, as may end up in infinite loop, while
         // end-of-stage tally decrements remaining missiles to add to score.
         if ((MC3D_DEFAULT_NUM_MISSILES == numLeftMissiles) &&
             (MC3D_DEFAULT_NUM_MISSILES == numCenterMissiles) &&
             (MC3D_DEFAULT_NUM_MISSILES == numRightMissiles))
         {
            bigpemu_jag_write8(MC3D_LEFT_MISSILE_ADDR, MC3D_MAX_NUM_MISSILES);
            bigpemu_jag_write8(MC3D_CENTER_MISSILE_ADDR, MC3D_MAX_NUM_MISSILES);
            bigpemu_jag_write8(MC3D_RIGHT_MISSILE_ADDR, MC3D_MAX_NUM_MISSILES);
         }
      }

      if (citiesSettingValue > 0)
      {
         bigpemu_jag_write8(MC3D_3D_CITY1_HEALTH_ADDR, MC3D_3D_CITY_HEALTH_VALUE);
         bigpemu_jag_write8(MC3D_3D_CITY2_HEALTH_ADDR, MC3D_3D_CITY_HEALTH_VALUE);
         bigpemu_jag_write8(MC3D_3D_CITY3_HEALTH_ADDR, MC3D_3D_CITY_HEALTH_VALUE);
         bigpemu_jag_write8(MC3D_3D_CITY4_HEALTH_ADDR, MC3D_3D_CITY_HEALTH_VALUE);
         bigpemu_jag_write8(MC3D_3D_CITY5_HEALTH_ADDR, MC3D_3D_CITY_HEALTH_VALUE);
         bigpemu_jag_write8(MC3D_3D_CITY6_HEALTH_ADDR, MC3D_3D_CITY_HEALTH_VALUE);
      }

      if (launchersSettingValue > 0)
      {
         // 
         uint8_t launcher1Status = bigpemu_jag_read8(MC3D_3D_LAUNCHER1_HEALTH_ADDR);
         uint8_t launcher2Status = bigpemu_jag_read8(MC3D_3D_LAUNCHER2_HEALTH_ADDR);
         uint8_t launcher3Status = bigpemu_jag_read8(MC3D_3D_LAUNCHER3_HEALTH_ADDR);

         bigpemu_jag_write8(MC3D_3D_LAUNCHER1_HEALTH_ADDR, MC3D_3D_LAUNCHER_EXISTS_VALUE);
         bigpemu_jag_write8(MC3D_3D_LAUNCHER2_HEALTH_ADDR, MC3D_3D_LAUNCHER_EXISTS_VALUE);
         bigpemu_jag_write8(MC3D_3D_LAUNCHER3_HEALTH_ADDR, MC3D_3D_LAUNCHER_EXISTS_VALUE);

         // Game logic set number of missiles remaining to zero, when game logic marked
         // launcher as destroyed.  So set the number of remaining missiles to max number
         // of missiles.  Be lazy and just do this for all launchers.
         if ((MC3D_3D_LAUNCHER_DESTROYED_VALUE == launcher1Status) || 
             (MC3D_3D_LAUNCHER_DESTROYED_VALUE == launcher2Status) || 
             (MC3D_3D_LAUNCHER_DESTROYED_VALUE == launcher3Status))
         {
            bigpemu_jag_write8(MC3D_LEFT_MISSILE_ADDR, MC3D_MAX_NUM_MISSILES);
            bigpemu_jag_write8(MC3D_CENTER_MISSILE_ADDR, MC3D_MAX_NUM_MISSILES);
            bigpemu_jag_write8(MC3D_RIGHT_MISSILE_ADDR, MC3D_MAX_NUM_MISSILES);
         }
      }

      if (missilesSettingValue > 0)
      {
         // Using only the above...
         //     At one point, during a level, saw number of missiles go from 99,99,99, to 20,0,20.
         //     If had paid closer attention, probably would have noticed one of launchers got
         //     destroyed, taking that launchers number of missils to 0.  Then the remaining
         //     launchers probably went to 20, after picking up ammo resupply.  Suspect ammo 
         //     resupply put number of missiles to what game meant to be max number of
         //     missiles: 20.
         //
         // To partially work around ammo resupply issue, check if more then one missile launcher
         // is set to 20 missiles.  If so, increase to 99.
         // If not, assume this is end of stage tallying, or there is only one missile launcher
         // remaining.
         //
         // If there is only one missile launcher remaining, and player obtains ammo resupply pod,
         // and game then sets number of missiles to 20, well too bad so sad.
         // Don't feel like figuring out a better solution at moment (like figuring out where in
         // memory missile launcher status (alive vs dead) is stored.  Especially given that 140
         // bytes seperates the memory for number of missiles for each missile launcher.
         //
         // Below logic probably makes above logic unneccessary, but keeping above logic, as could
         // see below logic changing or being removed.

// TODO: Below doesn't work.  Maybe cause was already firing missiles when Extra Ammo was picked up.
//       Maybe because below is logically flawed.
//       Whatever.  Infinite special weapons can be used, if player runs out of missiles.
//       Or player can shoot nothing, and allow infinite city health to take player to the next level.
//
//       One change that could work is to maintain state of missile counts from prior emulation frame.
//       If missile counts change over a certain delta between emulation frames, reset the missile counts to 99.
//       Don't really care enough at this point to implement that right now.
//
//         uint16_t numLaunchersAtDefaultValue = 0;
//         if (MC3D_DEFAULT_NUM_MISSILES == numLeftMissiles)
//         {
//            ++numLaunchersAtDefaultValue;
//         }
//         if (MC3D_DEFAULT_NUM_MISSILES == numCenterMissiles)
//         {
//            ++numLaunchersAtDefaultValue;
//         }
//         if (MC3D_DEFAULT_NUM_MISSILES == numRightMissiles)
//         {
//            ++numLaunchersAtDefaultValue;
//         }
//
//         if (numLaunchersAtDefaultValue > 1)
//         {
//            // TODO
//            // Commented out this prior to applying a fix, and then never got an ammo resupply pod to appear again, to test this, so commenting out for now.
//            bigpemu_jag_write8(MC3D_LEFT_MISSILE_ADDR, MC3D_MAX_NUM_MISSILES);
//            bigpemu_jag_write8(MC3D_CENTER_MISSILE_ADDR, MC3D_MAX_NUM_MISSILES);
//            bigpemu_jag_write8(MC3D_RIGHT_MISSILE_ADDR, MC3D_MAX_NUM_MISSILES);
//         }
      }

      if (specialSettingValue > 0)
      {
         bigpemu_jag_write8(MC3D_3D_BOMB_ADDR, MC3D_3D_BOMB_VALUE);
         bigpemu_jag_write8(MC3D_3D_CASCADE_ADDR, MC3D_3D_CASCADE_VALUE);
      }
   }
   return 0;
}

void bigp_init()
{
   void* pMod = bigpemu_get_module_handle();

   bigpemu_set_module_usage_flags(pMod, BIGPEMU_MODUSAGE_DETERMINISMWARNING);

   // Register cheat settings with BigPEmu GUI interface.
   const int catHandle = bigpemu_register_setting_category(pMod, "MC3D 3D Mode");
   sMissilesSettingHandle = bigpemu_register_setting_bool(catHandle, "Infinite Missiles", 1);
   sSpecialSettingHandle = bigpemu_register_setting_bool(catHandle, "Infinite Special Weapon", 1);
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
   sSpecialSettingHandle = INVALID;
}