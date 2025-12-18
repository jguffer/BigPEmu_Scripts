//__BIGPEMU_SCRIPT_MODULE__
//__BIGPEMU_META_DESC__    "Infinite time and recovery, in Val d'Isere Skiing And Snowboarding."
//__BIGPEMU_META_AUTHOR__  "jguff"

#include "bigpcrt.h"

// Found using BigPEmu System->Load Software screen and highlighting ROM file.
#define VAL_SKI_HASH                0x8551C87570272A14ULL

// Found using Pro Action Replay like counter value search script.
#define VAL_SKI_MODE_ADDR           0x00141453
#define VAL_SKI_FREERIDE_TIME_ADDR  0x001EC961
#define VAL_SKI_COMPETE_TIME_ADDR   0x001416e0
#define VAL_SKI_TRAINING_TIME_ADDR  0x00141700
#define VAL_SKI_RECOVER_TIME_ADDR   0x001EBEDE
#define VAL_SKI_FREERIDE_TIME_VALUE 47
#define VAL_SKI_COMPETE_TIME_VALUE  12
#define VAL_SKI_TRAINING_TIME_VALUE 0
#define VAL_SKI_RECOVER_TIME_VALUE  0x2040

// Found by inspection of memory, using Noesis Memory Window.
#define VAL_SKI_TIME_LOOPING_ADDR   0x001416e1

#define INVALID                     -1

enum {FREERIDE, TRAINING, COMPETE} modeEnum;

// Boilerplate.
static int32_t sOnLoadEvent = INVALID;
static int32_t sOnEmuFrame = INVALID;
static int32_t sMcFlags = 0;
static const int32_t skMcFlag_Loaded = (1 << 0);

// Setting handles.
static int sTimeSettingHandle = INVALID;
static int sRecoverySettingHandle = INVALID;

static uint32_t on_sw_loaded(const int eventHandle, void* pEventData)
{
   const uint64_t hash = bigpemu_get_loaded_fnv1a64();

   sMcFlags = 0;

   if (VAL_SKI_HASH == hash)
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
      int recoverySettingValue = INVALID;
      bigpemu_get_setting_value(&timeSettingValue, sTimeSettingHandle);
      bigpemu_get_setting_value(&recoverySettingValue, sRecoverySettingHandle);

      if (timeSettingValue > 0)
      {
         // The time cheats for each mode are incompatible with
         // the other modes, causing game hangups in other modes.
         //
         // Read the mode and determine which cheat to apply.
         const uint8_t mode = bigpemu_jag_read8(VAL_SKI_MODE_ADDR);
         if (FREERIDE == mode)
         {
            bigpemu_jag_write8(VAL_SKI_FREERIDE_TIME_ADDR, VAL_SKI_FREERIDE_TIME_VALUE);
         }
         else if (TRAINING == mode)
         {
            bigpemu_jag_write8(VAL_SKI_TRAINING_TIME_ADDR, VAL_SKI_TRAINING_TIME_VALUE);
         }
         else if (COMPETE == mode)
         {
            // Writing to this for time-increasing gameplay modes
            // causes an infinite loop, right before gameplay begins.
            // Writing to this address using Noesis Fill Memory
            // functionality, during middle of increasing-time mode
            // gameplay works as expected though.
            bigpemu_jag_write8(VAL_SKI_COMPETE_TIME_ADDR, VAL_SKI_COMPETE_TIME_VALUE);

            // Luckily setting fractional part of seconds does not
            // crash gameplay (at least that i've found during limited
            // testing).  Setting fractional part of seconds, prevents
            // full seconds from ever increasing.
            // 
            // NEEDS FURTHER TESTING.
            //    Only tested on one round of downhill, competitive,
            //    snowboard.
            //    Did notice graphical glitch at end of round, where
            //    the value this sets is displayed on screen as
            //    hexidecimal in pink/red instead of the correct last
            //    digit of fractional seconds.
            //    Also seems like the record times screen displayed
            //    glitched as well.  Showed time as 00:12:0 while
            //    others formatted as 00:53:60, 00:57:99.  Glitch is
            //    seconds field is populated in fashion don't understand
            //    and partial seconds field is missing a digit.
            bigpemu_jag_write8(VAL_SKI_TIME_LOOPING_ADDR, 13);

            // May be simpler to write 0 to the commented out addr write.
            // Found Training address week later, and writing 0 to seconds
            // address was less complicated for Training.
         }
      }

      if (recoverySettingValue > 0)
      {
         // Believe first byte is likely status of recovering, and
         // 2nd byte is time/ticks remaining before recover ends.
         bigpemu_jag_write16(VAL_SKI_RECOVER_TIME_ADDR, VAL_SKI_RECOVER_TIME_VALUE);
      }
   }
   return 0;
}

void bigp_init()
{
   void* pMod = bigpemu_get_module_handle();

   bigpemu_set_module_usage_flags(pMod, BIGPEMU_MODUSAGE_DETERMINISMWARNING);

   const int catHandle = bigpemu_register_setting_category(pMod, "Val Ski");
   sTimeSettingHandle = bigpemu_register_setting_bool(catHandle, "Infinite Time", 1);
   sRecoverySettingHandle = bigpemu_register_setting_bool(catHandle, "Invincible", 1);

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

   sTimeSettingHandle = INVALID;
   sRecoverySettingHandle = INVALID;
}