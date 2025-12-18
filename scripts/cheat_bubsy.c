//__BIGPEMU_SCRIPT_MODULE__
//__BIGPEMU_META_DESC__    "Infinite lives, time, tshirt invincibility to hits and level select, for Bubsy."
//__BIGPEMU_META_AUTHOR__  "jguff"

// "DAVE7's not here man."

#include "bigpcrt.h"

// Found using BigPEmu System->Load Software screen and highlighting ROM file.
#define BUBSY_HASH      0x5C0C7827948F0F27ULL

// According to Bubsy game manual, Bubsy becomes invincible for short period
// of time after collecting a flashing tshirt.  Skimmed a Bubsy playthrough
// on youtube, and didn't notice any flashing tshirts available, until first
// couple minutes of Level6 and then didn't see anything displayed indicating
// when bubsy was and was no longer invincible (no flickering whatsoever).
// Had intended to play game to collect a tshirt, and then use Pro Action
// Replay decreasing search script to locate invincible address.
// However, am not going to play through 5 levels of this game.
//
//////around 7:30 in level 6 non flashing tshirt with exclamation point.  bubsy attains
// and no flickering or any change.
//
// Located a copy of some version of Jaguar Bubsy's source code, and noticed
// in BUBRAM.S: 
//     INVTIMER (comment says for invincibility)
//     DSCORE (comment says for displayed score).
// Saw some other files reference INVTIMER, leading me to believe that
// INVTIMER is the address i want to write to.
//
// Used Noesis Memory Window and Find Data functionality to locate a couple
// addresses that contained displayed game score address.  Then counted
// 28 byte offset (as specified in BUBRAM.S) to locate the address which to
// write for INVTIMER (below).  Noted BUBSTARTTIME supposed to be next 4 bytes,
// and next 4 bytes contained the display digits of start time, as hex.
#define BUBSY_INVINCIBLE_ADDR    0x0002D10E
#define BUBSY_INVINCIBLE_VALUE   0x7FFF

// From BUBSRAM.S:
//     INVTIMER    ds.w  1  ; INVINCABILITY TIMER
//     SHADOWTIMER ds.w  1  ; SHADOW TIMER DUH
//
// Wasn't sure what SHADOWTIMER was, but given proximity to invincability
// and to similar fashion it is documented, searched through code a bit.
// Saw some references to shadow timer near references to invincability.
// Tried writing same value to shadowtimer to see what results were.
// Appears Shadow makes bubsy invincable to spikes and perhaps other objects.
// Previously had noted INVTIMER made bubsy invincable to enemies but not
// spikes.  So Shadow increases the invincability factor some.
// Still can't avoid falling into pits though.
//
// Atari Jaguar Bubsy Fractured Furry Tales manual documents the following:
//     Number T-shirts: Gives Bubsy more lives.
//     Flashing T-shirt: Makes Bubsy invincable for a short time.
//
// Super Nintendo Bubsy Claws Encounters Of The Furred Kind documents the following:
//     Numbered T-Shirts: More Bubsy: See the big number on these shirts?  That's how
//                        many more lives <bubsy> get<s>.
//     Black T-Shirts: Shadow Bubsy: When <bubsy> wear<s> one of these black T-shirts,
//                     the Woolies can't even see <bubsy>.
//     Flashing T-Shirts: Super Bubsy:  <bubsy> is totally invincible to Woolies and
//                     their henchmen (of course water, spikes and other natural
//                     disasters can have a deadly effect on me).
#define BUBSY_SHADOW_ADDR  BUBSY_INVINCIBLE_ADDR+2
#define BUBSY_SHADOW_VALUE 0x7FFF

// Located TIME, as defined in BUBSRAM.S, using similar method as above.
#define BUBSY_TIME_ADDR    0x0002D13E

// Using Noesis Memory Window, searched for data value of 0x0009 near the
// INVTIMER address.  Then calculated byte offset to LIVES, from INVTIMER,
// in BUBSRAM.S.  Then confirmed in Noesis Memory Window that address points
// to value of 9 (for number of lives displayed on screen).  Was off by 4 bytes
// in calculation, or else looking at old revision of source code.  Value was
// 8 (0-based), instead of 9 as well.  Confirmed data value decrements
// everytime Bubsy dies.
#define BUBSY_LIVES_ADDR   0x0002D163
#define BUBSY_LIVES_VALUE  8

// Source code makes reference to NoWater address.  Writes 0 and 1 to the address,
// for transition logic and level initialization, but nothing ever reads and applies
// that address.
// Don't know that Bubsy can get more playable, then with what is above.

// 6 byte ascii password.
//
// Found after starting game, selecting Options/Password screen, then entering in
// password of 965874, then using Noesis Memory Window Find capability, to
// find address pointing to 965874.
#define BUBSY_PASSWORD_ADDR   0x0002EFF6
#define BUBSY_PASSWORD_LENGTH 6
#define BUBSY_NUM_LEVELS      15

#define INVALID               -1

// Boilerplate.
static int32_t sOnLoadEvent = INVALID;
static int32_t sOnEmuFrame = INVALID;
static int32_t sMcFlags = 0;
static const int32_t skMcFlag_Loaded = (1 << 0);

// Setting handles.
static int sLivesSettingHandle = INVALID;
static int sTimeSettingHandle = INVALID;
static int sInvincibleSettingHandle = INVALID;
static int sShadowSettingHandle = INVALID;
static int sPasswordSettingHandle = INVALID;

// Passwords for bubsy.  6 digits each.  Prefixed 00 digits are not used, only there to pad out each to full 32 bits.
static uint32_t const passwords[BUBSY_NUM_LEVELS+1] = {
   0x00000000, // Dummy value for non-existant Level 0
   0x00000000, // Dummy value for non-needed Level 1
   0x00392652, // Level 2
   0x00458227,
   0x00958936,
   0x00739294, // Level 5
   0x00184792,
   0x00812615,
   0x00781367,
   0x00126712,
   0x00236721, // Level 10
   0x00673167,
   0x00792323,
   0x00672328,
   0x00782389,
   0x00672345  // Level 15
};

static uint32_t on_sw_loaded(const int eventHandle, void* pEventData)
{
   const uint64_t hash = bigpemu_get_loaded_fnv1a64();

   sMcFlags = 0;

   if (BUBSY_HASH == hash)
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
      int timeSettingValue = INVALID;
      int invincibleSettingValue = INVALID;
      int shadowSettingValue = INVALID;
      int passwordSettingValue = INVALID;
      bigpemu_get_setting_value(&livesSettingValue, sLivesSettingHandle);
      bigpemu_get_setting_value(&timeSettingValue, sTimeSettingHandle);
      bigpemu_get_setting_value(&invincibleSettingValue, sInvincibleSettingHandle);
      bigpemu_get_setting_value(&shadowSettingValue, sShadowSettingHandle);
      bigpemu_get_setting_value(&passwordSettingValue, sPasswordSettingHandle);

      if (livesSettingValue)
      {
         bigpemu_jag_write8(BUBSY_LIVES_ADDR, BUBSY_LIVES_VALUE);
      }

      if (timeSettingValue)
      {
         // Setting time to 1 minute 11 seconds.  There is a field
         // named BUBSTARTTIME defined in BUBSRAM.S, and probably
         // doesn't matter, but would like to ensure the value
         // writen here is smaller then BUBSTARTTIME is ever set to
         // for any level, just in case.  Presumably BUBSTARTTIME
         // is never set below 1 minute 11 seconds, as Level1
         // sets BUBSTARTTIME to 9 minutes 59 seconds.
         //
         // Some chance this will cause an infinite loop at
         // end-of-level score tally.  Suspect a copy of remaining
         // time is made at end-of-level score tally and will not
         // go into infinite loop.  Youtube playthrough definitely
         // shows that remaining time is part of end-of-level score
         // tally.
         bigpemu_jag_write16(BUBSY_TIME_ADDR, 0x0111);

         // TODO: Someone needs to test that do not enter infinite loop
         //       when complete level.
      }

      if (invincibleSettingValue)
      {
         bigpemu_jag_write16(BUBSY_INVINCIBLE_ADDR, BUBSY_INVINCIBLE_VALUE);
      }

      if (shadowSettingValue)
      {
         bigpemu_jag_write16(BUBSY_SHADOW_ADDR, BUBSY_SHADOW_VALUE);
      }

      // Must be password for level greater then Level 1.
      if (passwordSettingValue > 1)
      {
         // This auto populates the password, in the Options/Password screen of the game,
         // based on configuration setting for level to skip to.
         // Should make using/locating/knowing-about these passwords easier.
         uint32_t password = passwords[passwordSettingValue];
         uint32_t destByteIdx = ((uint32_t) BUBSY_PASSWORD_LENGTH - 1);
         for (uint32_t bytesWritten = 0; bytesWritten < ((uint32_t) BUBSY_PASSWORD_LENGTH); ++bytesWritten, --destByteIdx)
         {
            const uint8_t byteValue = ((password >> (bytesWritten * 4)) & 0x0F);
            const uint8_t asciiByteValue = ((uint8_t)'0') + byteValue;
            bigpemu_jag_write8(BUBSY_PASSWORD_ADDR + destByteIdx, asciiByteValue);
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
   const int catHandle = bigpemu_register_setting_category(pMod, "Bubsy");
   sLivesSettingHandle = bigpemu_register_setting_bool(catHandle, "Infinite Lives", 1);
   sTimeSettingHandle = bigpemu_register_setting_bool(catHandle, "Infinite Time", 1);
   sInvincibleSettingHandle = bigpemu_register_setting_bool(catHandle, "Invincible To Enemy Hits", 1);
   sShadowSettingHandle = bigpemu_register_setting_bool(catHandle, "Shadow Invincible To Object Hits", 1);
   sPasswordSettingHandle = bigpemu_register_setting_int_full(catHandle, "Start Level With Filled In Password", INVALID, INVALID, BUBSY_NUM_LEVELS, 1);

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
   sTimeSettingHandle = INVALID;
   sInvincibleSettingHandle = INVALID;
   sShadowSettingHandle = INVALID;
   sPasswordSettingHandle = INVALID;
}