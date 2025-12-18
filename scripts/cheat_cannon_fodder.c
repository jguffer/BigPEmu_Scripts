//__BIGPEMU_SCRIPT_MODULE__
//__BIGPEMU_META_DESC__    "Infinite ammo, recruits, mission select, in Cannon Fodder."
//__BIGPEMU_META_AUTHOR__  "jguff"

#include "bigpcrt.h"

// Found using BigPEmu System->Load Software screen and highlighting ROM file.
#define CFODDER_HASH             0xB1EDD5D934A2102CULL

// Found using Pro Action Replay like script.
#define CFODDER_RECRUITS_ADDR    0x001905A1
#define CFODDER_GRENADES_ADDR    0x00191DA7
#define CFODDER_RECRUITS_VALUE   64
#define CFODDER_AMMO_VALUE       6

// Found this address near grenades address, using Noesis Memory Window.
//
// All 16 bytes for/following CFODDER_GRENADES_ADDR were zeros, other then
// grenades address.  Used Noesis Fill Memory feature to write 0x01
// to those 16 bytes, and sure enough got whole bunch of bazooka ammo.
// Grenades and bazooka may be represented as 2 bytes actually, but
// only need the lower byte for this.
// Process of elimation after that, to find the exact bazooka ammo
// address.
//
// Way easier/quicker then having to play through 5+ phases to get
// access to first bazooka, and then run the Pro Action Replay like
// script.  Script is fine, but no patience to play through 5+ phases.
#define CFODDER_BAZOOKA_ADDR  0x00191DAD

// Found using Pro-Action Replay like increasing count search script.
// This is total num kills (increases across missions, shared between
// soldiers).
//
// The count does not increase until the shot enemy soldier disappears.
// Player can maim/render an enemy soldier harmless, but if enemy soldier
// has not disappeared yet (or in some cases requires another shot to
// be really killed), then the count doesn't go up.  
//
// Current iteration of Pro-Action Replay like script assumes a different
// value for every capture of memory.
#define CFODDER_NUM_KILLS_ADDR   0x00190C86

// Found with read breakpoint on 0x00F15000.  Had tried searching contents of RAM
// for first half of first line of EPROM, and didn't get a hit.
// And now remember why: the EPROM words get byte swapped when copied to JAGUAR RAM.
// Was using wrong format to search for data.
#define CFODDER_EPROM_CONTENTS_ADDR 0x000E5040

// Found using Pro-Action Replay like counter script.  0-based mission number.
// Setting this to value N did not make game skip to Mission N, though the pre-mission
// screen did reflect some of Mission N's information.
#define CFODDER_MISSION_ADDR  0x00019059B

// Found via inspection, near CFODDER_MISSION_ADDR, using Noesis Memory Window.
// This is 0-based cumulative phase number.  After first 1-phase Mission1 and 2-phase
// Mission3, this address had value of 3.
#define CFODDER_PHASE_ADDR 0x00019058F

// Writing to CFODDER_MISSION_ADDR and CFODDER_PHASE_ADDR causes the gameplay to skip
// to the specified values.  However, regardless of values written, pre-mission screen
// states the mission has 1 phase.  And after complete the first phase, the game skips
// to the next mission, even if additional phases should have followed before completion
// of mission.
//
// Still missing something.  Presumably need to update a variable which represents
// number of phases in mission.
//
// 01. Noted the following text is displayed in pre-mission screen:
//	"PHASE <n1> of <CFODDER_NUM_PHASES_IN_MISSION>"
// 02. Searched RAM for ASCII "PHASE".
// 03. 0x0005E699 points to essentially "PHASE -1 OF -1"
// 04. Placed read breakpoint on 0x0005E69D (the 'E' of "PHASE")
// 05. Restarted game, and caused pre-mission screen to be displayed which prints text of "PHASE <n1> OF <CFODDER_NUM_PHASES_IN_MISSION>"
// 06. Breakpoint hit
// 07. Disassembly shows 0x0005E69D was copied to 0x00191E62.  0x00191E69 is where the second -1 is at.
// 08. Placed write breakpoint on 0x00191E69 to see when 0x00191E69 gets populated with the number-of-phases-in-mission
// 09. Restarted game, loaded EEPROM slot at Mission 5, selected to begin Mission5
// 10. Breakpoint hit.
// 11. In disassembly, stepped out of couple subroutines to find that 0x001905C3 is source for replacement
// 12. Restarted game, and flipped back and forth between EEPROM save slots
// 13. Noted 0x001905C3 seems to be updated to number of phases for mission
// 14. Noted 0x001905C1 seems to be updated to number of phases for mission, as well.
// 15. Performed some additional testing.
// 16. Appears 0x001905C3 is used for text displayed in pre-mission screen, and
//     0x001905C1 is used to determine when to end current mission and start new mission.
//     Both these addresses seem to hold same value.
//     Both these values are 1-based.
#define CFODDER_NUM_PHASES_FOR_MISSION_ADDRA 0x001905C1
#define CFODDER_NUM_PHASES_FOR_MISSION_ADDRB 0x001905C3 

// Writing to the values above works fine, if you only skip to first phase of mission.
// If skip to other phase in mission, then the counts are off, and Mission3-Phase1 is
// considered Mission2-Phase2 for example.
//
// 17. 0x00191E64 is where the first -1 is set at.
// 18. Breakpoint onto 0x00191E64.
// 19. Restart game, and similar to above.
// 20. Breakpoint hit.
// 21. Disassembly shows 0x0019059D is source of value.
//     0-based value.
#define CFODDER_PHASE_ADDR_B                 0x0019059D

#define CFODDER_NUM_MISSIONS                 24
#define CFODDER_NUM_PHASES_FOR_MISSION_IDX   0
#define CFODDER_ACCUMULATED_NUM_PHASES_IDX   1

// It may be a bad idea to allow skipping to non-first phase of a mission.
// The game was not designed to do so, even from EEPROM save slot restore.
// The game logic may need some major hacking to make work.
// If someone cares enough about skipping to non-first-phase of mission,
// they can hack this script further to do so.
// I'm moving on...

// Since moving on...
// Below are some additional findings which are not currently utilized by the
// script.  Someone else may find useful, if someone else wants to add some
// functionality to the script.
//
//
//
// The values for below addresses update when cabin door opens to release
// enemy soldier(s), during Mission2 and Mission3.
//
// Someone could possibly figure out a way to prevent doors from opening
// and releasing enemy soldiers.
//
// 0x000190596
// 0x000190598
//
//
//
// Had bit more trouble locating mission and phase addresses above, them the above
// comments suggest (wasn't expecting phase number to be cumulative across missions)
// and started coming at locating the addresses from a different angle:  EEPROM
// contents.
//
// Placed write breakpoint on mission number address and loaded slot from EEPROM.
// Disassembly then pointed to the following addresses.
// #define CFODDER_SELECTED_EEPROM_SLOT   0x000E5000
// #define CFODDER_EEPROM_SLOT1           0x000E5040
// #define CFODDER_EEPROM_SLOT2           0x000E5080
//
// These addresses point to EEPROM data, as copied by game into Jaguar RAM.
// Each is 64 bytes wide.
// CFODDER_SELECTED_EEPROM_SLOT data is copy of either CFODDER_EEPROM_SLOT1
// data or CFODDER_EEPROM_SLOT2 data.
//
// +0x00000004 is number of recruits remaining
// +0x00000006 is 0-based mission number
// +0x00000007 is 0-based cumulative phase number
// +0x00000008 is likely 2 byte number of players soldiers that have died
// +0x0000000A is 2 byte number of enemy soldiers killed
// +0x0000003E is 2 byte checksum/hash
//
// Toyed around with modifying the copy of EEPROM contents, contained in Jaguar RAM.
// That included stepping through disassembly of checksum/hash and implementing the
// checksum/hash.  Example code to implement the checksum/hash is below:
//
// (Have had nothing but nightmares using bigpemu_jag_read16 and/or uint16_ts.)
// (Use uint32_t and mask out the top 2 bytes, after every assign, without exception,)
// (no matter how much it might not be needed.)
// uint32_t wordValue = 0;
// uint32_t checksum = 0;
// uint32_t plus22 = 0;
// uint32_t numBytesPerSlot = CFODDER_EEPROM_SLOT2 - CFODDER_EEPROM_SLOT1;
// uint32_t numWordsPerSlot = numBytesPerSlot / 2;
// uint32_t numWordsToSumPerSlot = numWordsPerSlot / 2;
// uint32_t baseAddr = CFODDER_SELECTED_EEPROM_SLOT;
// uint32_t bytesPerChecksum = 2;
// for (uint32_t byteOffset = 0; byteOffset < (numBytesPerSlot - bytesPerChecksum); byteOffset+=2)
// {
//    wordValue = ((uint32_t) bigpemu_jag_read16(baseAddr + byteOffset));
//    wordValue = wordValue & 0x0000FFFF;
//    checksum += wordValue;
//    checksum = checksum & 0x0000FFFF;
//    plus22 = wordValue + 22;
//    plus22 = plus22 & 0x0000FFFF;
//    checksum += plus22;
//    checksum = checksum & 0x0000FFFF;
// }
// bigpemu_jag_write8(baseAddr + 0x0E, ((uint8_t) (checksum >> 8)));
// bigpemu_jag_write8(baseAddr + 0x3F, ((uint8_t) (checksum & 0xFF)));

#define	INVALID -1

// Boilerplate.
static int32_t sOnLoadEvent = INVALID;
static int32_t sOnEmuFrame = INVALID;
static int32_t sMcFlags = 0;
static const int32_t skMcFlag_Loaded = (1 << 0);

// Cannon fodder consists of 72 phases split (not equally) over 24 missions.
//
// https://www.youtube.com/watch?v=TX0LM-YkQpY
//
// {num-phases, cumulative-num-phases}, // Mission<number>
//
// Will initialize the cumulative-num-phases, when software loaded.
// Probably ought to make 2 field structure, but meh...
static uint8_t sMissionPhases[CFODDER_NUM_MISSIONS][2] = {
   {1, 0}, // Mission01
   {2, 0}, // Mission02
   {1, 0}, // Mission03
   {4, 0}, // Mission05
   {3, 0}, // Mission05
   {2, 0}, // Mission06
   {3, 0}, // Mission07
   {4, 0}, // Mission08
   {2, 0}, // Mission09
   {5, 0}, // Mission10
   {3, 0}, // Mission11
   {6, 0}, // Mission12
   {1, 0}, // Mission13
   {3, 0}, // Mission14
   {3, 0}, // Mission15
   {2, 0}, // Mission16
   {1, 0}, // Mission17
   {5, 0}, // Mission18
   {1, 0}, // Mission19
   {4, 0}, // Mission20
   {1, 0}, // Mission21
   {4, 0}, // Mission22
   {5, 0}, // Mission23
   {6, 0}  // Mission24
};

// Setting handles.
static int sRecruitsSettingHandle = INVALID;
static int sGrenadesSettingHandle = INVALID;
static int sBazookasSettingHandle = INVALID;
static int sMissionSettingHandle = INVALID;

static uint32_t on_sw_loaded(const int eventHandle, void* pEventData)
{
   const uint64_t hash = bigpemu_get_loaded_fnv1a64();

   sMcFlags = 0;

   if (CFODDER_HASH == hash)
   {
      sMcFlags |= skMcFlag_Loaded;

      // Initialize accumulated-num-phases.
      for (int i = 1; i < CFODDER_NUM_MISSIONS; ++i)
      {
         // Accumlated phases is the accumlated phases to get to the ith mission, so add (i-1)th num-phases to (i-1)th accumlated-num-phases
         sMissionPhases[i][CFODDER_ACCUMULATED_NUM_PHASES_IDX] = sMissionPhases[i-1][CFODDER_NUM_PHASES_FOR_MISSION_IDX] + sMissionPhases[i-1][CFODDER_ACCUMULATED_NUM_PHASES_IDX];
      }
   }
   return 0;
}

static uint32_t on_emu_frame(const int eventHandle, void* pEventData)
{
   if (sMcFlags & skMcFlag_Loaded)
   {
      // Get setting values.  Doing this once per frame, allows player
      // to modify setting during game session.
      int recruitsSettingValue = INVALID;
      int grenadesSettingValue = INVALID;
      int bazookasSettingValue = INVALID;
      int missionSettingValue = INVALID;
      bigpemu_get_setting_value(&recruitsSettingValue, sRecruitsSettingHandle);
      bigpemu_get_setting_value(&grenadesSettingValue, sGrenadesSettingHandle);
      bigpemu_get_setting_value(&bazookasSettingValue, sBazookasSettingHandle);
      bigpemu_get_setting_value(&missionSettingValue, sMissionSettingHandle);

      if (recruitsSettingValue > 0)
      {
         bigpemu_jag_write8(CFODDER_RECRUITS_ADDR, CFODDER_RECRUITS_VALUE);
      }
      if (grenadesSettingValue > 0)
      {
         bigpemu_jag_write8(CFODDER_GRENADES_ADDR, CFODDER_AMMO_VALUE);
      }
      if (bazookasSettingValue > 0)
      {
         bigpemu_jag_write8(CFODDER_BAZOOKA_ADDR, CFODDER_AMMO_VALUE);
      }

      if (missionSettingValue > 1)
      {
         // Get value for number of kills.
         // Take no chances of 16-bit assign memory clobbering bug.
         const uint32_t numKills = (bigpemu_jag_read8(CFODDER_NUM_KILLS_ADDR + 0) << 8) | bigpemu_jag_read8(CFODDER_NUM_KILLS_ADDR + 1);
         if (0 == numKills)
         {
            const uint8_t missionNum = missionSettingValue; // 1-based
            const uint8_t phaseNumCumulative = sMissionPhases[missionSettingValue - 1][CFODDER_ACCUMULATED_NUM_PHASES_IDX]; // 0-based, cumulative
            const uint8_t phaseNumForMission = 0; // 0-based, non cumulative
            const uint8_t numPhasesForMission = sMissionPhases[missionSettingValue - 1][CFODDER_NUM_PHASES_FOR_MISSION_IDX]; // 1-based

            bigpemu_jag_write8(CFODDER_MISSION_ADDR, missionNum);
            bigpemu_jag_write8(CFODDER_PHASE_ADDR, phaseNumCumulative);
            bigpemu_jag_write8(CFODDER_PHASE_ADDR_B, phaseNumForMission);
            bigpemu_jag_write8(CFODDER_NUM_PHASES_FOR_MISSION_ADDRA, numPhasesForMission);
            bigpemu_jag_write8(CFODDER_NUM_PHASES_FOR_MISSION_ADDRB, numPhasesForMission);
         }
      }
   }
   return 0;
}

void bigp_init()
{
   void* pMod = bigpemu_get_module_handle();

   bigpemu_set_module_usage_flags(pMod, BIGPEMU_MODUSAGE_DETERMINISMWARNING);

   // Register cheat settings with BigPEmu GUI interface.
   const int catHandle = bigpemu_register_setting_category(pMod, "Cannon Fodder");
   sRecruitsSettingHandle = bigpemu_register_setting_bool(catHandle, "Infinite Recruits", 1);
   sGrenadesSettingHandle = bigpemu_register_setting_bool(catHandle, "Infinite Grenades", 1);
   sBazookasSettingHandle = bigpemu_register_setting_bool(catHandle, "Infinite Bazookas", 1);
   sMissionSettingHandle = bigpemu_register_setting_int_full(catHandle, "Start At Mission", INVALID, INVALID, CFODDER_NUM_MISSIONS, 1);

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

   sRecruitsSettingHandle = INVALID;
   sGrenadesSettingHandle = INVALID;
   sBazookasSettingHandle = INVALID;
   sMissionSettingHandle = INVALID;
}