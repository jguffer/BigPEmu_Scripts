//__BIGPEMU_SCRIPT_MODULE__
//__BIGPEMU_META_DESC__    "Infinite health, ammo, level select, strider in Iron Soldier 2."
//__BIGPEMU_META_AUTHOR__  "jguff"

#include "bigpcrt.h"

// Found using BigPEmu System->Load Software screen and highlighting ROM file.
#define IS2CART_SUPPORTED_HASH      0x39159E865AB73158ULL
#define IS2CD_SUPPORTED_HASH        0x962CCBEDFAAC5F28ULL

// Found 2 of these using Pro Action Replay like scripts on memory dump from Noesis.
// Remainder found by inspecting memory, using Noesis Memory Window.
#define IS2_RIFLE_ADDR              0x0000479C
#define IS2_GATLING_ADDR            0x000047A2
#define IS2_LAUNCHER_ADDR           0x000047B0
#define IS2_MACHINE_ADDR            0x000047B4
#define IS2_SHOTGUN_ADDR            0x000047B6
#define IS2_TWO_BYTE_AMMO_VALUE     0x3200

// Found by inspecting memory, using Noesis Memory Window.
#define IS2_UNGUIDED_MISSILE_ADDR   0x0000479E
#define IS2_GRENADES_ADDR           0x000047A4
#define IS2_RAILGUN_ADDR            0x000047A6
#define IS2_CRUISE_MISSILE_ADDR     0x000047AA
#define IS2_GUIDED_MISSILE_ADDR     0x000047B2
#define IS2_UNGUIDED_MISSILE_VALUE  12
#define IS2_GRENADES_VALUE          8
#define IS2_RAILGUN_VALUE           50
#define IS2_CRUISE_MISSILE_VALUE    1
#define IS2_GUIDED_MISSILE_VALUE    6

// Found using Pro Action Replay like script on memory dump from Noesis.
#define IS2_UNLOCK_ADDR             0x0000461A
#define IS2_UNLOCK_LVLS_N_WPNS_VAL  8
#define IS2_UNLOCK_STRIDER_VAL      16
#define IS2CART_HEALTH_ADDR         0x00013860

// Found via memory inspecting, using Noesis Memory Window, after having already
// located the address of cart health.  16 bytes difference between compact disc
// and cart for some reason.
#define IS2CD_HEALTH_ADDR           0x00013870
#define IS2_HEALTH_VALUE            0x7900

#define INVALID                     -1

// Boilerplate.
static int32_t sOnLoadEvent = INVALID;
static int32_t sOnEmuFrame = INVALID;
static int32_t sMcFlags = 0;
static const int32_t skMcFlag_Loaded = (1 << 0);

static int32_t sHealthAddr = INVALID;

// Setting handles.
static int sAmmoSettingHandle = INVALID;
static int sUnlockLevelsWeaponsSettingHandle = INVALID;
static int sHealthSettingHandle = INVALID;
static int sUnlockStriderSettingHandle = INVALID;

static uint32_t on_sw_loaded(const int eventHandle, void* pEventData)
{
   const uint64_t hash = bigpemu_get_loaded_fnv1a64();

   sMcFlags = 0;

   if (IS2CART_SUPPORTED_HASH == hash)
   {
      sMcFlags |= skMcFlag_Loaded;
      sHealthAddr = IS2CART_HEALTH_ADDR;

      printf_notify("This version of Iron Soldier 2 Cart IS supported.\n");
   }
   else if (IS2CD_SUPPORTED_HASH == hash)
   {
      sMcFlags |= skMcFlag_Loaded;
      sHealthAddr = IS2CD_HEALTH_ADDR;
      printf_notify("This version of Iron Soldier 2 CD IS supported.\n");
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
      int unlockLevelsWeaponsSettingValue = INVALID;
      int healthSettingValue = INVALID;
      int unlockStriderSettingValue = INVALID;
      bigpemu_get_setting_value(&ammoSettingValue, sAmmoSettingHandle);
      bigpemu_get_setting_value(&unlockLevelsWeaponsSettingValue, sUnlockLevelsWeaponsSettingHandle);
      bigpemu_get_setting_value(&healthSettingValue, sHealthSettingHandle);
      bigpemu_get_setting_value(&unlockStriderSettingValue, sUnlockStriderSettingHandle);

      if (unlockLevelsWeaponsSettingValue > 0)
      {
         bigpemu_jag_write8(IS2_UNLOCK_ADDR, IS2_UNLOCK_LVLS_N_WPNS_VAL);
      }

      if (ammoSettingValue > 0)
      {
         // Set word length ammo values.
         const uint8_t valueHiByte = IS2_TWO_BYTE_AMMO_VALUE >> 8;
         const uint8_t valueLoByte = IS2_TWO_BYTE_AMMO_VALUE & 0xFF;
         bigpemu_jag_write8(IS2_RIFLE_ADDR + 0, valueHiByte);
         bigpemu_jag_write8(IS2_RIFLE_ADDR + 1, valueLoByte);
         bigpemu_jag_write8(IS2_GATLING_ADDR + 0, valueHiByte);
         bigpemu_jag_write8(IS2_GATLING_ADDR + 1, valueLoByte);
         bigpemu_jag_write8(IS2_LAUNCHER_ADDR + 0, valueHiByte);
         bigpemu_jag_write8(IS2_LAUNCHER_ADDR + 1, valueLoByte);
         bigpemu_jag_write8(IS2_MACHINE_ADDR + 0, valueHiByte);
         bigpemu_jag_write8(IS2_MACHINE_ADDR + 1, valueLoByte);
         bigpemu_jag_write8(IS2_SHOTGUN_ADDR + 0, valueHiByte);
         bigpemu_jag_write8(IS2_SHOTGUN_ADDR + 1, valueLoByte);

         // Set byte length ammo values.
         bigpemu_jag_write8(IS2_UNGUIDED_MISSILE_ADDR, IS2_UNGUIDED_MISSILE_VALUE);
         bigpemu_jag_write8(IS2_GRENADES_ADDR, IS2_GRENADES_VALUE);
         bigpemu_jag_write8(IS2_RAILGUN_ADDR, IS2_RAILGUN_VALUE);
         bigpemu_jag_write8(IS2_CRUISE_MISSILE_ADDR, IS2_CRUISE_MISSILE_VALUE);
         bigpemu_jag_write8(IS2_GUIDED_MISSILE_ADDR, IS2_GUIDED_MISSILE_VALUE);
      }

      if (healthSettingValue > 0)
      {
         bigpemu_jag_write8(sHealthAddr + 0, IS2_HEALTH_VALUE >> 8);
         bigpemu_jag_write8(sHealthAddr + 1, IS2_HEALTH_VALUE & 0xFF);
      }

      if (unlockStriderSettingValue > 0)
      {
         uint8_t unlockValue = IS2_UNLOCK_STRIDER_VAL;
         if (unlockLevelsWeaponsSettingValue > 0)
         {
            unlockValue |= IS2_UNLOCK_LVLS_N_WPNS_VAL;
         }
         bigpemu_jag_write8(IS2_UNLOCK_ADDR, unlockValue);
      }
   }
   return 0;
}

void bigp_init()
{
   void* pMod = bigpemu_get_module_handle();

   bigpemu_set_module_usage_flags(pMod, BIGPEMU_MODUSAGE_DETERMINISMWARNING);

   // Register cheat settings with BigPEmu GUI interface.
   const int catHandle = bigpemu_register_setting_category(pMod, "Iron Soldier 2 Cart");
   sAmmoSettingHandle = bigpemu_register_setting_bool(catHandle, "Infinite Ammo", 1);
   sUnlockLevelsWeaponsSettingHandle = bigpemu_register_setting_bool(catHandle, "Unlock Levels And Weapons", 1);
   sHealthSettingHandle = bigpemu_register_setting_bool(catHandle, "Infinite Health", 1);
   sUnlockStriderSettingHandle = bigpemu_register_setting_bool(catHandle, "Unlock Satyr War Strider", 0);

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
   sUnlockLevelsWeaponsSettingHandle = INVALID;
   sHealthSettingHandle = INVALID;
   sUnlockStriderSettingHandle = INVALID;
}