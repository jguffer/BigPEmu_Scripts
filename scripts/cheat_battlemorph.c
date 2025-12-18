//__BIGPEMU_SCRIPT_MODULE__
//__BIGPEMU_META_DESC__    "Infinite lives, health, ammo, unlock weapons and clusters, in Battlemorph."
//__BIGPEMU_META_AUTHOR__  "jguff"

#include "bigpcrt.h"

// Found using BigPEmu System->Load Software screen and highlighting ROM file.
#define BMORPH_HASH        0x1AEFCB23C3BFBE19ULL

// Found using Pro Action Replay like script.
#define BMORPH_LIVES_ADDR  0x00068F79
#define BMORPH_LIVES_VALUE 5

// Located this nearby lives, via inspecting Noesis Memory Window.
#define BMORPH_HEALTH_ADDR    0x00068F8A 
#define BMORPH_HEALTH_VALUE   800

// Next went looking for ammo.
//
// Found by inspection, near health and lives addresses, using
// Noesis Memory Window.  Memory at ALT_ADDRs seems to be copy of
// memory at ADDRs.
// These addresses point to a copy of weapon ammo values.
// Witnessed the values decrementing in Noesis Memory Window, while
// firing shots in gameplay.
//
// Appears there is a weapon type/index, 2 bytes before ammo value.
//    cruise missile has value of 2
//    decoy has value of 4
// 
// Further analysis indicates this is likely where selected weapons
// are mapped to the HUD weaon bays and mapped to controller keypad
// inputs.
// And the weapon type/index is likely index into a table of weapon
// descriptors, described later.
//
// #define BMORPH_WEAPONA_TYPE_ADDR 0x00068EBD
// #define BMORPH_WEAPONA_ADDR      0x00068EBF
// #define BMORPH_WEAPONB_ADDR      0x00068EC3
// #define BMORPH_WEAPONC_ADDR      0x00068EC7
// #define BMORPH_WEAPOND_ADDR      0x00068ECB
// #define BMORPH_WEAPONA_ALT_ADDR  0x00068EE9
// #define BMORPH_WEAPONB_ALT_ADDR  0x00068EED
// #define BMORPH_WEAPONC_ALT_ADDR  0x00068EF2
// #define BMORPH_WEAPOND_ALT_ADDR  0x00068EF6

// Direct below need to be shifted left 4 bits to get ammo values.
//
// Found these addresses by placing Noesis breakpoint on
// BMORPH_WEAPONA_ADDR and analyzing disassembled 68K code,
// 68000 registers, and Memory Window.
//
// The weapon ammo numbers decrement as shots are fired during
// gameplay.  Writing to these values, updates the actual number
// of ammo available in gameplay.
//
// They are each 32 bytes apart.  Always in the same order, and
// always present, regardless of whether weapon type has been
// obtained in gameplay or not.
//
// Theorizing this is table of weapon descriptions.
// There could easily be 16+ bytes of info to describe a weapon:
// weapon index, max available, actual available, num frames on screen,
// trajectory on screen, length of explostion, amount of damage,
// etc, etc.  Just a theory though.
//
// Blah, blah, blah. Blah.
// 0x00057FEC is extremly likely the start of weapons data table
#define BMORPH_MINE_SRC_ADDR           0x00058026
#define BMORPH_MISSILE_SRC_ADDR        0x00058046
#define BMORPH_INCINERATOR_SRC_ADDR    0x00058066
#define BMORPH_DECOY_SRC_ADDR          0x00058086
#define BMORPH_SHIELD_SRC_ADDR         0x000580A6
#define BMORPH_FLASH_SRC_ADDR          0x000580C6
#define BMORPH_HAMMER_SRC_ADDR         0x000580E6
#define BMORPH_MORTAR_SRC_ADDR         0x00058106
#define BMORPH_QUAKE_SRC_ADDR          0x00058126
#define BMORPH_MINE_SRC_VALUE          20
#define BMORPH_MISSILE_SRC_VALUE       20
#define BMORPH_INCINERATOR_SRC_VALUE   20
#define BMORPH_DECOY_SRC_VALUE         20
#define BMORPH_SHIELD_SRC_VALUE        20
#define BMORPH_FLASH_SRC_VALUE         5
#define BMORPH_HAMMER_SRC_VALUE        20
#define BMORPH_MORTAR_SRC_VALUE        20
#define BMORPH_QUAKE_SRC_VALUE         4

// Next went looking to unlock weapons.
//
// Atari Age documents the following, but would like to have all weapons on first stage, and
// by just setting script settings:
//     All codes entered as new player name. You will be given all weapons and
//     magazines you would have earned up to that point (including secret weapons).
//     Note that on the load-game screen it will say "Zephyr Cluster" even after
//     successfully entering a code.
//         Zephyr     (none)
//         Carmine    ATDC2
//         Ferial     ATDC3
//         Pan Alma   ATDC4
//         Straussen  ATDC5
//         Pyroxine   ATDC6
//         Cygnus     ATDC7
//         Pernish    ATDC8 
//
// Concluded addresses above don't seem to have unlock information near them, by using
// Noesis Memory Window to inspect nearby contents.
//
// Noted that Memory Track saved-game-slots record unlock information.
//
// Then tryed searching for name of Memory Track saved-game-slot in Jaguar RAM, using Noesis Memory Window Find capability.
// Found 1 hit for each saved-game-slot, in Jaguar RAM, at the following addresses:
#define BMORPH_SAVE_SLOT0_ADDR   0x00067D0E
#define BMORPH_SAVE_SLOT1_ADDR   0x00067D5C
#define BMORPH_SAVE_SLOT2_ADDR   0x00067DAA
#define BMORPH_SAVE_SLOT3_ADDR   0x00067DF8
#define BMORPH_SAVE_SLOT4_ADDR   0x00067E46
#define BMORPH_SAVE_SLOT5_ADDR   0x00067E94
// Values at the above address blocks are saved to Memory Track, after user completes a mission and likely after
// user creates/replaces a game slot.
//
// Big takeaway: When one game slot needs saved, all game slots are saved.
//
// Overwriting the unlock bytes in all the above memory blocks and causing a save to happen, will overwrite
// all slots.

// This is where lock/unlock status of weapons is located, for the last save slot.
// Found by inspecting memory, in Noesis Memory Window, after locating SAVE_SLOT5_ADDR.
//
// There are 9 bytes with either value of 0x00 or 0x0C.  One byte per unlockable weapon.
// 0x00 is locked.  0x0C is unlocked.
// Bytes represent lock/unlock status of mine, missile, incinerator, decoy, shield, flash,
// hammer, mortar and quake, in that order.
#define BMORPH_SAVE_SLOT5_WEAPONS_AVAIL_ADDR 0x00067EBB

// Calculate offset, from start of save slot, to weapons available bytes.
#define BMORPH_WEAPONS_AVAIL_STATUS_OFFSET   BMORPH_SAVE_SLOT5_WEAPONS_AVAIL_ADDR - BMORPH_SAVE_SLOT5_ADDR

// Calculate length of a save slot (answer: 78 bytes).
#define BMORPH_SAVE_SLOT_LENGTH  BMORPH_SAVE_SLOT2_ADDR - BMORPH_SAVE_SLOT1_ADDR

#define BMORPH_NUM_SAVE_SLOTS       6

// The number of unlockable weapons.
#define BMORPH_NUM_WEAPONS          9

// Value of byte when weapon is available.
#define BMORPH_WEAPON_AVAIL_VALUE   0x0C

// Found location of cluster state by repeatedly performing joypad-controller-cluster-select-cheat
// on Slot4, dumping memory, and then using Pro-Action Replay like script.
#define BMORPH_SAVE_SLOT4_CLUSTER_ADDR 0x00067E81
#define BMORPH_CLUSTER_OFFSET          BMORPH_SAVE_SLOT4_CLUSTER_ADDR - BMORPH_SAVE_SLOT4_ADDR
#define BMORPH_NUM_CLUSTERS            8

// This address stores value (valid values of 0 through 5) which is used
// as index for user selected save slot.
//
// Found this by placing read breakpoint on SAVE_SLOT5_ADDR + WEAPONS_AVAIL_STATUS_OFFSET,
// and selecting the last save slot, in the Select Game Screen, and then using
// Noesis Disassembly Window to note that this value was being multiplied by
// value of SAVE_SLOT_LENGTH and added to value of SAVE_SLOT5_ADDR.
#define BMORPH_SELECTED_SAVE_SLOT_IDX_ADDR   0x00069671

//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//
// Goals for below:
//    1) Unlock all weapons, for any game slot.
//    2) Select cluster, for any game slot.
//    2) Limit unlocking of weapons/cluster to selected/played game slot.
//
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// Aproach:
//     Determine when user has entered Select Game Screen (as all game slots have been copied to Jaguar RAM at that point).
//     Read/store contents of all game slots, from Jaguar RAM.
//     Overwrite unlock bytes of all game slots, with Unlocked value.
//     Detect when user has committed to using game slot.
//     Restore orginal contents of all games slots to Jaguar RAM, other then the game slot user has committed to using.
//
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// Caveats:
//     Believe soft reset (pressing '*' and '#' simultaneously on control pad) will cause this unlocking
//     capability of this script to stop functioning.
//     Oh well.
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

// The game logic which reads this value will be used to indicate that user has entered
// Select Game Screen.  Verifed using Noesis breakpoints, that the game logic
// reads this address before copying game slot contents from Memory Track to Jaguar RAM.
// Found using Noesis Memory Window Find capability.
#define BMORPH_SELECT_GAME_STR_ADDR          0x001FF1C7

// The game logic which reads this value will be used to indicate that user has committed
// to using selected game slot, by entering the Select Planet Screen.
// Found using Noesis Memory Window Find capability.
#define BMORPH_SELECT_PLANET_STR_ADDR        0x001FF22F

// These will aid in determining when user has entered Select Game and Select Planet screens.
// Found using Noesis Disassembly Window, after placing read breakpoints on
// above strings and having player enter the screen.
// They are the same instruction, cause same routine is being called to do something with
// saved-game-slot name text string at beginning at save slot data structure.
#define BMORPH_PC_VALUE_WHEN_SELECT_GAME     0x00004F58
#define BMORPH_PC_VALUE_WHEN_SELECT_PLANET   0x00004F58

#define INVALID                              -1

// Boilerplate.
static int32_t sOnLoadEvent = INVALID;
static int32_t sOnEmuFrame = INVALID;
static int32_t sMcFlags = 0;
static const int32_t skMcFlag_Loaded = (1 << 0);

// Setting handles.
static int sLivesSettingHandle = INVALID;
static int sHealthSettingHandle = INVALID;
static int sAmmoSettingHandle = INVALID;
static int sWeaponsSettingHandle = INVALID;
static int sClusterSettingHandle = INVALID;

// Will be marked done, after user commits to using saved game slot.
static int sUnlockDone = 0;
static int sGameSlotsBackedUp = 0;

// This is where original, unedited save slots data (as copied from Memory Track to Jaguar RAM)
// will be stored for later restoration.
static uint8_t sSaveSlotsBackup[BMORPH_NUM_SAVE_SLOTS * BMORPH_SAVE_SLOT_LENGTH];

// Breakpoint that unlocks weapons.
static void bm_text_breakpoint_handler(const uint32_t addr)
{
   if ((!sGameSlotsBackedUp) || (!sUnlockDone))
   {
      const uint32_t a0Value = bigpemu_jag_m68k_get_areg(0);

      //TODO: probably should move bulk of this to on_emu_frame?, though it may be too late at that point.

      if (!sGameSlotsBackedUp)
      {
         if (BMORPH_SELECT_GAME_STR_ADDR == a0Value)
         {
            // Settings describing whether to unlock-all-weapons, skip-to-cluster.
            int weaponsSettingValue = INVALID;
            int clusterSettingValue = INVALID;
            bigpemu_get_setting_value(&weaponsSettingValue, sWeaponsSettingHandle);
            bigpemu_get_setting_value(&clusterSettingValue, sClusterSettingHandle);

            // Backup saved game slots.
            printf_notify("Backup all saved game slots.\n");
            bigpemu_jag_sysmemread(sSaveSlotsBackup, BMORPH_SAVE_SLOT0_ADDR, ((uint32_t) BMORPH_NUM_SAVE_SLOTS * BMORPH_SAVE_SLOT_LENGTH));
            sGameSlotsBackedUp = 1;

            // Unlock-all-weapons and/or skip-to-cluster, on all game slots.
            //
            // Do this to all game slots, as game logic appears to read the weapon
            // unlock state before user has selected which slot to resume playing.
            //
            // Other logic in this breakpoint handler will restore all but user
            // selected slot, after user selects slot.
            printf_notify("Overwrite weapons-unlock state in all game slots.\n");
            for (uint32_t slotIdx = 0; slotIdx < BMORPH_NUM_SAVE_SLOTS; ++slotIdx)
            {
               const uint32_t saveSlotAddr = BMORPH_SAVE_SLOT0_ADDR + (((uint32_t)BMORPH_SAVE_SLOT_LENGTH) * ((uint32_t) slotIdx));

               if (weaponsSettingValue > 0)
               {
                  const uint32_t saveSlotWeaponsAddr = saveSlotAddr + BMORPH_WEAPONS_AVAIL_STATUS_OFFSET;
                  for(uint32_t weaponIdx = 0; weaponIdx < BMORPH_NUM_WEAPONS; ++weaponIdx)
                  {
                     bigpemu_jag_write8(saveSlotWeaponsAddr + weaponIdx, BMORPH_WEAPON_AVAIL_VALUE);
                  }
               }

               if (clusterSettingValue > 0)
               {
                  bigpemu_jag_write8(saveSlotAddr + BMORPH_CLUSTER_OFFSET, clusterSettingValue);
               }
            }
         }
      }

      if (sGameSlotsBackedUp && (!sUnlockDone))
      {
         // User has entered Select Planet Screen, thusly committing to using the
         // selected game slot.
         if (BMORPH_SELECT_PLANET_STR_ADDR == a0Value)
         {
            const uint8_t saveSlotIdx = bigpemu_jag_read8(BMORPH_SELECTED_SAVE_SLOT_IDX_ADDR);
            for (uint8_t i = 0; i < BMORPH_NUM_SAVE_SLOTS; ++i)
            {
               // If save slot is not the save slot user committed to using.
               if (saveSlotIdx != i)
               {
                  // Restore game slot.
                  //
                  // Considered saving and restoring only the 10 bytes per slot which
                  // are modified, but decided against as there is some minor chance
                  // that the modified 10 bytes cascade/propogate to other parts of the
                  // save slot.  Seems unlikely, but why take a chance, and code is 
                  // already written to save/restore entire save slots.
                  const uint32_t saveSlotAddr = BMORPH_SAVE_SLOT0_ADDR + (((uint32_t)BMORPH_SAVE_SLOT_LENGTH) * ((uint32_t) i));
                  bigpemu_jag_sysmemwrite(&sSaveSlotsBackup[((uint32_t)i)*((uint32_t)BMORPH_SAVE_SLOT_LENGTH)], saveSlotAddr, (uint32_t)BMORPH_SAVE_SLOT_LENGTH);
               }
            }

            sUnlockDone = 1;
            // TODO: Can breakpoint be unregistered now?  Is that safe to do from within a breakpoint handler?  Does it matter?
         }

      }
   }
}

static uint32_t on_sw_loaded(const int eventHandle, void* pEventData)
{
   const uint64_t hash = bigpemu_get_loaded_fnv1a64();

   sMcFlags = 0;

   if (BMORPH_HASH == hash)
   {
      sMcFlags |= skMcFlag_Loaded;

      sUnlockDone = 0;
      sGameSlotsBackedUp = 0;

      // If unlock-all-weapons and/or skip-to-cluster cheats applied.
      int weaponsSettingValue = INVALID;
      int clusterSettingValue = INVALID;
      bigpemu_get_setting_value(&weaponsSettingValue, sWeaponsSettingHandle);
      bigpemu_get_setting_value(&clusterSettingValue, sClusterSettingHandle);
      if ((weaponsSettingValue > 0) || (clusterSettingValue > 0))
      {
         // Setup software breakpoint to handle the cheat(s).
         //
         // Looks like bigpemu_jag_m68k_bp_add may only support instruction breakpoints.
         // Tried passing address of memory for the strings "Select Game" and "Select Planet",
         // instead of instruction address, but breakpoints never got called.
         //
         // PC_VALUE_WHEN_SELECT_PLANET == PC_VALUE_WHEN_SELECT_GAME, so below breakpoint
         // will cause breakpoint handler to be called for both Select Game and Select Planet
         // screens.
         bigpemu_jag_m68k_bp_add(BMORPH_PC_VALUE_WHEN_SELECT_PLANET, bm_text_breakpoint_handler);
      }
   }
   return 0;
}

static void overwrite_ammo_value(uint32_t address, uint16_t value)
{
   // Ammo value is stored in 8 bits of data that spans 2 bytes.
   // The second and third 4-bit nibbles are where ammo value
   // is stored.

   // Read the old 2 bytes.
   // But actually read 4 bytes, as have experienced problems with BigPEmu
   // 16 bit reads/writes clobbering adjacent memory.
   // And believe experienced this again when implementing this
   // method (crash on following line when used read16 to assign to uint16_t).
   uint32_t oldBytes = bigpemu_jag_read32(address - 2);

   // Shift the specified ammo value 4 bits.
   uint32_t newBytes = value<<4;

   // Bitwise AND the new ammo value into the 2 bytes, without
   // modifying the first and fourth 4 bit nibbles.
   oldBytes &= 0xFFFFF00F;
   newBytes = oldBytes | newBytes;

   // Write the updated 4 bytes back to memory.
   bigpemu_jag_write32(address - 2, newBytes);
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
      bigpemu_get_setting_value(&livesSettingValue, sLivesSettingHandle);
      bigpemu_get_setting_value(&healthSettingValue, sHealthSettingHandle);
      bigpemu_get_setting_value(&ammoSettingValue, sAmmoSettingHandle);

      if (livesSettingValue)
      {
         bigpemu_jag_write8(BMORPH_LIVES_ADDR, BMORPH_LIVES_VALUE);
      }

      if (healthSettingValue)
      {
         bigpemu_jag_write16(BMORPH_HEALTH_ADDR, BMORPH_HEALTH_VALUE);
      }

      if (ammoSettingValue)
      {
         overwrite_ammo_value(BMORPH_MINE_SRC_ADDR, BMORPH_MINE_SRC_VALUE);
         overwrite_ammo_value(BMORPH_MISSILE_SRC_ADDR, BMORPH_MISSILE_SRC_VALUE);
         overwrite_ammo_value(BMORPH_INCINERATOR_SRC_ADDR, BMORPH_INCINERATOR_SRC_VALUE);
         overwrite_ammo_value(BMORPH_DECOY_SRC_ADDR, BMORPH_DECOY_SRC_VALUE);
         overwrite_ammo_value(BMORPH_SHIELD_SRC_ADDR, BMORPH_SHIELD_SRC_VALUE);
         overwrite_ammo_value(BMORPH_FLASH_SRC_ADDR, BMORPH_FLASH_SRC_VALUE);
         overwrite_ammo_value(BMORPH_HAMMER_SRC_ADDR, BMORPH_HAMMER_SRC_VALUE);
         overwrite_ammo_value(BMORPH_MORTAR_SRC_ADDR, BMORPH_MORTAR_SRC_VALUE);
         overwrite_ammo_value(BMORPH_QUAKE_SRC_ADDR, BMORPH_QUAKE_SRC_VALUE);
      }
   }
   return 0;
}

void bigp_init()
{
   void* pMod = bigpemu_get_module_handle();

   bigpemu_set_module_usage_flags(pMod, BIGPEMU_MODUSAGE_DETERMINISMWARNING);

   // Register cheat settings with BigPEmu GUI interface.
   const int catHandle = bigpemu_register_setting_category(pMod, "Battlemorph");
   sLivesSettingHandle = bigpemu_register_setting_bool(catHandle, "Infinite Lives", 1);
   sHealthSettingHandle = bigpemu_register_setting_bool(catHandle, "Infinite Health", 1);
   sAmmoSettingHandle = bigpemu_register_setting_bool(catHandle, "Infinite Ammo", 1);
   //
   // Enabling these is risky as needs further testing and impacts EEPROM.
   sWeaponsSettingHandle = bigpemu_register_setting_bool(catHandle, "Unlock All Weapons - EPROM Risk", 0);
   sClusterSettingHandle = bigpemu_register_setting_int_full(catHandle, "Cluster - EPROM Risk", INVALID, INVALID, BMORPH_NUM_CLUSTERS, 1);

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
   sWeaponsSettingHandle = INVALID;
   sClusterSettingHandle = INVALID;
}
