//__BIGPEMU_SCRIPT_MODULE__
//__BIGPEMU_META_DESC__    "Always win, in White Men Can't Jump."
//__BIGPEMU_META_AUTHOR__  "jguff"

#include "bigpcrt.h"

// Found using BigPEmu System->Load Software screen and highlighting ROM file.
#define WMCJ_HASH             0xDC70C941F5AF75D6ULL

// Found using Pro-Action Replay like ascending counter search script.
#define WMCJ_SCOR1_ADDR       0x000058EF
#define WMCJ_SCOR2_ADDR       0x00005907

#define WMCJ_SCORE_ADVANTAGE  4

// At one point thought this was address for money, but not so certain it is.
// Especially given that it is in a low byte.
// #define WMCJ_MONEY_ADDR	0x000E96C3

#define INVALID               -1

// Boilerplate.
static int32_t sOnLoadEvent = INVALID;
static int32_t sOnEmuFrame = INVALID;
static int32_t sMcFlags = 0;
static const int32_t skMcFlag_Loaded = (1 << 0);

// Setting handles.
static int sScoreSettingHandle = INVALID;

static uint32_t on_sw_loaded(const int eventHandle, void* pEventData)
{
   const uint64_t hash = bigpemu_get_loaded_fnv1a64();

   sMcFlags = 0;

   if (WMCJ_HASH == hash)
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
      int scoreSettingValue = INVALID;
      bigpemu_get_setting_value(&scoreSettingValue, sScoreSettingHandle);

      // According to manual, White Men Can't Jump has game modes
      // that are time ended, number-of-points ended, and combination
      // of both.  Keeping Player1 ahead of Player2 may work for
      // all game modes.
      if (scoreSettingValue > 0)
      {
         // Always keep Player1 ahead by WMCJ_SCORE_ADVANTAGE points.
         const uint8_t score1 = bigpemu_jag_read8(WMCJ_SCOR2_ADDR);
         const uint8_t score2 = bigpemu_jag_read8(WMCJ_SCOR2_ADDR);
         if ((score1 - WMCJ_SCORE_ADVANTAGE - 1) <= score2)
         {
            bigpemu_jag_write8(WMCJ_SCOR1_ADDR, score2 + WMCJ_SCORE_ADVANTAGE);
         }
      }
   }
   return 0;
}

void bigp_init()
{
   void* pMod = bigpemu_get_module_handle();

   bigpemu_set_module_usage_flags(pMod, BIGPEMU_MODUSAGE_DETERMINISMWARNING);

   const int catHandle = bigpemu_register_setting_category(pMod, "WMCJ");
   sScoreSettingHandle = bigpemu_register_setting_bool(catHandle, "Always 4 Points Ahead", 1);

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

   sScoreSettingHandle = INVALID;
}