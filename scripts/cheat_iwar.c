//__BIGPEMU_SCRIPT_MODULE__
//__BIGPEMU_META_DESC__    "Unlimited lives, ammo, and unlock passwords, in Iwar."
//__BIGPEMU_META_AUTHOR__  "jguff"

#include "bigpcrt.h"

// Found using BigPEmu System->Load Software screen and highlighting ROM file.
#define IWAR_HASH          0x99A14C375429A786ULL

// Used the Pro Action Replay descreasing search script to locate health.
// Worked fine for Level1, and then didn't work for Level2.
// Looks like different location each level, and perhaps different location
// for bonus fly-through rounds.
//
// Infinite lives above, should suffice to get through game.  Progress
// is resumed from where left off, when die, so really no need for infinite
// health.
//
// If someone wants to go to trouble of overcoming this for some reason,
// perhaps poll every emulation frame for change in level.  When see level
// change, read through a presumed available object list to find player and
// then locate health field of player object:
//     EQUATES.S defines the Object structure, and ioclass is a member.
//     PLAYERS.S shows playerClass should be code for player ioclass object instance.
//     EQUATES.S defines playerClass
//     EQUATES.S defines ioenergy Object field, which i think is what needs updated.
//
// Possibly easier approach is to place write breakpoint on Level1 health address.
// Then find the instruction(s) that are decrementing the value stored at Level1
// health address.  Then replace those instructions with noops, and ensure rest
// of subroutine to decrement health is fine with that, and also ensure end of level
// score tally is not trying to decrement the health value to avoid any infinite loop
// at end of level.  (A limited test of this showed, that NOP replacement may not be
// enough.  The calling code may expect a flag to be set by the sub/decrement
// instruction.)
#define IWAR_HEALTH_LEVEL1_ADDR     0x0009FE3C
#define IWAR_HEALTH_LEVEL2_ADDR     0x000A193C
#define IWAR_HEALTH_LEVEL3_ADDR     0x0009FFBC
#define IWAR_HEALTH_LEVEL1_VALUE    0x03E7

// Located via Pro-Action Replay like scripts and memory inspection using Noesis
// Memory Window.  Afterwards, searched souce code and see this lines up with
// memory reserved in SEEDY\EQUATES.S.
#define IWAR_LASER_L_ADDR           0x000CACFC
#define IWAR_LASER_R_ADDR           0x000CACFD
#define IWAR_PLASMA_ADDR            0x000CACFE
#define IWAR_ENABLE_MISSILE_L_ADDR  0x000CACFF
#define IWAR_ENABLE_MISSILE_R_ADDR  0x000CAD00
#define IWAR_LASER_VALUE            2
#define IWAR_PLASMA_POWER_ADDR      0x000CAD01
#define IWAR_MISSILE_L_TYPE_ADDR    0x000CAD02
#define IWAR_MISSILES_L_ADDR        0x000CAD03
#define IWAR_MISSILE_R_TYPE_ADDR    0x000CAD04
#define IWAR_MISSILES_R_ADDR        0x000CAD05
#define IWAR_MINE_ADDR              0x000CAD06
#define IWAR_WEAPON_BACK_ADDR       0x000CAD07
#define IWAR_SHIELD_ADDR            0x000CAD08
#define IWAR_SHIELD_NUMBER_ADDR     0x000CAD09
#define IWAR_SHIELD_TIMER_ADDR      0x000CAD0A
#define IWAR_DRONE_ADDR             0x000CAD0B
#define IWAR_RADAR_ADDR             0x000CAD0C
#define IWAR_AUTO_ADDR              0x000CAD0D
#define IWAR_MISSILE_TYPE_VALUE     2
#define IWAR_MISSILE_LOAD_VALUE     10
#define IWAR_MINE_LOAD_VALUE        10
#define IWAR_SHIELD_LOAD_VALUE      10
#define IWAR_DRONE_VALUE            2
#define IWAR_RADAR_VALUE            2
#define IWAR_AUTO_VALUE             1

// Light, Medium, Heavy
#define IWAR_TANK_ADDR              0x000CAD12

// Used the Pro Action Replay method for below.
#define IWAR_LIVES_ADDR             0x000CAD2F
#define IWAR_LIVES_VALUE            5

// Located by applying joypad controller cheat to enter mega
// menu passcode entry menu, and then used Noesis Memory Window
// Find capability to find the entered password.
#define IWAR_NODE_CODE_ADDR         0x000CAC0A
// 10 digits

// Located by reviewing OPTIONS.S and seeing declared 7 bytes after end of password.
#define IWAR_MEGA_MENU_ADDR         0x000CAC1A

// Located by searching for 0x000CAC0A, using Noesis Memory Window Find capability, getting 4 hits.
// Then reviewsd those hits in disassembly window.
// Then reviewed OPTIONS.S, noting there are 4 hits for 0x000CAC0A.
// First hit has an instruction immediately above it, to load effective address below.
// These passcodes are stored as ASCII, unlike the user entered passcode stored at 0x000CAC0A.
#define IWAR_PASSCODES_ADDR         0x0005D192
#define IWAR_PASSCODE_LENGTH        10
#define IWAR_NUM_LEVELS             21

// Amount to subtract from ASCII value between '0' and '9', to convert to unsigned integer value between 0 and 9.
#define IWAR_ASCII_OFFSET           0x30

// Health address seemed to change after Level1.
// Placed a instruction breakpoint on IWAR_HEALTH_LEVEL1_ADDR, and
// noted that at the following addresses is an instruction that decrements
// health value.  There may be more then identified here...
//
// Missile launcher corresponded to ADDRA.
// Green and white bouncy ball corresponded to ADDRB.
// Tan, triangle-y, rotating ?mine? corresponded to ADDRB.
#define IWAR_HEALTH_DEC_INSTRUCT_ADDRA 0x00012918
#define IWAR_HEALTH_DEC_INSTRUCT_ADDRB 0x0000CD08
// Hex code for instruction: sub.w   D0, ($64,A0)
#define IWAR_HEALTH_DEC_INSTRUCTION    0x64006891

#define  INVALID                       -1

// Boilerplate.
static int32_t sOnLoadEvent = INVALID;
static int32_t sOnEmuFrame = INVALID;
static int32_t sMcFlags = 0;
static const int32_t skMcFlag_Loaded = (1 << 0);

// Setting handles.
static int sLivesSettingHandle = INVALID;
static int sHealthForLevel1SettingHandle = INVALID;
static int sLasersSettingHandle = INVALID;
static int sPlasmaSettingHandle = INVALID;
static int sMissilesSettingHandle = INVALID;
static int sMinesSettingHandle = INVALID;
static int sShieldsSettingHandle = INVALID;
static int sDroneSettingHandle = INVALID;
static int sRadarSettingHandle = INVALID;
static int sAutoSettingHandle = INVALID;
static int sLevelSettingHandle = INVALID;

static uint32_t on_sw_loaded(const int eventHandle, void* pEventData)
{
   const uint64_t hash = bigpemu_get_loaded_fnv1a64();

   sMcFlags = 0;

   if (IWAR_HASH == hash)
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
      int healthForLevel1SettingValue = INVALID;
      int lasersSettingValue = INVALID;
      int plasmaSettingValue = INVALID;
      int missilesSettingValue = INVALID;
      int minesSettingValue = INVALID;
      int shieldsSettingValue = INVALID;
      int droneSettingValue = INVALID;
      int radarSettingValue = INVALID;
      int autoSettingValue = INVALID;
      int levelSettingValue = INVALID;

      bigpemu_get_setting_value(&livesSettingValue, sLivesSettingHandle);
      bigpemu_get_setting_value(&healthForLevel1SettingValue, sHealthForLevel1SettingHandle);
      bigpemu_get_setting_value(&lasersSettingValue, sLasersSettingHandle);
      bigpemu_get_setting_value(&plasmaSettingValue, sPlasmaSettingHandle);
      bigpemu_get_setting_value(&missilesSettingValue, sMissilesSettingHandle);
      bigpemu_get_setting_value(&minesSettingValue, sMinesSettingHandle);
      bigpemu_get_setting_value(&shieldsSettingValue, sShieldsSettingHandle);
      bigpemu_get_setting_value(&droneSettingValue, sDroneSettingHandle);
      bigpemu_get_setting_value(&radarSettingValue, sRadarSettingHandle);
      bigpemu_get_setting_value(&autoSettingValue, sAutoSettingHandle);
      bigpemu_get_setting_value(&levelSettingValue, sLevelSettingHandle);

      if (livesSettingValue > 0)
      {
         bigpemu_jag_write8(IWAR_LIVES_ADDR, IWAR_LIVES_VALUE);
      }

      if (healthForLevel1SettingValue > 0)
      {
         // This only works for Level1.  After Level1, the address of health
         // changes.
         bigpemu_jag_write16(IWAR_HEALTH_LEVEL1_ADDR, IWAR_HEALTH_LEVEL1_VALUE);

         // Instead, overwrite the instruction that decrements health with NOPs.
         // There is some chance this could be a bad idea by itself, if
         // each time a health refresh item is picked up the health is increased,
         // but these nops prevent decreasing, and eventually an increment exceeds
         // max value for 2 bytes.
         //
         // bigpemu_jag_sysmemwrite seems to successfully overwrite ROM memory.
         // At least with BigPEmu v 1.19.
         //
         // (A close to production version of the source code is available.
         // Could be possible to analyze source code to see where health
         // address is expected, for every mission.
         // But simpily overwriting the instruction that decrements health seems
         // easier. If picking up health refresh items causes problems, may
         // need to try refer to source code to figure out where address is
         // for every level.)
         //
         // Tested the following approach of overwriting the instruction that
         // decrements health.  There are at least 2 addresses holding this
         // instruction, and one of them doesn't jive well with the transition
         // from start screen to beginning of Level1 gameplay (hangs game).
         // Tried reading the address to ensure equaled the instruction (in case
         // code was being swapped in/out at that address, prior to overwriting.
         // That had the same problem of hanging the game.
         // Suspect need to set/clear a processor flag that the calling routine
         // is expecting the replaced instruction to have set/cleared.
         // 
         // TODO: Just commenting out for now.  Come back to this later.  Source code
         //       review might be worthwhile at this point.
         //
         //const uint8_t rtsInstruction[2] = {0x4E, 0x71};
         //bigpemu_jag_sysmemwrite(rtsInstruction, IWAR_HEALTH_DEC_INSTRUCT_ADDRA + 0, 2);
         //bigpemu_jag_sysmemwrite(rtsInstruction, IWAR_HEALTH_DEC_INSTRUCT_ADDRA + 2, 2);
         //
         //const uint32_t fourBytes = bigpemu_jag_read32(IWAR_HEALTH_DEC_INSTRUCT_ADDRB);
         //const uint32_t fourBytesSwapped = byteswap_ulong(fourBytes);
         //if (IWAR_HEALTH_DEC_INSTRUCTION == fourBytesSwapped)
         //{
         //	bigpemu_jag_sysmemwrite(rtsInstruction, IWAR_HEALTH_DEC_INSTRUCT_ADDRB + 0, 2);
         //	bigpemu_jag_sysmemwrite(rtsInstruction, IWAR_HEALTH_DEC_INSTRUCT_ADDRB + 2, 2);
         //}
      }

      // For below, was fine finding just the missiles.  Infinite lives (above)
      // and infinite missiles can get you through the game with no problem.
      // But since a copy of the source code is available, and the other weapons
      // are layed out nearby in a data structure, will go ahead and set some
      // the other weapons as well.  (Without knowing the data structure layout,
      // wouldn't have bothered with the other weapons.)
      uint8_t tank = bigpemu_jag_read8(IWAR_TANK_ADDR);
      if (0 == tank) // Light Tank
      {
         if (lasersSettingValue > 0)
         {
            // Enable/upgrade both lasers to mark 2.
            // Heads Up Display (HUD) only seemed to recognize values upto 2.
            // The 3d display did recognize numbers up to 9, but didn't try higher.
            // Didn't check if damage to enemies recognized higher then 2.
            // Code showed references to setting to 2 as well.
            // Set to 2, to reduce likelyhood of breaking game.
            bigpemu_jag_write8(IWAR_LASER_L_ADDR, IWAR_LASER_VALUE);

            // Light Tank does not like right and left lasers both
            // being present.  Causes right laser to hit left laser
            // ammo, right in tank'face.  Uncomment to see, if you
            // want.
            //
            // Looked at source code and see that different tanks are
            // configured to max out weapons at different levels.  And
            // there is some reason to stick with game creators max
            // weapon levels.
            //
            // The other part of looking at source code was to verify
            // that player acquiring weapon upgrade during gameplay will
            // not increment weapon level past a valid value.  Looked
            // like the source was setting values on upgrade, not
            // incrementing.
            //bigpemu_jag_write8(IWAR_LASER_R_ADDR, IWAR_LASER_VALUE);
         }

         if (plasmaSettingValue > 0)
         {
            bigpemu_jag_write8(IWAR_PLASMA_ADDR, tank + 1);
         }
         if (missilesSettingValue > 0)
         {
            bigpemu_jag_write8(IWAR_ENABLE_MISSILE_L_ADDR, tank + 1);
            // Source code provides no missile launcher right, for Light Tank.
            bigpemu_jag_write8(IWAR_MISSILE_L_TYPE_ADDR, IWAR_MISSILE_TYPE_VALUE);
            bigpemu_jag_write8(IWAR_MISSILES_L_ADDR, IWAR_MISSILE_LOAD_VALUE);
         }
         if (minesSettingValue > 0)
         {
            bigpemu_jag_write8(IWAR_MINE_ADDR, IWAR_MINE_LOAD_VALUE);
            bigpemu_jag_write8(IWAR_WEAPON_BACK_ADDR, 4);
         }
         if (shieldsSettingValue > 0)
         {
            bigpemu_jag_write8(IWAR_SHIELD_ADDR, tank + 1);
            bigpemu_jag_write8(IWAR_SHIELD_NUMBER_ADDR, IWAR_SHIELD_LOAD_VALUE);
         }
         if (droneSettingValue > 0)
         {
            bigpemu_jag_write8(IWAR_DRONE_ADDR, IWAR_DRONE_VALUE);
         }
         if (radarSettingValue > 0)
         {
            bigpemu_jag_write8(IWAR_RADAR_ADDR, IWAR_RADAR_VALUE);
         }
         if (autoSettingValue > 0)
         {
            bigpemu_jag_write8(IWAR_AUTO_ADDR, IWAR_AUTO_VALUE);
         }
      }
      else if (1 == tank) // Medium Tank
      {
         if (lasersSettingValue > 0)
         {
            bigpemu_jag_write8(IWAR_LASER_L_ADDR, IWAR_LASER_VALUE);
            bigpemu_jag_write8(IWAR_LASER_R_ADDR, IWAR_LASER_VALUE);
         }
         if (plasmaSettingValue > 0)
         {
            bigpemu_jag_write8(IWAR_PLASMA_ADDR, tank + 1);
         }
         if (missilesSettingValue > 0)
         {
            bigpemu_jag_write8(IWAR_ENABLE_MISSILE_L_ADDR, tank + 1);
            // Source code provides no missile launcher right, for Medium Tank.
            bigpemu_jag_write8(IWAR_MISSILE_L_TYPE_ADDR, IWAR_MISSILE_TYPE_VALUE);
            bigpemu_jag_write8(IWAR_MISSILES_L_ADDR, 2 * IWAR_MISSILE_LOAD_VALUE);
         }
         if (minesSettingValue > 0)
         {
            bigpemu_jag_write8(IWAR_MINE_ADDR, 2 * IWAR_MINE_LOAD_VALUE);
            bigpemu_jag_write8(IWAR_WEAPON_BACK_ADDR, 5);
         }
         if (shieldsSettingValue > 0)
         {
            bigpemu_jag_write8(IWAR_SHIELD_ADDR, tank + 1);
            bigpemu_jag_write8(IWAR_SHIELD_NUMBER_ADDR, 2 * IWAR_SHIELD_LOAD_VALUE);
         }
         if (droneSettingValue > 0)
         {
            bigpemu_jag_write8(IWAR_DRONE_ADDR, IWAR_DRONE_VALUE);
         }
         if (radarSettingValue > 0)
         {
            bigpemu_jag_write8(IWAR_RADAR_ADDR, IWAR_RADAR_VALUE);
         }
         if (autoSettingValue > 0)
         {
            bigpemu_jag_write8(IWAR_AUTO_ADDR, IWAR_AUTO_VALUE);
         }
      }
      else if (2 == tank) // Heavy Tank
      {
         if (lasersSettingValue > 0)
         {
            bigpemu_jag_write8(IWAR_LASER_L_ADDR, IWAR_LASER_VALUE);
            bigpemu_jag_write8(IWAR_LASER_R_ADDR, IWAR_LASER_VALUE);
         }
         if (plasmaSettingValue > 0)
         {
            bigpemu_jag_write8(IWAR_PLASMA_ADDR, tank + 1);
         }
         if (missilesSettingValue > 0)
         {
            bigpemu_jag_write8(IWAR_ENABLE_MISSILE_L_ADDR, tank);
            bigpemu_jag_write8(IWAR_ENABLE_MISSILE_R_ADDR, tank);
            bigpemu_jag_write8(IWAR_MISSILE_L_TYPE_ADDR, IWAR_MISSILE_TYPE_VALUE);
            bigpemu_jag_write8(IWAR_MISSILES_L_ADDR, 2 * IWAR_MISSILE_LOAD_VALUE);
            bigpemu_jag_write8(IWAR_MISSILE_R_TYPE_ADDR, IWAR_MISSILE_TYPE_VALUE);
            bigpemu_jag_write8(IWAR_MISSILES_R_ADDR, 2 * IWAR_MISSILE_LOAD_VALUE);
         }
         if (minesSettingValue > 0)
         {
            bigpemu_jag_write8(IWAR_MINE_ADDR, 3 * IWAR_MINE_LOAD_VALUE);
            bigpemu_jag_write8(IWAR_WEAPON_BACK_ADDR, 6);
         }
         if (shieldsSettingValue > 0)
         {
            bigpemu_jag_write8(IWAR_SHIELD_ADDR, tank + 1);
            bigpemu_jag_write8(IWAR_SHIELD_NUMBER_ADDR, 3 * IWAR_SHIELD_LOAD_VALUE);
         }
         if (droneSettingValue > 0)
         {
            bigpemu_jag_write8(IWAR_DRONE_ADDR, IWAR_DRONE_VALUE);
         }
         if (radarSettingValue > 0)
         {
            bigpemu_jag_write8(IWAR_RADAR_ADDR, IWAR_RADAR_VALUE);
         }
         if (autoSettingValue > 0)
         {
            bigpemu_jag_write8(IWAR_AUTO_ADDR, IWAR_AUTO_VALUE);
         }
      }

      if (levelSettingValue > 0)
      {
         // Enable expanded menu that allows user to enter a passcode.
         bigpemu_jag_write8(IWAR_MEGA_MENU_ADDR, 0);

         // Populate user passcode, with the passcode for level specified by BigPEmu setting.
         // Copy passcode specified by BigPEmu user setting to the address used by expanded options menu.
         const uint32_t passcodeAddr = IWAR_PASSCODES_ADDR + ((levelSettingValue - 1) * IWAR_PASSCODE_LENGTH);
         for (uint32_t i = 0; i < IWAR_PASSCODE_LENGTH; ++i)
         {
            const uint8_t digit = bigpemu_jag_read8(passcodeAddr + i) - IWAR_ASCII_OFFSET;
            bigpemu_jag_write8(IWAR_NODE_CODE_ADDR + i, digit);
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
   const int catHandle = bigpemu_register_setting_category(pMod, "I-War");
   sLivesSettingHandle = bigpemu_register_setting_bool(catHandle, "Infinite Lives", 1);
   sHealthForLevel1SettingHandle = bigpemu_register_setting_bool(catHandle, "Infinite Health For Level 1", 0);
   sLasersSettingHandle = bigpemu_register_setting_bool(catHandle, "Max Laser Upgrades", 1);
   sPlasmaSettingHandle = bigpemu_register_setting_bool(catHandle, "Max Plasma Cannon Upgrades", 1);
   sMissilesSettingHandle = bigpemu_register_setting_bool(catHandle, "Infinite Missiles", 1);
   sMinesSettingHandle = bigpemu_register_setting_bool(catHandle, "Infinite Mines", 1);
   sShieldsSettingHandle = bigpemu_register_setting_bool(catHandle, "Infinite Shields", 1);
   sDroneSettingHandle = bigpemu_register_setting_bool(catHandle, "Max Drone", 1);
   sRadarSettingHandle = bigpemu_register_setting_bool(catHandle, "Max Radar", 1);
   sAutoSettingHandle = bigpemu_register_setting_bool(catHandle, "Auto Targeting", 1);
   sLevelSettingHandle = bigpemu_register_setting_int_full(catHandle, "Autofill Passcode For Level", 0, 0, IWAR_NUM_LEVELS, 1);

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
   sHealthForLevel1SettingHandle = INVALID;
   sLasersSettingHandle = INVALID;
   sPlasmaSettingHandle = INVALID;
   sMissilesSettingHandle = INVALID;
   sMinesSettingHandle = INVALID;
   sShieldsSettingHandle = INVALID;
   sDroneSettingHandle = INVALID;
   sRadarSettingHandle = INVALID;
   sAutoSettingHandle = INVALID;
   sLevelSettingHandle = INVALID;
}