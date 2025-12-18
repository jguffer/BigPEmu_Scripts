//__BIGPEMU_SCRIPT_MODULE__
//__BIGPEMU_META_DESC__    "Infinite lives, shield, health, ammo, and unlock missions/levels, in Hoverstrike Cartridge."
//__BIGPEMU_META_AUTHOR__  "jguff"

#include "bigpcrt.h"

// Found using BigPEmu System->Load Software screen and highlighting ROM file.
#define HSCART_SUPPORTED_HASH          0x67C944EE0406FAD9ULL

// Levels are composed of 6 missions and one secret mission.
// There are 5 levels.
#define HSCART_NUM_LEVELS              5

// Located using GAMEROM.DAT source file as reference, Noesis Memory Window Find capability, and then counting bytes.
// According to GAMEROM.DAT different variables for different difficulty levels, but NORMAL difficulty was only one that
// showed up in main RAM.
// GAMEROM.DAT declares similar variables for max missiles, mortars, flares with -1 as unlimited, but doesn't seem to function.
#define HSCART_MAX_HEALTH_ADDR         0x001D79F0
#define HSCART_MAX_SHIELD_ADDR         0x001D79FA
// Value is -1 for unlimited.
#define HSCART_HEALTH_VALUE            0xFFFF
#define HSCART_SHIELD_VALUE            0xFFFF

// Found using Pro Action Replay like scripts on memory dump from Noesis.
#define HSCART_LIVES_ADDR              0x001D5DAD
#define HSCART_REGULAR_MISSILES_ADDRA  0x0008F70D
#define HSCART_REGULAR_MISSILES_ADDRB  0x0008F70F
#define HSCART_GUIDED_MISSILES_ADDRA   0x0008F713
#define HSCART_GUIDED_MISSILES_ADDRB   0x0008F715
#define HSCART_MORTARS_ADDRA           0x0008F719
#define HSCART_MORTARS_ADDRB           0x0008F71B
#define HSCART_SECRET_MISSION_ADDR     0x000804AE
#define HSCART_LIVES_VALUE             7
#define HSCART_AMMO_VALUE              7
#define HSCART_SECRET_MISSION_VALUE    1

// 01. Located by searching through a revision of source code for "level", resulting in the following hit in BSSDATA.S:
//         gl_level:
//             .ds.w 1  ; ranges from 1 - 5
// 02. Noticed the following directly above _gl_level
//         CurInfoPtr: ; ptr to the current MAP INFO selected on the map screen.
//             .ds.l 1  ; Used for printing out data. Cleared when info is printed.
// 03. Started game and entered Mission Select Screen.
// 04. Used Noesis Memory Window Find Capability to locate <Mission Name> text from Mission Select Screen.
// 05. Located reference to <Mission Name> in a revision of source code, pointed at by wtext540.
// 06. Used Noesis Memory Window Find Capability to locate wtext540.
// 07. Located winfo54 pointing to wtext540, in that revision of source code.
// 08. Used Noesis Memory Window Find Capability to locate wtext54.
// 09. Used Noesis Memory Window Find Capability to locate _CurInfoPtr, which points to wtext54.
// 10. Used Noesis Memory Window to locate _gl_level 2 bytes to right of _CurInfoPtr.
#define HSCART_LEVEL_ADDR              0x00145FD8

// Currently unused, but in future might use to mark individual missions completed.
#define HSCART_LIKELY_MAP_STATUS_ARRAY 0x001D5D60

#define INVALID                        -1

// Boilerplate.
static int32_t sOnLoadEvent = INVALID;
static int32_t sOnEmuFrame = INVALID;
static int32_t sMcFlags = 0;
static const int32_t skMcFlag_Loaded = (1 << 0);

// Setting handles.
static int sLivesSettingHandle = INVALID;
static int sShieldSettingHandle = INVALID;
static int sHealthSettingHandle = INVALID;
static int sAmmoSettingHandle = INVALID;
static int sLevelSettingHandle = INVALID;
static int sSecretMissionSettingHandle = INVALID;

static uint32_t on_sw_loaded(const int eventHandle, void* pEventData)
{
   const uint64_t hash = bigpemu_get_loaded_fnv1a64();

   sMcFlags = 0;

   if (HSCART_SUPPORTED_HASH == hash)
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
      int shieldSettingValue = INVALID;
      int healthSettingValue = INVALID;
      int ammoSettingValue = INVALID;
      int levelSettingValue = INVALID;
      int secretMissionSettingValue = INVALID;
      bigpemu_get_setting_value(&livesSettingValue, sLivesSettingHandle);
      bigpemu_get_setting_value(&shieldSettingValue, sShieldSettingHandle);
      bigpemu_get_setting_value(&healthSettingValue, sHealthSettingHandle);
      bigpemu_get_setting_value(&ammoSettingValue, sAmmoSettingHandle);
      bigpemu_get_setting_value(&levelSettingValue, sLevelSettingHandle);
      bigpemu_get_setting_value(&secretMissionSettingValue, sSecretMissionSettingHandle);

      if (livesSettingValue > 0)
      {
         bigpemu_jag_write8(HSCART_LIVES_ADDR, HSCART_LIVES_VALUE);
      }

      if (shieldSettingValue > 0)
      {
         // (Don't trust 16 bit writes of any sort, anymore in BigPEmu).
         bigpemu_jag_write8(HSCART_MAX_SHIELD_ADDR, HSCART_SHIELD_VALUE >> 8);
      bigpemu_jag_write8(HSCART_MAX_SHIELD_ADDR+1, HSCART_SHIELD_VALUE & 0xFF);
      }
      if (healthSettingValue > 0)
      {
         // (Don't trust 16 bit writes of any sort, anymore in BigPEmu).
         bigpemu_jag_write8(HSCART_MAX_HEALTH_ADDR, HSCART_HEALTH_VALUE >> 8);
         bigpemu_jag_write8(HSCART_MAX_HEALTH_ADDR+1, HSCART_HEALTH_VALUE & 0xFF);
      }

      if (ammoSettingValue > 0)
      {
         bigpemu_jag_write8(HSCART_REGULAR_MISSILES_ADDRA, HSCART_AMMO_VALUE);
         bigpemu_jag_write8(HSCART_REGULAR_MISSILES_ADDRB, HSCART_AMMO_VALUE);
         bigpemu_jag_write8(HSCART_GUIDED_MISSILES_ADDRA, HSCART_AMMO_VALUE);
         bigpemu_jag_write8(HSCART_GUIDED_MISSILES_ADDRB, HSCART_AMMO_VALUE);
         bigpemu_jag_write8(HSCART_MORTARS_ADDRA, HSCART_AMMO_VALUE);
         bigpemu_jag_write8(HSCART_MORTARS_ADDRB, HSCART_AMMO_VALUE);
      }

      if (levelSettingValue > 0)
      {
         // Values 1 through 5 are valid.
         // 16-bit value, so write as 2 8-bit values to avoid any BigPEmu-16-bit-write-bugs.
         bigpemu_jag_write8(HSCART_LEVEL_ADDR, 0);
         bigpemu_jag_write8(HSCART_LEVEL_ADDR + 1, levelSettingValue);
         //
         // MAP_S.S performs the following, in a revision of the source code.
         // Believe below is run by the joypad controller level skip cheat, so that only the missions within a level are available to select.
         // Currently choosing not to clear the levels when use this BigPEmu script cheat.
         // Its a bit ugly not to clear the prior levels missions, but if it doesn't hurt anything, then why bother...
         // Also don't want to spend time testing clearing the missions for skipped levels.
         // Also unsure if production version of game lets you go back and complete prior level's missions, which this script will allow.
         //       cmp.w #MAX_GAME_LEVELS,_gl_level    ; if we're already at the max level, don't clear them 
         //       beq.w .not_max_level
         //
         //       move.w _gl_level,-(sp)              ; Clear all zones for this level
         //       jsr _ClearLevel
         //       addq #2,sp
         // ***Need to test that this works.  If it doesn't work, likely need to modify
         //    MapStatusArray declared in MAP_S.S.
      }

      if (secretMissionSettingValue > 0)
      {
         bigpemu_jag_write8(HSCART_SECRET_MISSION_ADDR, 0);
         bigpemu_jag_write8(HSCART_SECRET_MISSION_ADDR + 1, HSCART_SECRET_MISSION_VALUE);
      }
   }
   return 0;
}

void bigp_init()
{
   void* pMod = bigpemu_get_module_handle();

   bigpemu_set_module_usage_flags(pMod, BIGPEMU_MODUSAGE_DETERMINISMWARNING);

   // Register cheat settings with BigPEmu GUI interface.
   const int catHandle = bigpemu_register_setting_category(pMod, "Hoverstrike Cart");
   sLivesSettingHandle = bigpemu_register_setting_bool(catHandle, "Infinite Lives", 1);
   sShieldSettingHandle = bigpemu_register_setting_bool(catHandle, "Infinite Shield", 1);
   sHealthSettingHandle = bigpemu_register_setting_bool(catHandle, "Infinite Health", 1);
   sAmmoSettingHandle = bigpemu_register_setting_bool(catHandle, "Infinite Ammo", 1);
   sLevelSettingHandle = bigpemu_register_setting_int_full(catHandle, "Select Level", 0, 0, HSCART_NUM_LEVELS, 1);
   sSecretMissionSettingHandle = bigpemu_register_setting_bool(catHandle, "Unlock Secret Mission", 1);

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
   sShieldSettingHandle = INVALID;
   sHealthSettingHandle = INVALID;
   sAmmoSettingHandle = INVALID;
   sLevelSettingHandle = INVALID;
   sSecretMissionSettingHandle = INVALID;
}