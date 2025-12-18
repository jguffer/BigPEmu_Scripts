//__BIGPEMU_SCRIPT_MODULE__
//__BIGPEMU_META_DESC__    "Set difficulty, in Vid Grid."
//__BIGPEMU_META_AUTHOR__  "jguff"

#include "bigpcrt.h"

// Found using BigPEmu System->Load Software screen and highlighting ROM file.
#define VGRID_HASH                     0xFB286CFF629AC4AEULL

// Found using a Pro Action Replay like counter value search script, expecting them to represent
// difficulty level.  After deleting Memory Track file, the addresses had seemingly moved.
// In hind sight, may have switched which Save Slot was being used.
//      0x001FFE51
//      0x001FFE55

// Switched to memory inspection, using Noesis Memory Window, near above addresses.
// Via memory inspection, located all of the addresses which follow.

// The 5 slot locations.
#define VGRID_SLOT1_NAME_ADDR          0x001FFDD4
#define VGRID_SLOT2_NAME_ADDR          0x001FFE3C
#define VGRID_SLOT_LENGTH              (VGRID_SLOT2_NAME_ADDR - VGRID_SLOT1_NAME_ADDR)
#define VGRID_SLOT1_ADDR               VGRID_SLOT1_NAME_ADDR
#define VGRID_SLOT2_ADDR               VGRID_SLOT1_ADDR + (1 * VGRID_SLOT_LENGTH)
#define VGRID_SLOT3_ADDR               VGRID_SLOT1_ADDR + (2 * VGRID_SLOT_LENGTH)
#define VGRID_SLOT4_ADDR               VGRID_SLOT1_ADDR + (3 * VGRID_SLOT_LENGTH)
#define VGRID_SLOT5_ADDR               VGRID_SLOT1_ADDR + (4 * VGRID_SLOT_LENGTH)
#define VGRID_NUM_SLOTS                5

// Slot level attribute locations/offsets.
#define VGRID_NAME_OFFSET              0
#define VGRID_NAME_LENGTH              16
#define VGRID_SCORE_OFFSET             16
#define VGRID_DIFFICULTY_OFFSET        21
#define VGRID_NUM_VIDS_SOLVED_OFFSET   23
#define VGRID_VID_DATA_OFFSET          24

// The values for VGRID_DIFFICULTY
//     0 - LEVEL1
//     1 - LEVEL2
//     2 - LEVEL3
//     3 - LEVEL4
//     4 - LEVEL5
//     5 - End game applause video loop
//     6 - End game congratulations screen.
#define VGRID_NUM_LEVELS               7
#define VGRID_NUM_LEVELS_B             5

// Within each slot is a table of video data records, where each record is 8 bytes long.
// There is one data record per video, so 9 data records for Level1, Level2, Level3, and
// 10 data records for Level4 and Level5 (Level4 and Level5 contain the unlocked Nirvana video).
//
#define VGRID_VID_SCORE_OFFSET         6
#define VGRID_VID_RECORD_LENGTH        8
//
// Not quite certain about this.  Seemed to increase in value
// after solving a video, but not same amount for every video
// for level.  Also sometimes saw the 3rd byte, within the 8
// bytes, change value when solve a video, as well, which
// was not part of video score.
#define VGRID_VID_STATUS_OFFSET        1

#define INVALID                        -1

// Boilerplate.
static int32_t sOnLoadEvent = INVALID;
static int32_t sOnEmuFrame = INVALID;
static int32_t sMcFlags = 0;
static const int32_t skMcFlag_Loaded = (1 << 0);

// Setting handles.
static int sLevelSettingHandle = INVALID;
static int sInfoHandle1 = INVALID;
static int sInfoHandle2 = INVALID;

static uint32_t on_sw_loaded(const int eventHandle, void* pEventData)
{
   const uint64_t hash = bigpemu_get_loaded_fnv1a64();

   sMcFlags = 0;

   if (VGRID_HASH == hash)
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
      int levelSettingValue = INVALID;
      bigpemu_get_setting_value(&levelSettingValue, sLevelSettingHandle);

      if (levelSettingValue > 1) // BigPEmu user setting is base-1
      {
         // Caveat, performing this for Slot4 (slotIdx=3) or Slot5 (slotIdx=4) causes game to hang, on start up.
         //
         // Overwrite difficulty level, of the last slot possible.  Overwrite last, cause assume player
         // has already been saving/loading the first slots.
         //
         // Player can unlock the Nirvana video by setting difficulty level to 4 or 5.
         const uint32_t slotIdx = 2; // (Slot3 0-based)
         bigpemu_jag_write8( VGRID_SLOT1_ADDR + (slotIdx * VGRID_SLOT_LENGTH) + VGRID_DIFFICULTY_OFFSET, levelSettingValue-1);

         // Do not delete.  Commenting this out as example code.
         //
         // The following code will mark 8 videos solved, so that only 1 or 2 more videos need solved, before move to next difficulty level.
         // The write to VGRID_NUM_VIDS_SOLVED_OFFSET cannot be performed every emulation frame however,
         // if player wants to advance to next difficulty level, since VGRID_NUM_VIDS_SOLVED_OFFSET must equal
         // the number of videos in the difficulty level before moving to next difficulty level.  And overwriting
         // VGRID_NUM_VIDS_SOLVED_OFFSET every emulation frame, overwrites the game logic incrmeent of
         // VGRID_NUM_VIDS_SOLVED_OFFSET.  Player could start game with this logic uncommented, and then change
         // BigPEmu user setting to disable this code, and then game/player can transition to next difficulty
         // level.
         //
         // It isn't really important, either way.  Just documenting what has already been found...
         //
         //const uint32_t baseAddr = VGRID_SLOT1_ADDR + (slotIdx * VGRID_SLOT_LENGTH) + VGRID_VID_DATA_OFFSET;
         //const uint32_t numVideosSolved = 8;
         //for (uint32_t i = 0; i < (numVideosSolved); ++i)
         //{
         //   bigpemu_jag_write8(baseAddr + (i * VGRID_VID_RECORD_LENGTH) + VGRID_VID_STATUS_OFFSET, 1);
         //   bigpemu_jag_write8(baseAddr + (i * VGRID_VID_RECORD_LENGTH) + VGRID_VID_SCORE_OFFSET + 1, 200); // Just write to lower byte of score.
         //}
         //bigpemu_jag_write8( VGRID_SLOT1_ADDR + (slotIdx * VGRID_SLOT_LENGTH) + VGRID_NUM_VIDS_SOLVED_OFFSET, (uint8_t) numVideosSolved); // Just write to lower byte of score.
      }
   }
   return 0;
}

void bigp_init()
{
   void* pMod = bigpemu_get_module_handle();

   bigpemu_set_module_usage_flags(pMod, BIGPEMU_MODUSAGE_DETERMINISMWARNING);

   const int catHandle = bigpemu_register_setting_category(pMod, "Vid Grid");
   sLevelSettingHandle = bigpemu_register_setting_int_full(catHandle, "Slot3 Difficulty Level", INVALID, INVALID, VGRID_NUM_LEVELS_B, 1);

   // Describe joypad controller cheats to BigPEmu user, so don't have to reimplement the cheat in this script.
   sInfoHandle1 = bigpemu_register_setting_bool(catHandle, "Press 4, 7, 8 and B simultaneously to unscramble.", 1);
   sInfoHandle2 = bigpemu_register_setting_bool(catHandle, "Complete first 3 levels to unlock Nirvana.", 1);

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

   sLevelSettingHandle = INVALID;
   sInfoHandle1 = INVALID;
   sInfoHandle2 = INVALID;
}