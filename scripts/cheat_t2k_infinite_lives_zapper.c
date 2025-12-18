//__BIGPEMU_SCRIPT_MODULE__
//__BIGPEMU_META_DESC__    "Infinite lives and superzapper, in 2000 mode of Tempest 2000."
//__BIGPEMU_META_AUTHOR__  "jguff"

#include "bigpcrt.h"

// Found using BigPEmu System->Load Software screen and highlighting ROM file.
#define T2K_HASH           0x0030407D0269CE03ULL

// Found using Pro-Action Replay counter value search script.
#define T2K_LIVES_ADDR     0x00007D3D
#define T2K_ZAPPER_ADDR    0x0000CA68
#define T2K_LIVES_VAL      2
#define T2K_ZAPPER_VAL     1

#define INVALID            -1

// Boilerplate.
static int32_t sOnLoadEvent = INVALID;
static int32_t sOnEmuFrame = INVALID;
static int32_t sMcFlags = 0;
static const int32_t skMcFlag_Loaded = (1 << 0);

// Setting handles.
static int sLivesSettingHandle = INVALID;
static int sZapperSettingHandle = INVALID;

static uint32_t on_sw_loaded(const int eventHandle, void* pEventData)
{
   const uint64_t hash = bigpemu_get_loaded_fnv1a64();

   sMcFlags = 0;

   if (T2K_HASH == hash)
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
      int livesSettingValue = INVALID;
      int zapperSettingValue = INVALID;
      bigpemu_get_setting_value(&livesSettingValue, sLivesSettingHandle);
      bigpemu_get_setting_value(&zapperSettingValue, sZapperSettingHandle);

      if (livesSettingValue > 0)
      {
         bigpemu_jag_write8(T2K_LIVES_ADDR, T2K_LIVES_VAL);
      }

      if (zapperSettingValue > 0)
      {
         bigpemu_jag_write16(0x0000CA68, T2K_ZAPPER_VAL);
      }
   }
   return 0;
}

void bigp_init()
{
   void* pMod = bigpemu_get_module_handle();

   bigpemu_set_module_usage_flags(pMod, BIGPEMU_MODUSAGE_DETERMINISMWARNING);

   const int catHandle = bigpemu_register_setting_category(pMod, "Tempest 2000");
   sLivesSettingHandle = bigpemu_register_setting_bool(catHandle, "Infinite Lives", 1);
   sZapperSettingHandle = bigpemu_register_setting_bool(catHandle, "Infinite Superzapper", 1);

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

   sLivesSettingHandle = INVALID;
   sZapperSettingHandle = INVALID;
}