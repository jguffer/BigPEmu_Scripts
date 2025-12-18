//__BIGPEMU_SCRIPT_MODULE__
//__BIGPEMU_META_DESC__    "Limit number of shapes, in Zoop."
//__BIGPEMU_META_AUTHOR__  "jguff"

#include "bigpcrt.h"

// Found using BigPEmu System->Load Software screen and highlighting ROM file.
#define ZOOP_HASH             0x911E3C8231EE1D7AULL

// Found using Pro-Action Replay like script on memory dump.
#define ZOOP_SHAPES_ADDR      0x001718C9
#define ZOOP_MAX_NUM_SHAPES   200
 
#define INVALID               -1

// Boilerplate.
static int32_t sOnLoadEvent = INVALID;
static int32_t sOnEmuFrame = INVALID;
static int32_t sMcFlags = 0;
static const int32_t skMcFlag_Loaded = (1 << 0);

// Setting handles.
static int sShapesSettingHandle = INVALID;

static uint32_t on_sw_loaded(const int eventHandle, void* pEventData)
{
   const uint64_t hash = bigpemu_get_loaded_fnv1a64();

   sMcFlags = 0;

   if (ZOOP_HASH == hash)
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
      int shapesSettingValue = INVALID;
      bigpemu_get_setting_value(&shapesSettingValue, sShapesSettingHandle);

      if (shapesSettingValue > 0)
      {
         // Only write the BigPEmu user setting value, when BigPEmu
         // user setting value is less then what the game thinks
         // is the number of shapes remaining.
         const uint8_t numShapes = bigpemu_jag_read8(ZOOP_SHAPES_ADDR);
         if (numShapes > shapesSettingValue)
         {
            bigpemu_jag_write8(ZOOP_SHAPES_ADDR, shapesSettingValue);
         }
      }
   }
   return 0;
}

void bigp_init()
{
   void* pMod = bigpemu_get_module_handle();

   bigpemu_set_module_usage_flags(pMod, BIGPEMU_MODUSAGE_DETERMINISMWARNING);

   const int catHandle = bigpemu_register_setting_category(pMod, "Zoop");
   sShapesSettingHandle = bigpemu_register_setting_int_full(catHandle, "Num Shapes To Remove", INVALID, INVALID, ZOOP_MAX_NUM_SHAPES, 1);

   // NOTE1: Considered adding setting/logic to allow user to specify
   // level to skip to but don't see much point in that candidate setting:
   //	1) The game apparently has no end level.  You can beat levels after
   //	   Level99, but the screen display will not show a level higher then
   //	   99.
   //	2) After the 10th or 11th level, the background remains the same for
   //    any subsequent levels.
   //	3) Game already provides option to skip to 10th level.

   // NOTE2: For anyone whom cares, it appears the number of shapes to remove,
   //        as specified by game, is as follows:
   //          20
   //          50
   //          90
   //          100
   //          110
   //          120
   //          140
   //          160
   //          180
   //          200
   //          200
   //        Seems to keep repeating 200 for each subsequent level, though 
   //        didn't look to far beyond Level10.

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

   sShapesSettingHandle = INVALID;
}