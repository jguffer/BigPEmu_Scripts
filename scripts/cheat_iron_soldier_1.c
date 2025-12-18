//__BIGPEMU_SCRIPT_MODULE__
//__BIGPEMU_META_DESC__    "Infinite health, ammo, level select in Iron Soldier 1."
//__BIGPEMU_META_AUTHOR__  "jguff"

#include "bigpcrt.h"

// Found using BigPEmu System->Load Software screen and highlighting ROM file.
#define IS1_SUPPORTED_HASH             0x8429FDCFCDF35FA1ULL

// Found 2 of these using Pro Action Replay like scripts on memory dump from Noesis.
// Remainder found by inspecting memory, using Noesis Memory Window.
#define IS1_MISSILES_ADDR              0x00004720
#define IS1_GATLING_ADDR               0x00004724
#define IS1_GRENADES_ADDR              0x00004726
#define IS1_RAILGUN_ADDR               0x00004728
#define IS1_CRUISE_ADDR                0x0000472C
#define IS1_MISSILES_VALUE             12
#define IS1_GATLING_VALUE              0x3300
#define IS1_GRENADES_VALUE             8
#define IS1_RAILGUN_VALUE              50
#define IS1_CRUISE_VALUE               1

// Found using Pro Action Replay like script on memory dump from Noesis.
#define IS1_UNLOCK_LVLS_N_WPNS_ADDR    0x000045B6
#define IS1_UNLOCK_LVLS_N_WPNS_VALUE   8
#define IS1_HEALTH_ADDR                0x0000C5E6
#define IS1_HEALTH_VALUE               0x7000

#define INVALID                        -1

// Boilerplate.
static int32_t sOnLoadEvent = INVALID;
static int32_t sOnEmuFrame = INVALID;
static int32_t sMcFlags = 0;
static const int32_t skMcFlag_Loaded = (1 << 0);

// Setting handles.
static int sAmmoSettingHandle = INVALID;
static int sUnlockSettingHandle = INVALID;
static int sHealthSettingHandle = INVALID;

static uint32_t on_sw_loaded(const int eventHandle, void* pEventData)
{
   const uint64_t hash = bigpemu_get_loaded_fnv1a64();

   sMcFlags = 0;

   if (IS1_SUPPORTED_HASH == hash)
   {
      sMcFlags |= skMcFlag_Loaded;
      printf_notify("This version of Iron Soldier 1 IS supported.\n");
   }

   return 0;
}

static uint32_t on_emu_frame(const int eventHandle, void* pEventData)
{
   if (sMcFlags & skMcFlag_Loaded)
   {
      // Get setting values.  Doing this once per frame, allows player
      // to modify setting during game session.
      int ammoSettingValue = INVALID;
      int unlockSettingValue = INVALID;
      int healthSettingValue = INVALID;
      bigpemu_get_setting_value(&ammoSettingValue, sAmmoSettingHandle);
      bigpemu_get_setting_value(&unlockSettingValue, sUnlockSettingHandle);
      bigpemu_get_setting_value(&healthSettingValue, sHealthSettingHandle);

      if (unlockSettingValue > 0)
      {
         bigpemu_jag_write8(IS1_UNLOCK_LVLS_N_WPNS_ADDR, IS1_UNLOCK_LVLS_N_WPNS_VALUE);
      }

      if (ammoSettingValue > 0)
      {
         bigpemu_jag_write8(IS1_MISSILES_ADDR, IS1_MISSILES_VALUE);
         bigpemu_jag_write8(IS1_GATLING_ADDR + 0, IS1_GATLING_VALUE >> 8);
         bigpemu_jag_write8(IS1_GATLING_ADDR + 1, IS1_GATLING_VALUE & 0xFF);
         bigpemu_jag_write8(IS1_GRENADES_ADDR, IS1_GRENADES_VALUE);
         bigpemu_jag_write8(IS1_RAILGUN_ADDR, IS1_RAILGUN_VALUE);
         bigpemu_jag_write8(IS1_CRUISE_ADDR, IS1_CRUISE_VALUE);
      }

      if (healthSettingValue > 0)
      {
         bigpemu_jag_write8(IS1_HEALTH_ADDR + 0, IS1_HEALTH_VALUE >> 8);
         bigpemu_jag_write8(IS1_HEALTH_ADDR + 1, IS1_HEALTH_VALUE & 0xFF);
      }
   }
   return 0;
}

void bigp_init()
{
   void* pMod = bigpemu_get_module_handle();

   bigpemu_set_module_usage_flags(pMod, BIGPEMU_MODUSAGE_DETERMINISMWARNING);

   // Register cheat settings with BigPEmu GUI interface.
   const int catHandle = bigpemu_register_setting_category(pMod, "Iron Soldier 1");
   sAmmoSettingHandle = bigpemu_register_setting_bool(catHandle, "Infinite Ammo", 1);
   sUnlockSettingHandle = bigpemu_register_setting_bool(catHandle, "Unlock Levels And Weapons", 1);
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

   sAmmoSettingHandle = INVALID;
   sUnlockSettingHandle = INVALID;
   sHealthSettingHandle = INVALID;
}