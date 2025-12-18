//__BIGPEMU_SCRIPT_MODULE__
//__BIGPEMU_META_DESC__    "Infinite lives, health, ammo, and level select, in Cybermorph."
//__BIGPEMU_META_AUTHOR__  "jguff"

#include "bigpcrt.h"

// Found using BigPEmu System->Load Software screen and highlighting ROM file.
#define CM_SUPPORTED_HASH     0x3F97A08E8550667CULL
#define CM_REVB_HASH          0xB16C980E3FB809BBULL

// Found using Pro Action Replay like scripts on memory dump from Noesis
#define CM_LIVES_ADDR         0x0009AFAA
#define CM_LIVES_VALUE        5

// Found via Noesis Memory Window, near the lives address.
#define CM_HEALTH_ADDR        0x0009AF78
#define CM_HEALTH_VALUE       0xC80

#define CM_AMMO_VALUE         5
#define CM_NUM_SUPER_WEAPONS  3

// Found by locating ply.lives in GAME.INC and getting offsets to ply.weapon ARRAY and plr.superwcnt ELEMENT
#define CM_SUPER_WPN_ADDR     0x0009AF98
#define CM_NUM_SUPER_WPN_ADDR 0x0009AF99
#define CM_NUM_DOUBLE_ADDR    0x0009AF9D
#define CM_NUM_TRISHOT_ADDR   0x0009AF9F
#define CM_NUM_CRUISE_ADDR    0x0009AFA1
#define CM_NUM_MINE_ADDR      0x0009AFA3
#define CM_NUM_FLAME_ADDR     0x0009AFA5

// Found using Noesis Memory Window Find capability to locate "1008" as auto populated by
// Select Destination Screen game logic.
#define CM_PASSWORD_ADDR      0x0000B712

// Address of cluster number.  Read breakpoint on CM_PASSWORD_ADDR, showed CM_CLUSTERNUM_ADDR
// being used to index into list of cluster passwords stored in ROM.  That indexed password
// was then copied to CM_PASSWORD_ADDR.
//    move.w  ($37a0,A6), D0
//    lsl.w   #2, D0
//    move.l  (A0,D0.w), (A1)+
// A6 + $37A0 is clusternum
// clusternum used as index into source passwords, contained within ROM, to destination pointer specified by A1.
// lsl is to index based on long word size data, instead of byte data.
// A1 held value of CM_PASSWORD_ADDR
// A6=0x00008000
// A6 + $37A0 = 0x0000B7A0
//
// Setting cluster number (0-5) value here, will auto populate password for clusters and display
// the appropriate planets/moons for the cluster.
//
// Source code file SCREENS.M68 explains most of this as well:
//    clusternum
//    nxtcluster
//    accesscodes
//    various lines of code
//
#define CM_CLUSTERNUM_ADDR 0x0000B7A0
#define CM_NUM_CLUSTERS    6

#define INVALID            -1

// Boilerplate.
static int32_t sOnLoadEvent = INVALID;
static int32_t sOnEmuFrame = INVALID;
static int32_t sMcFlags = 0;
static const int32_t skMcFlag_Loaded = (1 << 0);

// Setting handles.
static int sLivesSettingHandle = INVALID;
static int sHealthSettingHandle = INVALID;
static int sAmmoSettingHandle = INVALID;
static int sSuperWeaponSettingHandle = INVALID;
static int sClusterSettingHandle = INVALID;

// Found in SCREENS.M68 and documented in various cheat webpages.
static uint32_t const sPasswords[CM_NUM_CLUSTERS] = {
   0x31303038, //1008
   0x31333238, //1328
   0x39333235, //9325
   0x39323236, //9226
   0x33343434, //3444
   0x36303039  //6009
};

static uint32_t on_sw_loaded(const int eventHandle, void* pEventData)
{
   const uint64_t hash = bigpemu_get_loaded_fnv1a64();

   sMcFlags = 0;

   if (CM_SUPPORTED_HASH == hash)
   {
      sMcFlags |= skMcFlag_Loaded;
      printf_notify("This version fo cybermorph IS supported.\n");
   }
   else if (CM_REVB_HASH == hash)
   {
      printf_notify("This version of Cybermorph is NOT supported.\n");
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
      int healthSettingValue = INVALID;
      int ammoSettingValue = INVALID;
      int superWeaponSettingValue = INVALID;
      int clusterSettingValue = INVALID;
      bigpemu_get_setting_value(&livesSettingValue, sLivesSettingHandle);
      bigpemu_get_setting_value(&healthSettingValue, sHealthSettingHandle);
      bigpemu_get_setting_value(&ammoSettingValue, sAmmoSettingHandle);
      bigpemu_get_setting_value(&superWeaponSettingValue, sSuperWeaponSettingHandle);
      bigpemu_get_setting_value(&clusterSettingValue, sClusterSettingHandle);

      if (livesSettingValue > 0)
      {
         bigpemu_jag_write8(CM_LIVES_ADDR, CM_LIVES_VALUE);
      }

      if (healthSettingValue > 0)
      {
         bigpemu_jag_write16(CM_HEALTH_ADDR, CM_HEALTH_VALUE);
      }

      if (superWeaponSettingValue > 0)
      {
         // Set which superweapon is available.
         //    0x08: thrust
         //    0x09: destroy enemies
         //    0x10: destroy buildings
         // If this is written every emulation frame, but CM_NUM_SUPER_WPN is not
         // written every emulation frame, and you use superweapon, there is a glitch
         // in HUD.  HUD Glitch is the count is displayed in hex fashion, but
         // using ascii characters beyond A through F for the 10s digit.
         //
         // Thought saw source code setting up equates for values of 0, 1, 3, but
         // those equates were written in word format, not the 1 byte format obviously
         // used by game when inspected by Noesis Memory Window.
         //    TYPE_THUNDER   equ $0001
         //    ETYPE_QUAKE    equ $0002
         //    ETYPE_NORMAL   equ $0003         ;extra types
         // which seemingly contradicts:
         //    ELEMENT plr.superw,BYTE          ;superweapon flag
         //    ELEMENT plr.superwcnt,BYTE       ;superweapon count
         //
         // BigPEmu user setting value is 1 based.
         // As documented above in this comment block, game logic value is
         // 8-based, for a difference of 7.
         bigpemu_jag_write8(CM_SUPER_WPN_ADDR, superWeaponSettingValue + 7);
      }

      if (ammoSettingValue > 0)
      {
         bigpemu_jag_write8(CM_NUM_SUPER_WPN_ADDR, CM_AMMO_VALUE);
         bigpemu_jag_write8(CM_NUM_DOUBLE_ADDR, CM_AMMO_VALUE);
         bigpemu_jag_write8(CM_NUM_TRISHOT_ADDR, CM_AMMO_VALUE);
         bigpemu_jag_write8(CM_NUM_CRUISE_ADDR, CM_AMMO_VALUE);
         bigpemu_jag_write8(CM_NUM_MINE_ADDR, CM_AMMO_VALUE);
         bigpemu_jag_write8(CM_NUM_FLAME_ADDR, CM_AMMO_VALUE);
      }

      if (clusterSettingValue > 1)
      {
         // Write the clusternum, until the game logic reads the clusternum value
         // and populates the appropriate password onto Planet Selection Screen.
         // Once password is displayed, it is safe to stop writing clusternum.
         //
         // Write the value until the ordinary game logic has updated the password
         // displayed on screen to reflect this value.  Don't want this value
         // to be still written out, when user completes a cluster.
         //
         // This transition is untested, and likely someone else will need to test.
         // Don't have time to be playing an entire cluster...
         //
         // Subtract one from setting value, as setting value is 1-based, and game is 0-based.
         const uint32_t screenPassword = bigpemu_jag_read32(CM_PASSWORD_ADDR);
         if (screenPassword != sPasswords[clusterSettingValue-1])
         {
            bigpemu_jag_write16(0x0000B7A0, clusterSettingValue - 1);
         }

         // TODO
         // 1) May need to set nxtcluster for game to proceed beyond this cluster
         //    From SCREENS.M68
         //       VAR clusternum,WORD     ;cluster number
         //       VAR nxtcluster,WORD     ;next cluser if successful
         //       VAR clusteribase,DWORD  ;base of current cluster (info)
         //       VAR clusterfbase,DWORD  ;base of current cluster (flags)
         //
         //    Noesis Memory Window is not showing what would expect to see 2 bytes to right of
         //    clusternum.  Seeing 0x0000 at (CM_CLUSTERNUM_ADDR + 2)
         //    But that happens when not running script and happens when running script as well.
         // 2) Guessing someone could modify cluster info pointed/offseted by clusteribase/clusterfbase
         //    to mark individual planets done.  That state has to be somewhere, and that looks like
         //    a candidate location.
      }
   }
   return 0;
}

void bigp_init()
{
   void* pMod = bigpemu_get_module_handle();

   bigpemu_set_module_usage_flags(pMod, BIGPEMU_MODUSAGE_DETERMINISMWARNING);

   // Register cheat settings with BigPEmu GUI interface.
   const int catHandle = bigpemu_register_setting_category(pMod, "Cybermorph");
   sLivesSettingHandle = bigpemu_register_setting_bool(catHandle, "Infinite Lives", 1);
   sHealthSettingHandle = bigpemu_register_setting_bool(catHandle, "Infinite Health", 1);
   sAmmoSettingHandle = bigpemu_register_setting_bool(catHandle, "Infinite Ammo", 1);
   sSuperWeaponSettingHandle = bigpemu_register_setting_int_full(catHandle, "Always Use Superweapon", INVALID, INVALID, CM_NUM_SUPER_WEAPONS, 1);
   sClusterSettingHandle = bigpemu_register_setting_int_full(catHandle, "Select Cluster", INVALID, INVALID, CM_NUM_CLUSTERS, 1);

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
   sHealthSettingHandle = INVALID;
   sAmmoSettingHandle = INVALID;
   sSuperWeaponSettingHandle = INVALID;
   sClusterSettingHandle = INVALID;
}