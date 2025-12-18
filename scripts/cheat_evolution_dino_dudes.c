//__BIGPEMU_SCRIPT_MODULE__
//__BIGPEMU_META_DESC__    "Infinite lives and time and level select, for Evolution Dino Dudes."
//__BIGPEMU_META_AUTHOR__  "jguff"

#include "bigpcrt.h"

// Found using BigPEmu System->Load Software screen and highlighting ROM file.
#define DINO_HASH             0x8ED8554F0955F6EFULL

// Number of extra dino dudes.
// Found with Pro Action Replay like descending search value script, 2 byte wide data.
// The sole search result.
// Played with setting values in Noesis and seemed to not like values greater then default value
// of 72.  Each time dude died, value of 8 was subtracted, while on easy mode.
#define DINO_LIVES_ADDR       0x00047BC2
#define DINO_LIVES_VALUE      0x4848

// Looks like 1s field of time.
//
// Found using Noesis Memory Window to inspect memory around DINO_LIVES_ADDR, and see pattern of different
// values:
//     0x00047B9A - minutes digit
//     0x00047BA4 - 10's seconds digit
//     0x00047BAE - 1s seconds digit
// None of these values increment by value of +-1.
//
// Use 1s seconds digit time below.
#define DINO_TIME_ADDR        0x00047BAE
#define DINO_TIME_VALUE       0x4040

// Found by entering password of AN02PCER into Password Screen of game,
// then ascii searching for A02PCER in RAM using Noesis Memory Window
// Find capability.
#define DINO_PASSWORD_ADDR    0x00047E16

#define DINO_NUM_LEVELS       80
#define DINO_PASSWORD_LENGTH  14
#define DINO_ASCII_0          0x30

// Have gone at passwords the wrong way.
// Password Screen doesn't read what script wrote.
//
// Then RTFM...
// Apparently the last level's password can be restored by the game, even
// after full power cycle of Jaguar.  Which means, in spite of having complicated
// password screen which annoyingly shakes left and right on purpose, they
// are saving the last level to EEPROM.
//
// Searched for the first and last passwords in RAM to locate the following
// 2 addresses.
//
// Put read breakpoint on DINO_GAME_PASSWORDS_START_ADDR, and deleted my EEPROM file.
// Started game and went to Password Selection Screen, and clicked <LP> for last password.
// Breakpoint hit.
// Disassembly shows DINO_LAST_LEVEL_ENTERED_ADDR used to index passwords, for last password,
// Which is likely the level state that is needed.
// It is a 1-based value.
//
#define DINO_GAME_PASSWORDS_START_ADDR	0x000D86B0
#define DINO_GAME_PASSWORDS_END_ADDR	0x000D8A3B
#define DINO_LAST_LEVEL_ENTERED_ADDR	0x00004EC9

// After getting password auto-filled/selectable, quickly exported
// RAM after starting different levels.  Then ran Pro Action Replay
// like counter value search script, and came up with the following
// addresses: '0x4ec9', '0x75c9', '0x4d311', '0x4d329', '0x504d7'
// That is 4 additional addresses.
// Changed setting value, softbooted, and second value synched with new first value.  The other 3 had different values and at least 2 of those addreesses contents changed while in/accessing non-game-play screens, so looks kinda dangerous to write to them.
// Loaded the password and none of last 3 synched.
// Clicked OK, and the last value synched.  was at main selection screen ladder go left right screen when checked.
//     this value may not have changed like the other two did
// Went to right, and at beach screen / level description, the third and fourth address contents synched up.
// Should try writing to 0x504d7.  might be able to completely avoid the password screen.
// Restarted Jaguar and Noesis shows me the last address has value of 1 constant until clicked OK, and then switches to password level number value
// Makes sense if you don't enter a password, you start at level 1, and the last address has to be the level-to-start-at 
//
// Unsure how emulator users will want this to be set up: a) password, b) no password, c) both.  There are advantages/disadvantages to at least the first 2 options.
#define DINO_START_LEVEL_ADDR          0x000504d7

// Searched RAM for "COMING UP... LEVEL <level number>" which is text displayed after user selects to start/resume level.
// Search results came back with 0x0004785C
// 0x00047870 is the starting address for <level number>.
#define DINO_RESUME_SCREEN_LEVEL_ADDR  0x00047870

#define INVALID                        -1

// Boilerplate.
static int32_t sOnLoadEvent = INVALID;
static int32_t sOnEmuFrame = INVALID;
static int32_t sMcFlags = 0;
static const int32_t skMcFlag_Loaded = (1 << 0);

// Setting handles.
static int sLivesSettingHandle = INVALID;
static int sTimeSettingHandle = INVALID;
static int sPasswordSettingHandle = INVALID;

// At this point, this is just for reference, in case something
// in script fails.  Script is no longer using this.
// Might have needed to pad each to 14 characters length, if this had been used.
static const uint8_t sPasswords [DINO_NUM_LEVELS][DINO_PASSWORD_LENGTH] = {
   "ROUND ONE",      // #1
   "LIBERTY ISLAND", // #2
   "STONE WALL",     // #3
   "G MEN",          // #4
   "WILD WEST",      // #5
   "LEMON ENTRY",    // #6
   "WAGON WHEEL",    // #7
   "OIL DRUM",       // #8
   "MOON ORBIT",     // #9
   "HARD ROCK",      //#10
   "TRIP AND FALL",  //#11
   "ALARM CLOCK",    //#12
   "BIG COUNTRY",    //#13
   "HOG TIED",       //#14
   "CAN CAN",        //#15
   "CUTE MOUSE",     //#16
   "SPARK PLUG",     //#17
   "PONY EXPRESS",   //#18
   "PADDED CELL",    //#19
   "LOG PLUME",      //#20
   "CANVAS SAIL",    //#21
   "GOLDEN ERA",     //#22
   "WIDE SEAT",      //#23
   "BAD KARMA",      //#24
   "CRASH BARRIER",  //#25
   "LIME GLASS",     //#26
   "SURF UP",        //#27
   "PENAL COLONY",   //#28
   "RELIEF ART",     //#29
   "TRIBAL DANCE",   //#30
   "SODA FOUNTAIN",  //#31
   "PARKING SPACE",  //#32
   "PIZZA DUDE",     //#33
   "CROW FLIES",     //#34
   "TILED ROOF",     //#35
   "SLATE MISSING",  //#36
   "OPENING TIME",   //#37
   "INNER PEACE",    //#38
   "BAD DOG",        //#39
   "SOUR BELLY",     //#40
   "LARGE MUG",      //#41
   "HALF A BET",     //#42
   "SING SING",      //#43
   "BROWN COW",      //#44
   "IRON HORSE",     //#45
   "WHITE MALE",     //#46
   "BOX OFFICE",     //#47
   "CORNY FUR",      //#48
   "ATOM CAT",       //#49
   "FREE WHEELING",  //#50
   "BRUSH FIRE",     //#51
   "CAR BRA",        //#52
   "PORK PIES",      //#53
   "STORMY WEATHER", //#54
   "STAGE COACH",    //#55
   "QUAY BORED",     //#56
   "SPLASH DOWN",    //#57
   "BUG POLITICS",   //#58
   "SHAKE SPEAR",    //#59
   "SCHOOL ZONE",    //#60
   "PINK MARBLE",    //#61
   "ROLLING PLAINS", //#62
   "ICON DRIVE",     //#63
   "CARROT TOP",     //#64
   "QUILL PEN",      //#65
   "TUTTI FRUTTI",   //#66
   "PUBLIC ENEMY",   //#67
   "BIG END",        //#68
   "TAN PARLOR",     //#69
   "NEVER READY",    //#70
   "SHARK FANGS",    //#71
   "STOOL PIGEON",   //#72
   "PROM QUEEN",     //#73
   "RED LETTER",     //#74
   "CORN PONE",      //#75
   "BILGE PUMP",     //#76
   "SIXTY FOUR BIT", //#77
   "HALF MAST",      //#78
   "WALKING BOSS",   //#79
   "SPACE TO LET",   //#80
};

static uint32_t on_sw_loaded(const int eventHandle, void* pEventData)
{
   const uint64_t hash = bigpemu_get_loaded_fnv1a64();

   sMcFlags = 0;

   if (DINO_HASH == hash)
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
      int levelSettingValue = INVALID;
      bigpemu_get_setting_value(&livesSettingValue, sLivesSettingHandle);
      bigpemu_get_setting_value(&timeSettingValue, sTimeSettingHandle);
      bigpemu_get_setting_value(&levelSettingValue, sPasswordSettingHandle);

      if (livesSettingValue > 0)
      {
         bigpemu_jag_write16(DINO_LIVES_ADDR, DINO_LIVES_VALUE);
      }

      if (timeSettingValue > 0)
      {
         bigpemu_jag_write16(DINO_TIME_ADDR, DINO_TIME_VALUE);
      }

      // Must be password for level greater then Level 1.
      if (levelSettingValue > 1)
      {
         // If the password has not been applied yet.  Do not want to keep writing
         // this once password is applied, else player may not be able to continue
         // to level after password selected level.
         int passwordApplied = 0;
         if (! passwordApplied)
         {
            // Commented out, as currently is OBE.  Somewhere down road may need to revert to this, so don't delete this.
            //for (uint32_t bytesIndex = 0; bytesIndex < ((uint32_t) DINO_PASSWORD_LENGTH); ++bytesIndex)
            //{
            //   // user views 1-based, code views 0-based
            //   uint8_t byteValue = sPasswords[levelSettingValue][bytesIndex];
            //   bigpemu_jag_write8(DINO_PASSWORD_ADDR + bytesIndex, byteValue);
            //}

            // Overwrite level information, until the level setting's value appears in the Resume Screen.
            const uint8_t level = levelSettingValue;
            const uint8_t levelTensDigit = level / 10;
            const uint8_t levelOnesDigit = level % 10;
            int overwrite = 1;
            uint8_t readByte = bigpemu_jag_read8(DINO_RESUME_SCREEN_LEVEL_ADDR);
            if (levelTensDigit > 0)
            {
               if (readByte == (levelTensDigit + DINO_ASCII_0))
               {
                  readByte = bigpemu_jag_read8(DINO_RESUME_SCREEN_LEVEL_ADDR + 1);
                  if (readByte == (levelOnesDigit + DINO_ASCII_0))
                  {
                     overwrite = 0;
                  }
               }
            }
            else
            {
               if (readByte == (levelOnesDigit + DINO_ASCII_0))
               {
                  overwrite = 0;
               }
            }

            if (overwrite)
            {
               // Commented out, as currently is OBE.  Somewhere down road may need to revert to this, so don't delete this.
               // Write level/password-index that should be applied by game.
               //bigpemu_jag_write8(DINO_LAST_LEVEL_ENTERED_ADDR, levelSettingValue);

               // Overwrite the level state.
               bigpemu_jag_write8(DINO_START_LEVEL_ADDR, levelSettingValue);
            }

            // TODO: Set passwordApplied when appropriate, if passwordApplied is still needed.
            //       If keep passwordApplied, rename passwordApplied to levelOverwritten.
            //       Initially, was overwriting password, then at some point switched to overwriting level number state.
            //       Then 3 months later, when cleaning up scripts, realized this looks like it may still need a bit more work.
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
   const int catHandle = bigpemu_register_setting_category(pMod, "Evo Dino Dudes");
   sLivesSettingHandle = bigpemu_register_setting_bool(catHandle, "Infinite Extra Dudes", 1);
   sTimeSettingHandle = bigpemu_register_setting_bool(catHandle, "Infinite Time", 1);
   sPasswordSettingHandle = bigpemu_register_setting_int_full(catHandle, "Set Last Used Password Level RTFM", INVALID, INVALID, DINO_NUM_LEVELS, 1);

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
   sPasswordSettingHandle = INVALID;
}