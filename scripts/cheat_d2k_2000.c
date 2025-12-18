//__BIGPEMU_SCRIPT_MODULE__
//__BIGPEMU_META_DESC__    "Infinite lives, bombs, and shields, unlock flossie, beest, and pong modes, and level/difficulty select, in 2000 mode of Defender 2000."
//__BIGPEMU_META_AUTHOR__  "jguff"

#include "bigpcrt.h"

// Found using BigPEmu System->Load Software screen and highlighting ROM file.
#define D2K_HASH                       0x6B591BA55B0BCBD7ULL

// Found using Pro Action Replay like counter search script.
#define D2K_2K_SCORE_MULTIPLIER_ADDR   0x000B6F23
#define D2K_2K_BOMBS_ADDR              0x001AF149
#define D2K_2K_LIVES_ADDR              0x001AF14B
#define D2K_2K_BOMBS_VALUE             11
#define D2K_2K_LIVES_VALUE             5

// Found by inspecting memory, near the above addresses, using Noesis Memory Window.
#define D2K_2K_SHIELD_ADDR             0x001AF0CD
#define D2K_2K_LEVEL_ADDR              0x001AF14D
#define D2K_2K_SHIELD_VALUE            2

// Some other identified addresses which this script is not using.
// Retain for any future script hacks.
// -
// 0x000B6ED1 countdown for display of messages
// 0x000B6EE2-0x000B6F21 copied to CLUT.  possibly for UFOs.
// 0x000B6F25 initialized at start of level to save value as BSF27.  counts down numerous times during level.
// 0x000B6F27 initialized at start of level to save value as BSF25.  Retains same value during level.  Decremented by 1, between levels.
// 0x000B6F2B looks to be type of power-up orb to release
// 0x000B6F2C when 0, the next enemy killed results in a orb release and oftentimes value goes back to 5 (at least for bonus round orbs).
// 0x000B6F2D maximum number of enemies to kill to release an orb.  max value for 0x000B6F2C
// 0x000B6F2F num orbs left to enter bonus round.
//            at start of level, initialized to 0x0003.
//            decrements during gameplay (presumably based on collecting item of specific type).
//            when hits zero, goes to 0xFFFF and enter bonus round.
//            the purple red orbs cause the decrement.
//            Beest Mode 6-key skips and does not touch this address (verified by read breakpoint not getting hit).
// 0x001AF143 bonus round index
// 0x001AF146-0x001AF147 0xFFFF for normal background.
//             0x0000 for trippyville plasma background, when no humanoids remain
// 0x001AF14D level
// 0x001AF151 game state of some sort.  0x00=play, 0x01 briefly when transition between levels, 0x02 when transition between levels
//             Writing a 0x02 here ends level and transitions to in between level tally.
// 0x001AF155 possibly number of times have died
// 0x001AF15A probably number of humanoids remaining
// 0x001AF268 score
#define MAX_NUM_ENEMIES_FOR_ORB_ADDR                  0x000B6F2D
#define NUM_ORBS_REMAINING_TO_ENTER_BONUS_ROUND_ADDR  0x000B6F2F

// Power-Up Orbs
// Value Of 0x000B6F2B	Description Of Powerup
// ----------------------------------------------
// 0->1                 droid
// 1->2                 shield
// 2->3                 2nd droid - green whisk around gray orb
// 3->4                 silver ball with lightning laser - green whisk around gray orb
// 4->5                 turbo lightning laser- orange whisk gray orb - does that add lightning or unlimited lightning to tesla ball?
// 5->6                 warp token - red whisk around purple orb
// 6->7                 warp token - red whisk around purple orb
// 7->8                 warp token - red whisk around purple orb
// 8->9                 warp token - red whisk around purple orb

#define D2K_2K_MAX_NUM_ENEMIES_FOR_ORB_RELEASE	33

// Steps taken to locate level/difficult unlock bits/bytes.
// 01. Deleted EEPROM file.
// 02. Started 2000 mode game.
// 03. While on first level, used Noesis Memory Window's Fill Memory capability to manually write 98 to D2K_2K_LEVEL_ADDR.
// 04. Completed first level, and between level tally screen said had just completed Level99.
// 05. Completed next level (last level of game (Level100)).
// 06. Then all levels were unlocked and Vindaloo difficulty was unlocked as well.
// 07. Restarted game.
// 08. Entered Select Play Options screen and exported RAM, with different levels selected.
// 09. Ran Pro Action Replay like decreasing values search script, with the following search result:
//     0x00180FF1 (call it D2K_2K_LEVEL_SELECT_DISPLAY_ADDR)
//     Using Noesis Memory Window, noted that D2K_2K_LEVEL_SELECT_DISPLAY_ADDR contains valid values of 0 thru 19, with value
//     representing: (starting_level - 1) / divided by 5.
// 10. Shutdown game.
// 10. Deleted EEPROM file.
// 11. Started game and entered Select Play Options screen.
// 12. Verified that Vindaloo Mode is not accessible.
// 13. Verified levels greater then Level6 are not accessible.
// 14. Wrote value of 7 to D2K_2K_LEVEL_SELECT_DISPLAY_ADDR.
// 15. Flipped through available levels, noting levels beyond 36 (7*5 + 1) were available, and noticed major background glitch.
// 17. Repeated Step11 through Step13.
// 16. Placed write breakpoint on D2K_2K_LEVEL_SELECT_DISPLAY_ADDR.
// 17. Flipped through available levels.
// 18. On write breakpoint, noted (that several instructions above the breakpoint) that comparison was made to contents of A2=0x00180E71 (call it D2K_2K_LEVEL_UNLOCK_ADDR).
// 19. Wrote a value of 15 to D2K_2K_LEVEL_UNLOCK_ADDR.
// 20. Successfully able to flip through levels, upto 71 ((15+1)+1). 
//     (There is another interesting address: 0x00180F27.  Possibly for second player.  Possibly mirror/shadow copy.
// 21. Switched back to using EEPROM with difficulty unlocked.
#define D2K_2K_LEVEL_SELECT_DISPLAY_ADDR     0x00180FF1
#define D2K_2K_LEVEL_UNLOCK_ADDRA            0x00180E71
#define D2K_2K_LEVEL_UNLOCK_ADDRB            0x00180F27

// Found via memory analysis, in Noesis Memory Window, near D2K_2K_LEVEL_SELECT_DISPLAY_ADDR.
// Index of which option is highlighted in Select Play Options Screen.
// Values of 0 thru 2.  0 for num players.  Believe 1 is the second down, and 2 the third down.
// Values of 1 and 2 represent different things, once Difficulty is unlocked.
#define D2K_2K_PLAY_OPTIONS_SCREEN_IDX_ADDR  0x00180FED

// 22. Noticed following value changing based on whether difficulty was unlocked or not:
//         0x00180FF4
//             0x00180E6C when difficulty locked
//             0x00180F22 when difficulty unlocked
// 23. Placed write breakpoint on 0x00180FF4
// 24. Instruction at 0x000A236E is hit
// 25. Several instructions above that (0x000A22A1 through 0x000A22B4):
//         conditionally sets A2 to either 0x00180E6C or 0x00180F22 (the values written to 0x00180FF4)
//         based on testing 7th bit of 0x000B4C00
//             lea     $180e6c.l, A2
//             btst    #$7, $b4c00.l
//             beq     $a22ba
//             lea     $180f22.l, A2
// 26. Looks like that 7th bit of 0x000B4C00 controls whether difficulty is unlocked.
// 27. Further review of disassembly shows the 2nd byte is read and possibly interpreted as level that is unlocked?.
//             move.w  $b4c02.l, D0
//             and.l   #$ff, D0
//             ori.b   #$ff, D0
//             divu.w  #$5, D0
//             add.w   #$1, D0
//             move.w  D0, ($4,A2)
// 28. 4th byte may be treated like 2nd byte as well.
#define D2K_2K_UNLOCK_ADDR             0x000B4C00

// Later pro-action replay script runs showed that other bits in byte1 control beest mode, flossie mode, pong pong unlock
#define D2K_2K_BEEST_MODE_BIT          0
#define D2K_2K_FLOSSIE_MODE_BIT        1
#define D2K_2K_PONG_UNLOCK_BIT         2
#define D2K_2K_DIFFICULTY_UNLOCK_BIT   7

#define INVALID                        -1

// Could possibly use Pro-Action Replay script to locate addresses to turn on droids, tesla ball, etc powers.
// However with shield turned on from beginning of gameplay, player is basically already invincible,
// and then its just a matter of collecting a few powerup orbs to get the droids, tesla ball, etc powers.
// Plus this scripts add settings to make these power ups appear more quickly as well.
// So not going to bother looking for the exact bits/bytes to turn those other powers on.

// Boilerplate.
static int32_t sOnLoadEvent = INVALID;
static int32_t sOnEmuFrame = INVALID;
static int32_t sMcFlags = 0;
static const int32_t skMcFlag_Loaded = (1 << 0);

// Setting handles.
static int sLivesSettingHandle = INVALID;
static int sBombsSettingHandle = INVALID;
static int sShieldsSettingHandle = INVALID;
static int sPongSettingHandle = INVALID;
static int sFlossieSettingHandle = INVALID;
static int sBeestSettingHandle = INVALID;
static int sDiffLvlSettingHandle = INVALID;
static int sNumEnemiesForOrbSettingHandle = INVALID;
static int sPreventOrbWarpingSettingHandle = INVALID;

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
      int shieldsSettingValue = INVALID;
      int pongSettingValue = INVALID;
      int flossieSettingValue = INVALID;
      int beestSettingValue = INVALID;
      int diffLvlSettingValue = INVALID;
      int numEnemiesForOrbSettingValue = INVALID;
      int preventOrbWarpingSettingValue = INVALID;
      bigpemu_get_setting_value(&livesSettingValue, sLivesSettingHandle);
      bigpemu_get_setting_value(&bombsSettingValue, sBombsSettingHandle);
      bigpemu_get_setting_value(&shieldsSettingValue, sShieldsSettingHandle);
      bigpemu_get_setting_value(&pongSettingValue, sPongSettingHandle);
      bigpemu_get_setting_value(&flossieSettingValue, sFlossieSettingHandle);
      bigpemu_get_setting_value(&beestSettingValue, sBeestSettingHandle);
      bigpemu_get_setting_value(&diffLvlSettingValue, sDiffLvlSettingHandle);
      bigpemu_get_setting_value(&numEnemiesForOrbSettingValue, sNumEnemiesForOrbSettingHandle);
      bigpemu_get_setting_value(&preventOrbWarpingSettingValue, sPreventOrbWarpingSettingHandle);

      if (livesSettingValue > 0)
      {
         bigpemu_jag_write8(D2K_2K_LIVES_ADDR, D2K_2K_LIVES_VALUE);
      }

      if (bombsSettingValue > 0)
      {
         bigpemu_jag_write8(D2K_2K_BOMBS_ADDR, D2K_2K_BOMBS_VALUE);
      }

      if (shieldsSettingValue > 0)
      {
         // Enables infinite shield.  Shield does not display until player collects
         // shield orb.  But the shield is enabled, even before shield orb is
         // collected.  Don't see the point in tracking down how to force display.
         // Actually kind of like not seeing the shield sometimes.
         bigpemu_jag_write8(D2K_2K_SHIELD_ADDR, D2K_2K_SHIELD_VALUE);
      }

      if (pongSettingValue > 0)
      {
         uint8_t value = bigpemu_jag_read8(D2K_2K_UNLOCK_ADDR);
         value |= (1 << D2K_2K_PONG_UNLOCK_BIT);
         bigpemu_jag_write8(D2K_2K_UNLOCK_ADDR, value);

         // Opens 4th play mode (along side 2000, Plus, and Classic).
         // Remains active until EPROM wiped/deleted.
      }

      if (flossieSettingValue > 0)
      {
         uint8_t value = bigpemu_jag_read8(D2K_2K_UNLOCK_ADDR);
         value |= (1 << D2K_2K_FLOSSIE_MODE_BIT);
         bigpemu_jag_write8(D2K_2K_UNLOCK_ADDR, value);

         // Must start Plus Mode with 'A' button.
         // Turns ship into much larger sheep, and humans into much larger camels,
         // and possibly other differences.
         // Doesn't appear to be saved to EPROM, although Atari Age Jaguar Cheats
         // page claims otherwise.
      }

      if (beestSettingValue > 0)
      {
         uint8_t value = bigpemu_jag_read8(D2K_2K_UNLOCK_ADDR);
         value |= (1 << D2K_2K_BEEST_MODE_BIT);
         bigpemu_jag_write8(D2K_2K_UNLOCK_ADDR, value);

         // Begin a Defender 2000 game and press 3 during game play to jump to the
         // next level. Press 6 during game play to access the warp screen.
         // Remains active until EPROM wiped/deleted.
      }

      if (diffLvlSettingValue > 0)
      {
         uint8_t value = bigpemu_jag_read8(D2K_2K_UNLOCK_ADDR);
         value |= (1 << D2K_2K_DIFFICULTY_UNLOCK_BIT);
         bigpemu_jag_write8(D2K_2K_UNLOCK_ADDR, value);

         // Disassembly showed 99 written to 2nd and 4th bytes.
         // (Writing 99 to only the 2nd byte, did not unlock
         // levels for either difficulty.)
         const uint8_t level = 0x63;
         bigpemu_jag_write8(D2K_2K_UNLOCK_ADDR + 1, level);
         bigpemu_jag_write8(D2K_2K_UNLOCK_ADDR + 3, level);

         // Remains active until EPROM wiped/deleted.
      }

      if (numEnemiesForOrbSettingValue > 0)
      {
         bigpemu_jag_write8(MAX_NUM_ENEMIES_FOR_ORB_ADDR, numEnemiesForOrbSettingValue - 1);
      }

      if (preventOrbWarpingSettingValue > 0)
      {
         // Writing the same number emulation frame, prevents counting down
         // to zero, thusly preventing collection of orbs initiating
         // bonus/warp rounds.
         //
         // Added this, as it can be annoying to enter bonus/warp round,
         // if player is wanting to just play through all levels.  The
         // addition of the numEnemiesForOrbSettingValue (for powering up
         // ship) makes the annoyance more frequently encountered otherwise.
         bigpemu_jag_write8(NUM_ORBS_REMAINING_TO_ENTER_BONUS_ROUND_ADDR, 3);
      }

      // Thought about searching disassembly for hit detection, but
      // don't think it worth it right now.  Don't have labels, (unlike
      // T2K), and level doesn't appear to restart on player death (unlike)
      // T2K), so think this script will make D2K pretty beatable, even
      // if all you want to do is use blaster and no bombs.
      //
      // Cheat for shield may OBE this.
   }
   return 0;
}

void bigp_init()
{
   void* pMod = bigpemu_get_module_handle();

   bigpemu_set_module_usage_flags(pMod, BIGPEMU_MODUSAGE_DETERMINISMWARNING);

   // Register cheat settings with BigPEmu GUI interface.
   const int catHandle = bigpemu_register_setting_category(pMod, "Defender 2K - 2K");
   sLivesSettingHandle = bigpemu_register_setting_bool(catHandle, "Infinite Lives", 1);
   sBombsSettingHandle = bigpemu_register_setting_bool(catHandle, "Infinite Bombs", 1);
   sShieldsSettingHandle = bigpemu_register_setting_bool(catHandle, "Infinite Shields", 1);
   sPongSettingHandle = bigpemu_register_setting_bool(catHandle, "Enable Plasma Pong", 1);
   sFlossieSettingHandle = bigpemu_register_setting_bool(catHandle, "Enable Flossie Plus Mode", 0);
   sBeestSettingHandle = bigpemu_register_setting_bool(catHandle, "Enable Beest Cheat (3 and 6 Keys)", 1);
   sDiffLvlSettingHandle = bigpemu_register_setting_bool(catHandle, "Unlock Difficulty And Levels", 1);
   sNumEnemiesForOrbSettingHandle = bigpemu_register_setting_int_full(catHandle, "Num Enemies For Orb Release", INVALID, INVALID, D2K_2K_MAX_NUM_ENEMIES_FOR_ORB_RELEASE, 1);
   sPreventOrbWarpingSettingHandle = bigpemu_register_setting_bool(catHandle, "Prevent Orbs Initiating Bonus Round", 0);

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
   sShieldsSettingHandle = INVALID;
   sPongSettingHandle = INVALID;
   sFlossieSettingHandle = INVALID;
   sBeestSettingHandle = INVALID;
   sDiffLvlSettingHandle = INVALID;
   sNumEnemiesForOrbSettingHandle = INVALID;
   sPreventOrbWarpingSettingHandle = INVALID;
}