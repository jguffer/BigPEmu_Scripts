//__BIGPEMU_SCRIPT_MODULE__
//__BIGPEMU_META_DESC__    "Infinite health and magic, in Towers II."
//__BIGPEMU_META_AUTHOR__  "jguff"

#include "bigpcrt.h"

// Found using BigPEmu System->Load Software screen and highlighting ROM file.
#define TOWERS2_HASH             0xA922F55DCAABCDFFULL

// Found using Pro Action Replay like script and Noesis Memory Window.
#define TOWERS2_MAGIC_ADDR       0x001ABAAC
#define TOWERS2_MAX_MAGIC_ADDR   0x001ABAAE
#define TOWERS2_HEALTH_ADDR      0x001ABAB0
#define TOWERS2_MAX_HEALTH_ADDR  0x001ABAB2

// Unsure what the max value for magic and health really is.
// Did verify that this 2 byte value decrements correctly
// (so really is two byte value, not one byte value).
// May need to increase this value, if a magic spell or a hit has a higher value.
#define TOWERS2_MAX_VALUE        0x0100

#define INVALID                  -1

// Boilerplate.
static int32_t sOnLoadEvent = INVALID;
static int32_t sOnEmuFrame = INVALID;
static int32_t sMcFlags = 0;
static const int32_t skMcFlag_Loaded = (1 << 0);

// Setting handles.
static int sMagicSettingHandle = INVALID;
static int sHealthSettingHandle = INVALID;

static uint32_t on_sw_loaded(const int eventHandle, void* pEventData)
{
   const uint64_t hash = bigpemu_get_loaded_fnv1a64();

   sMcFlags = 0;

   if (TOWERS2_HASH == hash)
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
      int magicSettingValue = INVALID;
      int healthSettingValue = INVALID;
      bigpemu_get_setting_value(&magicSettingValue, sMagicSettingHandle);
      bigpemu_get_setting_value(&healthSettingValue, sHealthSettingHandle);

      if (magicSettingValue > 0)
      {
         // Avoid 16 bit writes/assignments due to the 16-bit-writes-clobber-memory-bug.
         bigpemu_jag_write8(TOWERS2_MAGIC_ADDR + 0, (TOWERS2_MAX_VALUE >> 8));
         bigpemu_jag_write8(TOWERS2_MAGIC_ADDR + 1, (TOWERS2_MAX_VALUE & 0xFF));
         bigpemu_jag_write8(TOWERS2_MAX_MAGIC_ADDR + 0, (TOWERS2_MAX_VALUE >> 8));
         bigpemu_jag_write8(TOWERS2_MAX_MAGIC_ADDR + 1, (TOWERS2_MAX_VALUE & 0xFF));
      }

      if (healthSettingValue > 0)
      {
         bigpemu_jag_write8(TOWERS2_HEALTH_ADDR + 0, (TOWERS2_MAX_VALUE >> 8));
         bigpemu_jag_write8(TOWERS2_HEALTH_ADDR + 1, (TOWERS2_MAX_VALUE & 0xFF));
         bigpemu_jag_write8(TOWERS2_MAX_HEALTH_ADDR + 0, (TOWERS2_MAX_VALUE >> 8));
         bigpemu_jag_write8(TOWERS2_MAX_HEALTH_ADDR + 1, (TOWERS2_MAX_VALUE & 0xFF));
      }
   }
   return 0;
}

void bigp_init()
{
   void* pMod = bigpemu_get_module_handle();

   bigpemu_set_module_usage_flags(pMod, BIGPEMU_MODUSAGE_DETERMINISMWARNING);

   const int catHandle = bigpemu_register_setting_category(pMod, "Towers II");
   sHealthSettingHandle = bigpemu_register_setting_bool(catHandle, "Infinite Health", 1);
   sMagicSettingHandle = bigpemu_register_setting_bool(catHandle, "Infinite Magic", 1);

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

   sMagicSettingHandle = INVALID;
   sHealthSettingHandle = INVALID;
}