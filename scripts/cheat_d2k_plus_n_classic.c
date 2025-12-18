//__BIGPEMU_SCRIPT_MODULE__
//__BIGPEMU_META_DESC__    "Infinite lives, bombs, lightning, and level select, in classic and plus modes of Defender 2000."
//__BIGPEMU_META_AUTHOR__  "jguff"

#include "bigpcrt.h"

// Found using BigPEmu System->Load Software screen and highlighting ROM file.
#define D2K_HASH                       0x6B591BA55B0BCBD7ULL

// Found using Pro-Action Replay like counter and decreasing value scripts.
#define D2K_PLUS_N_CLASSIC_LIVES_ADDR  0x001AF0DF
#define D2K_PLUS_N_CLASSIC_BOMBS_ADDR  0x001AF0E1
#define D2K_PLUS_LIGHTNING_ADDR        0x001AF0F5
#define D2K_PLUS_N_CLASSIC_LEVEL_ADDR  0x001AF0DD
#define D2K_PLUS_N_CLASSIC_LIVES_VALUE 5
#define D2K_PLUS_N_CLASSIC_BOMBS_VALUE 11
#define D2K_PLUS_LIGHTNING_VALUE       0x0E

// Search for level also produced the following hit, which seemed be be
// zero based representation of level, while above is 1-based representation
// of level.  Writing to this address didn't seem to impact level.
//     0x0018CD93

// Found via inspection around the above addresses, using Noesis Memory Window.
// 8 bytes.  Each byte only assumes values betwen 0x00 and 0x09.
#define D2K_PLUS_SCORE_ADDR            0x001AF0E2
#define D2K_PLUS_NUM_SCORE_BYTES       8

#define D2K_PLUS_NUM_LEVELS            99

#define INVALID                        -1

// Boilerplate.
static int32_t sOnLoadEvent = INVALID;
static int32_t sOnEmuFrame = INVALID;
static int32_t sMcFlags = 0;
static const int32_t skMcFlag_Loaded = (1 << 0);

// Setting handles.
static int sLivesSettingHandle = INVALID;
static int sBombsSettingHandle = INVALID;
static int sLightningSettingHandle = INVALID;
static int sLevelSettingHandle = INVALID;

static uint32_t on_sw_loaded(const int eventHandle, void* pEventData)
{
   const uint64_t hash = bigpemu_get_loaded_fnv1a64();

   sMcFlags = 0;

   if (D2K_HASH == hash)
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
      int bombsSettingValue = INVALID;
      int lightningSettingValue = INVALID;
      int levelSettingValue = INVALID;
      bigpemu_get_setting_value(&livesSettingValue, sLivesSettingHandle);
      bigpemu_get_setting_value(&bombsSettingValue, sBombsSettingHandle);
      bigpemu_get_setting_value(&lightningSettingValue, sLightningSettingHandle);
      bigpemu_get_setting_value(&levelSettingValue, sLevelSettingHandle);

      if (livesSettingValue > 0)
      {
         bigpemu_jag_write8(D2K_PLUS_N_CLASSIC_LIVES_ADDR, D2K_PLUS_N_CLASSIC_LIVES_VALUE);
      }

      if (bombsSettingValue > 0)
      {
         bigpemu_jag_write8(D2K_PLUS_N_CLASSIC_BOMBS_ADDR, D2K_PLUS_N_CLASSIC_BOMBS_VALUE);
      }

      if (lightningSettingValue > 0)
      {
         bigpemu_jag_write8(D2K_PLUS_LIGHTNING_ADDR, D2K_PLUS_LIGHTNING_VALUE);
      }

      if (levelSettingValue > 1)
      {
         // digitSum will be non-zero once score changes.
         uint8_t digitSum = 0;
         for (uint32_t i = 0; i < D2K_PLUS_NUM_SCORE_BYTES; ++i)
         {
            digitSum += bigpemu_jag_read8(D2K_PLUS_SCORE_ADDR + i);
         }

         // Overwrite level information, until score changes.
         if (0 == digitSum)
         {
            bigpemu_jag_write8(D2K_PLUS_N_CLASSIC_LEVEL_ADDR, levelSettingValue); // 1-based value.
         }
      }

      // Thought about searching disassembly for hit detection, but
      // don't think it worth it right now.  Don't have labels, (unlike
      // T2K), and level doesn't appear to restart on player death (unlike)
      // T2K), so think this script will make D2K pretty beatable, even
      // if all you want to do is use blaster and no bombs.
   }
   return 0;
}

void bigp_init()
{
   void* pMod = bigpemu_get_module_handle();

   bigpemu_set_module_usage_flags(pMod, BIGPEMU_MODUSAGE_DETERMINISMWARNING);

   // Register cheat settings with BigPEmu GUI interface.
   const int catHandle = bigpemu_register_setting_category(pMod, "Defender 2K - + N Classic");
   sLivesSettingHandle = bigpemu_register_setting_bool(catHandle, "Infinite Lives", 1);
   sBombsSettingHandle = bigpemu_register_setting_bool(catHandle, "Infinite Bombs", 1);
   sLightningSettingHandle = bigpemu_register_setting_bool(catHandle, "+ Mode Infinite Lightning", 1);
   sLevelSettingHandle = bigpemu_register_setting_int_full(catHandle, "Start At Level", INVALID, INVALID, D2K_PLUS_NUM_LEVELS, 1);

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
   sBombsSettingHandle = INVALID;
   sLightningSettingHandle = INVALID;
   sLevelSettingHandle = INVALID;
}