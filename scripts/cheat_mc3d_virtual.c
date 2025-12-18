//__BIGPEMU_SCRIPT_MODULE__
//__BIGPEMU_META_DESC__    "Infinite launcher health and city health, ammo, in Virtual Mode of Missile Command 3D"
//__BIGPEMU_META_AUTHOR__  "jguff"

#include "bigpcrt.h"

// Found using BigPEmu System->Load Software screen and highlighting ROM file.
#define MC3D_HASH                      0x8A324BEEC06140ABULL

// Located using Proaction Replay like script.
// Infinite smart bombs alone makes the game easy and quick to beat.
#define MC3D_SMART_BOMB_ADDR           0x0005A02F

// Located using Proaction Replay like script to find decreasing values.
#define MC3D_CITY_L1_HEALTH_ADDR       0x00059513
//#define MC3D_CITY_L2_HEALTH_ADDR     0x000595A7
//#define MC3D_CITY_L3_HEALTH_ADDR     0x0005963b
//#define MC3D_CITY_L4_HEALTH_ADDR     0x000596cf
//#define MC3D_CITY_L5_HEALTH_ADDR     0x00059763
//#define MC3D_CITY_CENTER_HEALTH_ADDR 0x000597f7
#define MC3D_NUM_CITIES                6

// Offset between city health addresses.
#define MC3D_CITY_STRUCT_SIZE          148

// Located laser using decreasing script, then teased out the remaining
// addresses using Noesis Memory Window.
//
// Address of first weapon launcher.
#define MC3D_LAUNCHER1_ADDR            0x0005988B

// Partial description of weapon launcher data structure.
#define MC3D_LAUNCHER_HEALTH_OFFSETA   0
#define MC3D_LAUNCHER_HEALTH_OFFSETB   4
#define MC3D_MISSILE_OFFSETA           16
#define MC3D_MISSILE_OFFSETB           20
#define MC3D_LASER_OFFSETA             24
#define MC3D_LASER_OFFSETB             28
#define MC3D_LASER_TYPE_OFFSETA        56

// Size of weapon launcher data structure.
#define MC3D_LAUNCHER_STRUCT_SIZE      140
#define MC3D_NUM_LAUNCHERS             3

// Noesis Memory Window showed initial health for cities and launchers at 0x2E.
#define MC3D_CITY_HEALTH_VALUE         0x2E
#define MC3D_LAUNCHER_HEALTH_VALUE     0x2E
#define MC3D_LASER_VALUE               0x79
#define MC3D_MISSILE_VALUE             0x30
#define MC3D_LASER_TYPE_VALUE          3

#define INVALID                        -1

// Boilerplate.
static int32_t sOnLoadEvent = INVALID;
static int32_t sOnEmuFrame = INVALID;
static int32_t sMcFlags = 0;
static const int32_t skMcFlag_Loaded = (1 << 0);

// Setting handles.
static int sCityHealthSettingHandle = INVALID;
static int sLauncherHealthSettingHandle = INVALID;
static int sBombSettingHandle = INVALID;
static int sLaserSettingHandle = INVALID;
static int sMissileSettingHandle = INVALID;
static int sLaserTypeSettingHandle = INVALID;

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
      int cityHealthSettingValue = INVALID;
      int launcherHealthSettingValue = INVALID;
      int bombSettingValue = INVALID;
      int laserSettingValue = INVALID;
      int missileSettingValue = INVALID;
      int laserTypeSettingValue = INVALID;
      bigpemu_get_setting_value(&cityHealthSettingValue, sCityHealthSettingHandle);
      bigpemu_get_setting_value(&launcherHealthSettingValue, sLauncherHealthSettingHandle);
      bigpemu_get_setting_value(&bombSettingValue, sBombSettingHandle);
      bigpemu_get_setting_value(&laserSettingValue, sLaserSettingHandle);
      bigpemu_get_setting_value(&missileSettingValue, sMissileSettingHandle);
      bigpemu_get_setting_value(&laserTypeSettingValue, sLaserTypeSettingHandle);

      if (bombSettingValue > 0)
      {
         bigpemu_jag_write8(MC3D_SMART_BOMB_ADDR, 9);
      }

      // Loop through the weapons launchers.
      for (uint32_t launcherIdx = 0; launcherIdx < MC3D_NUM_LAUNCHERS; ++launcherIdx)
      {
         // Set such that lasers, missiles and launcher health are infinite/constant.
         const uint32_t launcherOffset = launcherIdx * MC3D_LAUNCHER_STRUCT_SIZE;
         const uint32_t launcherAddress = MC3D_LAUNCHER1_ADDR + launcherOffset;
         if (launcherHealthSettingValue > 0)
         {
            bigpemu_jag_write8(launcherAddress + MC3D_LAUNCHER_HEALTH_OFFSETA, MC3D_LAUNCHER_HEALTH_VALUE);
            bigpemu_jag_write8(launcherAddress + MC3D_LAUNCHER_HEALTH_OFFSETB, MC3D_LAUNCHER_HEALTH_VALUE);
         }

         if (laserSettingValue > 0)
         {
            bigpemu_jag_write8(launcherAddress + MC3D_LASER_OFFSETA, MC3D_LASER_VALUE);
            bigpemu_jag_write8(launcherAddress + MC3D_LASER_OFFSETB, MC3D_LASER_VALUE);
         }

         if (missileSettingValue > 0)
         {
            bigpemu_jag_write8(launcherAddress + MC3D_MISSILE_OFFSETA, MC3D_MISSILE_VALUE);
            bigpemu_jag_write8(launcherAddress + MC3D_MISSILE_OFFSETB, MC3D_MISSILE_VALUE);
         }

         if (laserTypeSettingValue > 0)
         {
            bigpemu_jag_write8(launcherAddress + MC3D_LASER_TYPE_OFFSETA, MC3D_LASER_TYPE_VALUE);
         }

         // Noted some display glitches with above.
         //    Launcher health bar displays almost empty, until hit, and then
         //    launcher health bar displays full.
         //    Missile bar displayed near empty sometimes.  For many tests plays
         //    missile bar displayed full, and then in subsquent test plays
         //    missile bar displayed near empty, but still had unlimited missiles.
      }

      if (cityHealthSettingValue > 0)
      {
         for (uint32_t cityIdx = 0; cityIdx < MC3D_NUM_CITIES; cityIdx++)
         {
            bigpemu_jag_write8(MC3D_CITY_L1_HEALTH_ADDR + (cityIdx * MC3D_CITY_STRUCT_SIZE), MC3D_CITY_HEALTH_VALUE);
         }
      }
   }
   return 0;
}

void bigp_init()
{
   void* pMod = bigpemu_get_module_handle();

   bigpemu_set_module_usage_flags(pMod, BIGPEMU_MODUSAGE_DETERMINISMWARNING);

   const int catHandle = bigpemu_register_setting_category(pMod, "MC3D - Virtual Mode");
   sCityHealthSettingHandle = bigpemu_register_setting_bool(catHandle, "Infinite City Health", 1);
   sLauncherHealthSettingHandle = bigpemu_register_setting_bool(catHandle, "Infinite Launcher Health", 1);
   sBombSettingHandle = bigpemu_register_setting_bool(catHandle, "Infinite Smart Bombs", 1);
   sLaserSettingHandle = bigpemu_register_setting_bool(catHandle, "Infinite Lasers", 1);
   sMissileSettingHandle = bigpemu_register_setting_bool(catHandle, "Infinite Missiles", 1);
   sLaserTypeSettingHandle = bigpemu_register_setting_bool(catHandle, "Max Laser Type", 1);

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

   sCityHealthSettingHandle = INVALID;
   sLauncherHealthSettingHandle = INVALID;
   sBombSettingHandle = INVALID;
   sLaserSettingHandle = INVALID;
   sMissileSettingHandle = INVALID;
   sLaserTypeSettingHandle = INVALID;
}