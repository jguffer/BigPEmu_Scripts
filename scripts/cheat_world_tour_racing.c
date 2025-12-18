//__BIGPEMU_SCRIPT_MODULE__
//__BIGPEMU_META_DESC__    "Always be deemed first place winner and password unlock, in World Tour Racing."
//__BIGPEMU_META_AUTHOR__  "jguff"

#include "bigpcrt.h"

// Found using BigPEmu System->Load Software screen and highlighting ROM file.
#define WTRACING_HASH            0xCF1DFEC53ADE6564ULL

#define WTRACING_POSITION_ADDR   0x0000804d
#define WTRACING_PASSWORD_ADDR   0x00007E2C
#define WTRACING_POSITION_VALUE  1

#define WTRACING_NUM_COURSES     15

// First place finish passwords.
const uint8_t nums[15][32] = {
   {0,0,0,0,0,0,0,0, 3,9,7,2,1,0,6,3, 3,7,8,8,9,8,1,2, 6,4,3,8,4,0,7,2}, //  #1 U.S.A.
   {0,0,0,0,0,0,0,0, 7,9,4,4,1,1,0,2, 4,0,1,5,1,1,0,9, 6,3,2,0,0,0,8,1}, //  #2 Hungary
   {0,0,0,0,0,0,0,1, 1,9,2,1,6,4,5,6, 3,0,7,8,0,3,9,4, 7,0,0,8,0,1,2,0}, //  #3 Germany
   {0,0,0,0,0,0,0,1, 5,8,9,9,1,6,3,8, 4,2,8,8,2,6,4,6, 8,3,5,2,0,1,6,8}, //  #4 Brazil
   {0,0,0,0,0,0,0,1, 9,8,7,6,6,9,9,2, 3,4,1,8,7,1,6,2, 4,1,9,2,0,2,0,0}, //  #5 San Marino
   {0,0,0,0,0,0,0,2, 2,9,8,0,1,5,6,9, 1,9,6,9,6,6,6,7, 4,4,3,2,0,2,4,4}, //  #6 Monaco
   {0,0,0,0,0,0,0,2, 6,9,5,7,7,2,6,4, 5,5,9,9,1,1,8,6, 2,2,7,2,0,3,1,2}, //  #7 Mexico
   {0,0,0,0,0,0,0,3, 0,9,2,9,7,6,4,5, 0,3,9,0,5,1,8,6, 6,1,1,2,0,3,2,5}, //  #8 Canada 
   {0,0,0,0,0,0,0,3, 4,8,9,6,3,3,9,3, 5,3,4,1,8,6,7,4, 9,9,5,2,0,3,9,6}, //  #9 France
   {0,0,0,0,0,0,0,3, 8,8,7,3,9,0,8,8, 8,9,7,0,9,0,2,5, 6,8,9,6,0,4,0,0}, // #10 England
   {0,0,0,0,0,0,0,4, 2,8,5,1,4,4,4,2, 8,0,3,3,8,3,1,0, 7,5,8,4,0,4,6,7}, // #11 Portugal
   {0,0,0,0,0,0,0,4, 6,8,1,8,0,1,8,4, 8,8,9,6,6,3,4,2, 2,4,6,4,0,4,8,7}, // #12 Italy
   {0,0,0,0,0,0,0,5, 0,8,0,6,5,4,8,7, 8,1,1,2,4,9,7,0, 0,8,6,4,0,5,5,6}, // #13 Egypt
   {0,0,0,0,0,0,0,5, 4,7,8,4,1,1,8,2, 0,8,7,1,1,5,1,7, 9,6,2,2,4,5,6,7}, // #14 Australia
   {0,0,0,0,0,0,0,5, 7,8,8,7,5,4,1,8, 5,4,5,8,7,7,9,4, 3,9,2,0,7,0,0,6}  // #15 Japan
};

#define INVALID                  -1

// Boilerplate.
static int32_t sOnLoadEvent = INVALID;
static int32_t sOnEmuFrame = INVALID;
static int32_t sMcFlags = 0;
static const int32_t skMcFlag_Loaded = (1 << 0);

static int sPositionSettingHandle = INVALID;
static int sCourseSettingHandle = INVALID;

static uint32_t on_sw_loaded(const int eventHandle, void* pEventData)
{
   const uint64_t hash = bigpemu_get_loaded_fnv1a64();

   sMcFlags = 0;

   if (WTRACING_HASH == hash)
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
      int positionSettingValue = INVALID;
      int courseSettingValue = INVALID;
      bigpemu_get_setting_value(&positionSettingValue, sPositionSettingHandle);
      bigpemu_get_setting_value(&courseSettingValue, sCourseSettingHandle);

      if (positionSettingValue > 0)
      {
         // Always be marked as position one (the winner), in spite
         // of your actual position or time.  Position is 0-based.
         bigpemu_jag_write8(WTRACING_POSITION_ADDR, WTRACING_POSITION_VALUE);
      }

      if (courseSettingValue > 0)
      {
         // Convert from 1-based courseSettingValue to 0-based courseIdx.
         const int courseIdx = courseSettingValue - 1;

         // Write the password to memory.
         // But only when the 10th digit of password memory has zero value.
         // Entering the Password Screen, zero's out the password.
         //
         // If this password is written every single emulation frame,
         // the game does not accept the password, reporting "BAD CODE"
         // instead.  Suspect the game is doing something like XORing
         // the password in place, then summing the in-place XOR results to
         // see if password is valid.  Did not take time to step through
         // assembly code to see if that is the case.
         //
         // Noticed the 10th digit, of all valid 15 passwords, never equals
         // zero.  So when the 10th digit goes to zero, assume it is safe
         // to write the password.
         //
         // This seems to work.  Really just conjecture as to why it works.
         // Don't be surprised if this stops working, or doesn't work for
         // someone else.  Surprised this does work.  ANDing in place
         // should have caused "BAD CODE", and XORing in place likely ought
         // to cause a problem even with the zero check on 10th digit.
         //
         // Update: Works for all courseIdx's other then 7 and 12.  Increment
         // testIdx by 1, and then works for courseIdxs of 7 and 12.
         // Whatever... Its a hack.  For a 30+ year old video game system that
         // few people played, and for a CD ROM unit that order of magnitude
         // fewer people played than people who played the base console.

         // Test 10th digit for most courses, and 11th digit for 2 of the courses.
         uint8_t testIdx = 9;
         if ((7 == courseIdx) || (12 == courseIdx))
         {
            ++testIdx;
         }

         // When test digit has zero value.
         const uint8_t testValue = bigpemu_jag_read8(WTRACING_PASSWORD_ADDR + testIdx);
         if (0 == testValue)
         {
            // Auto populate the password, in memory, for the user, based on user BigPEmu setting value.
            for (uint32_t pIdx = 0; pIdx < 32; ++pIdx)
            {
               bigpemu_jag_write8(WTRACING_PASSWORD_ADDR + pIdx, nums[courseIdx][pIdx]);
            }
         }
      }
   }
   return 0;
}

void bigp_init()
{
   void* pMod = bigpemu_get_module_handle();

   bigpemu_set_module_usage_flags(pMod, BIGPEMU_MODUSAGE_DETERMINISMWARNING);

   const int catHandle = bigpemu_register_setting_category(pMod, "World Tour Racing");
   sPositionSettingHandle = bigpemu_register_setting_bool(catHandle, "Always Finish First", 1);
   sCourseSettingHandle = bigpemu_register_setting_int_full(catHandle, "Populate Password For Course", INVALID, INVALID, WTRACING_NUM_COURSES, 1);

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

   sPositionSettingHandle = INVALID;
   sCourseSettingHandle = INVALID;
}