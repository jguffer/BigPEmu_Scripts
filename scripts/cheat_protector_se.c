//__BIGPEMU_SCRIPT_MODULE__
//__BIGPEMU_META_DESC__    "Infinite lives, shields, health, bombs, money, level select in Protector SE."
//__BIGPEMU_META_AUTHOR__  "jguff"

#include "bigpcrt.h"

// Found using BigPEmu System->Load Software screen and highlighting ROM file.
#define PSE_HASH                 0x8A5B268040C78624ULL

// Found using Pro Action Replay like script and Noesis Memory Window.
#define PSE_LIVES_ADDR           0x00021D75
#define PSE_BOMBS_ADDR           0x00021D79
#define PSE_MONEY_ADDR           0x00021D7C
#define PSE_SHIELDS_ADDR         0x00021D81
#define PSE_HEALTH_ADDR          0x00021D85
#define PSE_SCORE_ADDR           0x00021D60
#define PSE_LEVEL_ADDRA          0x0000C1B5
#define PSE_LEVEL_ADDRB          0x0000C265
#define PSE_LIVES_VALUE          2
#define PSE_BOMBS_VALUE          3
#define PSE_MONEY_VALUE          0x0909
#define PSE_SHIELDS_VALUE        3
#define PSE_HEALTH_VALUE         5
#define PSE_NUM_LEVELS           41

// NTSC
#define PSE_FRAMES_PER_SECOND    60

#define PSE_LEVEL_WINDOW_SECONDS 45
#define PSE_LEVEL_WINDOW_TICKS   PSE_FRAMES_PER_SECOND * PSE_LEVEL_WINDOW_SECONDS


// TODO: The second player values are stored 2 bytes to right of each of the first players values.
//       Suspect noone is using BigPEmu (or any other emulator) to play this game with 2 players,
//       so not going to bother implementing second player cheats.

#define INVALID                  -1

// Boilerplate.
static int32_t sOnLoadEvent = INVALID;
static int32_t sOnEmuFrame = INVALID;
static int32_t sMcFlags = 0;
static const int32_t skMcFlag_Loaded = (1 << 0);

// Setting handles.
static int sLivesSettingHandle = INVALID;
static int sBombsSettingHandle = INVALID;
static int sMoneySettingHandle = INVALID;
static int sShieldsSettingHandle = INVALID;
static int sHealthSettingHandle = INVALID;
static int sLevelSettingHandle = INVALID;

static int sLevelDelayCounter = -1;

static uint32_t on_sw_loaded(const int eventHandle, void* pEventData)
{
   const uint64_t hash = bigpemu_get_loaded_fnv1a64();

   sMcFlags = 0;

   if (PSE_HASH == hash)
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
      int moneySettingValue = INVALID;
      int shieldsSettingValue = INVALID;
      int healthSettingValue = INVALID;
      int levelSettingValue = INVALID;
      bigpemu_get_setting_value(&livesSettingValue, sLivesSettingHandle);
      bigpemu_get_setting_value(&bombsSettingValue, sBombsSettingHandle);
      bigpemu_get_setting_value(&moneySettingValue, sMoneySettingHandle);
      bigpemu_get_setting_value(&shieldsSettingValue, sShieldsSettingHandle);
      bigpemu_get_setting_value(&healthSettingValue, sHealthSettingHandle);
      bigpemu_get_setting_value(&levelSettingValue, sLevelSettingHandle);

      if (livesSettingValue > 0)
      {
         bigpemu_jag_write8(PSE_LIVES_ADDR, PSE_LIVES_VALUE);
      }

      if (bombsSettingValue > 0)
      {
         bigpemu_jag_write8(PSE_BOMBS_ADDR, PSE_BOMBS_VALUE);
      }

      if (moneySettingValue > 0)
      {
         bigpemu_jag_write8(PSE_MONEY_ADDR + 0, PSE_MONEY_VALUE >> 8);
         bigpemu_jag_write8(PSE_MONEY_ADDR + 1, PSE_MONEY_VALUE & 0xFF);
      }

      if (shieldsSettingValue > 0)
      {
         bigpemu_jag_write8(PSE_SHIELDS_ADDR, PSE_SHIELDS_VALUE);
      }

      if (healthSettingValue > 0)
      {
         bigpemu_jag_write8(PSE_HEALTH_ADDR, PSE_HEALTH_VALUE);
      }

      if (levelSettingValue > 1)
      {
         if (sLevelDelayCounter <= PSE_LEVEL_WINDOW_TICKS)
         {
            // Overwrite level information for set number of ticks.
            //
            // Tried writing this when score was equal to zero, and was unsuccessful.
            // The intro level text was correct, the level background was correct,
            // but the enemies were from Level1.
            //
            // PSE_LEVEL_WINDOW_TICKS can be set too short for early levels, such that player
            // has to repeat the level, if user completes level before ticks have completed.
            //
            // TODO: Develop better approach to know when to stop overwriting level information.
            //       Currently user must know about the window of ticks in which to get the
            //       cheat enabled.
            bigpemu_jag_write8(PSE_LEVEL_ADDRA, levelSettingValue - 1);
            bigpemu_jag_write8(PSE_LEVEL_ADDRB, levelSettingValue - 1);

            ++sLevelDelayCounter;
         }
      }
   }
   return 0;
}

void bigp_init()
{
   void* pMod = bigpemu_get_module_handle();

   bigpemu_set_module_usage_flags(pMod, BIGPEMU_MODUSAGE_DETERMINISMWARNING);

   const int catHandle = bigpemu_register_setting_category(pMod, "Protectr SE");
   sLivesSettingHandle = bigpemu_register_setting_bool(catHandle, "Infinite Lives", 1);
   sBombsSettingHandle = bigpemu_register_setting_bool(catHandle, "Infinite Bombs", 1);
   sMoneySettingHandle = bigpemu_register_setting_bool(catHandle, "Infinite Money", 1);
   sShieldsSettingHandle = bigpemu_register_setting_bool(catHandle, "Infinite Shields", 1);
   sHealthSettingHandle = bigpemu_register_setting_bool(catHandle, "Infinite Health", 1);
   sLevelSettingHandle = bigpemu_register_setting_int_full(catHandle, "Skip To Level", INVALID, INVALID, PSE_NUM_LEVELS, 1);

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
   sMoneySettingHandle = INVALID;
   sShieldsSettingHandle = INVALID;
   sHealthSettingHandle = INVALID;
   sLevelSettingHandle = INVALID;

   sLevelDelayCounter = INVALID;
}