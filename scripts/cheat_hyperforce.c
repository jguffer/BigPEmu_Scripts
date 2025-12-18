//__BIGPEMU_SCRIPT_MODULE__
//__BIGPEMU_META_DESC__    "Infinite lives, health, ammo, time, money, and level selct, in Hyperforce."
//__BIGPEMU_META_AUTHOR__  "jguff"

#include "bigpcrt.h"

// Found using BigPEmu System->Load Software screen and highlighting ROM file.
#define HFORCE_HASH           0x5E7D34816A56842FULL

// Found using Pro Action Replay like script.
#define HFORCE_LIVES_ADDR     0x000EF099
#define HFORCE_LIVES_VALUE    5

// Found by Noesis Memory Window inspection, after having located lives memory address.
#define HFORCE_HEALTH_ADDR    0x000EF086
#define HFORCE_MONEY_ADDR     0x000EF082   //(for display purposes 2 zeros trail the value at this address.)
#define HFORCE_HEALTH_VALUE   312

// 50,000 ought to help out.  Keep value under 2^16.
#define HFORCE_MONEY_VALUE    50000

//score_addr=0x000EF08B //-14 bytes for score (for display purposes 2 zeros trail.)

// Found by manually inspecting/needling of memory, with a bit of guesswork, after
// locating HFORCE_LIVES_ADDR, HFORCE_HEALTH_ADDR, HFORCE_MONEY_ADDR.
#define HFORCE_WEAPON2_UNLOCK_ADDR  0x000EF09b
#define HFORCE_WEAPON3_UNLOCK_ADDR  HFORCE_WEAPON2_UNLOCK_ADDR + 1
#define HFORCE_WEAPON4_UNLOCK_ADDR  HFORCE_WEAPON2_UNLOCK_ADDR + 2
#define HFORCE_WEAPON5_UNLOCK_ADDR  HFORCE_WEAPON2_UNLOCK_ADDR + 3
#define HFORCE_WEAPON2_AMMO_ADDR    0x000EF0A1
#define HFORCE_WEAPON3_AMMO_ADDR    HFORCE_WEAPON2_AMMO_ADDR + 1
#define HFORCE_WEAPON4_AMMO_ADDR    HFORCE_WEAPON2_AMMO_ADDR + 2
#define HFORCE_WEAPON5_AMMO_ADDR    HFORCE_WEAPON2_AMMO_ADDR + 3
#define HFORCE_UNLOCK_VALUE         1
#define HFORCE_AMMO_VALUE           20

// Found by running Pro Action Replay like script, after the timer shows up
// in gameplay.  Counts down from 99 to 0, and at 0 player dies.  This does
// not count down from beginning of level.  Some  other counter must be
// incrementing/decrementing, and at some point the other counter reaches a
// value, where code starts decrementing this counter.
// Didn't find the other counter.
#define HFORCE_NO_TIMEOUT_ADDR      0x000F5B37
#define HFORCE_NO_TIMEOUT_VALUE     100

// Found by analyzing memory near other addresses above, using Noesis Memory Window.
#define HFORCE_LEVEL_ADDR           0x000EF095

// NTSC
#define HFORCE_FRAMES_PER_SECOND    60

// Time/ticks to delay level change, until at least the title screen appears.
#define HFORCE_LEVEL_DELAY_SECONDS  5
#define HFORCE_LEVEL_DELAY_TICKS    HFORCE_FRAMES_PER_SECOND * HFORCE_LEVEL_DELAY_SECONDS

#define HFORCE_NUM_LEVELS           24

// After reviewing a youtube playthrough, it appears player enters weapons
// shop after 2-3 minute long first level.  At that point, player should be
// able to buy anything player wants.  The above cheats for weapon unlock
// and infinite ammo may be pointless, given infinite money.
// According to manual each weapon has powerbar associated to the weapon,
// and presumably powerbar can be upgraded as well.  Addresses should exist
// for weapon power, but again the infinite money and weapon shop after 2-3
// minute Level1 should allow player to max out weapon after 2-3 minutes of
// play.
// Stick a fork in this, its (over)done.

#define INVALID   -1

// Boilerplate.
static int32_t sOnLoadEvent = INVALID;
static int32_t sOnEmuFrame = INVALID;
static int32_t sMcFlags = 0;
static const int32_t skMcFlag_Loaded = (1 << 0);

static int sLevelDelayCounter = -1;

// Setting handles.
static int sLivesSettingHandle = INVALID;
static int sHealthSettingHandle = INVALID;
static int sMoneySettingHandle = INVALID;
static int sWeaponsSettingHandle = INVALID;
static int sAmmoSettingHandle = INVALID;
static int sNoTimeoutSettingHandle = INVALID;
static int sLevelSettingHandle = INVALID;

static uint32_t on_sw_loaded(const int eventHandle, void* pEventData)
{
   const uint64_t hash = bigpemu_get_loaded_fnv1a64();

   sMcFlags = 0;

   if (HFORCE_HASH == hash)
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
      int moneySettingValue = INVALID;
      int weaponsSettingValue = INVALID;
      int ammoSettingValue = INVALID;
      int noTimeoutSettingValue = INVALID;
      int levelSettingValue = INVALID;
      bigpemu_get_setting_value(&livesSettingValue, sLivesSettingHandle);
      bigpemu_get_setting_value(&healthSettingValue, sHealthSettingHandle);
      bigpemu_get_setting_value(&moneySettingValue, sMoneySettingHandle);
      bigpemu_get_setting_value(&weaponsSettingValue, sWeaponsSettingHandle);
      bigpemu_get_setting_value(&ammoSettingValue, sAmmoSettingHandle);
      bigpemu_get_setting_value(&noTimeoutSettingValue, sNoTimeoutSettingHandle);
      bigpemu_get_setting_value(&levelSettingValue, sLevelSettingHandle);

      if (livesSettingValue > 0)
      {
         bigpemu_jag_write8(HFORCE_LIVES_ADDR, HFORCE_LIVES_VALUE);
      }

      if (healthSettingValue > 0)
      {
         bigpemu_jag_write16(HFORCE_HEALTH_ADDR, HFORCE_HEALTH_VALUE);
      }

      if (moneySettingValue > 0)
      {
         // Need to get past first level, before you can spend/upgrade to
         // all weapons with max ammo for each weapon.
         bigpemu_jag_write16(HFORCE_MONEY_ADDR, HFORCE_MONEY_VALUE);
      }

      if (weaponsSettingValue > 0)
      {
         bigpemu_jag_write8(HFORCE_WEAPON2_UNLOCK_ADDR, HFORCE_UNLOCK_VALUE);
         bigpemu_jag_write8(HFORCE_WEAPON3_UNLOCK_ADDR, HFORCE_UNLOCK_VALUE);
         bigpemu_jag_write8(HFORCE_WEAPON4_UNLOCK_ADDR, HFORCE_UNLOCK_VALUE);
         bigpemu_jag_write8(HFORCE_WEAPON5_UNLOCK_ADDR, HFORCE_UNLOCK_VALUE);
      }

      if (ammoSettingValue > 0)
      {
         bigpemu_jag_write8(HFORCE_WEAPON2_AMMO_ADDR, HFORCE_AMMO_VALUE);
         bigpemu_jag_write8(HFORCE_WEAPON3_AMMO_ADDR, HFORCE_AMMO_VALUE);
         bigpemu_jag_write8(HFORCE_WEAPON4_AMMO_ADDR, HFORCE_AMMO_VALUE);
         bigpemu_jag_write8(HFORCE_WEAPON5_AMMO_ADDR, HFORCE_AMMO_VALUE);
      }

      if (noTimeoutSettingValue > 0)
      {
         bigpemu_jag_write8(HFORCE_NO_TIMEOUT_ADDR, HFORCE_NO_TIMEOUT_VALUE);
      }

      if (levelSettingValue > -1)
      {
         if (sLevelDelayCounter < HFORCE_LEVEL_DELAY_TICKS)
         {
            sLevelDelayCounter += 1;
         }
         if (HFORCE_LEVEL_DELAY_TICKS == sLevelDelayCounter)
         {
            // Entering demo mode prevents this cheat from working.  Someone else can fix this, if they care enough.
            // As it stands, if you know to start game before demo mode, and start game before demo mode, then level skip cheat works.
            printf_notify("Applying skip level cheat.  Entering demo mode will prevent cheat from taking effect.\n");

            // Level values are 0 through 5, 8 through 13, 16 through 21, 24 through 29.
            // Convert from a BigPEmu setting value of 0 through 23, to level values in comment directly above.
            const int numLevelsPerWorld = 6;
            const int worldValue = levelSettingValue / numLevelsPerWorld;
            const int levelWithinWorldValue = levelSettingValue % numLevelsPerWorld;
            const int convertedLevelValue = worldValue * 8 + levelWithinWorldValue;
            bigpemu_jag_write8(HFORCE_LEVEL_ADDR, convertedLevelValue);

            //TODO:  Someone test this further.  Currently very little testing of level skip has been done.
            //TODO:  Would polling score or money be better approach then time delay?

            // Make sure this block is only executed once.
            sLevelDelayCounter += 1;
         }
      }
   }
   return 0;
}

void bigp_init()
{
   void* pMod = bigpemu_get_module_handle();

   bigpemu_set_module_usage_flags(pMod, BIGPEMU_MODUSAGE_DETERMINISMWARNING);

   // Register cheat settings with BigPEmu GUI interface.
   const int catHandle = bigpemu_register_setting_category(pMod, "Hyperforce");
   sLivesSettingHandle = bigpemu_register_setting_bool(catHandle, "Infinite Lives", 1);
   sHealthSettingHandle = bigpemu_register_setting_bool(catHandle, "Infinite Health", 1);
   sMoneySettingHandle = bigpemu_register_setting_bool(catHandle, "Infinite Money", 1);
   sWeaponsSettingHandle = bigpemu_register_setting_bool(catHandle, "Unlock All Weapons", 1);
   sAmmoSettingHandle = bigpemu_register_setting_bool(catHandle, "Infinite Ammo", 1);
   sNoTimeoutSettingHandle = bigpemu_register_setting_bool(catHandle, "No Timeout", 1);
   sLevelSettingHandle = bigpemu_register_setting_int_full(catHandle, "Start At Level", INVALID, INVALID, HFORCE_NUM_LEVELS - 1, 1);

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

   sLevelDelayCounter = 1;
   sLivesSettingHandle = INVALID;
   sHealthSettingHandle = INVALID;
   sMoneySettingHandle = INVALID;
   sWeaponsSettingHandle = INVALID;
   sAmmoSettingHandle = INVALID;
   sNoTimeoutSettingHandle = INVALID;
   sLevelSettingHandle = INVALID;
}