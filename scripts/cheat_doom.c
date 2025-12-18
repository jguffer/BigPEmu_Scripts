//__BIGPEMU_SCRIPT_MODULE__
//__BIGPEMU_META_DESC__    "Infinite health, ammo and unlocked weapons, cards, powers, levels, in Doom"
//__BIGPEMU_META_AUTHOR__  "jguff"

#include "bigpcrt.h"

// There are some cheat flags defined in source code.  Want to override the
// source code cheat flags to increase fidelity of cheats (e.g. unlock one
// weapon, but not unlock another weapon, or unlock weapons but not cards).

// Found using BigPEmu System->Load Software screen and highlighting ROM file.
#define DOOM_HASH             0x75F2216BD4F54AC2ULL

// Found using Pro Action Replay like counter search script.  Script came up
// with 0x000550E3 as well, which is likely the old value, as writing to
// 0x000550E3 is immediately overwritten by contents of 0x0004DA43.
//
// Source code and Noesis Memory Window confirms this is 4 byte wide int.
#define DOOM_AMMO_ADDR              0x0004DA40
#define DOOM_AMMO_VALUE             555

// Found in source code.
#define DOOM_NUM_WEAPONS            8
#define DOOM_NUM_TYPES_AMMO         4
#define DOOM_NUM_CARDS              6
#define DOOM_NUM_POWERS             4

// Source code and Noesis confirm that all relevant data types are 4 bytes wide.
#define DOOM_DATA_WIDTH             4

// Found counting bytes, based on game source code, from address found above.
// Appears booleans are 4 bytes wide.
#define DOOM_WEAPONS_ENABLE_ADDR    0x0004DA20
#define DOOM_CARDS_ADDR             DOOM_WEAPONS_ENABLE_ADDR - ((5 + DOOM_NUM_CARDS) * DOOM_DATA_WIDTH)
#define DOOM_POWERS_ADDR            DOOM_CARDS_ADDR - (DOOM_NUM_POWERS * DOOM_DATA_WIDTH)
#define DOOM_ENABLE_ITEM_VALUE      1
#define DOOM_ENABLE_POWER_VALUE     11
#define DOOM_STRENGH_VALUE          65

#define DOOM_SELECTED_WEAPON_ADDR   0x0004DA1B

#define DOOM_GAME_ACTION_ADDR       DOOM_POWERS_ADDR - (12*4) - (2*4) - (5*4)
#define DOOM_GAME_MAP_ADDR          DOOM_GAME_ACTION_ADDR + (2*4)


// Address of value which controls which levels are available to user from the Game-Mode/Area/Difficulty Screen.
//
// Found by looking for source code that handles user selecting Game Mode, Area, and Difficulty.
// Encountered maxlevel declaration in d_main.c:
//    int maxlevel;	/* highest level selectable in menu (1-25) */
//
// Search for maxlevel leads to jagonly.c:
//    maxlevel = eeprombuffer[6];
//    ...
//    void WriteEEProm (void)
//    {
//       eeprombuffer[0] = IDWORD;
//       eeprombuffer[1] = startskill;
//       eeprombuffer[2] = startmap;
//       eeprombuffer[3] = sfxvolume;
//       eeprombuffer[4] = musicvolume;
//       eeprombuffer[5] = controltype;
//       eeprombuffer[6] = maxlevel;
//
// WriteEEProm appears to contiguously write default difficulty, default area, last selected sound effects volume, last selected music volume, something else, and maxlevel.
// Opened EEProm file and see one meaningful block of 16 bytes written:
//    31 44 02 00 01 00 C8 00 80 00 00 00 01 00 B6 75
// (Remember EEProm contents are stored in byte-swapped fashion.).
// Default seleted difficulty = 2, default area = 1, sfx, music, control type, maxlevel=1
//
// Don't want to overwrite EEProm contents, so search for memory location EEProm value is copied to.
//
// Used Noesis Memory Window Find capability to search memory for 0x00 0x02 0x00 0x01 0x00 0xC8 0x00 0x80 0x00 0x00 0x00 0x01
// One hit only: 0x00042ACA
// Placed a read breakpoint on 0x00042AD5, the byte for maxlevel.
// Soft reboot.
// Breakpoint hit at the following instructions:
//     move.w  $42ad4.l, D0
//     move.l  D0, $47d34.l
// and there is the address used by the Game-Mode/Area/Difficulty Screen.
//
#define DOOM_MAX_LEVEL_ADDR   0x00047D37
#define DOOM_NEXT_MAP_ADDR    0x00047D
#define DOOM_NUM_LEVELS       24

#define INVALID               -1

// Boilerplate.
static int32_t sOnLoadEvent = INVALID;
static int32_t sOnEmuFrame = INVALID;
static int32_t sMcFlags = 0;
static const int32_t skMcFlag_Loaded = (1 << 0);

// Setting handles.
static int sAmmoSettingHandle = INVALID;
static int sLevelsSettingHandle = INVALID;
static int sWeaponsSettingHandle = INVALID;
static int sCardsSettingHandle = INVALID;
static int sInvulnerableSettingHandle = INVALID;
static int sIronFeetSettingHandle = INVALID;
static int sStrengthSettingHandle = INVALID;
static int sMapSettingHandle = INVALID;

static uint32_t on_sw_loaded(const int eventHandle, void* pEventData)
{
   const uint64_t hash = bigpemu_get_loaded_fnv1a64();

   sMcFlags = 0;

   if (DOOM_HASH == hash)
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
      int ammoSettingValue = INVALID;
      int levelsSettingValue = INVALID;
      int weaponsSettingValue = INVALID;
      int cardsSettingValue = INVALID;
      int invulnerableSettingValue = INVALID;
      int ironFeetSettingValue = INVALID;
      int strengthSettingValue = INVALID;
      int mapSettingValue = INVALID;
      bigpemu_get_setting_value(&ammoSettingValue, sAmmoSettingHandle);
      bigpemu_get_setting_value(&levelsSettingValue, sLevelsSettingHandle);
      bigpemu_get_setting_value(&weaponsSettingValue, sWeaponsSettingHandle);
      bigpemu_get_setting_value(&cardsSettingValue, sCardsSettingHandle);
      bigpemu_get_setting_value(&invulnerableSettingValue, sInvulnerableSettingHandle);
      bigpemu_get_setting_value(&ironFeetSettingValue, sIronFeetSettingHandle);
      bigpemu_get_setting_value(&strengthSettingValue, sStrengthSettingHandle);
      bigpemu_get_setting_value(&mapSettingValue, sMapSettingHandle);

      if (ammoSettingValue > 0)
      {
         for (uint32_t ammoTypeIdx = 0; ammoTypeIdx < DOOM_NUM_TYPES_AMMO; ++ammoTypeIdx)
         {
            bigpemu_jag_write32(DOOM_AMMO_ADDR + (ammoTypeIdx * DOOM_DATA_WIDTH), DOOM_AMMO_VALUE);
         }
      }

      if (levelsSettingValue > 0)
      {
         // Unlock all levels.
         //
         // This gets saved to EEProm.
         // Oh well.  Doesn't really destroy anything important.
         bigpemu_jag_write8(DOOM_MAX_LEVEL_ADDR, ((uint8_t) DOOM_NUM_LEVELS));
      }

      if (weaponsSettingValue > 0)
      {
         // Enable all weapons, skipping first two (fist and pistol) which are already enabled.
         for (uint32_t weaponIdx = 2; weaponIdx < DOOM_NUM_WEAPONS; ++weaponIdx)
         {
            bigpemu_jag_write32(DOOM_WEAPONS_ENABLE_ADDR + (weaponIdx * DOOM_DATA_WIDTH), DOOM_ENABLE_ITEM_VALUE);
         }
      }

      if (cardsSettingValue > 0)
      {
         // Enable all cards.
         for (uint32_t cardIdx = 0; cardIdx < DOOM_NUM_CARDS; ++cardIdx)
         {
            bigpemu_jag_write32(DOOM_CARDS_ADDR + (cardIdx * DOOM_DATA_WIDTH), DOOM_ENABLE_ITEM_VALUE);
         }
      }

      // Enable invulnerability and ironfeet.
      //
      // From source code looks like want: (value <=60 && (! (value & 4)))
      // or else view has non-standard filtering/shading applied.
      // Source code refers to tic counter, so presume this value if
      // number of frames/similar, and not damage points.
      // Believe non-zero value here, short circuits damagecount to
      // not be impacted.  damagecount is seperate from health.
      if (invulnerableSettingValue > 0)
      {
         bigpemu_jag_write32(DOOM_POWERS_ADDR, DOOM_ENABLE_POWER_VALUE);
      }
      if (ironFeetSettingValue > 0)
      {
         bigpemu_jag_write32(DOOM_POWERS_ADDR + 8, DOOM_ENABLE_POWER_VALUE);
      }

      if (strengthSettingValue > 0)
      {
         // Enable Strength power.
         //
         // From source looks like want: value >= 64
         // or else view has non-standard filtering/shading applied.
         bigpemu_jag_write32(DOOM_POWERS_ADDR + 4, DOOM_STRENGH_VALUE);
      }

      if (mapSettingValue > 0)
      {
         // Enable Full Map
         //
         // Just a boolean
         bigpemu_jag_write32(DOOM_POWERS_ADDR + 12, DOOM_ENABLE_ITEM_VALUE);
      }
   }
   return 0;
}

void bigp_init()
{
   void* pMod = bigpemu_get_module_handle();

   bigpemu_set_module_usage_flags(pMod, BIGPEMU_MODUSAGE_DETERMINISMWARNING);

   // Register cheat settings with BigPEmu GUI interface.
   const int catHandle = bigpemu_register_setting_category(pMod, "Doom");
   sAmmoSettingHandle = bigpemu_register_setting_bool(catHandle, "Infinite Ammo", 1);
   sLevelsSettingHandle = bigpemu_register_setting_bool(catHandle, "Unlock All Levels", 1);
   sWeaponsSettingHandle = bigpemu_register_setting_bool(catHandle, "Unlock All Weapons", 1);
   sCardsSettingHandle = bigpemu_register_setting_bool(catHandle, "Unlock All Cards", 1);
   sInvulnerableSettingHandle = bigpemu_register_setting_bool(catHandle, "Invulnerable", 1);
   sIronFeetSettingHandle = bigpemu_register_setting_bool(catHandle, "Iron Feet", 1);
   sStrengthSettingHandle = bigpemu_register_setting_bool(catHandle, "Strength", 1);
   sMapSettingHandle = bigpemu_register_setting_bool(catHandle, "Full Map", 1);

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

   sAmmoSettingHandle = INVALID;
   sLevelsSettingHandle = INVALID;
   sWeaponsSettingHandle = INVALID;
   sCardsSettingHandle = INVALID;
   sInvulnerableSettingHandle = INVALID;
   sIronFeetSettingHandle = INVALID;
   sStrengthSettingHandle = INVALID;
   sMapSettingHandle = INVALID;
}