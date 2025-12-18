//__BIGPEMU_SCRIPT_MODULE__
//__BIGPEMU_META_DESC__    "Infinite health and ammo, in Space War"
//__BIGPEMU_META_AUTHOR__  "jguff"

#include "bigpcrt.h"

// Found using BigPEmu System->Load Software screen and highlighting ROM file.
#define SWAR_HASH          0xE05E39C0E80BE8FDULL

// Found using Pro Action Replay like script.
#define SWAR_HEALTH_ADDR   0x0000FB6C
#define SWAR_MINES_ADDR    0x0000880F

// Found via memory inspection, near SWAR_MINES_ADDR, using Noesis Memory Window.
#define SWAR_GUIDED_ADDR   0x00008807
#define SWAR_ACTIVE_ADDR   0x00008809
#define SWAR_PASSIVE_ADDR  0x0000880B
#define SWAR_SEEKER_ADDR   0x0000880D
#define SWAR_HEALTH_VAL    0x1000
#define SWAR_MINES_VAL     8
#define SWAR_MISSILE_VAL   2

#define INVALID            1

// Boilerplate.
static int32_t sOnLoadEvent = INVALID;
static int32_t sOnEmuFrame = INVALID;
static int32_t sMcFlags = 0;
static const int32_t skMcFlag_Loaded = (1 << 0);

// Setting handles.
static int sHealthSettingHandle = INVALID;
static int sAmmoSettingHandle = INVALID;

static uint32_t on_sw_loaded(const int eventHandle, void* pEventData)
{
   const uint64_t hash = bigpemu_get_loaded_fnv1a64();

   sMcFlags = 0;

   if (SWAR_HASH == hash)
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
      bigpemu_get_setting_value(&healthSettingValue, sHealthSettingHandle);
      bigpemu_get_setting_value(&ammoSettingValue, sAmmoSettingHandle);

      if (healthSettingValue > 0)
      {
         bigpemu_jag_write8(SWAR_HEALTH_ADDR + 0, SWAR_HEALTH_VAL >> 8);
         bigpemu_jag_write8(SWAR_HEALTH_ADDR + 1, SWAR_HEALTH_VAL & 0x00FF);
      }

      if (ammoSettingValue > 0)
      {
         bigpemu_jag_write8(SWAR_GUIDED_ADDR,  SWAR_MISSILE_VAL);
         bigpemu_jag_write8(SWAR_ACTIVE_ADDR,  SWAR_MISSILE_VAL);
         bigpemu_jag_write8(SWAR_PASSIVE_ADDR, SWAR_MISSILE_VAL);
         bigpemu_jag_write8(SWAR_SEEKER_ADDR,  SWAR_MISSILE_VAL);
         bigpemu_jag_write8(SWAR_MINES_ADDR,   SWAR_MINES_VAL);
      }

      // TODO: Is there anyway to power up laser (A button) or main weapon (B button)?
      //       If so, implement those cheats as well.
      // TODO: What are the rules for ending "mission"?  In limited time spent with
      //       game, "mission" seemed to end early.  Maybe could create a cheat to 
      //       extend time of "mission", or number of enemies to kill for "mission".
   }
   return 0;
}

void bigp_init()
{
   void* pMod = bigpemu_get_module_handle();

   bigpemu_set_module_usage_flags(pMod, BIGPEMU_MODUSAGE_DETERMINISMWARNING);

   const int catHandle = bigpemu_register_setting_category(pMod, "Space War");
   sHealthSettingHandle = bigpemu_register_setting_bool(catHandle, "Infinite Health", 1);
   sAmmoSettingHandle = bigpemu_register_setting_bool(catHandle, "Infinite Ammo", 1);

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
}