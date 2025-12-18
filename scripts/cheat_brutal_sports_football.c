//__BIGPEMU_SCRIPT_MODULE__
//__BIGPEMU_META_DESC__    "Infinite money and password unlock, in Brutal Sports Football."
//__BIGPEMU_META_AUTHOR__  "jguff"

#include "bigpcrt.h"

// Found using BigPEmu System->Load Software screen and highlighting ROM file.
#define BSFOOTBALL_HASH             0x04C9B92982B10EA5ULL

// Found using a Pro Action Replay like script that finds addresses of always decreasing values.
#define BSFOOTBALL_MONEY_ADDR       0x00092532
#define BSFOOTBALL_MONEY_VALUE      500000

// Found by entering BP1CQ3RDF into Password Screen within game, then ascii searching for
// BP1CQ3RDF within RAM using Noesis Memory Window Find capability
#define BSFOOTBALL_PASSWORD_ADDR    0x00092752
#define BSFOOTBALL_PASSWORD_LENGTH  18

#define INVALID                     -1

// Boilerplate.
static int32_t sOnLoadEvent = INVALID;
static int32_t sOnEmuFrame = INVALID;
static int32_t sMcFlags = 0;
static const int32_t skMcFlag_Loaded = (1 << 0);

// Setting handles.
static int sMoneySettingHandle = INVALID;
static int sPasswordSettingHandle = INVALID;

#define BSFOOTBALL_NUM_PASSWORDS    (2 * (6+6+6+5))
#define BSFOOTBALL_PASSWORD_LENGTH  18
static uint8_t passwords[BSFOOTBALL_NUM_PASSWORDS][BSFOOTBALL_PASSWORD_LENGTH] = {
                           // Demons Passwords
   "P167FV77777FF4FFFF",   // First League, First Match
   "VDQTD!PPPPPZVS2Z10",   // First League, Second Match
   "VBNHP4RRRRRX5LZYVW",   // First League, Third Match
   "V6077JXXXXXRC2GSSS",   // First League, Fourth Match
   "8ZPJXLQQQQQYFM!VV0",   // First League, Fifth Match
   "5TY48VZZZZZP!WGSLQ",   // First League, Final Match
   "9J105R44444JJ7JJJJ",   // Second League, First Match
   "2GRLKJDDDDD84K987!",   // Second League, Second Match
   "7Q5!9400000NDVQLPM",   // Second League, Third Match
   "!3MRQXJJJJJ40FV685",   // Second League, Fourth Match
   "L93ZCP22222L3XGNQS",   // Second League, Fifth Match
   "6N26XZ33333KY8LDFC",   // Second League, Final Match
   "M9BRJ511111MMYMMMM",   // Third League, First Match
   "K3M5WD99999CH6DCDC",   // Third League, Second Match
   "WRL7D6!!!!!BL5JDFC",   // Third League, Third Match
   "WQ2DWQBBBBB!YH1849",   // Third League, Fourth Match
   "XHWF8XJJJJJ4MF06W6",   // Third League, Fifth Match
   "LY0TB9DDDDD8MKX904",   // Third League, Final Match
   "HBKBNZ3333NHF8GKJH",   // Fourth League, Second Match
   "DVZHP2PPPP417SX102",   // Fourth League, Third Match
   "15XDXCRRRRRX9L62YW",   // Fourth League, Fourth Match
   "PYB44F!!!!!BV5QHGF",   // Fourth League, Fifth Match
   "Y4SYXHWWWWWS5ZCMNN",   // Fourth League, Final Match
                           //
                           // Savages Passwords
   "P167FV77777FF4FFFF",   // First League, First Match
   "VDQTD!PPPPPZVS2Z10",   // First League, Second Match
   "VBNHP4RRRRRX5LZYVW",   // First League, Third Match
   "V6077JXXXXXRC2GSSS",   // First League, Fourth Match
   "8ZPJXLQQQQQYFM!VV0",   // First League, Fifth Match
   "5TY48VZZZZZP!WGSLQ",   // First League, Final Match
   "9J105R44444JJ7JJJJ",   // Second League, First Match
   "2GRLKJDDDDD84K987!",   // Second League, Second Match
   "7Q5!9400000NDVQLPM",   // Second League, Third Match
   "!3MRQXJJJJJ40FV685",   // Second League, Fourth Match
   "L93ZCP22222L3XGNQS",   // Second League, Fifth Match
   "6N26XZ33333KY8LDFC",   // Second League, Final Match
   "M9BRJ511111MMYMMMM",   // Third League, First Match
   "K3M5WD99999CH6DCDC",   // Third League, Second Match
   "WRL7D6!!!!!BL5JDFC",   // Third League, Third Match
   "WQ2DWQBBBBB!YH1849",   // Third League, Fourth Match
   "XHWF8XJJJJJ4MF06W6",   // Third League, Fifth Match
   "LY0TB9DDDDD8MKX904",   // Third League, Final Match
   "HBKBNZ3333NHF8GKJH",   // Fourth League, Second Match
   "DVZHP2PPPP417SX102",   // Fourth League, Third Match
   "15XDXCRRRRRX9L62YW",   // Fourth League, Fourth Match
   "PYB44F!!!!!BV5QHGF",   // Fourth League, Fifth Match
   "Y4SYXHWWWWWS5ZCMNN",   // Fourth League, Final Match
};

static uint32_t on_sw_loaded(const int eventHandle, void* pEventData)
{
   const uint64_t hash = bigpemu_get_loaded_fnv1a64();

   sMcFlags = 0;

   if (BSFOOTBALL_HASH == hash)
   {
      sMcFlags |= skMcFlag_Loaded;

      // Use ascii code for '%' when encounter a '!'.
      // Developers of Brutal Sports Football either
      // a) mixed up ascii values for '!' and '%', or
      // b) developers' '%' font looks like most people expect an '!' to look like.
      for (uint8_t passwordIdx = 0; passwordIdx < BSFOOTBALL_NUM_PASSWORDS; ++passwordIdx)
      {
         for (uint8_t charIdx = 0; charIdx < BSFOOTBALL_PASSWORD_LENGTH; ++charIdx)
         {
            if ('!' == passwords[passwordIdx][charIdx])
            {
               passwords[passwordIdx][charIdx] = '%';
            }
         }
      }
   }
   return 0;
}

static uint32_t on_emu_frame(const int eventHandle, void* pEventData)
{
   if (sMcFlags & skMcFlag_Loaded)
   {
      // Get setting values.  Doing this once per frame, allows player
      // to modify setting during game session.
      int moneySettingValue = INVALID;
      int passwordSettingValue = INVALID;
      bigpemu_get_setting_value(&moneySettingValue, sMoneySettingHandle);
      bigpemu_get_setting_value(&passwordSettingValue, sPasswordSettingHandle);

      if (moneySettingValue > 0)
      {
         // At end of match (didn't even bother playing/watching the actual match)
         // transition screen showed zero dollars, but when entered Lockerroom
         // screen, the 500,000 showed up.  And more importantly was able to
         // upgrade every players health to max.
         bigpemu_jag_write32(BSFOOTBALL_MONEY_ADDR, BSFOOTBALL_MONEY_VALUE);
      }

      // Must be password for level greater then Level 1.
      if (passwordSettingValue > 1)
      {
         for (uint32_t bytesIndex = 0; bytesIndex < ((uint32_t) BSFOOTBALL_PASSWORD_LENGTH); ++bytesIndex)
         {
            // user views 1-based, code views 0-based
            const uint8_t byteValue = passwords[passwordSettingValue-1][bytesIndex];
            bigpemu_jag_write8(BSFOOTBALL_PASSWORD_ADDR + bytesIndex, byteValue);
         }

         // TODO: Create more appropriate GUI BigPEmu setting controls for this.
         //       Allow user to select from the two teams, the leagues, the matches.
         //       Likely noone will use anyways.
         //       So wait and see if anyone even notices.
         //       Probably not worth the effort.
      }
   }
   return 0;
}

void bigp_init()
{
   void* pMod = bigpemu_get_module_handle();

   bigpemu_set_module_usage_flags(pMod, BIGPEMU_MODUSAGE_DETERMINISMWARNING);

   // Register cheat settings with BigPEmu GUI interface.
   const int catHandle = bigpemu_register_setting_category(pMod, "BrS Football");
   sMoneySettingHandle = bigpemu_register_setting_bool(catHandle, "Infinite Money", 1);
   sPasswordSettingHandle = bigpemu_register_setting_int_full(catHandle, "Start Level With Filled In Password", INVALID, INVALID, BSFOOTBALL_NUM_PASSWORDS, 1);

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

   sMoneySettingHandle = INVALID;
   sPasswordSettingHandle = INVALID;
}