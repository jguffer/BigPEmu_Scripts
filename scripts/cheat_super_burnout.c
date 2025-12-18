//__BIGPEMU_SCRIPT_MODULE__
//__BIGPEMU_META_DESC__    "Always be deemed first place winner, in Super Burnout."
//__BIGPEMU_META_AUTHOR__  "jguff"

#include "bigpcrt.h"

// Found using BigPEmu System->Load Software screen and highlighting ROM file.
#define SBURNOUT_HASH            0x1D8C0621EB876398ULL

#define SBURNOUT_POSITION_ADDR   0x0006E4C3
#define SBURNOUT_POSITION_VALUE  1

// Some notes below when briefly attempted to unlock Punisher cycle
//
// Prefixed number is the display order, clicking right arrow key, after having deleted EEPROM.
//    5 - 0x0001C526 "wheels of
//    0 - 0x0001C58F "lightning
//    1 - 0x0001C5F6 "sliding
//    2 - 0x0001C3F0 "killing
//    3 - 0x0001C457 "super  1C461 "rabbit
//    4 - 0x0001C4C1 "reflex
//
//    1.5 WTF? - 0x0001C665 "punisher
//       Maybe not WTF so much.  This address follows 1, and addresses roll back to beginning at 2.0.

#define INVALID                  -1

// Boilerplate.
static int32_t sOnLoadEvent = INVALID;
static int32_t sOnEmuFrame = INVALID;
static int32_t sMcFlags = 0;
static const int32_t skMcFlag_Loaded = (1 << 0);

// Setting handles.
static int sPositionSettingHandle = INVALID;

static uint32_t on_sw_loaded(const int eventHandle, void* pEventData)
{
   const uint64_t hash = bigpemu_get_loaded_fnv1a64();

   sMcFlags = 0;

   if (SBURNOUT_HASH == hash)
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
      int positionSettingValue = INVALID;
      bigpemu_get_setting_value(&positionSettingValue, sPositionSettingHandle);

      if (positionSettingValue > 0)
      {
         // Always be marked as position one (the winner), in spite
         // of your actual position or time.
         bigpemu_jag_write8(SBURNOUT_POSITION_ADDR,SBURNOUT_POSITION_VALUE);
      }
   }
   return 0;
}

void bigp_init()
{
   void* pMod = bigpemu_get_module_handle();

   bigpemu_set_module_usage_flags(pMod, BIGPEMU_MODUSAGE_DETERMINISMWARNING);

   const int catHandle = bigpemu_register_setting_category(pMod, "Super Burnout");
   sPositionSettingHandle = bigpemu_register_setting_bool(catHandle, "Always 1st Place FINISH", 1);

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

   sPositionSettingHandle = INVALID;
}