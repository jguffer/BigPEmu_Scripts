//__BIGPEMU_SCRIPT_MODULE__
//__BIGPEMU_META_DESC__    "Infinite health, shield, ammo in Battlesphere."
//__BIGPEMU_META_AUTHOR__  "jguff"

#include "bigpcrt.h"

// Found using BigPEmu System->Load Software screen and highlighting ROM file.
#define BSPHERE_HASH    0x59785577BFBC435BULL

// Found using Pro Action Replay like counter match search script.
//
// Located while using Free-For-All game-mode.  None of the ships had
// CX2600_VIRUS_TORPEDO available.  Easy educated guess said where 
// CX2600_VIRUS_TORPEDO is in memory.
//
// Believe these are likely 4 byte, but going to address and write to
// as if single byte data.
//
// Weapon1 does not appear to be in memory at this location, probably
// because Weapon1 regenerates ammo, instead of only counting down.
#define BSPHERE_INFINITE_WEAPON2_ADDR  0x00166E4F
#define BSPHERE_INFINITE_WEAPON3_ADDR  0x00166E53
#define BSPHERE_INFINITE_WEAPON4_ADDR  0x00166E57
#define BSPHERE_INFINITE_WEAPON5_ADDR  0x00166E5B
#define BSPHERE_INFINITE_WEAPON6_ADDR  0x00166E5F
#define BSPHERE_WEAPON_VALUE           12

// Found using Pro Action Replay like decreasing search script.
#define BSPHERE_HEALTH_ADDR      0x00166D1C
#define BSPHERE_SHIELDS_ADDR     0x00166D28

// Commented out values were was taken from one ship, but when applied
// to a different ship, caused graphic glitches of shield and health
// meters extending too high on screen.
//#define BSPHERE_SHIELDS_VALUE  0x009AE600
//#define BSPHERE_HEALTH_VALUE   0x004A0000
#define BSPHERE_SHIELDS_VALUE    0x000FFFFF
#define BSPHERE_HEALTH_VALUE     0x000A0000

// Enumerate using the names of weapons, as specified by game user manual.
enum {STANDARD_LASER, PLASMA_TORPEDO, STASIS_BOLT, CX2600_VIRUS_TORPEDO, PROXIMITY_MINE, HOMING_MISSILE, NUM_WEAPONS} weaponsEnum;

#define INVALID                  -1

// Boilerplate.
static int32_t sOnLoadEvent = INVALID;
static int32_t sOnEmuFrame = INVALID;
static int32_t sMcFlags = 0;
static const int32_t skMcFlag_Loaded = (1 << 0);

// Setting handles.
static int sInfiniteWeapon2SettingHandle = INVALID;
static int sShieldSettingHandle = INVALID;
static int sHealthSettingHandle = INVALID;

static uint32_t on_sw_loaded(const int eventHandle, void* pEventData)
{
   const uint64_t hash = bigpemu_get_loaded_fnv1a64();

   sMcFlags = 0;

   if (BSPHERE_HASH == hash)
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
      int infiniteWeapon2SettingValue = INVALID;
      int shieldSettingValue = INVALID;
      int healthSettingValue = INVALID;
      bigpemu_get_setting_value(&infiniteWeapon2SettingValue, sInfiniteWeapon2SettingHandle);
      bigpemu_get_setting_value(&shieldSettingValue, sShieldSettingHandle);
      bigpemu_get_setting_value(&healthSettingValue, sHealthSettingHandle);

      if (infiniteWeapon2SettingValue > 0)
      {
         // This sets amount of ammo, and apparently if ammo is above 0, will unlock weapon for ship.
         // Don't know if there is any real unlock, but know when starting free-for-all game-mode,
         // some ships did not have all these weapons.  After enabling this script, all ships have all
         // these weapons.
         bigpemu_jag_write8(BSPHERE_INFINITE_WEAPON2_ADDR, BSPHERE_WEAPON_VALUE);
         bigpemu_jag_write8(BSPHERE_INFINITE_WEAPON3_ADDR, BSPHERE_WEAPON_VALUE);
         bigpemu_jag_write8(BSPHERE_INFINITE_WEAPON4_ADDR, BSPHERE_WEAPON_VALUE);
         bigpemu_jag_write8(BSPHERE_INFINITE_WEAPON5_ADDR, BSPHERE_WEAPON_VALUE);
         bigpemu_jag_write8(BSPHERE_INFINITE_WEAPON6_ADDR, BSPHERE_WEAPON_VALUE);
      }

      if (shieldSettingValue > 0)
      {
         bigpemu_jag_write32(BSPHERE_SHIELDS_ADDR, BSPHERE_SHIELDS_VALUE);
      }

      if (healthSettingValue > 0)
      {
         // This also takes care of health and weapon 1 regeneration/degeneration.
         bigpemu_jag_write32(BSPHERE_HEALTH_ADDR, BSPHERE_HEALTH_VALUE);
      }
   }
   return 0;
}

void bigp_init()
{
   void* pMod = bigpemu_get_module_handle();

   bigpemu_set_module_usage_flags(pMod, BIGPEMU_MODUSAGE_DETERMINISMWARNING);

   // Register cheat settings with BigPEmu GUI interface.
   const int catHandle = bigpemu_register_setting_category(pMod, "Battlesphere");
   sInfiniteWeapon2SettingHandle = bigpemu_register_setting_bool(catHandle, "Infinite Secondary Weapon Ammo", 1);
   sHealthSettingHandle = bigpemu_register_setting_bool(catHandle, "Infinite Health And Primary Weapon Ammo", 1);
   sShieldSettingHandle = bigpemu_register_setting_bool(catHandle, "Infinite Shield", 1);

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

   sInfiniteWeapon2SettingHandle = INVALID;
   sShieldSettingHandle = INVALID;
   sHealthSettingHandle = INVALID;
}