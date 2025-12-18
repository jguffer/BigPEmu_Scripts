//__BIGPEMU_SCRIPT_MODULE__
//__BIGPEMU_META_DESC__    "Tournament/round passwords, for Fever Pitch Soccer."
//__BIGPEMU_META_AUTHOR__  "jguff"

#include "bigpcrt.h"

// Found using BigPEmu System->Load Software screen and highlighting ROM file.
#define FPSOCCER_HASH                  0x537B347137EB7408ULL

// ***** User should RTFM, or user will never figure out how to enter a password for this game.

// 01. Entered password screen.
// 02. Entered a password, and then could not find that user entered password anywhere in
//     RAM. Tried 10+ different potential storage formats.
// 03. Eventually reverted to treating first letter of user password like any other counter, and set to highest value in letter selection row,
// 04. Performed memory dump to file.
// 05. Change first letter to what presumably is next lower character from selection row
// 06. Performed memory dump to file.
// 07. Repeat Step05 through Step06 several times.
// 08. Ran Pro Action Replay like decreasing value search script.
// 09. Came up with 4 addresses
// 10. Repeated procedure for 2nd letter of password, and winnowed down to below address.
#define FPSOCCER_PASSWORD_ADDR         0x00045373

// Constants related to password attributes.
#define FPSOCCER_PASSWORD_LENGTH       13
#define FPSOCCER_NUM_GAMES_PER_TOURN   7
#define FPSOCCER_NUM_TOURNAMENTS       4
#define FPSOCCER_NUM_ALL_STAR_GAMES    1
#define FPSOCCER_NUM_PASSWORDS         FPSOCCER_NUM_GAMES_PER_TOURN * FPSOCCER_NUM_TOURNAMENTS + FPSOCCER_NUM_ALL_STAR_GAMES

// Password character format:
//
//    caps B maps to 0x04
//    caps Z maps to 0x34
//    caps 1 maps to 0x38
//    caps 9 maps to 0x48
//
//         . maps to 0x4A
//         + maps to 0x4C
//         - maps to 0x4E
//         : maps to 0x50
//
//   lower b maps to 0x54
//   lower z maps to 0x84
//
//   lower 1 maps to 0x88
//   lower 9 maps to 0x98
//
//   Definitely not ASCII.
//   Looks like each character increments by 2.
//   Note the upper/yellow and lower/white versions of numerics.
//   Looks like A, a, E, e, I, i, O, o, v, V, 0, caps0 are not displayed
//   to user in the Resume screen, but still fit into the above scheme.

// From https://emucheats.emulation64.com/fever-pitch-soccer-world/
#define FPSOCCER_P1_SCORE_ADDR   0x0004383F
#define FPSOCCER_P2_SCORE_ADDR   0x00043887

#define INVALID                  -1

// Boilerplate.
static int32_t sOnLoadEvent = INVALID;
static int32_t sOnEmuFrame = INVALID;
static int32_t sMcFlags = 0;
static const int32_t skMcFlag_Loaded = (1 << 0);

// Setting handles.
static int sPasswordSettingHandle = INVALID;
// Passwords generated with Romania as protagonist.
static uint8_t const sPasswords[FPSOCCER_NUM_PASSWORDS][FPSOCCER_PASSWORD_LENGTH] = {
   {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},  // Tournament 1 (Asia),     Round 1    - <placeholder>
   {0x24, 0x54, 0x54, 0x54, 0x54, 0x54, 0x54, 0x56, 0x88, 0x56, 0x6A, 0x84, 0x4C},  // Tournament 1 (Asia),     Round 2    - Rbbbbbbc1cmz+
   {0x24, 0x54, 0x54, 0x54, 0x54, 0x54, 0x54, 0x22, 0x58, 0x7C, 0x96, 0x54, 0x4E},  // Tournament 1 (Asia),     Round 3    - RbbbbbbQdv8b          - doesn't work
   {0x24, 0x54, 0x54, 0x54, 0x54, 0x54, 0x54, 0x5E, 0x06, 0x5C, 0x56, 0x14, 0x64},  // Tournament 1 (Asia),     Round 4    - RbbbbbbgCfcJj
   {0x24, 0x54, 0x54, 0x54, 0x54, 0x54, 0x54, 0x60, 0x8A, 0x5E, 0x68, 0x26, 0x0E},  // Tournament 1 (Asia),     Round 5    - Rbbbbbbh2glSG         - doesn't work?
   {0x24, 0x54, 0x54, 0x54, 0x54, 0x54, 0x54, 0x66, 0x6A, 0x60, 0x7E, 0x8A, 0x5E},  // Tournament 1 (Asia),     Semi Final - Rbbbbbbkmhw2g         - doesn't work?
   {0x24, 0x54, 0x54, 0x54, 0x54, 0x54, 0x54, 0x68, 0x4C, 0x64, 0x54, 0x4E, 0x8A},  // Tournament 1 (Asia),     Final      - Rbbbbbbl+jb-2

   {0x24, 0x54, 0x54, 0x54, 0x54, 0x54, 0x54, 0x6C, 0x82, 0x70, 0x56, 0x72, 0x58},  // Tournament 2 (Africa),   Round 1    - Rbbbbbbnypcqd
   {0x24, 0x54, 0x54, 0x54, 0x54, 0x54, 0x54, 0x70, 0x1C, 0x72, 0x54, 0x84, 0x34},  // Tournament 2 (Africa),   Round 2    - RbbbbbbpNqbzZ
   {0x24, 0x54, 0x54, 0x54, 0x54, 0x54, 0x54, 0x72, 0x4A, 0x74, 0x56, 0x96, 0x42},  // Tournament 2 (Africa),   Round 3    - Rbbbbbbq.rc8<CAP6>
   {0x24, 0x54, 0x54, 0x54, 0x54, 0x54, 0x54, 0x76, 0x80, 0x76, 0x54, 0x14, 0x30},  // Tournament 2 (Africa),   Round 4    - RbbbbbbsxsbJX
   {0x24, 0x54, 0x54, 0x54, 0x54, 0x54, 0x54, 0x78, 0x1A, 0x78, 0x56, 0x26, 0x0E},  // Tournament 2 (Africa),   Round 5    - RbbbbbbtMtcSG
   {0x24, 0x54, 0x54, 0x54, 0x54, 0x54, 0x54, 0x7C, 0x4E, 0x7C, 0x54, 0x3A, 0x30},  // Tournament 2 (Africa),   Semi Final - Rbbbbbbv-vb<CAP2>X
   {0x24, 0x54, 0x54, 0x54, 0x54, 0x54, 0x54, 0x80, 0x7E, 0x80, 0x56, 0x4E, 0x4C},  // Tournament 2 (Africa),   Final      - Rbbbbbbxwxc-+

   {0x24, 0x54, 0x54, 0x54, 0x54, 0x54, 0x54, 0x82, 0x18, 0x8A, 0x54, 0x72, 0x64},  // Tournament 3 (Americas), Round 1    - RbbbbbbyL2bqj
   {0x24, 0x54, 0x54, 0x54, 0x54, 0x54, 0x54, 0x84, 0x48, 0x8C, 0x56, 0x84, 0x76},  // Tournament 3 (Americas), Round 2    - Rbbbbbbz<CAP9>3czs
   {0x24, 0x54, 0x54, 0x54, 0x54, 0x54, 0x54, 0x8A, 0x7C, 0x76, 0x54, 0x96, 0x96},  // Tournament 3 (Americas), Round 3    - Rbbbbbb2vsb88         - doesn't work
   {0x24, 0x54, 0x54, 0x54, 0x54, 0x54, 0x54, 0x8C, 0x16, 0x92, 0x56, 0x14, 0x46},  // Tournament 3 (Americas), Round 4    - Rbbbbbb3K6cJ<CAP8>
   {0x24, 0x54, 0x54, 0x54, 0x54, 0x54, 0x54, 0x8E, 0x46, 0x94, 0x54, 0x26, 0x4C},  // Tournament 3 (Americas), Round 5    - Rbbbbbb4<CAP8>7bS+
   {0x24, 0x54, 0x54, 0x54, 0x54, 0x54, 0x54, 0x92, 0x78, 0x96, 0x56, 0x3A, 0x72},  // Tournament 3 (Americas), Semi Final - Rbbbbbb6t8c<CAP2>q
   {0x24, 0x54, 0x54, 0x54, 0x54, 0x54, 0x54, 0x94, 0x14, 0x98, 0x54, 0x4E, 0x4C},  // Tournament 3 (Americas), Final      - Rbbbbbb7J9b-+

   {0x24, 0x54, 0x54, 0x54, 0x54, 0x54, 0x54, 0x96, 0x44, 0x0C, 0x56, 0x72, 0x6C},  // Tournament 4 (Europe),   Round 1    - Rbbbbbb8<CAP7>Fcqn
   {0x24, 0x54, 0x54, 0x54, 0x54, 0x54, 0x54, 0x04, 0x76, 0x14, 0x54, 0x84, 0x58},  // Tournament 4 (Europe),   Round 2    - RbbbbbbBsJbzd
   {0x22, 0x54, 0x54, 0x54, 0x54, 0x54, 0x54, 0x06, 0x10, 0x16, 0x56, 0x96, 0x3A},  // Tournament 4 (Europe),   Round 3    - RbbbbbbCHKc8<CAP2>
   {0x24, 0x54, 0x54, 0x54, 0x54, 0x54, 0x54, 0x08, 0x42, 0x2C, 0x54, 0x14, 0x96},  // Tournament 4 (Europe),   Round 4    - RbbbbbbD<CAP6>VbJ8    - doesn't work
   {0x24, 0x54, 0x54, 0x54, 0x54, 0x54, 0x54, 0x0E, 0x74, 0x2E, 0x56, 0x26, 0x68},  // Tournament 4 (Europe),   Round 5    - RbbbbbbGrWcSl
   {0x24, 0x54, 0x54, 0x54, 0x54, 0x54, 0x54, 0x10, 0x0E, 0x30, 0x54, 0x3A, 0x42},  // Tournament 4 (Europe),   Semi Final - RbbbbbbHGXb<CAP2><CAP6>
   {0x24, 0x54, 0x54, 0x54, 0x54, 0x54, 0x54, 0x14, 0x40, 0x32, 0x56, 0x4E, 0x64},  // Tournament 4 (Europe),   Final      - RbbbbbbJ<CAP5>Yc-j

   {0x24, 0x54, 0x54, 0x54, 0x54, 0x54, 0x54, 0x18, 0x72, 0x38, 0x54, 0x4E, 0x54}   // Tournament 5,            All Stars	- RbbbbbbLq<CAP1>b-b
};

static uint32_t on_sw_loaded(const int eventHandle, void* pEventData)
{
   const uint64_t hash = bigpemu_get_loaded_fnv1a64();

   sMcFlags = 0;

   if (FPSOCCER_HASH == hash)
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
      int passwordSettingValue = INVALID;
      bigpemu_get_setting_value(&passwordSettingValue, sPasswordSettingHandle);

      if (passwordSettingValue > 1)
      {
         // Check if either team has scored.
         const uint8_t scored = bigpemu_jag_read8(FPSOCCER_P1_SCORE_ADDR)
                              + bigpemu_jag_read8(FPSOCCER_P2_SCORE_ADDR);

         // Overwrite password, as long as neither team has scored.
         //
         // If you don't stop overwriting password, at completion of
         // win, game will incorrectly display this rounds password
         // as the password for the next round.
         if (0 == scored)
         {
            for (uint32_t idx = 0; idx < FPSOCCER_PASSWORD_LENGTH; ++idx)
            {
               // User gui setting is 1-based, code is 0-based.
               bigpemu_jag_write8(FPSOCCER_PASSWORD_ADDR + idx, sPasswords[passwordSettingValue-1][idx]);
            }

            // TODO: Unfortunately, this fails if demo mode starts and team scores
            //       before demo mode ends.
            //       Maybe this is good enough.  If you know to avoid demo mode,
            //       this works.
            //
            //       Could use more testing.
            //
            //       So far appears, game does not reset score variables to 0
            //       between conclusion of round and display of password for
            //       next round. So far... If game can enter state where score
            //       variables are reset to 0 between conclusion of round and
            //       display of password for next round, the displayed password
            //       will be incorrect for next round.
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
   const int catHandle = bigpemu_register_setting_category(pMod, "Fever Pitch Soccer");
   sPasswordSettingHandle = bigpemu_register_setting_int_full(catHandle, "Round Password", INVALID, INVALID, FPSOCCER_NUM_PASSWORDS, 1);

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

   sPasswordSettingHandle = INVALID;
}