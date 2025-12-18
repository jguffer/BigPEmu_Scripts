//__BIGPEMU_SCRIPT_MODULE__
//__BIGPEMU_META_DESC__    "Infinite money and time, and level select, in Power Drive Rally."
//__BIGPEMU_META_AUTHOR__  "jguff"

#include "bigpcrt.h"

// Found using BigPEmu System->Load Software screen and highlighting ROM file.
#define PDRALLY_HASH                0x3381B57A63BBDBCBULL

// Found using Pro Action Replay like script and Noesis Memory Window.
// Got 5-10 hits, based on increasing/decreasing search, 1 byte vs 2 bytes, etc.
//
// 0x00005a98 and 4 following bytes appear to be decimal display of time
// Updating one of those bytes updated the bytes's displayed digit, when
// the digit was scheduled to change.
// However, not unsurprisingly, the round then ended before displayed
// time equaled zero.
// 
// Below ended up being real time.  Non-script, standard game initialized to
// 0x8C8 for a 90 second skills test.
#define PDRALLY_TIME_ADDR           0x00005ABE
#define PDRALLY_TIME_VALUE          0x08C8

// Found using Pro Action Replay like script.
// Exported memory after completing first course, and lowering
// money 3 dollars at a time, in the maintenance screen, and
// exporting memory each time lowered the amount of money.
// (Since search script currently only handles byte values, did
// have to change name of exported file to match what lower byte
// value would be in decimal.  Did that conversion for one file,
// and then just increased that filename prefix by 3 for subsequent
// memory exports to file.)
#define PDRALLY_MONEY_ADDR          0x0000D976

// Lots of money.  Should be able to upgrade to top car after a couple
// courses.  30,000 may be large enough to accomplish that as well.
#define PDRALLY_MONEY_VALUE         100000

// Found using Pro Action Replay like script and Noesis Memory Window.
#define PDRALLY_LEVEL_ADDR          0x000064A9

// There are really 32 or 33 levels.  The levels past that are retreads of skills
// tests, that supply 8 minutes 20 seconds of goof off time.  Good for doing donuts. 
#define PDRALLY_NUM_LEVELS          37

// NTSC
#define PDRALLY_FRAMES_PER_SECOND   60

// Should write to level address until a race has started.
// For now be lazy and just write to level address for 2 minutes of first race + 30
// seconds to go from game bootup to start of first race.
// Time/ticks to level change, until at least the title screen appears.
#define PDRALLY_LEVEL_DELAY_SECONDS 150
#define PDRALLY_LEVEL_DELAY_TICKS   PDRALLY_FRAMES_PER_SECOND * PDRALLY_LEVEL_DELAY_SECONDS

#define INVALID                     -1

// Boilerplate.
static int32_t sOnLoadEvent = INVALID;
static int32_t sOnEmuFrame = INVALID;
static int32_t sMcFlags = 0;
static const int32_t skMcFlag_Loaded = (1 << 0);

static int sLevelDelayCounter = -1;

// Setting handles.
static int sTimeSettingHandle = INVALID;
static int sMoneySettingHandle = INVALID;
static int sLevelSettingHandle = INVALID;

static uint32_t on_sw_loaded(const int eventHandle, void* pEventData)
{
   const uint64_t hash = bigpemu_get_loaded_fnv1a64();

   sMcFlags = 0;

   if (PDRALLY_HASH == hash)
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
      int timeSettingValue = INVALID;
      int moneySettingValue = INVALID;
      int levelSettingValue = INVALID;
      bigpemu_get_setting_value(&timeSettingValue, sTimeSettingHandle);
      bigpemu_get_setting_value(&moneySettingValue, sMoneySettingHandle);
      bigpemu_get_setting_value(&levelSettingValue, sLevelSettingHandle);

      if (timeSettingValue > 0)
      {
         // Write into largest of 3 nibbles that are used.
         // Standard game (no script) initializes this word to
         // 0x08C8 (2248 decimal) for skills test countdown start
         // time of 90 seconds.
         //
         // When displayed time (not this time) hits zero, displayed
         // time rolls over to 9+ minutes.  Sometimes have seen some
         // graphics glitches as well, but nothing stopping player
         // from continuing through each course, without having to redo
         // course.
         //
         // Skills Test courses are now fun.  All the time player could ever
         // need and Skills Test courses have enough space to get up speed
         // and do some large donuts.
         bigpemu_jag_write16(PDRALLY_TIME_ADDR, PDRALLY_TIME_VALUE);
      }

      if (moneySettingValue > 0)
      {
         bigpemu_jag_write32(PDRALLY_MONEY_ADDR, PDRALLY_MONEY_VALUE);
      }

      if (levelSettingValue > 0)
      {
         // Only set level for a limited amount of time, so that after
         // completion of level, the game can increment the level value
         // to the next level.
         if (sLevelDelayCounter < PDRALLY_LEVEL_DELAY_TICKS)
         {
            bigpemu_jag_write8(PDRALLY_LEVEL_ADDR, levelSettingValue);

            // Make sure this block is only executed once.
            sLevelDelayCounter += 1;
         }

         // TODO: Find a better variable to poll, so don't have to rely on
         //       hidden timer, that only a reader of the script would know
         //       about.
      }
   }
   return 0;
}

void bigp_init()
{
   void* pMod = bigpemu_get_module_handle();

   bigpemu_set_module_usage_flags(pMod, BIGPEMU_MODUSAGE_DETERMINISMWARNING);

   const int catHandle = bigpemu_register_setting_category(pMod, "Power Drive Rally");
   sTimeSettingHandle = bigpemu_register_setting_bool(catHandle, "Infinite Time", 1);
   sMoneySettingHandle = bigpemu_register_setting_bool(catHandle, "Infinite Money", 1);
   sLevelSettingHandle = bigpemu_register_setting_int_full(catHandle, "Start Level", INVALID, INVALID, PDRALLY_NUM_LEVELS, 1);

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

   sTimeSettingHandle = INVALID;
   sMoneySettingHandle = INVALID;
   sLevelSettingHandle = INVALID;
}