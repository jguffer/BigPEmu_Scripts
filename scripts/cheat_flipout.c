//__BIGPEMU_SCRIPT_MODULE__
//__BIGPEMU_META_DESC__    "Level select, in Flipout."
//__BIGPEMU_META_AUTHOR__  "jguff"

#include "bigpcrt.h"

// Found using BigPEmu System->Load Software screen and highlighting ROM file.
#define FLIPOUT_HASH    0x7FB2D44F557ED585ULL

// Numbered approach to locate level number variables.
//
// 01. Started game and used joypad cheat code to skip forward from Level1 to Level7, saving
//     RAM at each level.  Never actually got the tiles flipping, just stayed at the preflip
//     state, before advancing to next level.
// 02. Used Pro Action Replay like descending value search script to identify the following
//     memory addresses: '0xcb9ea', '0xcb9ee', '0xcbae2', '0xea7f2', '0xea7f6', '0xea7fc', '0xea816'
// 03. Noesis Memory Window analysis led to conclusion that 0x000EA7FC is most likely candidate
//     to represent level number state.  Watched 0x000EA7FC value increment basically as soon
//     as joypad cheat was entered (when score started to be tallied for skipped over level
//     and well before new level tiles were placed).  Believe screen display of level number
//     didn't increment until several seconds later.
// 04. However, writing to 0x000EA7FC doesn't seem to do anything when start game or when
//     transition from one level to another.
// 05. So try a different approach...
// 06. Noted there are 5 save slots for the game, available from UI.
// 07. Power down, delete EPROM file, power on.
// 08. Beat first level and EPROM still not created.
// 09. EPROM file created, after selected to save to top save slot.
// 10. Within EPROM file 0x0040 and 0x0046 increment by one, everytime a level is completed,
//     and save to top save slot.  The values after the first save were 0x24 and 0x07.
// 11. Played thru 6 levels, saving after each level, and storing copies of EPROM files for comparison.
// 12. Looks like 0x0040 to 0x0060 are the only bytes written to (at least so far, and they skipped over first half of EPROM)
//     Looks like 2 bytes of dunno yet, and then 5 sets of 6 bytes.
//     When unswap bytes, first 3 of set correspond to name then 2 zeros and finally the level you saved at + 5.
//     First 2 bytes could be a checksum, but doesn't really behave like it.
// 13. Searched for byte-swapped contents of EPROM (first of two lines):
//     0x17 0x38 0x02 0x01 0x01 0x00 0x00 0x0B 0x03 0x03 0x03 0x00 0x00 0x0B 0x05 0x05
// 14. Located in Jaguar RAM at 0x000CFF76, and only one hit.
//     0x000CFF7D is byte holding Slot 1 level value.
// 15. Placed read breakpoint on 0x000CFF7D
// 16. Load Slot 1
// 17. Breakpoint hit and reviewed disassembly and memory.
//     Next instruction subtracts 5 from register copy of that value (remember EPROM on Level 2 had written value of 7).
//     0x000EA7F8 probably the level - 2 bytes
//     0x000EA7FA 0xDEAD - 2 bytes
//     Instructions, after that, loads 0x000EA7F8 to D3 and compares D3 to 0xDEAD.
//     Instructions, after that, jump to another subroutine and when back from subroutine they've
//     incremented to load 0x000EA7FA to D3 and compare to "DEAD".
//     0x000EA7FD seems to be number of levels completed in session (since starting or loading).
// 18. Restarted, loaded level 6 and completed a couple levels
//     0x000EA7F8: 0x00 0x08 0x00 0x03 0x00 0x02 0xDE 0xAD zeros onwards
//     While on Level 8 of 11.
// 19. Restarted, then applied joypad cheat repeatedly to skip to Level 9 and played through 3 levels, which took past
//     end of first planet, and past first level of second planet.
// 20. Saved to EPROM slots.
// 21. Save/Load UI screen doesn't mention anything about planets.
//     Save/Load UI screen displays Level 13 for Planet 2, Level 2.
//     EPROM contents don't seem to acknowledge planets either.  Level saved in EPROM as 0x12=(13+5).
// 22. Load a slot and inspected 0x000EA7F8 again, with the following unhelpful values being present:
//     0x00 0x00 0xDE 0xAD 0xDE 0xAD 0xDE 0xAD
// 23. Seems like overwriting the level value, which is copied from EEPROM to
//     Jaguar RAM (Step13) is the approach to take.
//
// 24. Writing to 0x000CFF7D successfully allows user to resume gameplay at the value specified
//     by 0x000CFF7D.  However, user has to resume Save Slot 1 to do so.
// 25. Placed read breakpoint onto 0x000CFF7D.
// 26. Restarted and selected to load Save Slot 1, from Load Game screen.
// 27. Breakpoint hit.
// 28. Disassembly shows 0x000CFF7D copied to 0x000CB9EE, 0x000CBAE2, 0x000CBB1A
// 29. Writing (level-number-value + 5) to these addresses causes gameplay to resume at level-number-value
#define FLIPOUT_LEVELS_ADDR   0x000CFF7D
#define FLIPOUT_LEVELS_ADDRA  0x000CB9EE
#define FLIPOUT_LEVELS_ADDRB  0x000CBAE2
#define FLIPOUT_LEVELS_ADDRC  0x000CBB1A

// Numbered approach to locate score variable.
//
// 30. Will need to stop writing to the FLIPOUT_LEVELS_ADDR variables, between
//     time that level is skipped to and time when next level begins.  Otherwise
//     game will make user play the same skipped-to level over and over again.
// 31. Locate score variable, so can poll change in score, to determine when should stop
//     overwriting the FLIPOUT_LEVELS_ADDR variables.
// 32. Looks like score is only updating after completion of level.
// 33. Setup script to overwrite FLIPOUT_LEVELS_ADDR variables with value of 6, so the game
//     will repeat Level1 in infinite loop.  Completing/winning Level1 is order of magnitude
//     easier then completing/winner any other levels.
// 34. Restarted.
// 35. Exported memory to file.
// 36. Completed Level 1
// 37. Exported memory to file.
// 38. Repeated Step35 through Step36 5 or more times.
// 39. Ran Pro-Action Replay ascending value search script on the memory dumps.
// 40. Identified 0x000EA7F2 and 0x000EA7F6 as variables holding score
//     These are likely actually 4 byte wide and begin 2 bytes earlier, but
//     for purposes of this script, the lower 2 bytes are all that is needed.
// 41. Some further analysis using Noesis Memory Window shows 0x000EA7F2 updates
//     when or shortly after tiles have all been placed into correct spaces.
//     0x000EA7F6 updates bit later in time (perhaps initiated by critter sprite
//     updating score sprites).
// 42. 0x000EA7F2 is address of interest for score
#define FLIPOUT_SCORE_ADDR    0x000EA7F2

// Youtube playthrough showed 54 levels, and verified that
// value of 54 takes player to last level.  Tried value of
// 55 and that takes player to the end game screen(s).
#define FLIPOUT_NUM_LEVELS    55

// The EPROM file contains a value of 6 for Level 1 and
// increments the value by 1 for each level beyond Level 1.
#define FLIPOUT_LEVEL_1_VALUE 6

#define INVALID               -1

// Boilerplate.
static int32_t sOnLoadEvent = INVALID;
static int32_t sOnEmuFrame = INVALID;
static int32_t sMcFlags = 0;
static const int32_t skMcFlag_Loaded = (1 << 0);

// Setting handles.
static int sLevelsSettingHandle = INVALID;

static uint32_t on_sw_loaded(const int eventHandle, void* pEventData)
{
   const uint64_t hash = bigpemu_get_loaded_fnv1a64();

   sMcFlags = 0;

   if (FLIPOUT_HASH == hash)
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
      int levelsSettingValue = INVALID;
      bigpemu_get_setting_value(&levelsSettingValue, sLevelsSettingHandle);

      if (levelsSettingValue > 1)
      {
         // Below line is intentionally commented out and intentionally not deleted.
         // When uncommented it will cause overwrite of level in Slot 1.
         // It may work well, and possibly even allow change of difficulty, but 
         // seemed awkward for user to have to configure BigPEmu user setting,
         // start game, then know to select Resume Game, and know to resume
         // Save Slot 1.
         //
         // Keep this line commented out and keep this line around, in case
         // the uncommented-out approach below proves to be an unsatisfactory
         // solution.
         //bigpemu_jag_write8(FLIPOUT_LEVELS_ADDR, levelsSettingValue + FLIPOUT_LEVEL_1_VALUE - 1);

         // Read score.
         // Don't trust any 16-bit read/assignment, given the 16-bit-assignment-memory-clobbering bug.
         const uint32_t score = (bigpemu_jag_read8(FLIPOUT_SCORE_ADDR) << 8) | bigpemu_jag_read8(FLIPOUT_SCORE_ADDR + 1);

         // When score is zero, gameplay for level has not ended, so safe to overwrite level number state.
         if (0 == score)
         {
            // Level values seem to be offset by 5 for some reason.
            const uint8_t offsetLevel = 5 + levelsSettingValue;

            // Overwrite level number state.
            //
            // This seems to jump game straight into the level after boot.
            // This thusly bypasses user being allowed to change difficulty.
            // Normal (easiest) difficulty is the default difficulty.
            //
            // Don't much care about supporting the other 3 harder difficulties.
            // Difficulty 3 and 4 gray out every tile.
            // Difficulty 2 grays out the top side of all tiles.
            //
            // This is a tile matching game, and the only distinguishing factor
            // for the tiles are color and whether tile is flashing to indicate
            // tile is in correct place.  The difficulties removing the
            // distinguishing factor for the tiles seems like ...
            //
            // Concluding not worth taking more time to address the difficulty modes.
            // Level skip of the normal Difficulty 1 is sufficient.
            // Anyone needing to have level skip for other difficulty modes can use the
            // joypad controller cheats.
            bigpemu_jag_write8(FLIPOUT_LEVELS_ADDRA + 0, 0);
            bigpemu_jag_write8(FLIPOUT_LEVELS_ADDRA + 1, offsetLevel);
            bigpemu_jag_write8(FLIPOUT_LEVELS_ADDRB + 0, 0);
            bigpemu_jag_write8(FLIPOUT_LEVELS_ADDRB + 1, offsetLevel); 
            bigpemu_jag_write8(FLIPOUT_LEVELS_ADDRC + 0, 0);
            bigpemu_jag_write8(FLIPOUT_LEVELS_ADDRC + 1, offsetLevel);
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
   const int catHandle = bigpemu_register_setting_category(pMod, "Flipout");
   sLevelsSettingHandle = bigpemu_register_setting_int_full(catHandle, "Start At Level", INVALID, INVALID, FLIPOUT_NUM_LEVELS, 1);

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

   sLevelsSettingHandle = INVALID;
}