//__BIGPEMU_SCRIPT_MODULE__
//__BIGPEMU_META_DESC__    "Infinite lives and ammo, in Trevor Mcfur."
//__BIGPEMU_META_AUTHOR__  "jguff"

#include "bigpcrt.h"

// Found using BigPEmu System->Load Software screen and highlighting ROM file.
#define TREVOR_HASH              0x68E321F5ECF43196ULL

// Found using Pro-Action Replay like script on memory dump.
#define TREVOR_BOMB_ADDR         0x001FFACD
#define TREVOR_MAGNET_ADDR       0x001FFAD1
#define TREVOR_TRACER_ADDR       0x001FFAD3
#define TREVOR_BEAM_ADDR         0x001FFAD5
#define TREVOR_FLASH_ADDR        0x001FFAD7
#define TREVOR_MISSILES_ADDR     0x001FFAD9
#define TREVOR_RING_ADDR         0x001FFADB
#define TREVOR_BOLT_ADDR         0x001FFADD
#define TREVOR_SHIELD_ADDR       0x001FFAEF
#define TREVOR_CUTTER_ADDR       0x001FFAE1
#define TREVOR_LIVES_ADDR        0x001FFD2B
#define TREVOR_PRIMARY_ADDR      0x001FFD41

#define TREVOR_MAX_BOMB_VAL      2
#define TREVOR_MAX_SPECIAL_VAL   9
#define TREVOR_MAX_LIVES_VAL     9
#define TREVOR_MAX_PRIMARY_VAL   5

#define INVALID                  -1

// Boilerplate.
static int32_t sOnLoadEvent = INVALID;
static int32_t sOnEmuFrame = INVALID;
static int32_t sMcFlags = 0;
static const int32_t skMcFlag_Loaded = (1 << 0);

// Setting handles.
static int sBombSettingHandle = INVALID;
static int sAmmoSettingHandle = INVALID;
static int sLivesSettingHandle = INVALID;
static int sPrimarySettingHandle = INVALID;

static uint32_t on_sw_loaded(const int eventHandle, void* pEventData)
{
   const uint64_t hash = bigpemu_get_loaded_fnv1a64();

   sMcFlags = 0;

   if (TREVOR_HASH == hash)
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
      int bombSettingValue = INVALID;
      int ammoSettingValue = INVALID;
      int livesSettingValue = INVALID;
      int primarySettingValue = INVALID;
      bigpemu_get_setting_value(&bombSettingValue, sBombSettingHandle);
      bigpemu_get_setting_value(&ammoSettingValue, sAmmoSettingHandle);
      bigpemu_get_setting_value(&livesSettingValue, sLivesSettingHandle);
      bigpemu_get_setting_value(&primarySettingValue, sPrimarySettingHandle);

      if (bombSettingValue > 0)
      {
         bigpemu_jag_write8(TREVOR_BOMB_ADDR, TREVOR_MAX_BOMB_VAL);
      }

      if (livesSettingValue > 0)
      {
         bigpemu_jag_write8(TREVOR_LIVES_ADDR, TREVOR_MAX_LIVES_VAL);
      }

      if (ammoSettingValue > 0)
      {
         bigpemu_jag_write8(TREVOR_MAGNET_ADDR, TREVOR_MAX_SPECIAL_VAL);
         bigpemu_jag_write8(TREVOR_TRACER_ADDR, TREVOR_MAX_SPECIAL_VAL);
         bigpemu_jag_write8(TREVOR_BEAM_ADDR, TREVOR_MAX_SPECIAL_VAL);
         bigpemu_jag_write8(TREVOR_FLASH_ADDR, TREVOR_MAX_SPECIAL_VAL);
         bigpemu_jag_write8(TREVOR_MISSILES_ADDR, TREVOR_MAX_SPECIAL_VAL);
         bigpemu_jag_write8(TREVOR_RING_ADDR, TREVOR_MAX_SPECIAL_VAL);
         bigpemu_jag_write8(TREVOR_BOLT_ADDR, TREVOR_MAX_SPECIAL_VAL);
         bigpemu_jag_write8(TREVOR_SHIELD_ADDR, TREVOR_MAX_SPECIAL_VAL);
         bigpemu_jag_write8(TREVOR_CUTTER_ADDR, TREVOR_MAX_SPECIAL_VAL);
      }

      if (primarySettingValue > 0)
      {
         bigpemu_jag_write8(TREVOR_PRIMARY_ADDR, TREVOR_MAX_PRIMARY_VAL);
      }
   }
   return 0;
}

void bigp_init()
{
   void* pMod = bigpemu_get_module_handle();

   bigpemu_set_module_usage_flags(pMod, BIGPEMU_MODUSAGE_DETERMINISMWARNING);

   const int catHandle = bigpemu_register_setting_category(pMod, "Trevor Mcfur");
   sBombSettingHandle = bigpemu_register_setting_bool(catHandle, "Max Bomb", 1);
   sLivesSettingHandle = bigpemu_register_setting_bool(catHandle, "Infinite Lives", 1);
   sAmmoSettingHandle = bigpemu_register_setting_bool(catHandle, "Infinite Ammo", 1);
   sPrimarySettingHandle = bigpemu_register_setting_bool(catHandle, "Max Primary Weapon", 1);

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

   sBombSettingHandle = INVALID;
   sAmmoSettingHandle = INVALID;
   sLivesSettingHandle = INVALID;
   sPrimarySettingHandle = INVALID;
}