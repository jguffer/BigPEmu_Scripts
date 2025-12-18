//__BIGPEMU_SCRIPT_MODULE__
//__BIGPEMU_META_DESC__    "Infinite health, ammo, unlock levels, in Zero 5."
//__BIGPEMU_META_AUTHOR__  "jguff"

#include "bigpcrt.h"

// Found using BigPEmu System->Load Software screen and highlighting ROM file.
#define ZERO5_HASH                  0x5E5F0DE6864B2AF8ULL

// Found using Pro-Action Replay like script on memory dump.
#define ZERO5_SMART_LASER_ADDR      0x001FAA33
#define ZERO5_INVINCIBLE_ADDR       0x001FAABB
#define ZERO5_UNLOCK_ADDR           0x001FAAAF
#define ZERO5_MAX_SMART_LASER_VAL   2
#define ZERO5_INVINCIBLE_VALUE      1
#define ZERO5_UNLOCK_VALUE          15

#define INVALID                     -1

// Boilerplate.
static int32_t sOnLoadEvent = INVALID;
static int32_t sOnEmuFrame = INVALID;
static int32_t sMcFlags = 0;
static const int32_t skMcFlag_Loaded = (1 << 0);

// Setting handles.
static int sAmmoSettingHandle = INVALID;
static int sHealthSettingHandle = INVALID;
static int sUnlockSettingHandle = INVALID;

static uint32_t on_sw_loaded(const int eventHandle, void* pEventData)
{
   const uint64_t hash = bigpemu_get_loaded_fnv1a64();

   sMcFlags = 0;

   if (ZERO5_HASH == hash)
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
      int ammoSettingValue = INVALID;
      int healthSettingValue = INVALID;
      int unlockSettingValue = INVALID;
      bigpemu_get_setting_value(&ammoSettingValue, sAmmoSettingHandle);
      bigpemu_get_setting_value(&healthSettingValue, sHealthSettingHandle);
      bigpemu_get_setting_value(&unlockSettingValue, sUnlockSettingHandle);

      if (ammoSettingValue > 0)
      {
         bigpemu_jag_write8(ZERO5_SMART_LASER_ADDR, ZERO5_MAX_SMART_LASER_VAL);
      }

      if (healthSettingValue > 0)
      {
         bigpemu_jag_write8(ZERO5_INVINCIBLE_ADDR, ZERO5_INVINCIBLE_VALUE);
      }

      if (unlockSettingValue > 0)
      {
         bigpemu_jag_write8(ZERO5_UNLOCK_ADDR, ZERO5_UNLOCK_VALUE);
      }
   }
   return 0;
}

void bigp_init()
{
   void* pMod = bigpemu_get_module_handle();

   bigpemu_set_module_usage_flags(pMod, BIGPEMU_MODUSAGE_DETERMINISMWARNING);

   const int catHandle = bigpemu_register_setting_category(pMod, "Zero 5");
   sAmmoSettingHandle = bigpemu_register_setting_bool(catHandle, "Infinite Ammo", 1);
   sHealthSettingHandle = bigpemu_register_setting_bool(catHandle, "Infinite Health", 1);
   sUnlockSettingHandle = bigpemu_register_setting_bool(catHandle, "Unlock All Levels", 1);

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
   sHealthSettingHandle = INVALID;
   sUnlockSettingHandle = INVALID;
}