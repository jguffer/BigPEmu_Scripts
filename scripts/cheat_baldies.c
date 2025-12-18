//__BIGPEMU_SCRIPT_MODULE__
//__BIGPEMU_META_DESC__    "Infinitely powered bars and password unlock, in Baldies."
//__BIGPEMU_META_AUTHOR__  "jguff"

#include "bigpcrt.h"

// Found using BigPEmu System->Load Software screen and highlighting ROM file.
#define BALDIES_HASH             0x23F072D1924E84ACULL

// Located red bar address using Pro Action Replay like decreasing value search script.
// Then inspected memory using Noesis Memory Window, while game running to find the
// other addresses.
#define BALDIES_RED_BAR_ADDR     0x000271e6
#define BALDIES_BLUE_BAR_ADDR    BALDIES_RED_BAR_ADDR + 2
#define BALDIES_GREEN_BAR_ADDR   BALDIES_RED_BAR_ADDR + 4
#define BALDIES_WHITE_BAR_ADDR   BALDIES_RED_BAR_ADDR + 6
#define BALDIES_BAR_VALUE        500

// 8 byte ascii password.
//
// Found after starting game, selecting Password screen, then entering in
// password of 75310864, then using Noesis Memory Window Find capability, to
// find address pointing to 75310864.
#define BALDIES_PASSWORD_ADDR    0x0005E134
#define BALDIES_PASSWORD_LENGTH  8
#define BALDIES_NUM_LEVELS       100

#define INVALID                  -1

// Boilerplate.
static int32_t sOnLoadEvent = INVALID;
static int32_t sOnEmuFrame = INVALID;
static int32_t sMcFlags = 0;
static const int32_t skMcFlag_Loaded = (1 << 0);

// Setting handles.
static int sPowerBarsSettingHandle = INVALID;
static int sPasswordSettingHandle = INVALID;

static uint32_t const passwords[101] = {
   0x0,        // Dummy value for non-existant Level 0
   0x0,        // Dummy value for non-needed Level 1
               // Green
   0x69317691, // Level 2
   0x32585431, // Level 3
   0x53199313, // Level 4
   0x95568176, // Level 5
   0x14799741, // Level 6
   0x99112932,
   0x25579427,
   0x94554648,
   0x95555366, // Level 10
   0x95532656,
   0x22938689,
   0x89232323,
   0x22513979,
   0x33728582, // Level 15
   0x66977534,
   0x22597714,
   0x94212475,
   0x65545266,
   0x26395999, // Level 20

               // Ice
   0x26481912, // Level 21
   0x37736169,
   0x29329995,
   0x25849779,
   0x44694221, // Level 25
   0x25259781,
   0x26827251,
   0x37495714,
   0x25899273,
   0x25141462, // Level 30
   0x98435959,
   0x69667792,
   0x24164317,
   0x55616442,
   0x96722219, // Level 35
   0x56299991,
   0x28619972,
   0x98692371,
   0x22721793,
   0x99933281, // Level 40

               // Circus
   0x99799799, // Level 41
   0x71394421,
   0x37118763,
   0x51776684,
   0x99584621, // Level 45
   0x96193782,
   0x55992751,
   0x75326691,
   0x36296862,
   0x17228223, // Level 50
   0x75478824,
   0x67788234,
   0x13324585,
   0x35133199,
   0x69751568, // Level 56
   0x11447799,
   0x29399112,
   0x72254579,
   0x89446554,
   0x69653555, // Level 60

               // Desert
   0x69556532, // Level 61
   0x92826938,
   0x38293232,
   0x92729513,
   0x23835728, // Level 65
   0x46365977,
   0x42127597,
   0x59744212,
   0x66652545,
   0x92969395, // Level 70
   0x22169481,
   0x93671736,
   0x52999329,
   0x92757849,
   0x14242694, // Level 75
   0x12857259,
   0x12562827,
   0x43177495,
   0x32752899,
   0x22654141, // Level 80

               // Hell
   0x99589435, // Level 81
   0x26997667,
   0x72143164,
   0x25454616,
   0x99162722, // Level 85
   0x15969299,
   0x22789619,
   0x19783692,
   0x32927721,
   0x19892933, // Level 90
   0x99997799,
   0x17214394,
   0x33677118,
   0x45816776,
   0x19296584, // Level 95
   0x29867193,
   0x15557992,
   0x17956326,
   0x23668296,
   0x31272228  // Level 100
};

static uint32_t on_sw_loaded(const int eventHandle, void* pEventData)
{
   const uint64_t hash = bigpemu_get_loaded_fnv1a64();

   sMcFlags = 0;

   if (BALDIES_HASH == hash)
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
      int powerBarsSettingValue = INVALID;
      int passwordSettingValue = INVALID;
      bigpemu_get_setting_value(&powerBarsSettingValue, sPowerBarsSettingHandle);
      bigpemu_get_setting_value(&passwordSettingValue, sPasswordSettingHandle);

      if (powerBarsSettingValue)
      {
         // Infinitely powered bars.
         //
         // This may be only for display purposes and not what
         // the game recognizes.
         //
         // Need someone who understands whats going on in this
         // game to confirm whether or not the actual game logic
         // recognizes these values.
         bigpemu_jag_write16(BALDIES_RED_BAR_ADDR, BALDIES_BAR_VALUE);
         bigpemu_jag_write16(BALDIES_BLUE_BAR_ADDR, BALDIES_BAR_VALUE);
         bigpemu_jag_write16(BALDIES_GREEN_BAR_ADDR, BALDIES_BAR_VALUE);
         bigpemu_jag_write16(BALDIES_WHITE_BAR_ADDR, BALDIES_BAR_VALUE);
      }

      // Must be password for level greater then Level 1.
      if (passwordSettingValue > 1)
      {
         // This works but difficult for anyone else to realize it will work.
         // The password screen will come up blank, but the password has been written in RAM as needed.
         // If user clicks Thumbs Up guy on the password screen with blank password (having never changed it from blank),
         // the game will take user to level specified by setting.
         //
         // Hopefully the setting name will be self explanatory enough to user.
         const uint32_t password = passwords[passwordSettingValue];
         uint32_t destByteIdx = ((uint32_t) BALDIES_PASSWORD_LENGTH - 1);
         for (uint32_t bytesWritten = 0; bytesWritten < ((uint32_t) BALDIES_PASSWORD_LENGTH); ++bytesWritten, --destByteIdx)
         {
            const uint8_t byteValue = ((password >> (bytesWritten * 4)) & 0x0F);
            const uint8_t asciiByteValue = ((uint8_t)'0') + byteValue;
            bigpemu_jag_write8(BALDIES_PASSWORD_ADDR + destByteIdx, asciiByteValue);
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
   const int catHandle = bigpemu_register_setting_category(pMod, "Baldies");
   sPowerBarsSettingHandle = bigpemu_register_setting_bool(catHandle, "Infinite Power Bars", 1);
   sPasswordSettingHandle = bigpemu_register_setting_int_full(catHandle, "Start Level (With Blank Password)", INVALID, INVALID, BALDIES_NUM_LEVELS, 1);

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

   sPowerBarsSettingHandle = INVALID;
   sPasswordSettingHandle = INVALID;
}