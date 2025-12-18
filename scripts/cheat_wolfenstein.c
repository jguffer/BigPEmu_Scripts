//__BIGPEMU_SCRIPT_MODULE__
//__BIGPEMU_META_DESC__    "Infinite health, ammo and all weapons, keys unlocked in Wolfenstein 3D."
//__BIGPEMU_META_AUTHOR__  "jguff"

#include "bigpcrt.h"

// Script version of joypad cheats.  Probably easier to use, then to remember joypad code.  More
// consistent to use script cheat as well, when running emulator.  Also could make cheats have
// more fidelity (e.g.  unlock all weapons, but do not unlock keys).

// Found using BigPEmu System->Load Software screen and highlighting ROM file.
#define WOLF3D_HASH              0xF714BD1B17D9375DULL

// Found using Pro Action Replay like counter match search script.  Came up
// with this address as well as 0x0001d8c7.  When Fill Memory at 0x0001D8C7,
// 0x0001D8C7 is immediately overwritten by contents of below.
#define WOLF3D_AMMO_ADDR         0x00021970

// Found by viewing Noesis Memory Window and executing joypad cheat to unlock all weapons keys.
#define WOLF3D_WPN_ENABLE_ADDR   (WOLF3D_AMMO_ADDR + 16)
#define WOLF3D_SELECTED_WPN_ADDR (WOLF3D_AMMO_ADDR + 36)
#define WOLF3D_KEYS_ENABLE_ADDR  (WOLF3D_AMMO_ADDR - 4)
#define WOLF3D_GOD_MODE_ADDR     0x00021948
#define WOLF3D_DATA_SIZE         4
#define WOLF3D_NUM_WPNS          4
#define WOLF3D_AMMO_VALUE        555
#define WOLF3D_ENABLE_VALUE      1
enum {KNIVE, PISTOL, MACHINE_GUN, CHAIN_GUN, FLAME_THROWER, ROCKET_LAUNCHER} weaponEnum;

#define INVALID                  -1

// Boilerplate.
static int32_t sOnLoadEvent = INVALID;
static int32_t sOnEmuFrame = INVALID;
static int32_t sMcFlags = 0;
static const int32_t skMcFlag_Loaded = (1 << 0);

// Setting handles.
static int sHealthSettingHandle = INVALID;
static int sAmmoSettingHandle = INVALID;
static int sAllWeaponsSettingHandle = INVALID;
static int sAllKeysSettingHandle = INVALID;
static int sLevelSelectHandle = INVALID;
static int sNextLevelHandle = INVALID;
static int sPrevLevelHandle = INVALID;

static uint32_t on_sw_loaded(const int eventHandle, void* pEventData)
{
   const uint64_t hash = bigpemu_get_loaded_fnv1a64();

   sMcFlags = 0;

   if (WOLF3D_HASH == hash)
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
      int ammoSettingValue = INVALID;
      int allWeaponsSettingValue = INVALID;
      int allKeysSettingValue = INVALID;
      bigpemu_get_setting_value(&healthSettingValue, sHealthSettingHandle);
      bigpemu_get_setting_value(&ammoSettingValue, sAmmoSettingHandle);
      bigpemu_get_setting_value(&allWeaponsSettingValue, sAllWeaponsSettingHandle);
      bigpemu_get_setting_value(&allKeysSettingValue, sAllKeysSettingHandle);

      if (healthSettingValue > 0)
      {
         bigpemu_jag_write32(WOLF3D_GOD_MODE_ADDR, WOLF3D_ENABLE_VALUE);
      }

      if (ammoSettingValue > 0)
      {
         for (uint32_t ammoIdx = 0; ammoIdx < WOLF3D_NUM_WPNS; ++ammoIdx)
         {
            bigpemu_jag_write32(WOLF3D_AMMO_ADDR + (ammoIdx * WOLF3D_DATA_SIZE), WOLF3D_AMMO_VALUE);
         }
      }

      if (allWeaponsSettingValue > 0)
      {
         for (uint32_t wpnIdx = 0; wpnIdx < WOLF3D_NUM_WPNS; ++wpnIdx)
         {
            // 4 byte booleans
            bigpemu_jag_write32(WOLF3D_WPN_ENABLE_ADDR + (wpnIdx * WOLF3D_DATA_SIZE), WOLF3D_ENABLE_VALUE);
         }

         //// Get player off default weapon, and onto weapon that game will option-button
         //// thru.
         //bigpemu_jag_write32(WOLF3D_SELECTED_WPN_ADDR, ROCKET_LAUNCHER);
         // TODO:
         //    Good idea, if don't execute every emulation frame...
         //    Maybe wait to execute this or wait to execute any of the cheat writes, until game has initialized the default weapons/ammo health
      }

      if (allKeysSettingValue > 0)
      {
         // 4 byte bitmask
         bigpemu_jag_write32(WOLF3D_KEYS_ENABLE_ADDR, (1 | 3));
      }
   }
   return 0;
}

void bigp_init()
{
   void* pMod = bigpemu_get_module_handle();

   bigpemu_set_module_usage_flags(pMod, BIGPEMU_MODUSAGE_DETERMINISMWARNING);

   // Register cheat settings with BigPEmu GUI interface.
   const int catHandle = bigpemu_register_setting_category(pMod, "Wolfenstein 3D");
   sHealthSettingHandle = bigpemu_register_setting_bool(catHandle, "Infinite Health", 1);
   sAmmoSettingHandle = bigpemu_register_setting_bool(catHandle, "Infinite Ammo", 1);
   sAllWeaponsSettingHandle = bigpemu_register_setting_bool(catHandle, "Unlock All Weapons", 1);
   sAllKeysSettingHandle = bigpemu_register_setting_bool(catHandle, "Unlock All Keys", 1);

   // Add some settings that are really just informational.
   //
   // Be easy enough to find the level state and modify level state in this script, but
   // there are complications of when and when not to overwrite the state such that
   // user can continue to the levels beyond the cheat selected level.
   //
   // The joypad controller cheat is pretty easy, so seems like informing the user of the
   // joypad controller cheat is path of least resistance. 
   sLevelSelectHandle = bigpemu_register_setting_bool(catHandle, "Level Select: 1, 3, 7, 9 simul at main menu", 1);
   sNextLevelHandle = bigpemu_register_setting_bool(catHandle, "Next Level: 4, 7, 8 then 6 during gameplay", 1);
   sPrevLevelHandle = bigpemu_register_setting_bool(catHandle, "Prev Level: 4, 6, 9, then 6 during gameplay", 1);

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
   sAmmoSettingHandle = INVALID;
   sAllWeaponsSettingHandle = INVALID;
   sAllKeysSettingHandle = INVALID;
   sLevelSelectHandle = INVALID;
   sNextLevelHandle = INVALID;
   sPrevLevelHandle = INVALID;
}