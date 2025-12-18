//__BIGPEMU_SCRIPT_MODULE__
//__BIGPEMU_META_DESC__    "Infinite lives and health, in Soccer Kid."
//__BIGPEMU_META_AUTHOR__  "jguff"

#include "bigpcrt.h"

// Found using BigPEmu System->Load Software screen and highlighting ROM file.
#define SOC_KID_HASH          0x8943633EBC1225BFULL

// Found using Pro Action Replay counter value search like script.
#define SOC_KID_LIVES_ADDR    0x000550DE
#define SOC_KID_HEALTH_ADDR   0x00055118
#define SOC_KID_SCORE_ADDR    0x000550DA
#define SOC_KID_LIVES_VALUE   5
#define SOC_KID_HEALTH_VALUE  4

// Found using Pro Action Replay decreasing value search like script.
//
// This address was the only search hit the script identified.
//
// Turns out Time while displayed (center top of screen below score),
// is not used by the game (at least on Level 1).  Let the timer run
// down to zero, and game didn't seem to care.  Perhaps if played thru
// end of level, the game would tell me to play level again.  Not certain,
// as didn't play to end of level.  Skimmed a youtube playthrough, and
// time remaining was not added to score at end of level.  
//
// BTW: This game continues clock/timer countdown, while the game is paused.
#define SOC_KID_TIME_ADDR     0x00053004

// Found by keying in a password, in the Enter Password Screen, then searching
// Jaguar RAM for the password text in ASCII format, using Noesis Memory Window
// Find capability.
#define SOC_KID_PASSWORD_ADDR 0x00058B76

// 1. Copy-and-paste below into Noesis Memory Window, to debug.
//       Level 5 - USA - Hard: FFMGJFHGKTKT
//       0x46 0x46 0x4D 0x47 0x4A 0x46 0x48 0x47 0x4B 0x54 0x4B 0x54
// 2. Searched for ASCII representation of password entered into ENTER PASSWORD screen.
// 3. Only one hit: 0x00058B76
// 4. Also noticed: 0x00058B9B - ENTER PASSWORD screen's password cursor position 1-based.
// 5. Nearby is alternative representation of user entered password, at 0x00058B88
//	      0x03 0x03 0x09 0x04 0x06 0x03 0x05 0x04 0x07 0x0F 0x07 0x0F
//	      each byte is the offset of screen character from 'B' character.
//	      with breakpoint on these bytes, byte that cursor points to is hit every frame.
// 6. Select START from ENTER PASSWORD SCREEN and/or START from Main screen.
// 7. One of the breakpoints hit and see the following:
//       There are 12 bytes for the password representations.
//       The first 8 bytes of the alternative representation are packed as nibbles into 32 bit long word, then rotated 4 bits, and then stored at 0x00058B70.
//       The last 4 bytes of the alternative representation are packed as nibbles into 16 bit word, then rotated 4 bits, and then stored to 0x00058B74.
//       There is a loop with rotates and sums nibbles/etc of 0x00058B70.
//           At that point the sum value (in a register) is compared to 0x000548B74 and do jump when they are equal.  They were equal.
//       At the jump they eor  #$3edfacb7 into 0x00058B70.
//           Then they start parsing the eored 0x00058B70, copying nibbles/bits/bytes to other-locations-in-memory.
//       Then they copy from the other-locations-in-memory to yet-different-locations-in-memory.
//       Some of the yet-different-locations-in-memory are the locations that are identified above (lives, score).
//       Somewhere probably after the EOR, each individual piece of data is manipulated differently (one might have 2 added to it, another might be multiplied by 6).
//       At one point (maybe before searching for user entered password) i suspected 0x00058B09 of being level, but writing to it every frame did not make a difference.
//       The level information is somewhere in there.
// Working passwords and some relevant discussion at: https://forums.atariage.com/topic/208911-anyone-have-passwords-for-soccer-kid/page/1/
//
// TODO: Let someone else pick up from here to find level information, if someone else cares enough.

#define INVALID      -1

// Boilerplate.
static int32_t sOnLoadEvent = INVALID;
static int32_t sOnEmuFrame = INVALID;
static int32_t sMcFlags = 0;
static const int32_t skMcFlag_Loaded = (1 << 0);

// Setting handles.
static int sLivesSettingHandle = INVALID;
static int sHealthSettingHandle = INVALID;

static uint32_t on_sw_loaded(const int eventHandle, void* pEventData)
{
   const uint64_t hash = bigpemu_get_loaded_fnv1a64();

   sMcFlags = 0;

   if (SOC_KID_HASH == hash)
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
      bigpemu_get_setting_value(&livesSettingValue, sLivesSettingHandle);
      bigpemu_get_setting_value(&healthSettingValue, sHealthSettingHandle);

      if (livesSettingValue > 0)
      {
         bigpemu_jag_write8(SOC_KID_LIVES_ADDR, SOC_KID_LIVES_VALUE);
      }

      if (healthSettingValue > 0)
      {
         bigpemu_jag_write8(SOC_KID_HEALTH_ADDR, SOC_KID_HEALTH_VALUE);
      }

      // Commenting out, due to time not affecting game.  If wrong about
      // time not affecting game, then this can be uncommented.
      //
      //// Time is stored such that only digits 0 through 9 are used, when viewing as hex.
      //// Set to 500 seconds.
      //bigpemu_jag_write16(SOC_KID_TIME_ADDR, SOC_KID_TIME_VALUE);
   }
   return 0;
}

void bigp_init()
{
   void* pMod = bigpemu_get_module_handle();

   bigpemu_set_module_usage_flags(pMod, BIGPEMU_MODUSAGE_DETERMINISMWARNING);

   const int catHandle = bigpemu_register_setting_category(pMod, "Soccer Kid");
   sLivesSettingHandle = bigpemu_register_setting_bool(catHandle, "Infinite Lives", 1);
   sHealthSettingHandle = bigpemu_register_setting_bool(catHandle, "Infinite Health", 1);

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
}