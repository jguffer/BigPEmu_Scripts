//__BIGPEMU_SCRIPT_MODULE__
//__BIGPEMU_META_DESC__    "Infinite health and password unlock, in Flashback."
//__BIGPEMU_META_AUTHOR__  "jguff"

#include "bigpcrt.h"

// Found using BigPEmu System->Load Software screen and highlighting ROM file.
#define FBACK_HASH                  0xDE2D2EFB0D1B64DEULL

// Found using Pro Action Replay like scripts on memory dump from Noesis
#define FBACK_SHIELD_ADDR           0x00004811
#define FBACK_SHIELD_VALUE          9

// 6 byte ascii password.
//
// Found after starting game, selecting Password screen, then entering in
// password of BKTVMD, then using Noesis Memory Window Find capability, to
// find address pointing to BKTVMD.
#define FBACK_PASSWORD_INPUT_ADDR   0x0000D4D6
#define FBACK_PASSWORD_LENGTH       6
#define FBACK_NUM_STAGES            7

// Found using Pro-Action Replay like increasing value search script.
//
// If you need to stop overwriting the password at some point after
// password is applied, FBACK_SCORE might be a decent variable to poll
// for state change to indicate that stage has been started and can stop
// overwriting password.
// So far looks like this will be unneccessary.
#define FBACK_SCORE_ADDR            0x0000CDAC

// Found using Noesis Memory Window Find capability to search for ASCII password
// which is displayed in top-middle of screen, when user starts new level.
//
// Seperate locations for input and output of passwords, suggests
// that continually writing to the input password before, during, and after
// stage for that password will not bork the transitions between stages.
#define FBACK_PASSWORD_OUTPUT_ADDR  0x0000CE36
#define FBACK_NUM_LEVELS            7

#define INVALID                     -1

// Boilerplate.
static int32_t sOnLoadEvent = INVALID;
static int32_t sOnEmuFrame = INVALID;
static int32_t sMcFlags = 0;
static const int32_t skMcFlag_Loaded = (1 << 0);

// Setting handles.
static int sHealthSettingHandle = INVALID;
static int sEasySettingHandle = INVALID;
static int sMediumSettingHandle = INVALID;
static int sHardSettingHandle = INVALID;

enum difficulty{EASY, MEDIUM, HARD, NUM_DIFFICULTIES} difficultyEnum;
static const uint8_t sPasswords[FBACK_NUM_STAGES][NUM_DIFFICULTIES][FBACK_PASSWORD_LENGTH] = {
   {"LETY__", "RISING", "RODEO_"},  // Stage 1 - Planet Titan
   {"BOXER_", "ORDO__", "BINGO_"},  // Stage 2 - New Washington
   {"EAGLE_", "PROFIT", "LSTED_"},  // Stage 3 - Death Tower
   {"STKTON", "PRIZE_", "DARTS_"},  // Stage 4 - Earth
   {"TICKET", "SKAEPS", "BUDDY_"},  // Stage 5 - Secret Base
   {"SUITE_", "HITTER", "MUSIC_"},  // Stage 6 - Morph Planet I
   {"PHASER", "TWIN__", "SHOGI_"}   // Stage 7 - Morph Planet II
};

static uint32_t on_sw_loaded(const int eventHandle, void* pEventData)
{
   const uint64_t hash = bigpemu_get_loaded_fnv1a64();

   sMcFlags = 0;

   if (FBACK_HASH == hash)
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
      int easySettingValue = INVALID;
      int mediumSettingValue = INVALID;
      int hardSettingValue = INVALID;
      bigpemu_get_setting_value(&healthSettingValue, sHealthSettingHandle);
      bigpemu_get_setting_value(&easySettingValue, sEasySettingHandle);
      bigpemu_get_setting_value(&mediumSettingValue, sMediumSettingHandle);
      bigpemu_get_setting_value(&hardSettingValue, sHardSettingHandle);

      if (healthSettingValue > 0)
      {
         bigpemu_jag_write8(FBACK_SHIELD_ADDR, FBACK_SHIELD_VALUE);
      }

      // Determine stage and difficulty, if requested to overwrite
      // password.  Precedence given to higher difficulty.
      int stage = INVALID;
      int difficulty = INVALID;
      if (hardSettingValue > 0)
      {
         stage = hardSettingValue;
         difficulty = HARD;
      }
      else if (mediumSettingValue > 0)
      {
         stage = mediumSettingValue;
         difficulty = MEDIUM;
      }
      else if (easySettingValue > 0)
      {
         stage = easySettingValue;
         difficulty = EASY;
      }

      // Overwrite password if appropriate.
      if (stage > 0)
      {
         for (uint32_t charIdx = 0; charIdx < FBACK_PASSWORD_LENGTH; ++charIdx)
         {
            // Subtract 1 from stage, to convert from 1-based BigPEmu user setting, to 0-based array index.
            bigpemu_jag_write8(FBACK_PASSWORD_INPUT_ADDR + charIdx, sPasswords[stage - 1][difficulty][charIdx]);
         }
      }

      // TODO Someone needs to apply password and then finish stage,
      //      to verify that do not need to stop writing password
      //      at some point.
   }
   return 0;
}

void bigp_init()
{
   void* pMod = bigpemu_get_module_handle();

   bigpemu_set_module_usage_flags(pMod, BIGPEMU_MODUSAGE_DETERMINISMWARNING);

   // Register cheat settings with BigPEmu GUI interface.
   const int catHandle = bigpemu_register_setting_category(pMod, "Flashback");
   sHealthSettingHandle = bigpemu_register_setting_bool(catHandle, "Infinite Health", 1);
   sEasySettingHandle = bigpemu_register_setting_int_full(catHandle, "Password For Easy Stage", INVALID, INVALID, FBACK_NUM_LEVELS, 1);
   sMediumSettingHandle = bigpemu_register_setting_int_full(catHandle, "Password For Medium Stage", INVALID, INVALID, FBACK_NUM_LEVELS, 1);
   sHardSettingHandle = bigpemu_register_setting_int_full(catHandle, "Password For Hard Stage", INVALID, INVALID, FBACK_NUM_LEVELS, 1);

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
   sEasySettingHandle = INVALID;
   sMediumSettingHandle = INVALID;
   sHardSettingHandle = INVALID;
}