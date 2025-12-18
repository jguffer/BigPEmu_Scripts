//__BIGPEMU_SCRIPT_MODULE__
//__BIGPEMU_META_DESC__    "Infinite lives, health, time, bombs, unlock levels, in Zool 2."
//__BIGPEMU_META_AUTHOR__  "jguff"

#include "bigpcrt.h"

// Found using BigPEmu System->Load Software screen and highlighting ROM file.
#define ZOOL2_HASH               0x55C927B5C2AD250EULL

// Found using Pro-Action Replay like script on memory dump.
#define ZOOL2_LIVES_ADDR         0x0000C465
#define ZOOL2_HEALTH_ADDR        0x0000C451
#define ZOOL2_TIME_ADDR          0x0000C45C
#define ZOOL2_BOMB_ADDR          0x0000417F
#define ZOOL2_TOKENS_ADDR        0x0000410A
#define ZOOL2_LEVEL_ADDR         0x00004A48
#define ZOOL2_STAGE_ADDR         0x00004A49
#define ZOOL2_LIVES_VALUE        9
#define ZOOL2_HEALTH_VALUE       5
#define ZOOL2_TIME_VALUE         999
#define ZOOL2_BOMB_VALUE         1
#define ZOOL2_NUM_LEVELS         6
#define ZOOL2_NUM_STAGES_IN_LVL  3

// Boilerplate.
static int32_t sOnLoadEvent = -1;
static int32_t sOnEmuFrame = -1;
static int32_t sMcFlags = 0;
static const int32_t skMcFlag_Loaded = (1 << 0);

#define INVALID	-1

// Setting handles.
static int sLivesSettingHandle = INVALID;
static int sHealthSettingHandle = INVALID;
static int sTimeSettingHandle = INVALID;
static int sBombSettingHandle = INVALID;
static int sLevelSettingHandle = INVALID;

static uint32_t on_sw_loaded(const int eventHandle, void* pEventData)
{
   const uint64_t hash = bigpemu_get_loaded_fnv1a64();

   sMcFlags = 0;

   if (ZOOL2_HASH == hash)
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
      int healthSettingValue = INVALID;
      int timeSettingValue = INVALID;
      int bombSettingValue = INVALID;
      int levelSettingValue = INVALID;
      bigpemu_get_setting_value(&livesSettingValue, sLivesSettingHandle);
      bigpemu_get_setting_value(&healthSettingValue, sHealthSettingHandle);
      bigpemu_get_setting_value(&timeSettingValue, sTimeSettingHandle);
      bigpemu_get_setting_value(&bombSettingValue, sBombSettingHandle);
      bigpemu_get_setting_value(&levelSettingValue, sLevelSettingHandle);

      if (livesSettingValue > 0)
      {
         bigpemu_jag_write8(ZOOL2_LIVES_ADDR, ZOOL2_LIVES_VALUE);
      }

      if (healthSettingValue > 0)
      {
         bigpemu_jag_write8(ZOOL2_HEALTH_ADDR, ZOOL2_HEALTH_VALUE);
      }

      if (timeSettingValue > 0)
      {
         // Get the number of items/tokens collected.  Avoid the 16-bit-assignment-clobbers-memory bug as well.
         const uint32_t numTokens = (bigpemu_jag_read8(ZOOL2_TOKENS_ADDR + 0) << 8) | bigpemu_jag_read8(ZOOL2_TOKENS_ADDR + 1);

         // Set time every emulation frame would likely result in infinite loop when end-of-stage score tally occurs
         // (and decrements time down to zero as part of that score tally).
         //
         // Instead set extended time value, while no items/tokens have been collected.
         // Once first item/token is collected, time countdown occurs.
         //
         // This approach should avoid infinite loop at end of stage, and be applied
         // at the beginning of every stage.
         //
         // TODO: someone should test this a bit more.
         if (0 == numTokens)
         {
            // Avoid the 16-bit-assignment-clobbers-memory bug.
            bigpemu_jag_write8(ZOOL2_TIME_ADDR + 0, ZOOL2_TIME_VALUE >> 8);
            bigpemu_jag_write8(ZOOL2_TIME_ADDR + 1, ZOOL2_TIME_VALUE & 0xFF);
         }
      }

      if (bombSettingValue > 0)
      {
         bigpemu_jag_write8(ZOOL2_BOMB_ADDR, ZOOL2_BOMB_VALUE);
      }

      if (levelSettingValue > 0)
      {
         // User has selected to start beyond 1st level, 1st stage.
         //
         // If jaguar memory has level set to first level and stage set to first stage.
         const uint8_t level = bigpemu_jag_read8(ZOOL2_LEVEL_ADDR);
         const uint8_t stage = bigpemu_jag_read8(ZOOL2_STAGE_ADDR);
         if ((0 == level) && (0 == stage))
         {
            // The level and stage values in Jaguar memory are 0-based.
            // The BigPEmu level value is 1-based and combination of level and stage.
            //
            // Write the level and stage specified by user.
            //
            // TODO: Someone should test this further.
            bigpemu_jag_write8(ZOOL2_LEVEL_ADDR, (levelSettingValue - 1) / ZOOL2_NUM_STAGES_IN_LVL);
            bigpemu_jag_write8(ZOOL2_STAGE_ADDR, (levelSettingValue - 1) % ZOOL2_NUM_STAGES_IN_LVL);
         }
      }
   }
   return 0;
}

void bigp_init()
{
   void* pMod = bigpemu_get_module_handle();

   bigpemu_set_module_usage_flags(pMod, BIGPEMU_MODUSAGE_DETERMINISMWARNING);

   const int catHandle = bigpemu_register_setting_category(pMod, "Zool 2");
   sLivesSettingHandle = bigpemu_register_setting_bool(catHandle, "Infinite Lives", 1);
   sHealthSettingHandle = bigpemu_register_setting_bool(catHandle, "Infinite Health", 1);
   sTimeSettingHandle = bigpemu_register_setting_bool(catHandle, "Extra Time", 1);
   sBombSettingHandle = bigpemu_register_setting_bool(catHandle, "Infinite Bombs", 1);
   const int maxValue = ((ZOOL2_NUM_LEVELS - 1) * ZOOL2_NUM_STAGES_IN_LVL) + 1;  // The last level has only 1 stage.
   sLevelSettingHandle = bigpemu_register_setting_int_full(catHandle, "Start At Level", INVALID, INVALID, maxValue, 1);

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
   sHealthSettingHandle = INVALID;
   sTimeSettingHandle = INVALID;
   sBombSettingHandle = INVALID;
   sLevelSettingHandle = INVALID;
}