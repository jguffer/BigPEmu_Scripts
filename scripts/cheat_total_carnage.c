//__BIGPEMU_SCRIPT_MODULE__
//__BIGPEMU_META_DESC__    "Infinite lives, ammo, weapon upgrade, keys, password unlock, in Total Carnage"
//__BIGPEMU_META_AUTHOR__  "jguff"

#include "bigpcrt.h"

// Found using BigPEmu System->Load Software screen and highlighting ROM file.
#define CARNAGE_HASH             0xBFB6ACCFCB95E76EULL

// Found using Pro Action Replay like script.
#define CARNAGE_LIVES_ADDR       0x00022529
#define CARNAGE_BOMBS_ADDR       0x0002252B

#define CARNAGE_LIVES_VALUE      5
#define CARNAGE_BOMBS_VALUE      5

// Located via inspection, using Noesis Memory Window.
#define CARNAGE_WEAPON_ID_ADDR   0x0002252D
// Valid values are:
//    0 - machine gun
//    1 - plasma machine gun
//    2 - missile
//    3 - spray fire rifle
//    4 - red flame thrower
//    5 - blue flame thrower
//    6 - grenade launcher
#define CARNAGE_NUM_WEAPONS      7

// Located via inspection, using Noesis Memory Window.
// Not really anything useful here.  Pretty much what you
// see in end of mission statistics screens.  Identified
// these locations, in process of elimination to locate
// other more useful memory addresses.
#define CARNAGE_SCORE16_ADDR                 0x00022524
#define CARNAGE_NUM_PEOPLE_SAVED16_ADDR      0x00022526
#define CARNAGE_SHOTS_FIRED16_ADDR           0x00022540
#define CARNAGE_NUM_WEAPONS_PICKED_UP16_ADDR 0x00022546
#define CARNAGE_NUM_BOMBS_USED16_ADDR        0x00022548
#define CARNAGE_NUM_MISS_STRIKES16_ADDR      0x0002254A
#define CARNAGE_NUM_BIG_STUFF_BONUS16_ADDR   0x0002254C
#define CARNAGE_NUM_EXTRA_MEN_COLL16_ADDR    0x0002254E
#define CARNAGE_NUM_MINES_STEPPED_ON16_ADDR  0x00022550
#define CARNAGE_NUM_KEYS16_ADDR              0x00022552
#define CARNAGE_NUM_FLAGS16_ADDR             0x00022554
#define CARNAGE_NUM_GEMSTONES16_ADDR         0x00022556
#define CARNAGE_NUM_KEYS_VALUE               200

// Located this instruction address when searching for memory location which hold
// ammo-remaining-for-weapon-upgrade.
//
// Turns out the address changed over different missions or game sessions (unsure which).
//
// Had used Pro-Action Replay like descending counter value search script to locate
// address holding ammo-remaining-for-weapon-upgrade.
// When noticed the address changes for ammo-remaining-for-weapon-upgrade, placed a write
// breakpoint on ammo-remaining-for-weapon-upgrade address for session ammo-remaining-for-weapon-upgrade
// was located in.
// Noted a single 68000 instruction was decrementing the ammo-remaining-for-weapon-upgrade
// value by 1, and then returning from subroutine.
//
// For posterity, noted 0x001F1408 held ammo-remaining-for-weapon-upgrade, in one session, when
// memory was dumped during Mission2.  Noted 0x001F1458 held ammo-remaining-for-weapon-upgrade,
// in a different session, when memory was dumped during Mission1.
// Those addresses were referenced by instructions that applied 0x74 byte index offset to A4 register.
#define CARNAGE_AMMO_DECREMENT_INSTRUCTION_ADDR 0x00008CA2

// Located user warp code password via the following steps.
// 01. Start game.
// 02. Walk onto warp disc
// 03. Set CFEF as code for ENTER WARP CODE dialog, but do not close the dialog.
// 04. ASCII search for CFEF in Noesis Memory Window.
// 05. One hit only: 0x000221F8
#define CARNAGE_WARP_CODE_ENTRY_ADDR            0x000221F8

//--. Some/all warp codes documented at: https://forums.atariage.com/topic/221632-does-anyone-have-cheatspasswordscodes-for-total-carnage/

// Then went down a rabbit hole to locate the level state that the warp codes are applied to.
// 06. Restart game.
// 07. ASCII search for GOOB (a known working code), in Noesis Memory Window.
// 08. One hit only: 0x00024DDC
// 09. Search for ZULU.  ZULU is known working warp code that probably warps to warp point immediately following GOOB's warp point.
// 10. One hit only: 0x00024DE6
// 11. Search for DNAH.  DNAH is known working warp code, that probably warps to warp point immediately preceeding GOOB's warp point.  DNAH is also likely the beginning of game.
// 12. One hit only: 0x00024DD8.
// 13. Search for ORCS.  ORCS is known working warp code, that probably warps to two warp points immediately following GOOB's warp point.
// 14. One hit only: 0x00024DF0
// 15. Searched and bookmarked the remaining known working warp codes.
// 16. Analyzed in Noesis Memory Window.
// 17. Conclusions:
// 17a.	There are 17, and 17 only, warp codes (or 16 warp codes and the beginning-of-game code), as documented at atariage link.
// 17b.  There are 6 bytes following each ASCII warp code, with the exception of the first/beginning-of-game code.  The first code has no bytes between itself and the 2nd code.
// 17c.  Memory dump, cross referenced with atariage link codes
//          0x000024DD8:   0x44 0x4E 0x41 0x48                                   - DNAH- Landing In Desert
//          0x000024DDC:   0x47 0x4F 0x4F 0x42   0x00 0x02 0x4E 0xAF 0x00 0x01   - GOOB- Weasel Warp Bonus
//          0x000024DE6:   0x5A 0x55 0x4C 0x55   0x00 0x02 0x4E 0xA3 0x00 0x01   - ZULU- Incoming Transmission
//          0x000024DF0:   0x4F 0x52 0x43 0x53   0x00 0x02 0x4E 0xC7 0x00 0x01   - ORCS- Orcus Warp Bonus
//          0x000024DFA:   0x52 0x4F 0x41 0x44   0x00 0x02 0x4E 0xD1 0x00 0x00   - ROAD- All Smash TV Players Should Quit Now And Flee From This Machine
//          0x000024E04:   0x4C 0x49 0x50 0x53   0x00 0x02 0x4E 0xDE 0x00 0x00   - LIPS- Wipe Out General Akhboob's Air Power
//          0x000024E0E:   0x4C 0x49 0x43 0x4B   0x00 0x02 0x4E 0xDE 0x00 0x01   - LICK- Warping
//          0x000024E18:   0x46 0x49 0x52 0x45   0x00 0x02 0x4E 0xDE 0x00 0x03   - FIRE- Warping
//          0x000024E22:   0x53 0x48 0x4F 0x4B   0x00 0x02 0x4E 0xEA 0x00 0x01   - SHOK- You Have Been Captured! There Is No Escape! Submit To The Pain! Tap Fire To Escape
//          0x000024E2C:   0x46 0x4F 0x4F 0x44   0x00 0x02 0x4E 0xF5 0x00 0x00   - FOOD- You Have Survived This Far. I Am Impressed. Can You Last To See The Shocking Conclusion?
//          0x000024E36:   0x45 0x41 0x54 0x53   0x00 0x02 0x4E 0xA3 0x00 0x05   - EATS- Message From Headquarters
//          0x000024E40:   0x48 0x55 0x53 0x48   0x00 0x02 0x4F 0x0F 0x00 0x00   - HUSH- Destroy General Akhboob's Secret Nuclear Capability
//          0x000024E4A:   0x42 0x55 0x52 0x4E   0x00 0x02 0x4F 0x1B 0x00 0x00   - BURN- Taking Elevator Down To Basement
//          0x000024E54:   0x4E 0x41 0x5A 0x49   0x00 0x02 0x4F 0x28 0x00 0x00   - NAZI- Destroy General Akhboob's War Machines. Do Not Let Him Escape!
//          0x000024E5E:   0x44 0x4F 0x4D 0x45   0x00 0x02 0x4F 0x34 0x00 0x01   - DOME- Mission Complete: The Pleasure Domes Await.
//          0x000024E68:   0x57 0x4F 0x52 0x4D   0x00 0x02 0x4F 0x34 0x00 0x02   - WORM- Mission Complete: The Pleasure Domes Await.
//          0x000024E72:   0x41 0x5A 0x41 0x5A   0x00 0x02 0x4F 0x34 0x00 0x03   - AZAZ- Mission Complete: The Pleasure Domes Await.
//          0x000024E7C:   0xFF 
// 17d. Out of the 6 bytes, appears first 4 bytes are likely pointer to text filename and last 2 bytes are likely related to first argument.  Note pleasuredomes have values of 1, 2, 3.  Similar for the others.
//      The first 4 bytes are pointers to the following:
//          console.bin
//          desert1.bin.
//          desert2.bin.
//          orcus.bin.
//          roadeast.bin.
//          airport.bin.
//          echair.bin.
//          roadwest.bin.
//          babymilk.bin.
//          secpath.bin.
//          basement.bin.
//          bosspit.bin.
//          plesdome.bin.
//
// Search for desert1.bin:  0x00001F260, 0x000024EAF (above), 0x000025A4C
// Search for desert2.bin:  0x00001F26C, 0x0000247C2, 0x000024EBB (above)
// Search for orcus.bin:    0x00001F278, 0x000024EC7 (above), -----
// Search for roadeast.bin: 0x000014898, 0x00001F282, 0x000024ED1 (above)
// Search for airport.bin:  0x00001F290, 0x000024EDE(above), -----
// 1F260 addresses are repeat or superset repeat of filenames above.
//
// Seemed to be going down a rabbit hole, so opted to just auto fill in the
// warp codes for the users.
#define CARNAGE_NUM_WARP_CODES   17

const static uint8_t sWarpCodes[CARNAGE_NUM_WARP_CODES][4] = {
   "DNAH", // Landing In Desert
   "GOOB", // Weasel Warp Bonus
   "ZULU", // Incoming Transmission
   "ORCS", // Orcus Warp Bonus
   "ROAD", // All Smash TV Players Should Quit Now And Flee From This Machine
   "LIPS", // Wipe Out General Akhboob's Air Power
   "LICK", // Warping
   "FIRE", // Warping
   "SHOK", // You Have Been Captured! There Is No Escape! Submit To The Pain! Tap Fire To Escape
   "FOOD", // You Have Survived This Far. I Am Impressed. Can You Last To See The Shocking Conclusion?
   "EATS", // Message From Headquarters
   "HUSH", // Destroy General Akhboob's Secret Nuclear Capability
   "BURN", // Taking Elevator Down To Basement
   "NAZI", // Destroy General Akhboob's War Machines. Do Not Let Him Escape!
   "DOME", // Mission Complete: The Pleasure Domes Await.
   "WORM", // Mission Complete: The Pleasure Domes Await.
   "AZAZ"  // Mission Complete: The Pleasure Domes Await.
};

#define INVALID                  -1

// Boilerplate.
static int32_t sOnLoadEvent = INVALID;
static int32_t sOnEmuFrame = INVALID;
static int32_t sMcFlags = 0;
static const int32_t skMcFlag_Loaded = (1 << 0);

// Setting handles.
static int sLivesSettingHandle = INVALID;
static int sBombsSettingHandle = INVALID;
static int sWeaponIdSettingHandle = INVALID;
static int sAmmoSettingHandle = INVALID;
static int sWarpCodeSettingHandle = INVALID;
static int sKeysSettingHandle = INVALID;

static uint32_t on_sw_loaded(const int eventHandle, void* pEventData)
{
   const uint64_t hash = bigpemu_get_loaded_fnv1a64();

   sMcFlags = 0;

   if (CARNAGE_HASH == hash)
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
      int bombsSettingValue = INVALID;
      int weaponIdSettingValue = INVALID;
      int ammoSettingValue = INVALID;
      int warpCodeSettingValue = INVALID;
      int keysSettingValue = INVALID;
      bigpemu_get_setting_value(&livesSettingValue, sLivesSettingHandle);
      bigpemu_get_setting_value(&bombsSettingValue, sBombsSettingHandle);
      bigpemu_get_setting_value(&weaponIdSettingValue, sWeaponIdSettingHandle);
      bigpemu_get_setting_value(&ammoSettingValue, sAmmoSettingHandle);
      bigpemu_get_setting_value(&warpCodeSettingValue, sWarpCodeSettingHandle);
      bigpemu_get_setting_value(&keysSettingValue, sKeysSettingHandle);

      if (livesSettingValue > 0)
      {
         bigpemu_jag_write8(CARNAGE_LIVES_ADDR, CARNAGE_LIVES_VALUE);
      }

      if (bombsSettingValue > 0)
      {
         bigpemu_jag_write8(CARNAGE_BOMBS_ADDR, CARNAGE_BOMBS_VALUE);
      }

      if (weaponIdSettingValue > 0)
      {
         bigpemu_jag_write8(CARNAGE_WEAPON_ID_ADDR, weaponIdSettingValue);
      }

      if (ammoSettingValue > 0)
      {
         // Overwrite the instruction that decrements weapon ammo with NOPs.
         // There is some chance this could be a bad idea by itself, if
         // each time a weapon is picked up the ammo is increased, but these
         // nops prevent decreasing, and eventually an increment exceeds
         // max value for 1 byte?
         //
         // bigpemu_jag_sysmemwrite seems to successfully overwrite ROM memory.
         // At least with BigPEmu v 1.19.
         const uint8_t rtsInstruction[2] = {0x4E, 0x71};
         bigpemu_jag_sysmemwrite(rtsInstruction, CARNAGE_AMMO_DECREMENT_INSTRUCTION_ADDR + 0, 2);
         bigpemu_jag_sysmemwrite(rtsInstruction, CARNAGE_AMMO_DECREMENT_INSTRUCTION_ADDR + 2, 2);

         // TODO: If someone cares enough they could add an else condition, to
         //       this if statement, to restore the original instructions, so
         //       that this cheat can be toggled on/off without having to restart game.
      }

      if (warpCodeSettingValue > 1) // BigPEmu user setting is 1-based
      {
         // sWarpCodes is 0-based
         const uint32_t warpCode = (sWarpCodes[warpCodeSettingValue - 1][0] << 24) |
                                   (sWarpCodes[warpCodeSettingValue - 1][1] << 16) |
                                   (sWarpCodes[warpCodeSettingValue - 1][2] << 8) |
                                   (sWarpCodes[warpCodeSettingValue - 1][3]);
         bigpemu_jag_write32(CARNAGE_WARP_CODE_ENTRY_ADDR, warpCode);

         // CAVEAT/TODO: Overwriting this value causes some odd behavior on cut-scenes and transitions.
         //              Recommend disabling after you warp to where you want to go, and especially
         //              disable before the end of game scenes, or else end of game scenes flash by
         //              in probably less then a second.
      }

      if (keysSettingValue > 1)
      {
         bigpemu_jag_write32(CARNAGE_NUM_KEYS16_ADDR + 0, CARNAGE_NUM_KEYS_VALUE << 8);
         bigpemu_jag_write32(CARNAGE_NUM_KEYS16_ADDR + 1, CARNAGE_NUM_KEYS_VALUE & 0xFF);
      }
   }
   return 0;
}

void bigp_init()
{
   void* pMod = bigpemu_get_module_handle();

   bigpemu_set_module_usage_flags(pMod, BIGPEMU_MODUSAGE_DETERMINISMWARNING);

   const int catHandle = bigpemu_register_setting_category(pMod, "Total Carnage");
   sLivesSettingHandle = bigpemu_register_setting_bool(catHandle, "Infinite Lives", 1);
   sBombsSettingHandle = bigpemu_register_setting_bool(catHandle, "Infinite Bombs", 1);
   sWeaponIdSettingHandle = bigpemu_register_setting_int_full(catHandle, "Weapon Always Upgraded", INVALID, INVALID, CARNAGE_NUM_WEAPONS - 1, 1);
   sAmmoSettingHandle = bigpemu_register_setting_bool(catHandle, "Infinite Ammo", 1);
   sWarpCodeSettingHandle = bigpemu_register_setting_int_full(catHandle, "Auto Populate Warp Code (As A____)", INVALID, INVALID, CARNAGE_NUM_WARP_CODES, 1);
   sKeysSettingHandle = bigpemu_register_setting_bool(catHandle, "Keys For Best Ending", 1);

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
   sBombsSettingHandle = INVALID;
   sWeaponIdSettingHandle = INVALID;
   sAmmoSettingHandle = INVALID;
   sWarpCodeSettingHandle = INVALID;
   sKeysSettingHandle = INVALID;
}