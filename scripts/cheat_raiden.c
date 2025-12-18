//__BIGPEMU_SCRIPT_MODULE__
//__BIGPEMU_META_DESC__    "Infinite lives and bombs, unlock weapon upgrades, level select in Raiden."
//__BIGPEMU_META_AUTHOR__  "jguff"

#include "bigpcrt.h"

// Found using BigPEmu System->Load Software screen and highlighting ROM file.
#define RAIDEN_HASH                    0x5FA30C19C9DCD87FULL

// Found using Pro Action Replay like scripts on memory dump from Noesis.
#define RAIDEN_P1_LIVES_ADDR           0x00058276
#define RAIDEN_P2_LIVES_ADDR           0x000589E2
#define RAIDEN_LEVEL_ADDR              0x00048592
#define RAIDEN_LIVES_VALUE             9

// Calculate offset between player data structures.
#define RAIDEN_P2_OFFSET               RAIDEN_P2_LIVES_ADDR - RAIDEN_P1_LIVES_ADDR

// Found inspecting memory, via Noesis Memory Window, after locating RAIDEN_LIVES_ADDR.
#define RAIDEN_P1_SCORE_ADDR           0x00058270
#define RAIDEN_P1_BOMBS_ADDR           0x00058277
#define RAIDEN_BOMBS_VALUE             9

// NTSC
#define RAIDEN_FRAMES_PER_SECOND       60

// Should write to level address until a level has started.
// For now be lazy and just write to level address for 5 minutes after hard boot.
// Time/ticks to level change, until at least a level has _likely_ started.
#define RAIDEN_LEVEL_DELAY_SECONDS     180
#define RAIDEN_LEVEL_DELAY_TICKS       RAIDEN_FRAMES_PER_SECOND * RAIDEN_LEVEL_DELAY_SECONDS
#define RAIDEN_NUM_LEVELS              8


// Type of gun.
// Value of 0 for spread
// Value of 1 for laser.
// Value of 2 happens to be missiles (not side missiles, but primary gun as steady stream of missiles).
// Value of 3 sometimes put game into infinite loop.
// Value of 4-6 were of lesser quantity missiles
// Value of 7 was back to twin spread shot.
// Suspect values over 3 just happen to do something.
// Value of 2 may have been used for testing, or possibly an additional weapon that wasn't/didn't test proper.
#define RAIDEN_P1_GUN_ADDR             0X0005827F
#define RAIDEN_EASTER_EGG_GUN_VALUE    2

// Value of 0 is spread.
// Value of 1 is laser.
// Value of 2 is lots of missiles as primary gun.  Don't think value-3 is documented or meant for production.
#define RAIDEN_P1_GUN_LEVEL_ADDR       0x00058280

// Game logic caps gun level at value of 7.
//
// Writing values above 7 start causing weird shot behavior.
// Tried 8 and 9.  One of those values caused ship to shoot,
// while the other value caused ship to birth ships backwards
// and those ships did kill enemies.
#define RAIDEN_GUN_LEVEL_VALUE         7

// Value of 0xFF for no missiles.
// Value of 0 for non-homing missiles.
// Value of 1 for homing missiles.
#define RAIDEN_P1_MISSILE_ADDR         0x00058281

// Power level of missiles.  Valid values of 0 thru 4.
#define RAIDEN_P1_MISSILE_LEVEL_ADDR   0x00058282
#define RAIDEN_MISSILE_LEVEL_VALUE     4

#define INVALID                        -1

// Boilerplate.
static int32_t sOnLoadEvent = INVALID;
static int32_t sOnEmuFrame = INVALID;
static int32_t sMcFlags = 0;
static const int32_t skMcFlag_Loaded = (1 << 0);

static int sLevelDelayCounter = -1;

// Setting handles.
static int sLivesSettingHandle = INVALID;
static int sBombsSettingHandle = INVALID;
static int sGunLevelSettingHandle = INVALID;
static int sMissileLevelSettingHandle = INVALID;
static int sP1MissilesAsGunSettingHandle = INVALID;
static int sP2MissilesAsGunSettingHandle = INVALID;
static int sLevelSettingHandle = INVALID;

static uint32_t on_sw_loaded(const int eventHandle, void* pEventData)
{
   const uint64_t hash = bigpemu_get_loaded_fnv1a64();

   sMcFlags = 0;

   if (RAIDEN_HASH == hash)
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
      int gunLevelSettingValue = INVALID;
      int missileLevelSettingValue = INVALID;
      int p1MissilesAsGunSettingValue = INVALID;
      int p2MissilesAsGunSettingValue = INVALID;
      int levelSettingValue = INVALID;

      bigpemu_get_setting_value(&livesSettingValue, sLivesSettingHandle);
      if (livesSettingValue > 0)
      {
         bigpemu_jag_write8(RAIDEN_P1_LIVES_ADDR, RAIDEN_LIVES_VALUE);
         bigpemu_jag_write8(RAIDEN_P1_LIVES_ADDR + RAIDEN_P2_OFFSET, RAIDEN_LIVES_VALUE);
      }

      bigpemu_get_setting_value(&bombsSettingValue, sBombsSettingHandle);
      if (bombsSettingValue > 0)
      {
         bigpemu_jag_write8(RAIDEN_P1_BOMBS_ADDR, RAIDEN_BOMBS_VALUE);
         bigpemu_jag_write8(RAIDEN_P1_BOMBS_ADDR + RAIDEN_P2_OFFSET, RAIDEN_BOMBS_VALUE);
      }

      bigpemu_get_setting_value(&gunLevelSettingValue, sGunLevelSettingHandle);
      if (gunLevelSettingValue > 0)
      {
         bigpemu_jag_write8(RAIDEN_P1_GUN_LEVEL_ADDR, RAIDEN_GUN_LEVEL_VALUE);
         bigpemu_jag_write8(RAIDEN_P1_GUN_LEVEL_ADDR + RAIDEN_P2_OFFSET, RAIDEN_GUN_LEVEL_VALUE);
      }

      bigpemu_get_setting_value(&missileLevelSettingValue, sMissileLevelSettingHandle);
      if (missileLevelSettingValue > 0)
      {
         // Game logic caps at value of RAIDEN_MISSILE_LEVEL_VALUE.
         //
         // Player will have to pick missiles up, before this takes affect.
         // Could enable missiles in this conditional block, but then
         // player would be locked into that type of missile for game,
         // unless someone wants to make this script more complex.
         bigpemu_jag_write8(RAIDEN_P1_MISSILE_LEVEL_ADDR, RAIDEN_MISSILE_LEVEL_VALUE);
         bigpemu_jag_write8(RAIDEN_P1_MISSILE_LEVEL_ADDR + RAIDEN_P2_OFFSET, RAIDEN_MISSILE_LEVEL_VALUE);
      }

      bigpemu_get_setting_value(&p1MissilesAsGunSettingValue, sP1MissilesAsGunSettingHandle);
      if (p1MissilesAsGunSettingValue > 0)
      {
         // There will be no way for player to turn this off, unless they
         // go into BigPEmu Settings screen, during gameplay, to turn off.
         //
         // Allowing this to be turned on, per player.
         bigpemu_jag_write8(RAIDEN_P1_GUN_ADDR, RAIDEN_EASTER_EGG_GUN_VALUE);

         // Override any gun level settings applied above, and just give
         // player maxed out non-homing missiles.
         // Value of 0 is stream of 2 missiles
         // Value of 1 is stream of 3 missiles.
         // Value of 2 is stream of 4 missiles.
         // Value of 3 is stream of 4 missiles, and see no difference from value of 2.
         // Value of 4 is stream of 4 missiles, and see no difference from value of 3.
         // Value of 5 and above switches to homing missiles, which don't find fun/useful as gun/primary-weapon.
         bigpemu_jag_write8(RAIDEN_P1_GUN_LEVEL_ADDR, 2);
      }

      if (p2MissilesAsGunSettingValue > 0)
      {
         // See comments for player 1 conditional block directly above.
         bigpemu_jag_write8(RAIDEN_P1_GUN_ADDR + RAIDEN_P2_OFFSET, RAIDEN_EASTER_EGG_GUN_VALUE);
         bigpemu_jag_write8(RAIDEN_P1_GUN_LEVEL_ADDR + RAIDEN_P2_OFFSET, RAIDEN_GUN_LEVEL_VALUE);
      }

      bigpemu_get_setting_value(&levelSettingValue, sLevelSettingHandle);
      if (levelSettingValue > 0)
      {
         // Only set level for a limited amount of time, so that after
         // completion of level, the game can increment the level value
         // to the next level.
         if (sLevelDelayCounter < RAIDEN_LEVEL_DELAY_TICKS)
         {
            bigpemu_jag_write8(RAIDEN_LEVEL_ADDR, levelSettingValue);

            // Make sure this block is only executed once.
            sLevelDelayCounter += 1;
         }
         // TODO: Might be better to poll score, instead of time.
      }
   }
   return 0;
}

void bigp_init()
{
   void* pMod = bigpemu_get_module_handle();

   bigpemu_set_module_usage_flags(pMod, BIGPEMU_MODUSAGE_DETERMINISMWARNING);

   // Register cheat settings with BigPEmu GUI interface.
   const int catHandle = bigpemu_register_setting_category(pMod, "Raiden");
   sLivesSettingHandle = bigpemu_register_setting_bool(catHandle, "Infinite Lifes", 1);
   sBombsSettingHandle = bigpemu_register_setting_bool(catHandle, "Infinite Bombs", 1);
   sGunLevelSettingHandle = bigpemu_register_setting_bool(catHandle, "Max Gun Level", 1);
   sMissileLevelSettingHandle = bigpemu_register_setting_bool(catHandle, "Max Missile Level", 1);

   // Default Player 1 to turned off, and Player 2 turned on by default.
   // Don't want this applied by default to player, but perhaps applying by
   // default to Player2 will make it more obvious to players that there is
   // a setting for this state.  Not everyone is going to dig into individual
   // BigPEmu script settings.  Most people will probably stop at enabling
   // and disabling the script.
   sP1MissilesAsGunSettingHandle = bigpemu_register_setting_bool(catHandle, "P1 Stream Of Missiles As Gun", 0);
   sP2MissilesAsGunSettingHandle = bigpemu_register_setting_bool(catHandle, "P2 Stream Of Missiles As Gun", 1);

   sLevelSettingHandle = bigpemu_register_setting_int_full(catHandle, "Start At Level", INVALID, INVALID, RAIDEN_NUM_LEVELS, 1);

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

   sLevelDelayCounter = -1;

   sLivesSettingHandle = INVALID;
   sBombsSettingHandle = INVALID;
   sGunLevelSettingHandle = INVALID;
   sMissileLevelSettingHandle = INVALID;
   sP1MissilesAsGunSettingHandle = INVALID;
   sP2MissilesAsGunSettingHandle = INVALID;
   sLevelSettingHandle = INVALID;
}