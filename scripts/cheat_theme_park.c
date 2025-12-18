//__BIGPEMU_SCRIPT_MODULE__
//__BIGPEMU_META_DESC__    "Infinite money, in Theme Park."
//__BIGPEMU_META_AUTHOR__  "jguff"

#include "bigpcrt.h"

// Found using BigPEmu System->Load Software screen and highlighting ROM file.
#define THEME_PARK_HASH                0x75F4878C35A30A60ULL

// Found using a Pro Action Replay like script that finds addresses of always decreasing values.
#define THEME_PARK_MONEY_ADDR          0x000a3684

// Unsure what the most expensive item/transaction is.
// Guessing 50,000,000 is high enough.
// Japan theme park, at start of game, seemed most expensiv at cost of 20,000,000.
// So likely nothing more expensive then 20,000,000, so 50,000,000 ought to be plenty enough.
#define THEME_PARK_MONEY_VALUE         50000000

// Found using a Pro Action Replay like script.
#define THEME_PARK_MONEY_AT_START_ADDR 0x000a3690

#define INVALID                        -1

// Boilerplate.
static int32_t sOnLoadEvent = INVALID;
static int32_t sOnEmuFrame = INVALID;
static int32_t sMcFlags = 0;
static const int32_t skMcFlag_Loaded = (1 << 0);

// Setting handles.
static int sMoneySettingHandle = INVALID;

static uint32_t on_sw_loaded(const int eventHandle, void* pEventData)
{
   const uint64_t hash = bigpemu_get_loaded_fnv1a64();

   sMcFlags = 0;

   if (THEME_PARK_HASH == hash)
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
      int moneySettingValue = INVALID;
      bigpemu_get_setting_value(&moneySettingValue, sMoneySettingHandle);

      if (moneySettingValue)
      {
         bigpemu_jag_write32(THEME_PARK_MONEY_ADDR, THEME_PARK_MONEY_VALUE);

         // Appears Select Initial Theme Park Location screen
         // has its budget stored at seperate address below.
         // However, it didn't seem to ever respect this address on initial
         // park location selection.
         // After one sim year in UK location, you can auction off UK
         // location and purchase the most expensive 20,000,000 Japan
         // location
         bigpemu_jag_write32(THEME_PARK_MONEY_AT_START_ADDR, THEME_PARK_MONEY_VALUE);
      }
   }
   return 0;
}

void bigp_init()
{
   void* pMod = bigpemu_get_module_handle();

   bigpemu_set_module_usage_flags(pMod, BIGPEMU_MODUSAGE_DETERMINISMWARNING);

   const int catHandle = bigpemu_register_setting_category(pMod, "Theme Park");
   sMoneySettingHandle = bigpemu_register_setting_bool(catHandle, "Infinite Money", 1);

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

   sMoneySettingHandle = INVALID;
}