//__BIGPEMU_SCRIPT_MODULE__
//__BIGPEMU_META_DESC__    "Infinite health, fuel, ammo, in Skyhammer."
//__BIGPEMU_META_AUTHOR__  "jguff"

#include "bigpcrt.h"

// Found using BigPEmu System->Load Software screen and highlighting ROM file.
#define SKYHAM_HASH              0xC6A8442BCE267C50ULL

// Found two of the weapons using Pro Action Replay like script and
// Noesis Memory Window.  Others found by inspecting memory in vicinity
// of those weapons.
#define SKYHAM_SHIELD_ADDR	      0x001C1BB0
#define SKYHAM_DAMAGE_ADDR       0x001C1BB4
#define SKYHAM_MINI_MISS_ADDR    0x001C1BBB
#define SKYHAM_HOMING_ADDR       0x001C1BBF
#define SKYHAM_BOMB_ADDR         0x001C1BC3
#define SKYHAM_SMART_BOMB_ADDR   0x001C1BC7
#define SKYHAM_SMART_MISS_ADDR   0x001C1BCB
#define SKYHAM_GATLING_ADDR      0x001C1BCE
#define SKYHAM_FUEL_ADDR         0x001C1BD4
#define SKYHAM_SHIELD_VALUE      0x00190000
#define SKYHAM_DAMAGE_VALUE      0x00190000
#define SKYHAM_MINI_MISS_VALUE   0x80
#define SKYHAM_HOMING_VALUE      0x20
#define SKYHAM_BOMB_VALUE        8
#define SKYHAM_SMART_BOMB_VALUE  8
#define SKYHAM_SMART_MISS_VALUE  8
#define SKYHAM_GATLING_VALUE     0x0800
#define SKYHAM_FUEL_VALUE        0x0008F03F

#define INVALID                  -1

// Boilerplate.
static int32_t sOnLoadEvent = INVALID;
static int32_t sOnEmuFrame = INVALID;
static int32_t sMcFlags = 0;
static const int32_t skMcFlag_Loaded = (1 << 0);

// Setting handles.
static int sHealthSettingHandle = INVALID;
static int sAmmoSettingHandle = INVALID;
static int sFuelSettingHandle = INVALID;

static uint32_t on_sw_loaded(const int eventHandle, void* pEventData)
{
   const uint64_t hash = bigpemu_get_loaded_fnv1a64();

   sMcFlags = 0;
   if (SKYHAM_HASH == hash)
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
      int fuelSettingValue = INVALID;
      bigpemu_get_setting_value(&healthSettingValue, sHealthSettingHandle);
      bigpemu_get_setting_value(&ammoSettingValue, sAmmoSettingHandle);
      bigpemu_get_setting_value(&fuelSettingValue, sFuelSettingHandle);

      if (healthSettingValue > 0)
      {
         bigpemu_jag_write32(SKYHAM_SHIELD_ADDR, SKYHAM_SHIELD_VALUE);
         bigpemu_jag_write32(SKYHAM_DAMAGE_ADDR, SKYHAM_DAMAGE_VALUE);
      }

      if (ammoSettingValue > 0)
      {
         bigpemu_jag_write8(SKYHAM_MINI_MISS_ADDR, SKYHAM_MINI_MISS_VALUE);
         bigpemu_jag_write8(SKYHAM_HOMING_ADDR, SKYHAM_HOMING_VALUE);
         bigpemu_jag_write8(SKYHAM_BOMB_ADDR, SKYHAM_BOMB_VALUE);
         bigpemu_jag_write8(SKYHAM_SMART_BOMB_ADDR, SKYHAM_SMART_BOMB_VALUE);
         bigpemu_jag_write8(SKYHAM_SMART_MISS_ADDR, SKYHAM_SMART_MISS_VALUE);
         bigpemu_jag_write8(SKYHAM_GATLING_ADDR + 0, SKYHAM_GATLING_VALUE >> 8);
         bigpemu_jag_write8(SKYHAM_GATLING_ADDR + 1, SKYHAM_GATLING_VALUE & 0xFF);
      }

      if (fuelSettingValue > 0)
      {
         bigpemu_jag_write32(SKYHAM_FUEL_ADDR, SKYHAM_FUEL_VALUE);
      }
   }
   return 0;
}

void bigp_init()
{
   void* pMod = bigpemu_get_module_handle();

   bigpemu_set_module_usage_flags(pMod, BIGPEMU_MODUSAGE_DETERMINISMWARNING);

   const int catHandle = bigpemu_register_setting_category(pMod, "Skyhammer");
   sHealthSettingHandle = bigpemu_register_setting_bool(catHandle, "Infinite Health", 1);
   sAmmoSettingHandle = bigpemu_register_setting_bool(catHandle, "Infinite Ammo", 1);
   sFuelSettingHandle = bigpemu_register_setting_bool(catHandle, "Infinite Fuel", 1);

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
   sFuelSettingHandle = INVALID;
}