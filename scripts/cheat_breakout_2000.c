//__BIGPEMU_SCRIPT_MODULE__
//__BIGPEMU_META_DESC__    "Infinite balls, superball, attract, catch, cannon, split, no cracks or exploding balls, and level select in Breakout 2000."
//__BIGPEMU_META_AUTHOR__  "jguff"

#include "bigpcrt.h"

// Found using BigPEmu System->Load Software screen and highlighting ROM file.
#define B2K_HASH                 0x13C35D4055BF1FC3ULL

// Found using Pro Action Replay like counter search script.
#define B2K_NUM_BALLS_ADDR       0x001d2fc1
#define B2K_BALL_STYLE_ADDR      B2K_NUM_BALLS_ADDR + 1
#define B2K_NUM_BALLS_VALUE      6

// Found by counting bytes (as specified by source code) backwards from above. 
#define B2K_SCORE_ADDR           0x001D2FA8
#define B2K_ATTRACT_ADDR         0x001D1A22
#define B2K_CATCH_ADDR           B2K_ATTRACT_ADDR + 4
#define B2K_CANNON_ADDR          B2K_ATTRACT_ADDR + 5
#define B2K_CRACKS_ADDR          B2K_ATTRACT_ADDR + 9
#define B2K_ENABLE_VALUE         1
#define B2K_CRACKS_VALUE         0

// Found by counting bytes (as specified by source code) backwards from above.
// Confirmed correctness using Noesis Memory Window.
#define B2K_BALL0_ADDR           0x001D1970
#define B2K_BALL1_ADDR           0x001D19A8
#define B2K_BALL_LENGTH          B2K_BALL1_ADDR - B2K_BALL0_ADDR
#define B2K_BALL_ACTIVE_OFFSET   0
#define B2K_BALL_SPEEDX_OFFSET   22
#define B2K_BALL_STYLE_OFFSET    B2K_BALL_SPEEDX_OFFSET + 26
#define B2K_GAME_MODE_ADDR       B2K_NUM_BALLS_ADDR + 11

// The screen/level: 0 - 49
// Found by inspecting memory prior to memory reserved for players,
// using Noesis Memory Window
#define B2K_SCREENCOUNT_ADDR     0x001D193F

// Found stepping through disassembly, after placing write breakpoint
// on B2K_SCREENCOUNT_ADDR.
//
// Neither B2K_SCREENCOUNT_ADDR nor B2K_SCREENCOUNT_ADDR_B match what
// expected to find/modify according to b2k_game.h/.c source code.
// b2k_game.s declares two 2 ints that should reside next to one another
// and match in value other then one is module 50(num screens).  But
// can't find that.  Must be missing something.  Or have older version
// of source code.
//
// From b2k_game.h/.c:
//    /** internal data **/
//    int ScreenCount;
//    int Screen;
//    <other lines of code>
//    ScreenCount++;
//    Screen = ScreenCount%NUM_SCREENS;
#define B2K_SCREENCOUNT_ADDR_B   0x001D107F

// From game source code.
#define B2K_NUM_SCREENS          50

#define B2K_SPEED_DELTA          8

#define INVALID                  -1

enum BallStyleEnum {ballLAUNCH, ballNORMAL, ballSUPER, ballEXPLODE, ballCLASSIC, ballBREAKTHRU};
enum GameModeEnum {
   modeTITLE, modeSTARTGAME, modeENDGAME, 
   modeSTARTSCREEN, modeRESUMESCREEN, modeENDSCREEN, 
   modeDOBONUS, modeSHOOT, modePLAY,
   modeKILLSCREEN, modeREVIVESCREEN, modeHIGHSCORE, modeIDLE,
   modePONGSTART, modePONGSHOOT, modePONGPLAY, modePONGEND,
   modeTARGETSTART, modeTARGETPLAY, modeTARGETEND
} ;

// Boilerplate.
static int32_t sOnLoadEvent = INVALID;
static int32_t sOnEmuFrame = INVALID;
static int32_t sMcFlags = 0;
static const int32_t skMcFlag_Loaded = (1 << 0);

// Setting handles.
static int sBallsSettingHandle = INVALID;
static int sSuperballSettingHandle = INVALID;
static int sNoExplodingBallsSettingHandle = INVALID;
static int sAttractSettingHandle = INVALID;
static int sCatchSettingHandle = INVALID;
static int sCannonSettingHandle = INVALID;
static int sCracksSettingHandle = INVALID;
static int sSplitsSettingHandle = INVALID;
static int sLevelSettingHandle = INVALID;

static uint32_t on_sw_loaded(const int eventHandle, void* pEventData)
{
   const uint64_t hash = bigpemu_get_loaded_fnv1a64();

   sMcFlags = 0;

   if (B2K_HASH == hash)
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
      int ballsSettingValue = INVALID;
      int superballSettingValue = INVALID;
      int noExplodingBallsSettingValue = INVALID;
      int attractSettingValue = INVALID;
      int catchSettingValue = INVALID;
      int cannonSettingValue = INVALID;
      int cracksSettingValue = INVALID;
      int splitsSettingValue = INVALID;
      int levelSettingValue = INVALID;
      bigpemu_get_setting_value(&ballsSettingValue, sBallsSettingHandle);
      bigpemu_get_setting_value(&superballSettingValue, sSuperballSettingHandle);
      bigpemu_get_setting_value(&noExplodingBallsSettingValue, sNoExplodingBallsSettingHandle);
      bigpemu_get_setting_value(&attractSettingValue, sAttractSettingHandle);
      bigpemu_get_setting_value(&catchSettingValue, sCatchSettingHandle);
      bigpemu_get_setting_value(&cannonSettingValue, sCannonSettingHandle);
      bigpemu_get_setting_value(&cracksSettingValue, sCracksSettingHandle);
      bigpemu_get_setting_value(&splitsSettingValue, sSplitsSettingHandle);
      bigpemu_get_setting_value(&levelSettingValue, sLevelSettingHandle);

      if (ballsSettingValue > 0)
      {
         bigpemu_jag_write8(B2K_NUM_BALLS_ADDR, B2K_NUM_BALLS_VALUE);
      }

      if (superballSettingValue > 0)
      {
         bigpemu_jag_write8(B2K_BALL_STYLE_ADDR, ballSUPER);
      }

      if (noExplodingBallsSettingValue > 0)
      {
         const uint8_t ballStyle = bigpemu_jag_read8(B2K_BALL_STYLE_ADDR);
         if (ballEXPLODE == ballStyle)
         {
            bigpemu_jag_write8(B2K_BALL_STYLE_ADDR, ballNORMAL);
         }
      }

      if (attractSettingValue > 0)
      {
         bigpemu_jag_write8(B2K_ATTRACT_ADDR, B2K_ENABLE_VALUE);
      }

      if (catchSettingValue > 0)
      {
         bigpemu_jag_write8(B2K_CATCH_ADDR, B2K_ENABLE_VALUE);
      }

      if (cannonSettingValue > 0)
      {
         bigpemu_jag_write8(B2K_CANNON_ADDR, B2K_ENABLE_VALUE);
      }

      if (cracksSettingValue > 0)
      {
         // Caveat:
         //    This may be risky to turn on.  Graphics don't update as want them to.
         //    Even though always write a 0, to overwrite the stinger hits from
         //    incrementing to maximum-of-4/death, the paddle still displays cracks.
         //    Cracks disappear when you let a ball go past paddle.
         //
         //    When test wrote a 1 value to this address, in this method, to verify the
         //    correct address, the paddle initialized and remained invisible.
         //    Probably due to paddle.c not intializing (correctly):
         //    void SetPaddleSize(short who, int size)
         //    {
         //       if(!Player[who].Paddle.Cracks)
         //
         //    However, don't really see why the cracks display and continue to display
         //    when running this conditional block.  And really don't care to take any
         //    more time to track down.
         //
         // This value is incremented by game logic when paddle is hit be stinger.
         // This cheat prevents the incrementing.
         bigpemu_jag_write8(B2K_CRACKS_ADDR, B2K_CRACKS_VALUE);
      }

      if (splitsSettingValue > 0)
      {
         uint8_t active0 = bigpemu_jag_read8(B2K_BALL0_ADDR + B2K_BALL_ACTIVE_OFFSET);
         if (active0)
         {
            // Caveat:
            //    This glitches/freaks-out when both balls get past the paddle at the same time.
            //    The game recovers once paddle hits last remaining freaking out ball.
            //    Possibly only happens when have infinite balls turned on.
            //    Not inclined to fix, unless see easy answer.
            const uint8_t active1 = bigpemu_jag_read8(B2K_BALL1_ADDR + B2K_BALL_ACTIVE_OFFSET);
            if (! active1)
            {
               const uint8_t style0 = bigpemu_jag_read8(B2K_BALL0_ADDR + B2K_BALL_STYLE_OFFSET);
               const uint8_t style1 = bigpemu_jag_read8(B2K_BALL1_ADDR + B2K_BALL_STYLE_OFFSET);
               if ((style0 != ballEXPLODE) && (style1 != ballEXPLODE) && (style0 != ballLAUNCH) && (style1 != ballLAUNCH))
               {
                  // Avoiding jag_read16 assigning to uint16_t.  uint16_t assignments has caused nothing but memory clobberying problems.
                  const uint32_t speedX = (uint32_t) bigpemu_jag_read16(B2K_BALL0_ADDR + B2K_BALL_SPEEDX_OFFSET);

                  // Creating split ball, at first opportunity possible, increases
                  // frequency of the glitch/freak-out described above.
                  // Wait until ball0 has some velocity.
                  if (speedX > 8)
                  {
                     // Copy Ball object's data to other Ball object.
                     uint8_t ball0Data[B2K_BALL_LENGTH];
                     bigpemu_jag_sysmemread(ball0Data, B2K_BALL0_ADDR, ((uint32_t) B2K_BALL_LENGTH));
                     bigpemu_jag_sysmemwrite(ball0Data, B2K_BALL1_ADDR, ((uint32_t) B2K_BALL_LENGTH));

                     // Mimic goody.c::DoSplitBall code:
                     //    Player[who].Ball[sourceBall].SpeedX += 4;
                     //    Player[who].Ball[split].SpeedX -= 4;
                     bigpemu_jag_write16(B2K_BALL0_ADDR + B2K_BALL_SPEEDX_OFFSET, ((uint16_t) (speedX + B2K_SPEED_DELTA)));
                     bigpemu_jag_write16(B2K_BALL0_ADDR + B2K_BALL_SPEEDX_OFFSET, ((uint16_t) (speedX - B2K_SPEED_DELTA)));
                  }
               }
            }
         }
      }

      if (levelSettingValue > 0)
      {
         // Overwrite level information until player has non-zero score.
         const uint32_t score = bigpemu_jag_read32(B2K_SCORE_ADDR);
         if (0 == score)
         {
            // Writing here makes playfield/screen advance, when playing level for first time since starting.
            // However, after winning that playfield/screen, playfield/screens start back at the beginning.
            bigpemu_jag_write8(B2K_SCREENCOUNT_ADDR, levelSettingValue);

            // Writing here makes it advance after beat first level. But won't advance prior to beating the
            // first level.
            bigpemu_jag_write8(B2K_SCREENCOUNT_ADDR_B, levelSettingValue);

            // Really looks like have to write to both addresses.
         }
      }
   }

   // TODO: One time, got into infinite loop of split balls passing paddle, 
   //       and coudn't hit the balls.  Not sure how that happened, considering
   //       all other items the loop could be ended by moving paddle into
   //       ball(s) which had passed paddle.
   //
   //       Wait and see if anyone else reproduces this.

   return 0;
}

void bigp_init()
{
   void* pMod = bigpemu_get_module_handle();

   bigpemu_set_module_usage_flags(pMod, BIGPEMU_MODUSAGE_DETERMINISMWARNING);

   // Register cheat settings with BigPEmu GUI interface.
   const int catHandle = bigpemu_register_setting_category(pMod, "Breakout 2000");
   sBallsSettingHandle = bigpemu_register_setting_bool(catHandle, "Infinite Balls", 1);
   sSuperballSettingHandle = bigpemu_register_setting_bool(catHandle, "Constant Superball", 1);
   sNoExplodingBallsSettingHandle = bigpemu_register_setting_bool(catHandle, "No Exploding Balls", 0);
   sAttractSettingHandle = bigpemu_register_setting_bool(catHandle, "Always Attract", 1);
   sCatchSettingHandle = bigpemu_register_setting_bool(catHandle, "Always Catch", 1);
   sCannonSettingHandle = bigpemu_register_setting_bool(catHandle, "Cannon Always Available", 1);
   sCracksSettingHandle = bigpemu_register_setting_bool(catHandle, "Stinger Cracks Don't Kill", 0);
   sSplitsSettingHandle = bigpemu_register_setting_bool(catHandle, "Infinite Split Balls", 1);
   sLevelSettingHandle = bigpemu_register_setting_int_full(catHandle, "Start At Level", INVALID, INVALID, B2K_NUM_SCREENS, 1);

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

   sBallsSettingHandle = INVALID;
   sSuperballSettingHandle = INVALID;
   sNoExplodingBallsSettingHandle = INVALID;
   sAttractSettingHandle = INVALID;
   sCatchSettingHandle = INVALID;
   sCannonSettingHandle = INVALID;
   sCracksSettingHandle = INVALID;
   sSplitsSettingHandle = INVALID;
   sLevelSettingHandle = INVALID;
}