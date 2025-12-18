//__BIGPEMU_SCRIPT_MODULE__
//__BIGPEMU_META_DESC__     "Infinite health and ammo, play or model as other (unused) characters, use flower, title screen sword, plank, etc as weapon, test unused animations, freeze enemies, unlock keys, tools, maps, orders, warp to teleporter room, in Highlander."
//__BIGPEMU_META_AUTHOR__   "jguff"

#include "bigpcrt.h"

// DBMBD
//
// You will have massive headaches if you try assigning to uint16_t (unsigned short), which are not declared static.
// Memory clobbering, hangs, crashes.  Sometimes randomly as well.
// Perhaps this extends to int16 writes as well, but did not test.
// Perhaps writes to static uint16_t are broken too, but more randomly.
// bigpemu_jag_write16 calls may be ok, or may just happen to work when overwriting 32 bit values with 16 bit values.
//
// uint16_t declared in structure worked fine for few hours, then one new line of code that wrote to a uint16_t structure field
// (and that structure field continues to be written to fine from other prior lines of code) bombed the emulator.  Don't know 
// why either, as Noesis was showing the uint16_t immediately before it as not being clobbered.
//
// Workaround was to declare uint32_t in the structures instead of uint16_t.
//    Breaks simple bytewise deserialization of the structure from compact disc, and breaks simple bytewise serialization of the stucture to Jaguar memory
//
// To view this bug yourself, the following will clobber memory:
//    uint16_t ding = 65535;
//    uint16_t dong = 65535;
//    printf_notify("%u, %u", ding, dong);
// will result in the following being printed:
//    0, 65535
// as dong clobbers ding.
// Performing the following:
//    uint16_t ding = 65535;
//    uint32_t safety = 0;
//    uint32_t safetyExtra = 0; // probably not neccessary
//    uint16_t dong = 65535;
//    printf_notify("%u, %u", ding, dong);
// will result in the following being printed:
//    65535, 65535
//
// When you see extra lines of code to avoid 16 bit writes: DBMBD.
// When you see structures being populated/copied one field at a time, instead of using bigpemu_jag_sysmemread, bigpemu_jag_sysmemwrite, and memcpy: DBMBD.

// There is extensive memory map, as well as documentation of memory structures, located at bottom of this script file.

// Found using BigPEmu System->Load Software screen and highlighting ROM file.
#define HL_HASH     0xF08A239FFA8F47C4ULL

// Below are some commonly referenced addresses when debugging.  These addresses
// will get stuck in your memory eventually.  You might as well remember these
// addresses sooner, rather then later.
//    0x00032660 Quentin's (player's) World State entry. Also the first entry in World State.
//    0x00034A64 Quentin's (player's) character sheet.  Also the first entry in the character sheets.
//    0x00042300 Set Data

// Description of joypad controller cheats, implemented in production version of game:
//  https://atariage.com/software_hints.php?SoftwareHintID=213
//  All Items Cheat
//      Press 9, 4, 4, 6, 9, 2, 1, 3 during game play before any items are collected.
//      Only unlocks the tools.  Does not unlock weapons, maps, orders, or food items.
//      Option button continues to work as well.
//      Might have missed something.
//  Invincibility Cheat
//      Press 4, 1, 3, 3, 5, 6, 4 during game play.
//      Verified this impacts Life field, and does not affect Sanity, Personality, Strength fields.
//      Cannot be toggled off.
//  Rogues Gallery Cheat
//      Press 2, 1, 8, 8, 3, 5, 3, 8, 9 during game play.
//      Scene field set to 0x06C0
//      Portraits of development team members on the wall.
//      Normal game play may also allow access to this area.
//  Teleporter Cheat
//      Enable the "Rogues gallery" code. Place a flower in the vase in the Rogues gallery.
//      This unlocks Rubber Chicken weapon.
//      This enables a 6 (maybe 8) sided teleporter in center of room.

// Found using Pro Action Replay like decreasing value search script.
#define HL_HEALTH_ADDR     0x0003267E
#define HL_HEALTH_VALUE    0xFF

// Address of item list.
//
// Found by counting bytes backwards from  HL_HEALTH_ADDR after
// analyzing revision of source code.
//
// Item List contains Items.  Items are used to represent
// fightable characters (Quentin, enemy fighters) and
// collectables (weapons, keys, tools, maps, orders, food, etc).
#define HL_ITEM_LIST_ADDR   0x00032660

// Items are 32 bytes wide.
#define HL_ITEM_SIZE        32

#define HL_PERSON_RADIUS    0x0064

// Calculate number of items.  Should be 197 items.
#define HL_NUM_ITEMS        (0x00033F00 - HL_ITEM_LIST_ADDR) / HL_ITEM_SIZE

// Bottom of this script file contains a description of world state entry fields.
typedef struct
{
    uint16_t mScene;
    uint16_t mRadius;
    uint32_t mSheet;
    uint32_t mOwner;
    uint32_t mXpos;
    uint32_t mYpos;
    uint32_t mZpos;
    uint8_t  mXface;
    uint8_t  mYface;
    uint8_t  mZface;
    uint8_t  mSanity;
    uint8_t  mPersonality;
    uint8_t  mStrength;
    uint8_t  mLife;
    uint8_t  mFlags;
}  HlItem;

#define HL_WSTDeactivated   6

// Still cannot figure out anyway to assign to a uint16_t.  Always ends up with
// memory getting clobbered and headaches.  99+% confident this is bug in 
// BigPEmu scripting capabilities.
// Since the swap routine defined in bigpcrt.h doesn't work on 16 bit data,
// use this for swapping 16 bit data, instead.
void workingByteSwap16(uint16_t* pValue)
{
#ifdef __BIGPVM_NATIVEBE__
    // This is untested.
    //
    // Don't byteswap, if don't need to byteswap.
    ;
#else
    uint8_t* const bytes = ((uint8_t* const) (pValue));
    const uint8_t wasUpperByte = bytes[0];
    bytes[0] = bytes[1];
    bytes[1] = wasUpperByte;
#endif
}

// Define the addresses of the character sheets.
//
// Bottom of this script file contains a description of character sheet fields/layout.
//
// Character sheet layout is located in SHEETS.S of game source code.
// This matches SHEET.S, with the following exceptions:
//    -SHEET.S does not have a second Gunman Sheet.
//    -SHEET.S does not declare a sheet for CHICKEN
//    -TURBINE, TANK, TURRET, HAND, RED_WATER, GREEN_WATER, BLUE_WATER, follow WATERWHEEL,
//     in game source code.
//
// HOOD_SOLDIER is known as the following, in game souce code:
//    CHARSHEET_HUNTB
//    CSHSwordHunter
// HAIR_SOLDIER is known as the following, in game source code:
//    CHARSHEET_HUNTC
//    CSHOfficerHunter
// GUNMAN1 is known as the following, in the game source code:
//    CHARSHEET_HUNTD:
//    CSHGunHunter:
// DUNDEEs are village people of the village of Dundee
// TURBINE is known as GRINDER, in game source code
// RED, GREEN, BLUE WATER are for red, green blue control panel found twice:
//    in middle of game
//    near end of game
//
// Sheets that were not tied to any world state entry in world state were:
//    Title Screen Sword
//    RAMIREZ
//    MASKED_WOMAN
//    MANGUS
//    ARAK
//    KORTAN
//    CLYDE
//    DUNDEE_B
//    DUNDEE_D
//
// Think polygon forms of Clyde, Dundee_B and Dundee_D show up for a fraction of a second
// in the end game scenes.  Dundee_B and Dundee_D don't move, while Clyde animates for
// quarter second at best, in the end game scenes.  Otherwise, don't think Clyde,
// Dundee B, or Dundee D show up in the game.
//
// Not aware of Ramirez, Masked Woman, Mangus, Arak, or Kortan showing up in the
// game, in realtime polygon form.
//
// Clyde, Dundee B, and Dundee D Items' scene fields contain value of 0x0840.
// Presumably 0x0840 is the end game scene.
//
// If you hack Quentin's world state entry to use Ramirez, Masked Woman, Mangus, Arak, or
// Kortan character sheet, Quentin dressed as Ramirez, Masked Woman, Managus, Arak,
// Kortan can walk around, but not fight.
//
// If you hack Quentin's world state entry to use Clyde, Dundee B, Dundee D character sheet,
// Quentin dressed as Clyde, Dundee B, Dundee D can not walk around, but only rotate.
// Until you arm a weapon...  Then Clyde, Dundee B, Dundee D flop backwards (as if
// hit/defeated) repeatedly, like a possessed rag doll.  Possessed rag doll behavior
// occurs because Clyde, Dundee B, Dundee D only have 3 animations, and when one of their
// character sheets is assigned to quentin one of the first 3 animations is played by
// default, but when Quentin switches to a weapon, the weapons animations are used, and the
// animations depicting hit-action are the first 3 animations that weapon provides.
// So the default animation ends up being an animation for getting hit by fighter, in that case.
// Possessed rag doll behavior may occur when Quentin uses Ramirez, Masked Woman, Mangus, Arak
// or Kortan character sheet, and equips a weapon (did not attempt to do so, so unsure, but seems
// likely).
//
// Define addresses for character sheets which are referenced in the revision of game
// source code.
#define HL_QUENTIN_SHEET_P_ADDR         0x00034A64
#define HL_HOOD_SOLDIER_SHEET_P_ADDR    0x00034B4C
#define HL_HAIR_SOLDIER_SHEET_P_ADDR    0x00034C2C
#define HL_GUNMAN1_SHEET_P_ADDR         0x00034D0C
#define HL_GUNMAN2_SHEET_P_ADDR         0x00034DEC
#define HL_CLAW_SHEET_P_ADDR            0x00034ECC
#define HL_RAMIREZ_SHEET_P_ADDR         0x00034FAC
#define HL_MASKED_WOMAN_SHEET_P_ADDR    0x00035024
#define HL_MANGUS_SHEET_P_ADDR          0x0003509C
#define HL_ARAK_SHEET_P_ADDR            0x00035114
#define HL_KORTAN_SHEET_P_ADDR          0x0003518C
#define HL_CLYDE_SHEET_P_ADDR           0x00035204 
#define HL_DUNDEE_B_SHEET_P_ADDR        0x00035278
#define HL_DUNDEE_D_SHEET_P_ADDR        0x000352EC
#define HL_TURBINE_SHEET_P_ADDR         0x00035360
#define HL_TANK_SHEET_P_ADDR            0x0003539C
#define HL_TURRET_SHEET_P_ADDR          0x000353D0
#define HL_HAND_SHEET_P_ADDR            0x00035404
#define HL_RED_WATER_SHEET_P_ADDR       0x00035434
#define HL_GREEN_WATER_SHEET_P_ADDR     0x00035464
#define HL_BLUE_WATER_SHEET_P_ADDR      0x00035494
#define HL_TITLE_SCREEN_SWORD_SHEET_P_ADDR  0x000354C4
#define HL_SWORD_SHEET_P_ADDR           0x000354EC
#define HL_GAS_GUN_SHEET_P_ADDR         0x00035594
#define HL_CHICKEN_SHEET_P_ADDR         0x0003563C
#define HL_WINE_SHEET_P_ADDR            0x000356E4
#define HL_CHEESE_SHEET_P_ADDR          0x000356F8
#define HL_LOAF_SHEET_P_ADDR            0x0003570C
#define HL_KEY_SHEET_P_ADDR             0x00035720
#define HL_LOCKET_SHEET_P_ADDR          0x00035734
#define HL_WATERWHEEL_SHEET_P_ADDR      0x00035748

#define HL_SWORD_MODEL_ADDR             0x0000EBA0
#define HL_TITLE_SCREEN_SWRD_MODEL_ADDR 0x0000E1F8
#define HL_FLOWER_MODEL_ADDR            0x00014E70
#define HL_GAS_GUN_MODEL_ADDR           0x0000EE98

// More sheet addresses, which are not referenced in the revision of game
// source code.
//
// All of these sheet addresses were located via reference from world state entries
// in World State.
#define HL_CROWBAR_SHEET_P_ADDR     0x0003575C
#define HL_LEVER_SHEET_P_ADDR       0x00035770
#define HL_MAP_SHEET_P_ADDR         0x00035784
#define HL_ORDERS_SHEET_P_ADDR      0x00035798
#define HL_STOPWATCH_SHEET_P_ADDR   0x000357AC
#define HL_STICK_SHEET_P_ADDR       0x000357C0
#define HL_PLANK_SHEET_P_ADDR       0x000357D4
#define HL_UNIFORM_SHEET_P_ADDR     0x000357E8
#define HL_FLOWER_SHEET_P_ADDR      0x000357FC

// NTSC
#define HL_FRAMES_PER_SECOND        60

// Delay costume change until at least the title screen sword has appeared, as the
// title screen sword requires its own Item and Character sheet be in memory.
#define HL_COSTUME_DELAY_SECONDS    6
#define HL_COSTUME_DELAY_TICKS      HL_FRAMES_PER_SECOND * HL_COSTUME_DELAY_SECONDS

#define HL_MAX_FREEZE_LENGTH        60*60

#define INVALID                     -1

enum phasesEnum {WAIT, DELAY, BEGIN, DONE};

// Boilerplate.
static int32_t sOnInputFrameEvent = INVALID;
static int32_t sOnLoadEvent = INVALID;
static int32_t sOnEmuFrame = INVALID;
static int32_t sMcFlags = 0;
static const int32_t skMcFlag_Loaded = (1 << 0);

// Setting handles.
static int sHealthSettingHandle = INVALID;
static int sWeaponsLabelSettingHandle = INVALID;
static int sFreezeSecondsSettingHandle = INVALID;

static int sSwordSettingHandle = INVALID;
static int sGunSettingHandle = INVALID;
static int sChickenSettingHandle = INVALID;
static int sAmmoSettingHandle = INVALID;
static int sTitleSwordSettingHandle = INVALID;
static int sPlankWeaponSettingHandle = INVALID;
static int sCrowbarWeaponSettingHandle = INVALID;
static int sStickWeaponSettingHandle = INVALID;
static int sWaterwheelWeaponSettingHandle = INVALID;
static int sFlowerWeaponSettingHandle = INVALID;
static int sKeysSettingHandle = INVALID;
static int sToolsSettingHandle = INVALID;
static int sMapsSettingHandle = INVALID;
static int sOrdersSettingHandle = INVALID;

static int sPlayAsClawSettingHandle = INVALID;
static int sPlayAsHoodSoldierSettingHandle = INVALID;
static int sPlayAsHairSoldierSettingHandle = INVALID;
static int sPlayAsGunman1SettingHandle = INVALID;
static int sPlayAsGunman2SettingHandle = INVALID;
static int sWalkAsRamirezSettingHandle = INVALID;

static int sModelQuentinAsClawSettingHandle = INVALID;
static int sModelQuentinAsHoodSettingHandle = INVALID;
static int sModelQuentinAsHairSettingHandle = INVALID;
static int sModelQuentinAsRamirezSettingHandle = INVALID;
static int sModelQuentinAsArakSettingHandle = INVALID;
static int sModelQuentinAsKortanSettingHandle = INVALID;
static int sModelQuentinAsMangusSettingHandle = INVALID;
static int sModelQuentinAsMaskedWomanSettingHandle = INVALID;
static int sModelQuentinAsClydeSettingHandle = INVALID;
static int sModelQuentinAsDundeeBSettingHandle = INVALID;
static int sModelQuentinAsDundeeDSettingHandle = INVALID;

// Proof of concept (POC) setting handles
static int sPocModelHoodAsRamirezSettingHandle = INVALID;
static int sPocModelHoodAsArakSettingHandle = INVALID;
static int sPocModelHoodAsKortanSettingHandle = INVALID;
static int sPocModelHoodAsMangusSettingHandle = INVALID;
static int sPocModelHoodAsMaskedWomanSettingHandle = INVALID;
static int sPocModelHoodAsClydeSettingHandle = INVALID;
static int sPocModelHoodAsDundeeBSettingHandle = INVALID;
static int sPocModelHoodAsDundeeDSettingHandle = INVALID;
static int sPocUnusedAnimationSettingHandle = INVALID;

// Work in progress (WIP) settings
static int sWipModelGunAsTurretSettingHandle = INVALID;

static int sItemsInitializedInRAM = 0;
static int sEnemyModelReplacedInRAM = 0;

// As of 2025-09-14, sHlItems only really used for saving fighter mLife state, for when
// put enemy fighters temporarily to sleep.
static HlItem sHlItems[HL_NUM_ITEMS];

// State regarding putting enemy fighters temporarily to sleep.
static uint32_t sAwakenEnemiesCountdown = 0;
static uint32_t sSleepEnemyCounts = 0;

// Note: Warping to Teleporter Cave can be accomplished by simply writing a 1-value to 0x000044AB-0x000044AB.
// The below programmatic invoked key sequences is being used instead, to warp to teleporter cave, for following reasons:
//    1) Below was functional, prior to locating this 0x44AB byte.
//    2) Below provides an example of alternative, kinda practical but hacky way, to port joypad controller cheats to scripts.
//
// Sequence contains 9 key-presses, along with 9 no-key-presses, and
// each key-press and no-key-press needs repeated for 6 frames.
//
// 1, 2, and 3 frame long press/no-presses did not work on my machine.
// 4 and 6 frame long press/no-presses did work on my machine.
#define HL_TELEPORTER_CHEAT_SEQ_LENGTH  (9*2*6)
#define HL_NO_KEY                       0xFF

// Attempted this sequence without duplication of keys and no-keys, with no luck.
// Its an ugly approach, but less risky then adding another counter variable
// that is maintained across method invocations.
// TODO: Clean up a bit, by removing the duplicate entries, and just have process loop do the duplication based on repeatKey variable value.
static const uint8_t sTeleporterCheatSequence[HL_TELEPORTER_CHEAT_SEQ_LENGTH] = {
    JAG_BUTTON_2,
    JAG_BUTTON_2,
    JAG_BUTTON_2,
    JAG_BUTTON_2,
    JAG_BUTTON_2,
    JAG_BUTTON_2,
    HL_NO_KEY,
    HL_NO_KEY,
    HL_NO_KEY,
    HL_NO_KEY,
    HL_NO_KEY,
    HL_NO_KEY,
    JAG_BUTTON_1,
    JAG_BUTTON_1,
    JAG_BUTTON_1,
    JAG_BUTTON_1,
    JAG_BUTTON_1,
    JAG_BUTTON_1,
    HL_NO_KEY,
    HL_NO_KEY,
    HL_NO_KEY,
    HL_NO_KEY,
    HL_NO_KEY,
    HL_NO_KEY,
    JAG_BUTTON_8,
    JAG_BUTTON_8,
    JAG_BUTTON_8,
    JAG_BUTTON_8,
    JAG_BUTTON_8,
    JAG_BUTTON_8,
    HL_NO_KEY,
    HL_NO_KEY,
    HL_NO_KEY,
    HL_NO_KEY,
    HL_NO_KEY,
    HL_NO_KEY,
    JAG_BUTTON_8,
    JAG_BUTTON_8,
    JAG_BUTTON_8,
    JAG_BUTTON_8,
    JAG_BUTTON_8,
    JAG_BUTTON_8,
    HL_NO_KEY,
    HL_NO_KEY,
    HL_NO_KEY,
    HL_NO_KEY,
    HL_NO_KEY,
    HL_NO_KEY,
    JAG_BUTTON_3,
    JAG_BUTTON_3,
    JAG_BUTTON_3,
    JAG_BUTTON_3,
    JAG_BUTTON_3,
    JAG_BUTTON_3,
    HL_NO_KEY,
    HL_NO_KEY,
    HL_NO_KEY,
    HL_NO_KEY,
    HL_NO_KEY,
    HL_NO_KEY,
    JAG_BUTTON_5,
    JAG_BUTTON_5,
    JAG_BUTTON_5,
    JAG_BUTTON_5,
    JAG_BUTTON_5,
    JAG_BUTTON_5,
    HL_NO_KEY,
    HL_NO_KEY,
    HL_NO_KEY,
    HL_NO_KEY,
    HL_NO_KEY,
    HL_NO_KEY,
    JAG_BUTTON_3,
    JAG_BUTTON_3,
    JAG_BUTTON_3,
    JAG_BUTTON_3,
    JAG_BUTTON_3,
    JAG_BUTTON_3,
    HL_NO_KEY,
    HL_NO_KEY,
    HL_NO_KEY,
    HL_NO_KEY,
    HL_NO_KEY,
    HL_NO_KEY,
    JAG_BUTTON_8,
    JAG_BUTTON_8,
    JAG_BUTTON_8,
    JAG_BUTTON_8,
    JAG_BUTTON_8,
    JAG_BUTTON_8,
    HL_NO_KEY,
    HL_NO_KEY,
    HL_NO_KEY,
    HL_NO_KEY,
    HL_NO_KEY,
    HL_NO_KEY,
    JAG_BUTTON_9,
    JAG_BUTTON_9,
    JAG_BUTTON_9,
    JAG_BUTTON_9,
    JAG_BUTTON_9,
    JAG_BUTTON_9,
    HL_NO_KEY,
    HL_NO_KEY,
    HL_NO_KEY,
    HL_NO_KEY,
    HL_NO_KEY,
    HL_NO_KEY
};
static int sTeleporterCheatCursor = -1;
static uint64_t sFrameCount = 0;

static int sCharacterModelsInitialized = 0;

static enum phasesEnum sCostumeEnum = DONE;
static int sCostumeDelayCounter = -1;

static uint32_t weaponCharSheets[3] = {HL_SWORD_SHEET_P_ADDR, HL_GAS_GUN_SHEET_P_ADDR, HL_CHICKEN_SHEET_P_ADDR};
static uint32_t weaponLastAnimAddrs[3] = {0, 0, 0};

#define HL_WORLD_STATE_TABLE_ADDR               0x00032660
#define HL_WORLD_STATE_TABLE_END_ADDR           0x00033F00 - 1
#define HL_CHARACTER_SHEETS_ADDR                0x00034A64
#define HL_CHARACTER_SHEETS_END_ADDR            0x00035810 - 1
#define HL_DRAW_DATA_AREA_START_ADDR            0x0003CCC0
#define HL_QUENTIN_DRAW_DATA_AREA_START_ADDR    0x0003CEA0
#define HL_CHARACTER_INSTANCE_TABLE_START_ADDR  0x0003DCC0

#define HL_RAM_BLOCK_SIZE           448
#define HL_CD_BLOCK_START_ADDR      0x000559B6
#define HL_TRACK_8_BLOCK_START_ADDR 0x0005598E
#define HL_CD_BLOCK_SIZE            2352
#define HL_CD_NUM_BLOCKS_PER_READ   56

// Compact disc Track5 block offsets, to characters models/animations/logics (but not WAVEs)
#define HL_QUENTIN_BLOCK_OFFSET     0x0000
#define HL_HOOD_BLOCK_OFFSET        0x01C0
#define HL_HAIR_BLOCK_OFFSET        0x0230
#define HL_GUNMAN1_BLOCK_OFFSET     0x02A0
#define HL_GUNMAN2_BLOCK_OFFSET     0x0310
#define HL_CLAW_BLOCK_OFFSET        0x0380
#define HL_ARAK_BLOCK_OFFSET        0x03F0
#define HL_KORTAN_BLOCK_OFFSET      0x0428
#define HL_MANGUS_BLOCK_OFFSET      0x0460
#define HL_RAMIREZ_BLOCK_OFFSET     0x0498
#define HL_MASKED_WOMAN_BLOCK_OFFSET    0x04D0
#define HL_CLYDE_BLOCK_OFFSET       0x0508
#define HL_DUNDEE_B_BLOCK_OFFSET    0x0540
#define HL_DUNDEE_D_BLOCK_OFFSET    0x0578
#define HL_TURBINE_BLOCK_OFFSET     0x05B0
#define HL_TANK_BLOCK_OFFSET        0x05E8
#define HL_TURRET_BLOCK_OFFSET      0x0620
#define HL_HAND_QUES_BLOCK_OFFSET   0x0658

// Height offsets to be applied to animations, when copy fighters model data over quentin model data.
#define HL_NO_HEIGHT_OFFSET         0
#define HL_CLAW_HEIGHT_OFFSET       65
#define HL_HOOD_HEIGHT_OFFSET       15
#define HL_HAIR_HEIGHT_OFFSET       25
#define HL_RAMIREZ_HEIGHT_OFFSET    50
#define HL_MANGUS_HEIGHT_OFFSET     30
#define HL_ARAK_HEIGHT_OFFSET       40
#define HL_KORTAN_HEIGHT_OFFSET     60
#define HL_MASKED_WOMAN_HEIGHT_OFFSET   20
#define HL_CLYDE_HEIGHT_OFFSET      -130
#define	HL_DUNDEE_B_HEIGHT_OFFSET   30
#define HL_DUNDEE_D_HEIGHT_OFFSET   15

enum FightersEnum {QUENTIN, HOOD, HAIR, GUNMAN1, GUNMAN2, CLAW, // Fighters used in production version of game.  Quentin has 30 animations, the rest have 28 animations.
                   ARAK, KORTAN,                                // Enemy fighters not used in production version, but game hides model along with 4 animations
                   MANGUS, RAMIREZ, MASKED_WOMAN,               // Friendly fighters not used in production version, but game hides model along with 4 animations
                   CLYDE, DUNDEE_B, DUNDEE_D,                   // Friendly fighters not used in production version, but game hides model along with 3 animations
                   TURBINE, TANK, TURRET,                       // Enemy object fighters/obstructions.
                   HAND,                                        //
                   NUM_FIGHTERS};                               //

// This is the number of facets which are needed for right hand model of Ramirez, Arak, and
// Kortan, Hood, Hair, Claw, Gunman 1 and Gunman 2, if you don't want 
// sword/walking-stick/gun displayed.
#define HL_NUMBER_FACETS_IN_RIGHT_HAND_MODEL_WITHOUT_WEAPON 24

typedef struct
{
    uint32_t mBlockOffset;
    int mSupportPlayAs;             // Apparently bool data type is not suported by scripting system.
    int mSupportModelAs;
    int mSupportTitleScreenSword;
    int mHeightOffset;
} FighterHackAttributes;

typedef struct
{
    uint32_t modelLength;
} RomMetadata;

typedef struct
{
    uint32_t numBlocks;
    uint32_t pCharacterSheetPtr;
} RamMetadata;

// Model Header structure.
// This prevents ModelHeader from being bytewise serialize/deserialize safe, due to some fields being declared as 32 bit, when the actual format in ROM and RAM is 16 bit.
// This is not sizeof safe for determining size of model header..
//
// Note1: this is really 16 bits, but padding to 32 bits to keep whitehouse's bs 16-bit-assignment-clobbers-memory(sometimes-randomly) bug at bey
typedef struct
{
    uint32_t modelLength;     // ********* See Note1.  2 byte model length in bytes
    uint8_t  originNumber;
    uint8_t  numOrigins;      // Note: Right hand model has value of 1, and left hand model has value of 0, since right hand can hold/articulate a weapon, while left hand cannot hold/articulate a weapon.
    uint32_t numVertices;     // ********* See Note1.  2 byte number of vertices.
    uint32_t numFacets;       // ********* See Note1.  2 byte number of facets.
    uint32_t pVerticesStart;  // Pointer to variable length list of vertices and origin/vertices (the vertices and origin/vertices are defined in this model).  (8 bytes per vertice / origin/vertice
    uint32_t pFacetsStart;    // Pointer to variable length list of facets (the facets are defined in this model)
    uint32_t pSaveoutsStart;  // Pointer to "saveout" vertices ("saveout" vertices are "defined" in this model)
    uint32_t pDunnoStart;     // Pointer to dunno.  Perhaps always null.  One comment in 3DENGINE.GAS referes to CLP.  Never saw this populated.
} ModelHeader;

typedef struct
{
    uint8_t modelData[38*HL_RAM_BLOCK_SIZE];
} ModelData;

typedef struct
{
    ModelHeader header;
    ModelData data;
} Model;

// Size of model header in RAM and ROM.  Doesn't include the padding of 2 byte values to 4 byte values, which was needed for structure bug workaround.
#define HL_SIZE_OF_MODEL_HEADER         24

#define HL_SIZE_OF_ROM_METADATA         4
#define HL_SIZE_OF_RAM_METADATA         8
#define HL_SIZE_OF_VERTICE              8
#define HL_SIZE_OF_FACET                16
#define HL_SIZE_OF_CHAR_SHEET_HEADER    16
#define HL_SIZE_OF_POINTER              4
#define HL_SIZE_OF_FILE_REF             8
#define HL_NUM_MODELS_PER_FIGHTER       15
#define HL_NUM_ANIMATIONS_QUENTIN       30
#define HL_NUM_MISC_FIGHTER             5
#define HL_NUM_FILE_REFS_QUENTIN        2
#define HL_LENGTH_QUENTIN_CHAR_SHEET    (HL_SIZE_OF_CHAR_SHEET_HEADER + ((HL_NUM_MODELS_PER_FIGHTER + HL_NUM_ANIMATIONS_QUENTIN + HL_NUM_MISC_FIGHTER) * HL_SIZE_OF_POINTER) + (HL_NUM_FILE_REFS_QUENTIN * HL_SIZE_OF_FILE_REF))
#define HL_NUM_FILE_REFS_OFFSET         13
#define HL_NUM_MODELS_PER_WEAPON        1
#define HL_SIZE_OF_CHAR_INSTANCE_ENTRY  48
#define HL_SIZE_OF_DRAW_DATA_AREA_ENTRY 16

static enum FightersEnum mFighter = QUENTIN;

void initFighterHackAttributes(FighterHackAttributes* pAttr, uint32_t blockOffset, int supportPlayAs, int supportModelAs, int supportTitleScreenSword, int heightOffset)
{
    if (0 != pAttr)
    {
        pAttr->mBlockOffset = blockOffset;
        pAttr->mSupportPlayAs = supportPlayAs;
        pAttr->mSupportModelAs = supportModelAs;
        pAttr->mSupportTitleScreenSword = supportTitleScreenSword;
        pAttr->mHeightOffset = heightOffset;
    }
};

// Allocate array of fighter hack attributes.
static FighterHackAttributes gFighterHackAttributes[NUM_FIGHTERS];

// Creates test animation sheet for Track8 animations discovery.
//
// Valid block offsets and joypad controls to invoke are documented
// in the Animations Layout section.
uint32_t createTestSheet(uint32_t blockOffset)
{
    // Copy Quentin character sheet, to first free memory spot beyond the last character sheet.
    const uint32_t baseSource = HL_QUENTIN_SHEET_P_ADDR;
    const uint32_t lengthSourceCharSheet = HL_LENGTH_QUENTIN_CHAR_SHEET;
    const uint32_t baseDest = HL_FLOWER_SHEET_P_ADDR + HL_ITEM_SIZE;
    for (uint32_t copyIdx = 0; copyIdx < lengthSourceCharSheet; copyIdx += sizeof(uint32_t))
    {
        const uint32_t value32 = bigpemu_jag_read32(baseSource+copyIdx);
        bigpemu_jag_write32(baseDest+copyIdx, value32);
    }

    // Null out the ptr-to-next-character-sheet in destination character sheet, as this is the new end node in linked list of character sheets.
    bigpemu_jag_write32(baseDest, 0x00000000);

    // Append an additional file reference onto end of character sheet.  Set the additional
    // file reference to overwrite the 19th and forward animations, with animations
    // at the specified block offset on Track8(1-based).
    //
    // Specify entry point and track number.
    const uint32_t entryPoint = HL_NUM_MODELS_PER_FIGHTER + 19; // Start at 19th animation ptr.
    const uint32_t trackNumber = 6; //(Track 8 (1-based))
    bigpemu_jag_write32(baseDest + lengthSourceCharSheet, (entryPoint << 16) | trackNumber);
    //
    // Specify block offset.
    // The following block offsets are valid for Track8(1-based):
    //     0x0000, 0x0038, 0x0070, 0x00A8, 0x00E0, 0x0118, 0x0150, 0x0188, 0x01C0, 0x01F8)
    bigpemu_jag_write32(baseDest + lengthSourceCharSheet + 4, blockOffset);

    // Increment the number-of-file-references for the new/replacement character sheet.
    uint8_t numFileReferences = bigpemu_jag_read8(baseDest + HL_NUM_FILE_REFS_OFFSET);
    numFileReferences += 1;
    bigpemu_jag_write8(baseDest + HL_NUM_FILE_REFS_OFFSET, numFileReferences);

    // Set previous last character sheet to point to new character sheet.
    bigpemu_jag_write32(HL_FLOWER_SHEET_P_ADDR, baseDest);

    // Return address of new test sheet.
    return baseDest;
}

// Copy facets and sword vertices from right hand model to macleod sword model.
//
// Assumptions:
//    All relevant facets and sword vertices will be contained in the first block of the right hand model.
//    Rely on numFacets being provided as an argument to this method.
//    The calling method will reduce the num facets field value to equal the number of facets excluding the weapon facets, even though the model will still retain the weapon facets.
//    By specifying numFacets as method argument, this method can be called before or after number of facets is decreased to exclude the display of weapon facets.

void copySwordToMacleodSwordModel(uint8_t* pModel, uint32_t numVerticeBytes, uint32_t numFacets)
{
    // Copy sword data from right hand model to macleod sword model.
    // This way, when enabling the sword, the sword model data associated to this
    // model will be displayed, instead of macleod sword model data.

    // macleod sword has 21 vertices and 34 facets = 21*8 + 34*16 = 168 + 544 = 712 bytes available to overwrite
    //
    //    claw right hand model had 20 vertices and 38 facets - need room for 20 vertices and 14 facets = 160 + 224 = 384
    //    arak right hand model had 31 vertices and 48 facets - need room for 31 vertices and 23 facets = 248 + 368 = 616
    //    hood right hand model had 31 vertices and 48 facets - need room for 31 vertices and 24 facets = 248 + 384 = 632
    //  kortan right hand model had 31 vertices and 48 facets - need room for 31 vertices and 24 facets = 248 + 384 = 632
    //    hair right hand model had 31 vertices and 49 facets - need room for 31 vertices and 25 facets = 248 + 400 = 648
    // ramirez right hand model had 33 vertices and 51 facets - need room for 33 vertices and 27 facets = 264 + 432 = 696
    //
    // looks like ramirez walking-stick squeaks into space of sword model, with 16 bytes to spare

    // Copy vertices from right hand model to Macleod sword model data.
    // Copy all those vertices, even if not referenced by the facets
    // representing the weapon, so that facets can use the same vertice ids/positions.
    uint32_t weaponDestAddr = HL_SWORD_MODEL_ADDR + HL_SIZE_OF_MODEL_HEADER;
    const uint32_t destVerticesAddr = weaponDestAddr;
    uint32_t skipBytes = 0;
    for (uint32_t byteIdx = skipBytes; byteIdx < (numVerticeBytes + skipBytes); byteIdx += HL_SIZE_OF_VERTICE)
    {
        bigpemu_jag_write8(weaponDestAddr+0, pModel[byteIdx+0]);
        bigpemu_jag_write8(weaponDestAddr+1, pModel[byteIdx+1]);

        // This help in y considerably.  need to move in other dimension as well to get into hand
        int32_t yVert = ((pModel[byteIdx+2])<<8) & pModel[byteIdx+3];
        yVert += 16;
        pModel[byteIdx+2] = yVert >> 8;
        pModel[byteIdx+3] = 0xFF & yVert;

        bigpemu_jag_write8(weaponDestAddr+2, pModel[byteIdx+2]);
        bigpemu_jag_write8(weaponDestAddr+3, pModel[byteIdx+3]);

        bigpemu_jag_write8(weaponDestAddr+4, pModel[byteIdx+4]);
        bigpemu_jag_write8(weaponDestAddr+5, pModel[byteIdx+5]);

        bigpemu_jag_write8(weaponDestAddr+6, pModel[byteIdx+6]);
        bigpemu_jag_write8(weaponDestAddr+7, pModel[byteIdx+7]);
        weaponDestAddr += HL_SIZE_OF_VERTICE;
    }

    const uint32_t destFacetsAddr = weaponDestAddr;

    // Number of bytes for the vertices.
    skipBytes += numVerticeBytes;

    // Number of facets that remain displayed in right hand model
    skipBytes += (HL_NUMBER_FACETS_IN_RIGHT_HAND_MODEL_WITHOUT_WEAPON * HL_SIZE_OF_FACET);

    const uint32_t numFacetsForWeapon = numFacets - HL_NUMBER_FACETS_IN_RIGHT_HAND_MODEL_WITHOUT_WEAPON;

    // Copy facets for weapon, from right hand model above, to Macleod sword model data.
    // Since all vertices for right hand model were copied to MacLeod sword model data,
    // the vertice ids referenced by facets will still be valid.
    for (uint32_t byteIdx = skipBytes; byteIdx < skipBytes + (numFacetsForWeapon * HL_SIZE_OF_FACET); ++byteIdx)
    {
        bigpemu_jag_write8(weaponDestAddr, pModel[byteIdx]);
        ++weaponDestAddr;
    }

    // Update the Macleod sword model's header data.
    const uint32_t modelLength = HL_SIZE_OF_MODEL_HEADER + numVerticeBytes + (numFacetsForWeapon * HL_SIZE_OF_FACET);

    bigpemu_jag_write8(HL_SWORD_MODEL_ADDR, ((modelLength >> 8) & 0xFF));
    bigpemu_jag_write8(HL_SWORD_MODEL_ADDR + 1, (modelLength & 0xFF));
    bigpemu_jag_write8(HL_SWORD_MODEL_ADDR + 5, numVerticeBytes / HL_SIZE_OF_VERTICE);
    bigpemu_jag_write8(HL_SWORD_MODEL_ADDR + 7, numFacetsForWeapon);
    bigpemu_jag_write32(HL_SWORD_MODEL_ADDR + 8, destVerticesAddr);
    bigpemu_jag_write32(HL_SWORD_MODEL_ADDR + 12, destFacetsAddr);

    // Write 2 words zero padding and then "SWORD~" to last 6 bytes of world state.  ~ is treated as string terminator.
    bigpemu_jag_write32(0x00034658, 0x00005357);
    bigpemu_jag_write32(0x0003465C, 0x4F52447E);
    const const uint32_t swordTextAddr = 0x0003465A;

    // Update the 16 bytes immediately following the end of model data with text pointers for the sword.
    // <text id><english text ptr><french text ptr><german text pointer>
    bigpemu_jag_write32(HL_SWORD_MODEL_ADDR + modelLength, 0x00000000);
    bigpemu_jag_write32(HL_SWORD_MODEL_ADDR + modelLength + 4, swordTextAddr);
    bigpemu_jag_write32(HL_SWORD_MODEL_ADDR + modelLength + 8, swordTextAddr);
    bigpemu_jag_write32(HL_SWORD_MODEL_ADDR + modelLength + 12, swordTextAddr);
    // NOTE1: If writing text to end of memory reserved for world state causes any problems,
    //        update the above to just write nulls for the pointers, and Options Menu will
    //        display no text instead.
    // NOTE2: In general, when overwriting these models which have text pointers following them,
    //        make certain you have text pointers pointing to valid '~' termintated text or have
    //        have text pointers set to null.  Otherwise 'garbage' will be displayed for the text,
    //        and you run the very real risk of crashing Jaguar/emulator if not null and no
    //        text terminating tilde is found soon enough.
};

// The Title Screen Sword is not available for view/display/use in this game, outside of the Title Screen, unless this script is utilized to do so.
// This script provides an option in BigPEmu Script Settings to equip the Title Screen Sword in lieu of the Rubber Chicken weapon.
// This script implements that by setting Rubber Chicken's World State Item's character sheet to point to the Title Screen Sword character sheet, instead of the Rubber Chicken character sheet.
// Character sheets point to the model (3d model defining vertices/facets/colors) data used for display.
//
// The Title Screen Sword is not usable for fighters which have their sword vertices/facets embedded into right hand model.
// This script copies those fighters embedded sword vertices/facets over the Macleod Sword model, so that user can select to equip/unequip the fighter characters correct/non-macleod sword.
//
// The Title Screen Sword's model is located in memory immediateley before the Macleod Sword model, with ZERO bytes in between the two models.
// Other selected items' models which appear in the Options screen (including the Macleod Sword model), have 24 bytes following their model (and immediately before the next model).
// Those 24 bytes have 3 different pointers to english text name of item, french text name of item, and german text name of item.
// The item name texts are variable length, terminated by a '~' character.
// The item name text is displayed underneath the item, in the Options screen.
//
// When the Title Screen Sword is displayed in the Options dialog, the Options dialog ends up using 4 bytes in the Macleod Sword model's header as the address to lookup text name of the Title Screen Sword.
// The Options Screen, reads and displays characters from that address until it comes upon a '~' character.
// ***Looks strongly like*** the encountered '~' character is not to far away from the "address" that was read from Macleod Sword model.
// However, when copying other fighter characters' model vertices/facets/header-info over the Macleod Sword Model, that "address" does not "point" to a memory location with a '~' character reasonably close, and then game hangs up in infinite-loop, stack-overflow, something bad.
//
// Using Title Screen Sword was implemented two weeks prior to this showing up, and was not fun to figure out.
// Correctly/intentionally overwriting Macleod Sword model blows up model display of Title Screen Sword that had been functioning for 2 weeks.
//
// Title screen sword usage only made available to Model As <Character> settings.
// The Play As <Character> settings will never overwrite the Macleod Sword model data.
void orientTitleScreenSword(void)
{
    // swap vertices to orient sword to correct position when in hand
    // nudge some vertices to position sword correctly into hand
    // start vertices = HL_TITLE_SCREEN_SWRD_MODEL_ADDR + HL_SIZE_OF_MODEL_HEADER
    const uint32_t modelStartAddr = HL_TITLE_SCREEN_SWRD_MODEL_ADDR;
    const uint32_t numVertices = ((uint32_t) (bigpemu_jag_read8(modelStartAddr + 5)));
    for (uint32_t vertIdx = 0; vertIdx < numVertices; ++vertIdx)
    {
        uint8_t xHigh = bigpemu_jag_read8(modelStartAddr + HL_SIZE_OF_MODEL_HEADER + (vertIdx * HL_SIZE_OF_VERTICE) + 0);
        uint8_t xLow  = bigpemu_jag_read8(modelStartAddr + HL_SIZE_OF_MODEL_HEADER + (vertIdx * HL_SIZE_OF_VERTICE) + 1);
        uint8_t zHigh = bigpemu_jag_read8(modelStartAddr + HL_SIZE_OF_MODEL_HEADER + (vertIdx * HL_SIZE_OF_VERTICE) + 4);
        uint8_t zLow  = bigpemu_jag_read8(modelStartAddr + HL_SIZE_OF_MODEL_HEADER + (vertIdx * HL_SIZE_OF_VERTICE) + 5);

        const uint8_t tmpHigh = xHigh;
        const uint8_t tmpLow = xLow;

        xHigh = zHigh;
        xLow = zLow;
        zHigh = tmpHigh;
        zLow = tmpLow;

        int32_t value = (zHigh << 8) | zLow;
        value += 40;
        zHigh = value >> 8;
        zLow = value & 0xFF;

        bigpemu_jag_write8(modelStartAddr + HL_SIZE_OF_MODEL_HEADER + (vertIdx * HL_SIZE_OF_VERTICE) + 0, xHigh);
        bigpemu_jag_write8(modelStartAddr + HL_SIZE_OF_MODEL_HEADER + (vertIdx * HL_SIZE_OF_VERTICE) + 1, xLow);
        bigpemu_jag_write8(modelStartAddr + HL_SIZE_OF_MODEL_HEADER + (vertIdx * HL_SIZE_OF_VERTICE) + 4, zHigh);
        bigpemu_jag_write8(modelStartAddr + HL_SIZE_OF_MODEL_HEADER + (vertIdx * HL_SIZE_OF_VERTICE) + 5, zLow);
    }
}

void orientFlower(void)
{
    // swap vertices to orient flower to correct position when in hand
    // nudge some vertices to position flower correctly into hand
    const uint32_t modelStartAddr = HL_FLOWER_MODEL_ADDR;
    const uint32_t numVertices = ((uint32_t) (bigpemu_jag_read8(modelStartAddr + 5)));
    for (uint32_t vertIdx = 0; vertIdx < numVertices; ++vertIdx)
    {
        uint8_t yHigh = bigpemu_jag_read8(modelStartAddr + HL_SIZE_OF_MODEL_HEADER + (vertIdx * HL_SIZE_OF_VERTICE) + 2);
        uint8_t yLow  = bigpemu_jag_read8(modelStartAddr + HL_SIZE_OF_MODEL_HEADER + (vertIdx * HL_SIZE_OF_VERTICE) + 3);
        uint8_t zHigh = bigpemu_jag_read8(modelStartAddr + HL_SIZE_OF_MODEL_HEADER + (vertIdx * HL_SIZE_OF_VERTICE) + 4);
        uint8_t zLow  = bigpemu_jag_read8(modelStartAddr + HL_SIZE_OF_MODEL_HEADER + (vertIdx * HL_SIZE_OF_VERTICE) + 5);

        const uint8_t tmpHigh = yHigh;
        const uint8_t tmpLow = yLow;

        yHigh = zHigh;
        yLow = zLow;
        zHigh = tmpHigh;
        zLow = tmpLow;

        int32_t value = (yHigh << 8) | yLow;
        value += 40;
        yHigh = value >> 8;
        yLow = value & 0xFF;

        bigpemu_jag_write8(modelStartAddr + HL_SIZE_OF_MODEL_HEADER + (vertIdx * HL_SIZE_OF_VERTICE) + 2, yHigh);
        bigpemu_jag_write8(modelStartAddr + HL_SIZE_OF_MODEL_HEADER + (vertIdx * HL_SIZE_OF_VERTICE) + 3, yLow);
        bigpemu_jag_write8(modelStartAddr + HL_SIZE_OF_MODEL_HEADER + (vertIdx * HL_SIZE_OF_VERTICE) + 4, zHigh);
        bigpemu_jag_write8(modelStartAddr + HL_SIZE_OF_MODEL_HEADER + (vertIdx * HL_SIZE_OF_VERTICE) + 5, zLow);

        zHigh = bigpemu_jag_read8(modelStartAddr + HL_SIZE_OF_MODEL_HEADER + (vertIdx * HL_SIZE_OF_VERTICE) + 4);
        zLow  = bigpemu_jag_read8(modelStartAddr + HL_SIZE_OF_MODEL_HEADER + (vertIdx * HL_SIZE_OF_VERTICE) + 5);
        value = (zHigh << 8) | zLow;
        value += 65;
        zHigh = value >> 8;
        zLow = value & 0xFF;
        bigpemu_jag_write8(modelStartAddr + HL_SIZE_OF_MODEL_HEADER + (vertIdx * HL_SIZE_OF_VERTICE) + 4, zHigh);
        bigpemu_jag_write8(modelStartAddr + HL_SIZE_OF_MODEL_HEADER + (vertIdx * HL_SIZE_OF_VERTICE) + 5, zLow);
    }
}

// World State Items point to Character Sheet.
// Character Sheet points to models, animations, miscellaneous, and "files".
//
// There are character sheets for Ramirez, Kortain, Arak, Mangus, Masked Woman, Clyde, Dundee B, Dundee D which are not pointed to by any World State Items.
// These character sheets have only 3-4 animations, versus 28 for attackers, and 30 for Quentin.
//
// Want to be able to utilize some of the character sheet models.
// For now, want to assign one of these character sheet's models to Quentin's character sheet.
//
// You can assign one of these character sheets to Quentin's World State Item, but then you only get 3-4 animations, and can turn left, turn right, stand, and possibly walk.
//
// You can shallow copy Quentins character sheet model pointers to point to a different World State Items' character sheet's models, but then that World State Item has to be active within the scene/set.  If not, will end up pointing to null data or 'garbage'.
// Made for a reasonable proof of concept, to assign Quentin's character sheet pointers to point to Hood Soldiers character sheets' models, but once player went off screen from that particular Hood Soldier, pointers went null, and crash/freeze/whatever of game.
//
// So real solution is a deep copy of model data over Quentin's model data.
//
// All people characters are comprised of 15 models.
// Each model, when stored in Jaguar memory, is comprised of blocks of size HL_RAM_BLOCK_SIZE bytes.
// Character sheet model pointer values actually point to 8 bytes into the first block of each model, and the first 8 actual bytes store number of blocks in model as a 32 bit value, and then the location of the character sheet pointer to this model is stored as 32 bit value.
//
// Animations immediately follow the models.
// This presents an extra hurdle when the source model set comprises more blocks then the target model set.

//Looks OBE.
//void byteswap(uint8_t* pStart, uint32_t length)
//{
//  uint8_t* end = pStart + length;
//  while (pStart < end)
//  {
//      uint8_t tmp = *pStart;
//      *pStart = *(pStart + 1);
//      *(pStart + 1) = tmp;
//      pStart += 2;
//  }
//}

int readModelsFromCompactDisc(enum FightersEnum fighter, Model models[], int numModels)
{
    const uint32_t requestedBlockOffset = gFighterHackAttributes[fighter].mBlockOffset;

    //TODO: Remove hard coded path
    //      Possibly just find better way.
    //      BigPEmu currently seems to restrict read locations.
    //      Would like to just read from the same location that BigPEmu reads compact disc images files from.
    const uint64_t trackFileHandle = fs_open(".\\Highlander\\Highlander - The Last of the MacLeods (USA) (Track 5).bin", 0);
    printf_notify("trackFileHandle is: %u\n\n\n", trackFileHandle);

    if (0 == trackFileHandle)
    {
        return 0;
    }

    // Read file blocks, up through the requested block.
    uint8_t trackData[HL_CD_BLOCK_START_ADDR + 64]; //HL_CD_BLOCK_START_ADDR is longest stretch of memory will need to read (and then add 64 bytes just to be "safe"/superstitious);
    const uint64_t bytesRead = fs_read(trackData, HL_CD_BLOCK_START_ADDR, trackFileHandle);
    uint32_t cursorBlockOffset = 0;
    while (cursorBlockOffset <= requestedBlockOffset)
    {
        const uint64_t bytesRead2 = fs_read(trackData, HL_CD_BLOCK_SIZE * HL_CD_NUM_BLOCKS_PER_READ, trackFileHandle);
        cursorBlockOffset += HL_CD_NUM_BLOCKS_PER_READ;
    }
    fs_close(trackFileHandle);

    const uint8_t* pTrackIter = trackData;
    RomMetadata romMetadata[HL_NUM_MODELS_PER_FIGHTER];

    // Copy track data into arrays of data structures.
    for (uint32_t modelIdx = 0; modelIdx < numModels; ++modelIdx)
    {
        // The models on compact disc track appear to have 4 bytes of metadata, then a header, and then data.

        // Model length does not include the metadata.
        romMetadata[modelIdx].modelLength = (*(pTrackIter + 1) << 24) | (*(pTrackIter + 0) << 16) |
                                            (*(pTrackIter + 3) << 8)  | (*(pTrackIter + 2));
        pTrackIter += 4;

        // model length fields are present in both the metadata and the header.
        models[modelIdx].header.modelLength =  (*(pTrackIter + 1) << 8) | (*(pTrackIter + 0));
        pTrackIter += 2;

        models[modelIdx].header.originNumber   = *(pTrackIter + 1);
        models[modelIdx].header.numOrigins     = *pTrackIter;
        pTrackIter += 2;
        models[modelIdx].header.numVertices    = (*(pTrackIter + 1) << 8) | (*(pTrackIter + 0));
        pTrackIter += 2;
        models[modelIdx].header.numFacets      = (*(pTrackIter + 1) << 8) | (*(pTrackIter + 0));
        pTrackIter += 2;
        models[modelIdx].header.pVerticesStart = (*(pTrackIter + 1) << 24) | (*(pTrackIter + 0) << 16) |
                                                 (*(pTrackIter + 3) << 8)  | (*(pTrackIter + 2));
        pTrackIter += 4;
        models[modelIdx].header.pFacetsStart   = (*(pTrackIter + 1) << 24) | (*(pTrackIter + 0) << 16) |
                                                 (*(pTrackIter + 3) << 8)  | (*(pTrackIter + 2));
        pTrackIter += 4;
        models[modelIdx].header.pSaveoutsStart = (*(pTrackIter + 1) << 24) | (*(pTrackIter + 0) << 16) |
                                                 (*(pTrackIter + 3) << 8)  | (*(pTrackIter + 2));
        pTrackIter += 4;
        models[modelIdx].header.pDunnoStart    = (*(pTrackIter + 1) << 24) | (*(pTrackIter + 0) << 16) |
                                                 (*(pTrackIter + 3) << 8)  | (*(pTrackIter + 2));
        pTrackIter += 4;

        // Copy the byteswapped (at word level) track supplied model header and model data (including the already read model length).
        for (uint32_t modelDataIdx = 0; modelDataIdx < (models[modelIdx].header.modelLength - HL_SIZE_OF_MODEL_HEADER); modelDataIdx += 2)
        {
            models[modelIdx].data.modelData[modelDataIdx + 1] = *(pTrackIter++);
            models[modelIdx].data.modelData[modelDataIdx + 0] = *(pTrackIter++);
        }
    }

    return 1;
}

void getTargetLocations(Model models[], uint32_t targetCharacterSheetAddr, uint32_t* pStartAddr, uint32_t* pResumeAddr, uint32_t* pNumModelsThatFit)
{
    if (0 == pStartAddr || 0 == pResumeAddr || 0 == pNumModelsThatFit)
    {
        return;
    }

    // Determine first and last addresses of target model data.
    const uint32_t targetModelPtrsAddr = targetCharacterSheetAddr + HL_SIZE_OF_CHAR_SHEET_HEADER;
    const uint32_t targetAnimationPtrsAddr = targetModelPtrsAddr + (HL_NUM_MODELS_PER_FIGHTER*HL_SIZE_OF_POINTER);
    const uint32_t targetFirstModelAddr = bigpemu_jag_read32(targetModelPtrsAddr);
    const uint32_t targetFirstAnimationAddr = bigpemu_jag_read32(targetAnimationPtrsAddr);

    // Determine number of bytes in target model area.
    const uint32_t numTargetModelBytes = targetFirstAnimationAddr - targetFirstModelAddr;

    // Determine number of blocks in target model area.
    const uint32_t numTargetBlocks = numTargetModelBytes / HL_RAM_BLOCK_SIZE;

    // Determine how many blocks are in each of the source/costume models.
    // Number of blocks for model is 1st and 2nd bytes in model
    uint32_t numCostumeModelBlocks[HL_NUM_MODELS_PER_FIGHTER];
    uint64_t totalNumCostumeModelBlocks = 0;
    for (uint32_t modelIdx = 0; modelIdx < HL_NUM_MODELS_PER_FIGHTER; ++modelIdx)
    {
        const uint32_t modelLength = models[modelIdx].header.modelLength;
        const uint32_t remainder = modelLength % HL_RAM_BLOCK_SIZE;
        uint32_t numBlocksForModel = modelLength / HL_RAM_BLOCK_SIZE;
        if (remainder > 0)
        {
            ++numBlocksForModel;
        }
        numCostumeModelBlocks[modelIdx] = numBlocksForModel;
        totalNumCostumeModelBlocks += numBlocksForModel;
    }

    // Calculate number of source models that do not fit into memory blocks already
    // reserved for target model data.
    uint32_t numCostumeBlocksThatFit = 0;
    uint32_t modelIdx = 0;
    while ((numCostumeBlocksThatFit + numCostumeModelBlocks[modelIdx]) <= numTargetBlocks)
    {
        numCostumeBlocksThatFit += numCostumeModelBlocks[modelIdx];
        ++modelIdx;
    }
    const uint32_t numModelsThatFit = modelIdx;
    printf_notify("numModelsThatFit is: %d", numModelsThatFit);

    // Find the start of first free memory block which can hold remaining source data
    // models.  (Animation/miscellaneous/file data follow the target model data, and
    // model/animation/miscellaneous/file data for other character sheets can follow
    // (and you will likely find model/animation/miscellaneous/file data for other
    // character sheets prior to target model data).)
    uint32_t numAnimations = HL_NUM_ANIMATIONS_QUENTIN;
    if (HL_QUENTIN_SHEET_P_ADDR != targetCharacterSheetAddr)
    {
        // Other fighters have 2 less animations then Quentin.  One or both of those extra animations are for eat/drink.
        numAnimations -= 2;
    }
    uint32_t targetLastAnimationPtrAddr = targetAnimationPtrsAddr + (numAnimations * HL_SIZE_OF_POINTER); // Beyond the animation pointer.
    targetLastAnimationPtrAddr -= HL_SIZE_OF_POINTER; // The last animation pointer.
    uint32_t targetLastAnimationAddr = bigpemu_jag_read32(targetLastAnimationPtrAddr);
    targetLastAnimationAddr -= 8; // Have to subtract 8 from character sheet ptr, to get to actual beginning of animation.  Bytes 0 and 1 are, Bytes 2 and 3 are number of blocks, Byte4 through Byte7 are pointer to address in character sheet pointing to this animation.
                                  // Models/miscellaneous/files all follow same block pattern of 8 bytes, data, and block size of HL_RAM_BLOCK_SIZE bytes, with trailing zero padding.
    uint32_t targetDataAddr2 = targetLastAnimationAddr;
    uint32_t numDataBlocks = 0;
    do
    {
        // Read number of blocks.  Zero value indicates no data block.
        numDataBlocks = bigpemu_jag_read32(targetDataAddr2);

        // Skip over any data blocks.
        targetDataAddr2 += (numDataBlocks * HL_RAM_BLOCK_SIZE);
    } while (numDataBlocks > 0);

    //TODO: ensure there is enough free space.

    const uint32_t destAddr = targetFirstModelAddr - 8;// Have to subtract 8 from character sheet ptr, to get to actual beginning of model.  First four bytes are number-of-blocks, next 4 bytes are pointer to address in character sheet pointing to this model.

    *pStartAddr = targetFirstModelAddr;
    *pResumeAddr = targetDataAddr2;
    *pNumModelsThatFit = numModelsThatFit;
}

// Populate metadata in format used for Jaguar RAM.
uint32_t populateRamMetadata(int insertVertexForWeapon, uint32_t modelLength, uint32_t ptrFromCharSheet, RamMetadata* pRamMetadata)
{
   if (0 == pRamMetadata)
   {
      return modelLength;
   }

   // Update model length to account for any inserted vertice.
   modelLength += (insertVertexForWeapon * HL_SIZE_OF_VERTICE);

   // Calculate/set number of blocks in Jaguar memory.
   const uint32_t remainder = modelLength % HL_RAM_BLOCK_SIZE;
   uint32_t numBlocks = modelLength / HL_RAM_BLOCK_SIZE;
   if (remainder > 0)
   {
      ++numBlocks;
   }
   pRamMetadata->numBlocks = numBlocks;

   // Address of character sheet pointer, which points to this model.
   pRamMetadata->pCharacterSheetPtr = ptrFromCharSheet;
   //TODO: setting above line was added 2025-11-25, after everything else implemented.  Some possiblility causes dynamic freeing problems.

   // Return updated model length.
   return modelLength;
}

// Massage/convert ROM formatted model header to RAM formatted model header, and prepare for vertice injection if needed
void convertModelHeader(uint32_t destAddr, int insertVertexForWeapon, int weaponEmbeddedInRightHandModel, int ramMetadataPresent, ModelHeader* pModelHeader)
{
   if (0 == pModelHeader)
   {
      return;
   }

   // Update num origins if inserting a vertex to hold weapon.
   if (insertVertexForWeapon)
   {
      // Set to 1, the byte which represents number of special trailing vertices that 
      // apparently are used as origin of connected/articulating objects.
      // Already established the value is currently 0, when this block was entered.
      pModelHeader->numOrigins = 1;
   }

   // Update number of facets, if weapon is embedded into right hand model.
   if (insertVertexForWeapon && weaponEmbeddedInRightHandModel)
   {
      // Update the num facets such that only the facets used for displaying
      // hand are displayed.  This update will prevent display of facets of any
      // sword embedded within the right hand model.  This is neccessary so that
      // Hood, Hair, Claw, Gunman1, Gunman2, Ramirez, Arak, Kortan can animate/display
      // without their right hand model embedded weapon displaying at all times.
      pModelHeader->numFacets = HL_NUMBER_FACETS_IN_RIGHT_HAND_MODEL_WITHOUT_WEAPON;
   }

   // Update relative addresses to account for any inserted vertice.
   const uint32_t verticesRelAddr = pModelHeader->pVerticesStart;
   uint32_t facetsRelAddr   = pModelHeader->pFacetsStart;
   uint32_t saveoutsRelAddr = pModelHeader->pSaveoutsStart;
   if (insertVertexForWeapon)
   {
      facetsRelAddr += (insertVertexForWeapon * HL_SIZE_OF_VERTICE);
      if (0 != saveoutsRelAddr)
      {
         saveoutsRelAddr += (insertVertexForWeapon * HL_SIZE_OF_VERTICE);
      }
   }

   // Vertices always start after the header.
   uint32_t verticesAbsoluteAddr = destAddr + HL_SIZE_OF_MODEL_HEADER;
   if (ramMetadataPresent)
   {
      verticesAbsoluteAddr += HL_SIZE_OF_RAM_METADATA;
   }

   // Convert other relative addresses to absolute addresses.
   const uint32_t facetsAbsoluteAddr = verticesAbsoluteAddr + (facetsRelAddr - verticesRelAddr);
   uint32_t saveoutsAbsoluteAddr = 0;
   if (0 != saveoutsRelAddr)
   {
      saveoutsAbsoluteAddr = verticesAbsoluteAddr + (saveoutsRelAddr - verticesRelAddr);
   }
   pModelHeader->pVerticesStart = verticesAbsoluteAddr;
   pModelHeader->pFacetsStart   = facetsAbsoluteAddr;
   pModelHeader->pSaveoutsStart = saveoutsAbsoluteAddr;
   pModelHeader->pDunnoStart    = 0; // Always seems to be null.
}

// Copy RAM format of metadata to Jaguar memory
void copyRamMetadata(const RamMetadata* pRamMetadata, uint32_t destAddr)
{
   if (0 == pRamMetadata)
   {
      return;
   }

   bigpemu_jag_write32(destAddr, pRamMetadata->numBlocks);
   destAddr += 4;
   bigpemu_jag_write32(destAddr, pRamMetadata->pCharacterSheetPtr);
   destAddr += 4;
}

// Copy model header to Jaguar memory
void copyModelHeader(const ModelHeader* pModelHeader, uint32_t destAddr)
{
   if (0 == pModelHeader)
   {
      return;
   }

   bigpemu_jag_write16(destAddr, pModelHeader->modelLength);
   destAddr += 2;
   bigpemu_jag_write8(destAddr, pModelHeader->originNumber);
   destAddr += 1;
   bigpemu_jag_write8(destAddr, pModelHeader->numOrigins);
   destAddr += 1;
   bigpemu_jag_write16(destAddr, (uint16_t) pModelHeader->numVertices);
   destAddr += 2;
   bigpemu_jag_write16(destAddr, pModelHeader->numFacets);
   destAddr += 2;
   bigpemu_jag_write32(destAddr, pModelHeader->pVerticesStart);
   destAddr += 4;
   bigpemu_jag_write32(destAddr, pModelHeader->pFacetsStart);
   destAddr += 4;
   bigpemu_jag_write32(destAddr, pModelHeader->pSaveoutsStart);
   destAddr += 4;
   bigpemu_jag_write32(destAddr, pModelHeader->pDunnoStart);  // 4th pointer always seems to be null (at least for 15-model people characters)
   destAddr += 4;
}

uint32_t copyModelData(const Model* pModel, int insertVertexForWeapon, int weaponEmbeddedInRightHandModel, uint32_t oldNumFacets, uint32_t destAddr)
{
   if (0 == pModel)
   {
      return;
   }

   const uint32_t oldDestAddr = destAddr;

   //=-=-=-=-=-=-=-= Copy vertice list to Jaguar memory =-=-=-=-=-=-=-=
   // Get number of vertices, for later use.
   const uint32_t numVerticesBytes = pModel->header.numVertices * HL_SIZE_OF_VERTICE;
   // Vertices begin at start of model data.
   uint32_t modelBytesIdx = 0;
   for (; modelBytesIdx < numVerticesBytes; ++modelBytesIdx, ++destAddr)
   {
      bigpemu_jag_write8(destAddr, pModel->data.modelData[modelBytesIdx]);
   }

   //=-=-=-=-=-=-=-= If needed, save/add a vertex to use as origin for weapon holding. =-=-=-=-=-=-=-=
   if (insertVertexForWeapon)
   {
      // Byte values of the vertex which should be suffixed to existing vertice data.
      const uint8_t quentinRightHandSwordVertex[HL_SIZE_OF_VERTICE] = {0xFF, 0xF0, 0xFF, 0xEB, 0x00, 0x08, 0x00, 0x90};

      // Insert vertice bytes at insertion point.  Do not update modelBytesIdx.
      for (uint32_t newVerticeBytesIdx=0; newVerticeBytesIdx < HL_SIZE_OF_VERTICE; ++newVerticeBytesIdx, ++destAddr)
      {
         bigpemu_jag_write8(destAddr, quentinRightHandSwordVertex[newVerticeBytesIdx]);
      }
   }

   //=-=-=-=-=-=-=-= Copy remainder of model data to Jaguar memory =-=-=-=-=-=-=-=
   // modelLength includes 24 byte header
   // modelLength includes any inserted bytes that are not part of modelData.
   const uint32_t modelLength = pModel->header.modelLength;
   for (; modelBytesIdx < (modelLength - HL_SIZE_OF_MODEL_HEADER - (insertVertexForWeapon * HL_SIZE_OF_VERTICE)); ++modelBytesIdx, ++destAddr)
   {
      bigpemu_jag_write8(destAddr, pModel->data.modelData[modelBytesIdx]);
   }

   //=-=-=-=-=-=-=-= Zero pad to end of block. =-=-=-=-=-=-=-=
   // (This could have bug where writes beyond block, but we'd never know, because the next model would overwrite.)
   const uint32_t remainder = modelLength % HL_RAM_BLOCK_SIZE;
   int zeroPadLength = HL_RAM_BLOCK_SIZE - ((int) remainder);
   // Subtract 8, for the next model's metadata.
   // Don't think this should matter, but just in case.
   // If don't do this, and overwrite first model, that first model overwrite wipes out the 8 byte metadata of second model
   // Really doubt trailing zero padding is neccessary anyways, though trailing zero padding does aid debugging
   zeroPadLength -= HL_SIZE_OF_RAM_METADATA;
   if (zeroPadLength < 0)
   {
      zeroPadLength = 0;
   }
   for (uint32_t zIdx = 0; zIdx < ((uint32_t) zeroPadLength); ++zIdx, ++destAddr)
   {
      bigpemu_jag_write8(destAddr, 0);
   }

   // Return the number of bytes written.
   return destAddr - oldDestAddr;
}

//Work In Progress (WIP)
//Likely need to add some facets to create 'bottom' of turret so looks 'correct', from all angles.
void copyTurretToGasgunModel(void)
{
   Model model;
   readModelsFromCompactDisc(TURRET, &model, 1);
   uint32_t destAddr = HL_GAS_GUN_MODEL_ADDR;
   convertModelHeader(destAddr, 0, 0, 0, &(model.header));

   // Reorient the vertices.
   for (uint32_t verticesIdx = 0; verticesIdx < model.header.numVertices; ++verticesIdx)
   {
      int32_t xVert = (model.data.modelData[(verticesIdx * HL_SIZE_OF_VERTICE) + 0] << 8) |
                      (model.data.modelData[(verticesIdx * HL_SIZE_OF_VERTICE) + 1]);
      int32_t yVert = (model.data.modelData[(verticesIdx * HL_SIZE_OF_VERTICE) + 2] << 8) |
                      (model.data.modelData[(verticesIdx * HL_SIZE_OF_VERTICE) + 3]);
      int32_t zVert = (model.data.modelData[(verticesIdx * HL_SIZE_OF_VERTICE) + 4] << 8) |
                      (model.data.modelData[(verticesIdx * HL_SIZE_OF_VERTICE) + 5]);

      // WIP: Need to orient to more sensible position/angle.  May need to actually write some code to rotate the vertices of model.
      yVert -= 1140;

      int32_t tmp = zVert;
      zVert = yVert;
      yVert = tmp;

      //int32_t tmp = xVert;
      //xVert = yVert;
      //yVert = tmp;

      tmp = xVert;
      xVert = zVert;
      zVert = tmp;

      model.data.modelData[(verticesIdx * HL_SIZE_OF_VERTICE) + 0] = xVert >> 8;
      model.data.modelData[(verticesIdx * HL_SIZE_OF_VERTICE) + 1] = xVert & 0x000000FF;
      model.data.modelData[(verticesIdx * HL_SIZE_OF_VERTICE) + 2] = yVert >> 8;
      model.data.modelData[(verticesIdx * HL_SIZE_OF_VERTICE) + 3] = yVert & 0x000000FF;
      model.data.modelData[(verticesIdx * HL_SIZE_OF_VERTICE) + 4] = zVert >> 8;
      model.data.modelData[(verticesIdx * HL_SIZE_OF_VERTICE) + 5] = zVert & 0x000000FF;
   }

   copyModelHeader(&(model.header), destAddr);
   destAddr += HL_SIZE_OF_MODEL_HEADER;

   // Copy this statically loaded model data, without any trailing zero padding, because statically loaded models do not zero pad.
   for (uint32_t byteIdx = 0; byteIdx < model.header.modelLength - HL_SIZE_OF_MODEL_HEADER; ++byteIdx)
   {
      bigpemu_jag_write8(destAddr++, model.data.modelData[byteIdx]);
   }

   // Write "TURRET~" text string, to memory, 16 bytes after end of the model.
   const uint32_t textAddr = destAddr+16;
   bigpemu_jag_write32(textAddr+0, 0x54555252);
   bigpemu_jag_write32(textAddr+4, 0x45547E00);

   // Write pointers to memory, immediately after the end of model (but before the text), which points to the the text.
   bigpemu_jag_write32(destAddr, 0);
   destAddr += 4;
   bigpemu_jag_write32(destAddr, textAddr);
   destAddr += 4;
   bigpemu_jag_write32(destAddr, textAddr);
   destAddr += 4;
   bigpemu_jag_write32(destAddr, textAddr);
   destAddr += 4;
};

// Contract: The amount of memory currently allocated to the specified destination
//           animation is large enough to hold the source animation.
// Track8 animations do not have model data preceding them.
// sourceAnimNum is 0-based
// destAnimNum is 0-based
void readAnimationFromCompactDiscTrack8(uint32_t requestedBlockOffset, int sourceAnimNum, uint32_t destCharSheet, int destAnimNum)
{
   //TODO: remove hard coded path - possibly just find better way
   const uint64_t trackFileHandle = fs_open(".\\Highlander\\Highlander - The Last of the MacLeods (USA) (Track 8).bin", 0);
   printf_notify("trackFileHandle is: %u\n\n\n", trackFileHandle);
   if (0 == trackFileHandle)
   {
      return;
   }

   // Read file blocks, up through the requested block.
   uint8_t trackData[HL_TRACK_8_BLOCK_START_ADDR + 64]; //HL_TRACK_8_BLOCK_START_ADDR is longest stretch of memory will need to read (and then add 64 bytes just to be "safe"/superstitious);
   uint64_t bytesRead = fs_read(trackData, HL_TRACK_8_BLOCK_START_ADDR, trackFileHandle);
   uint32_t cursorBlockOffset = 0;
   while (cursorBlockOffset <= requestedBlockOffset)
   {
      bytesRead = fs_read(trackData, HL_CD_BLOCK_SIZE * HL_CD_NUM_BLOCKS_PER_READ, trackFileHandle);
      cursorBlockOffset += HL_CD_NUM_BLOCKS_PER_READ;
   }
   fs_close(trackFileHandle);

   // Skip through bytesRead, to the requested animation data.
   int trackDataIdx = 0;
   for (int animIdx = 0; animIdx < sourceAnimNum; ++animIdx)
   {
      // Remember that data from compact disc is byte swapped.
      const uint32_t animSize = (trackData[trackDataIdx + 3] << 8) | trackData[trackDataIdx + 2];
      trackDataIdx += (animSize + HL_SIZE_OF_ROM_METADATA);
   }
 
   // Populate RAM Metadata structure.
   int insertVertex = 0;
   const uint32_t modelLength = (trackData[trackDataIdx + 3] << 8) | trackData[trackDataIdx + 2];
   const uint32_t animPtr = destCharSheet + HL_SIZE_OF_CHAR_SHEET_HEADER + (HL_NUM_MODELS_PER_FIGHTER * HL_SIZE_OF_POINTER) + (destAnimNum * HL_SIZE_OF_POINTER);
   RamMetadata ramMetadata;
   populateRamMetadata(insertVertex,  modelLength, animPtr, &ramMetadata);
   trackDataIdx += HL_SIZE_OF_ROM_METADATA;

   // Copy metadata to RAM.
   uint32_t animAddr = bigpemu_jag_read32(animPtr);
   copyRamMetadata(&ramMetadata, animAddr - HL_SIZE_OF_RAM_METADATA);

   // Copy the byte-swapped animation data over the target animation data.
   for (uint32_t animDataIdx = 0; animDataIdx < modelLength; animDataIdx +=2, trackDataIdx += 2)
   {
      bigpemu_jag_write8(animAddr++, trackData[trackDataIdx + 1]);
      bigpemu_jag_write8(animAddr++, trackData[trackDataIdx + 0]);
   }
}

static void costumeChange(uint32_t targetCharacterSheet, enum FightersEnum fighter)
{
   const uint32_t requestedBlockOffset = gFighterHackAttributes[fighter].mBlockOffset;

   // Read models, from compact disc, for specified fighter.
   Model models[HL_NUM_MODELS_PER_FIGHTER];
   if (readModelsFromCompactDisc(fighter, &models[0], HL_NUM_MODELS_PER_FIGHTER))
   {
      // Determine locations where to copy models: 1) where target model begins, and
      // potentially 2) where there is free space in case that source model does not
      // fit within space allocated to destination model.
      uint32_t targetFirstModelAddr = -1;
      uint32_t targetDataAddr2 = -1;
      uint32_t numModelsThatFit = -1;
      getTargetLocations(models, targetCharacterSheet, &targetFirstModelAddr, &targetDataAddr2, &numModelsThatFit);
      const uint32_t targetModelPtrsAddr = targetCharacterSheet + HL_SIZE_OF_CHAR_SHEET_HEADER;

      // Address to write metadata of first model.
      uint32_t destAddr = targetFirstModelAddr - 8;// Have to subtract 8 from character sheet ptr, to get to actual beginning of model.  First four bytes are number-of-blocks, next 4 bytes are pointer to address in character sheet pointing to this model.
      printf_notify("destAddr is: %x", destAddr);

      uint32_t modelStartAddrs[HL_NUM_MODELS_PER_FIGHTER];
      for (uint32_t modelIdx = 0; modelIdx < HL_NUM_MODELS_PER_FIGHTER; ++modelIdx)
      {
         // Determine if should add a vertex to this model, for weapon display.
         // Only Right Hand model, which does not already have origin number set
         // to 1, needs to have a vertex inserted.
         // Only when Quentin character sheet is target character sheet.
         // Also determine if weapon is embedded in right hand model.
         int insertVertexForWeapon = 0;
         int weaponEmbeddedInRightHandModel = 0;
         if (HL_QUENTIN_SHEET_P_ADDR == targetCharacterSheet)
         {
            if (0x8C == models[modelIdx].header.originNumber)
            {
               if (0 == models[modelIdx].header.numOrigins)
               {
                  insertVertexForWeapon = 1;
                  weaponEmbeddedInRightHandModel = models[modelIdx].header.numFacets > HL_NUMBER_FACETS_IN_RIGHT_HAND_MODEL_WITHOUT_WEAPON;
               }
            }
         }

         // If model did not fit into memory reserved for initial Quentin models.
         if (modelIdx >= numModelsThatFit)
         {
            // Update/calculate position of where to copy the model
            destAddr = targetDataAddr2; //already subtracted 8;
            //TODO: LUCKY - luck that this works. the models that don't fit into quentin's initial model space, are each conveniently fitting into one ram_block per model.
            //              should add past the length of the model, with length ceilinged to multiple of blocksize 
            printf_notify("TDO: LUCKY modelLength is %x\n", models[modelIdx].header.modelLength);
            destAddr += ((modelIdx - numModelsThatFit) * HL_RAM_BLOCK_SIZE);
            printf("MODEL DID NOT FIT INTO MEMORY RESERVED FOR INITIAL QUENTIN MODELS\n\n\n\n\n\n");
         }

         //=-=-=-=-=-=-=-= Populate metadata in format used for Jaguar RAM =-=-=-=-=-=-=-=
         RamMetadata ramMetadata;
         modelStartAddrs[modelIdx] = destAddr + HL_SIZE_OF_RAM_METADATA;
         const uint32_t ptrFromCharSheet = targetCharacterSheet + HL_SIZE_OF_CHAR_SHEET_HEADER + (modelIdx * HL_SIZE_OF_POINTER);
         models[modelIdx].header.modelLength = populateRamMetadata(insertVertexForWeapon, models[modelIdx].header.modelLength, ptrFromCharSheet, &ramMetadata);

         //=-=-=-=-=-=-=-= Massage/convert ROM formatted model header to RAM formatted model header, and prepare for vertice injection if needed =-=-=-=-=-=-=-=
         const uint32_t oldNumFacets = models[modelIdx].header.numFacets;
         convertModelHeader(destAddr, insertVertexForWeapon, weaponEmbeddedInRightHandModel, 1, &(models[modelIdx].header));

         //=-=-=-=-=-=-=-= Copy RAM format of metadata to Jaguar memory. =-=-=-=-=-=-=-=
         copyRamMetadata(&ramMetadata, destAddr);
         destAddr += HL_SIZE_OF_RAM_METADATA;

         //=-=-=-=-=-=-=-= Copy model header to Jaguar memory =-=-=-=-=-=-=-=
         copyModelHeader(&(models[modelIdx].header), destAddr);
         destAddr += HL_SIZE_OF_MODEL_HEADER;

         //=-=-=-=-=-=-=-= Copy model data to Jaguar memory =-=-=-=-=-=-=-=
         // Inserts a vertice in right hand models for weapon handling, when appropriate, as well.
         const uint32_t numBytesWritten = copyModelData(&(models[modelIdx]), insertVertexForWeapon, weaponEmbeddedInRightHandModel, oldNumFacets, destAddr);
         destAddr += numBytesWritten;

         //=-=-=-=-=-=-=-=  When weapon data is embedded in right hand model, copy weapon data from right hand model over the macleod sword model data =-=-=-=-=-=-=-=.
         // This will impact Hood, Hair, Claw, Ramirez, Arak, and Kortan.
         // (This entire method is not applicable to Gunman1 and Gunman2, as they look like Hood and there would be additional complexity that their embedded weapon is not a sword (so different sounds/etc))
         // Other code will ensure the sword data in right hand model is not displayed, as part of right hand model.
         // Doing this, so affected fighters can select to equip bare hands, gun, chicken, or Macleod Sword displayed as the fighter's actual sword.
         if (insertVertexForWeapon && weaponEmbeddedInRightHandModel)
         {
            // Copy vertices and sword facets from right hand model over macleod sword model.
            // This way, when enabling the sword, the sword model data associated to this
            // model will be displayed, instead of macleod sword model data.
            const uint32_t numVerticesBytes = models[modelIdx].header.numVertices * HL_SIZE_OF_VERTICE;
            copySwordToMacleodSwordModel(models[modelIdx].data.modelData, numVerticesBytes, oldNumFacets);
         }
      }

      //=-=-=-=-=-=-=-= Update character sheet pointers to point to starting addresses of the models  =-=-=-=-=-=-=-=
      uint32_t targetPtrsAddr = targetModelPtrsAddr;
      for (uint8_t modelIdx = 0; modelIdx < HL_NUM_MODELS_PER_FIGHTER; ++modelIdx)
      {
         bigpemu_jag_write32(targetPtrsAddr, modelStartAddrs[modelIdx]);
         targetPtrsAddr += HL_SIZE_OF_POINTER;

         // Update the Draw Data Area entries.  These still have pointers to old model locations.

         // First field in character instance entry is character sheet.
         // A character instance entry, with a null character sheet, marks
         // the end of the character instance table.

         // Get the first character instance entry's character sheet.
         uint32_t charInstanceEntryAddr = HL_CHARACTER_INSTANCE_TABLE_START_ADDR;
         uint32_t characterSheetAddr = bigpemu_jag_read32(charInstanceEntryAddr);

         // While not at end of character instance table
         while (0 != characterSheetAddr)
         {
            // If character instance entry's character sheet matches the target character sheet.
            if (targetCharacterSheet == characterSheetAddr)
            {
               // Update the character instance entry's draw data area's entry to point to location of model.
               const uint32_t drawDataAreaStart = bigpemu_jag_read32(charInstanceEntryAddr + 4);
               bigpemu_jag_write32(drawDataAreaStart + (modelIdx * HL_SIZE_OF_DRAW_DATA_AREA_ENTRY) + 12, modelStartAddrs[modelIdx]);
            }

            // Increment to next character instance entry.
            charInstanceEntryAddr += HL_SIZE_OF_CHAR_INSTANCE_ENTRY;

            // Get the character instance entry's character sheet.
            characterSheetAddr = bigpemu_jag_read32(charInstanceEntryAddr);
         }
      }
   }
}

static uint32_t on_sw_loaded(const int eventHandle, void* pEventData)
{
   const uint64_t hash = bigpemu_get_loaded_fnv1a64();

   sMcFlags = 0;

   if (HL_HASH == hash)
   {
      sMcFlags |= skMcFlag_Loaded;
      sCostumeEnum = WAIT;
      sCostumeDelayCounter = -1;

      // Initialize the fighter hack attribute array of structures.  Due to C struct limitations, this ends
      // up being about the most consise way to initialize the array when you want to explicitly assign
      // enumerated index values.
      //
      // For each indexed structure: Pass block offset, supportPlayAs, supportModelAs, and supportTitleScreenSword.
      initFighterHackAttributes(&(gFighterHackAttributes[QUENTIN]),      HL_QUENTIN_BLOCK_OFFSET,      0, 0, 1, HL_NO_HEIGHT_OFFSET);	          // Game already allows play and model as Quentin.
      initFighterHackAttributes(&(gFighterHackAttributes[HOOD]),         HL_HOOD_BLOCK_OFFSET,         1, 1, 0, HL_HOOD_HEIGHT_OFFSET);          // Fighters that are equipped with sword, cannot support title screen sword.  Other documentation references why.
      initFighterHackAttributes(&(gFighterHackAttributes[HAIR]),         HL_HAIR_BLOCK_OFFSET,         1, 1, 0, HL_HAIR_HEIGHT_OFFSET);
      initFighterHackAttributes(&(gFighterHackAttributes[GUNMAN1]),      HL_GUNMAN1_BLOCK_OFFSET,      1, 0, 0, HL_NO_HEIGHT_OFFSET);	          // Gunman without gun models like Hood, so don't bother.
      initFighterHackAttributes(&(gFighterHackAttributes[GUNMAN2]),      HL_GUNMAN2_BLOCK_OFFSET,      1, 0, 0, HL_NO_HEIGHT_OFFSET);	          // Gunman without gun models like Hood, so don't bother.
      initFighterHackAttributes(&(gFighterHackAttributes[CLAW]),         HL_CLAW_BLOCK_OFFSET,         1, 1, 1, HL_CLAW_HEIGHT_OFFSET);
      initFighterHackAttributes(&(gFighterHackAttributes[ARAK]),         HL_ARAK_BLOCK_OFFSET,         0, 1, 0, HL_ARAK_HEIGHT_OFFSET);
      initFighterHackAttributes(&(gFighterHackAttributes[KORTAN]),       HL_KORTAN_BLOCK_OFFSET,       0, 1, 0, HL_KORTAN_HEIGHT_OFFSET);
      initFighterHackAttributes(&(gFighterHackAttributes[MANGUS]),       HL_MANGUS_BLOCK_OFFSET,       0, 1, 1, HL_MANGUS_HEIGHT_OFFSET);
      initFighterHackAttributes(&(gFighterHackAttributes[RAMIREZ]),      HL_RAMIREZ_BLOCK_OFFSET,      0, 1, 0, HL_RAMIREZ_HEIGHT_OFFSET);
      initFighterHackAttributes(&(gFighterHackAttributes[MASKED_WOMAN]), HL_MASKED_WOMAN_BLOCK_OFFSET, 0, 1, 1, HL_MASKED_WOMAN_HEIGHT_OFFSET);
      initFighterHackAttributes(&(gFighterHackAttributes[CLYDE]),        HL_CLYDE_BLOCK_OFFSET,        0, 1, 1, HL_CLYDE_HEIGHT_OFFSET);
      initFighterHackAttributes(&(gFighterHackAttributes[DUNDEE_B]),     HL_DUNDEE_B_BLOCK_OFFSET,     0, 1, 1, HL_DUNDEE_B_HEIGHT_OFFSET);
      initFighterHackAttributes(&(gFighterHackAttributes[DUNDEE_D]),     HL_DUNDEE_D_BLOCK_OFFSET,     0, 1, 1, HL_DUNDEE_D_HEIGHT_OFFSET);
      initFighterHackAttributes(&(gFighterHackAttributes[TURBINE]),      HL_TURBINE_BLOCK_OFFSET,      0, 0, 0, HL_NO_HEIGHT_OFFSET);
      initFighterHackAttributes(&(gFighterHackAttributes[TANK]),         HL_TANK_BLOCK_OFFSET,         0, 0, 0, HL_NO_HEIGHT_OFFSET);
      initFighterHackAttributes(&(gFighterHackAttributes[TURRET]),       HL_TURRET_BLOCK_OFFSET,       0, 0, 0, HL_NO_HEIGHT_OFFSET);
      initFighterHackAttributes(&(gFighterHackAttributes[HAND]),         HL_HAND_QUES_BLOCK_OFFSET,    0, 0, 0, HL_NO_HEIGHT_OFFSET);

// The partially filled in character sheets are copied to Jaguar RAM, when Start Game | Language | Credits screen id displayed.
// Quentin's character sheet is fully populated on that screen as well, and stays populated when select to start game and proceed thru game
// Will get unpopulated if you stay at screen long enough for cinepak sequence to start back up
// MAIN.S handles this screen.  And is 68k code.
// MAIN.S handles the player confirming to start game (via a b or c button, with selection cursor on Play Game)
// The following instruction is where want to place software breakpoint:
//    bne   gamestart
// Within MAIN.S lonoff and lambient are declared next to each other and set to 0x0010 and 0x77777
// Search thorugh Jaguar RAM for 0x00107777 comes up with one hit:
//    0x000042B8
// Write breakpoint on that address.
// Go to instruction in disassembly window that writes to breakpoint
// Scroll down in disassembly window to locate the instruction we want to place breakpoint on
// its address is 0x00005230
//    bigpemu_jag_m68k_bp_add(0x00005230, handleStartGameBreakPoint);
// above is not the start of game.  it is the start of display of start/language/credits screen
// Think 68 copies the list of partially filled character sheets to memory, then has GPU load up quentin fully
// and think GPU loads up title screen sword
// a 68k software breakpoint probably doesn't handle well for determining end of quentin read, as by time 68 realizes quentin fully populated, gpu may already be on its way to populating sword and other character sheets fully
// maybe best to put breakpoint on 68k getting the list of partially populated character sheets blitted to memory
// then immediately fill in quentin before gpu knows anything about requests to populate list
//
// Don't know if the 68k is providing a good opportunity (or any opportunity at all) to determine when list of partially populated character sheets has been copied to RAM but before GPU starts fully populating
// quinten character sheet ends up full populated by/around time user sees options for start-game/language/credits
// F it for now, and use on_emu frame to detect when quentin's character sheet miscellanous ptrs have been populated,
//     maybe wait a few seconds
//     then start surgery on character-sheet and RAM
//     and some boolean lock to prevent from having to do this again until quinten character sheet last file pointer goes back to default value
//     don't operate until see other charactersheets that are expected to be fully populated are fully populated
//     alternatively maybe a breakpoint in engage will work.
   }
   return 0;
}

static uint32_t on_input_frame(const int32_t eventHandle, void* pEventData)
{
   if (sMcFlags & skMcFlag_Loaded)
   {
      TBigPEmuInputFrameParams *pParams = (TBigPEmuInputFrameParams *)pEventData;
      if (pParams <= 0)
      {
         // Whitehouse Raiden example does not return a value.  Do the same in this script as well...
         return;
      }
      TBigPEmuInput *pInput = pParams->mpInputs;
      if (pInput <= 0)
      {
         return;
      }

      // If executing teleporter cheat sequence
      if ((sTeleporterCheatCursor >= 0) && (sTeleporterCheatCursor < HL_TELEPORTER_CHEAT_SEQ_LENGTH))
      {
         // Programatically calling button presses likely presses and releases the button
         // within span of 1-2 emulation frames.  Real world joypad interation isn't that quick and 
         // press is likely over several frames.  Game developers probably didn't expect
         // press&release to happen within span of 1-2 vertical-refreshes/frames.
         //
         // So while programatically executing the cheat sequence, wait until buttons
         // are not pressed, before progamatically pressing the next button in the sequence.
         // Keep in mind that the button sequence has been modified to have duplicate, contiguous
         // copies of the joypad button presses.
         //
         // Hacky, ugly, likely prone to breaking in subsequent BigPEmu version, but whatever.
         // For now, user doesn't need to look up a cheat code off website.
         if (0 == pInput->mButtons)
         {
            // Check if actually need to programatically press a key or not.
            if (HL_NO_KEY != sTeleporterCheatSequence[sTeleporterCheatCursor])
            {
               // Programatically press the key.
               pInput->mButtons |= (1 << sTeleporterCheatSequence[sTeleporterCheatCursor]);
            }

            // Advance the key sequence by one.
            ++sTeleporterCheatCursor;
         }
      }
      if (sTeleporterCheatCursor >= HL_TELEPORTER_CHEAT_SEQ_LENGTH)
      {
         // The attempt to enter the joypad cheat code has completed, so mark/report as such.
         sTeleporterCheatCursor = -1;
         printf_notify("Teleportation joypad sequence entered.\n");
      }

      if (pParams->mInputCount > 0)
      {
         // Don't bother checking for user button to sleep enemies, if they have set to sleep for less then 10 frames.
         if (sSleepEnemyCounts >= 10)
         {
            // If button 0 pressed.
            if (pInput->mButtons & (1 << JAG_BUTTON_0))
            {
               // Ignore 0 button press, if already in middle of sleep session.
               if (0 == sAwakenEnemiesCountdown)
               {
                  sAwakenEnemiesCountdown = sSleepEnemyCounts;
               }
            }
         }

         // Check if user selected to use Teleporter.
         if (pInput->mButtons & (1 << JAG_BUTTON_7))
         {
            // If not already in process of applying Teleporter sequence.
            if ((sTeleporterCheatCursor < 0) || (sTeleporterCheatCursor >= HL_TELEPORTER_CHEAT_SEQ_LENGTH))
            {
               // Start Teleporter Cheat sequence.
               // The sequence will be executed over several invocations of this method.
               sTeleporterCheatCursor = 0;
            }
         }
      }
   }
}

static void readItemList()
{
   // Read the Item list.
   bigpemu_jag_sysmemread(sHlItems, HL_ITEM_LIST_ADDR, HL_NUM_ITEMS * HL_ITEM_SIZE);

   // Swap bytes, to match Jaguar endiness.
   for (uint8_t itemIdx = 0; itemIdx < HL_NUM_ITEMS; ++itemIdx)
   {
      HlItem* const pItem = &(sHlItems[itemIdx]);
      workingByteSwap16(&(pItem->mScene));
      workingByteSwap16(&(pItem->mRadius));
      pItem->mSheet = byteswap_ulong(pItem->mSheet);
      pItem->mOwner = byteswap_ulong(pItem->mOwner);
   }
}

static void applyHeightOffset(uint32_t targetSheetAddr, enum FightersEnum fighter, int weaponInduced)
{
   // Start address of the animations ptrs, within Quentin or weapons character sheet.
   uint32_t addr = targetSheetAddr;
   addr += HL_SIZE_OF_CHAR_SHEET_HEADER;
   uint32_t numModels = HL_NUM_MODELS_PER_FIGHTER;
   if (weaponInduced)
   {
      numModels = 1;
   }
   addr += (numModels * HL_SIZE_OF_POINTER);

   // Offset to apply to animations, to get the copied model data's feet to touch (near) the ground.
   const int heightOffset = gFighterHackAttributes[fighter].mHeightOffset;

   // Apply the offset to all animations referenced by Quentin character sheet.
   // Without this, Clyde is significantly hovering in the air, and the remaining fighters have 'concrete' feet that are below the ground.
   // If someone happens upon a better way to to this, please pipe up.
   uint32_t numAnimations = HL_NUM_ANIMATIONS_QUENTIN;
   if (QUENTIN != fighter)
   {
      numAnimations -= 2;
   }
   for (uint32_t animIdx = 0; animIdx < numAnimations; ++animIdx, addr += HL_SIZE_OF_POINTER)
   {
      uint32_t animAddr = bigpemu_jag_read32(addr);
      animAddr += (12);
      uint32_t height = bigpemu_jag_read32(animAddr);
      height = height >> 16;
      height += heightOffset;
      height = height << 16;
      bigpemu_jag_write32(animAddr, height);
   }

   // Note: Attempted to apply height offset, via World State entry's 32 bit mZpos field, with no luck.
   //       Adding value of 0x0000007F was not noticeably making a difference.
   //       Adding value of 0x0000017F caused transition from cinepak to start of game to instead transition back to start/selection screen.
   //       Before that, may have attempted to simply set World State mZpos every emulation frame, with weird results.
}

static uint32_t on_emu_frame(const int eventHandle, void* pEventData)
{
   // Most/all of these cheats can be toggled, on per frame basis (in same session).
   // So user can unlock sword at one point in session, then 5 minutes later unlock gas gun.
   //
   // If user toggles cheat off during session, probably not reasonable to relock weapons,
   // keys, tools, maps, orders.  Other then items obtained from dead enemy fighters, it would
   // be difficult to determine whether player legitimately obtained the item after enabling
   // the unlock cheats, or illegitamately obtained the item using the unlock cheats.

   if (sMcFlags & skMcFlag_Loaded)
   {
      sFrameCount = bigpemu_jag_get_frame_count();

      // Get setting values.  Doing this once per frame, allows player
      // to modify setting during game session.
      int healthSettingValue = INVALID;
      int freezeSecondsSettingValue = INVALID;
      int swordSettingValue = INVALID;
      int chickenSettingValue = INVALID;
      int gunSettingValue = INVALID;
      int ammoSettingValue = INVALID;
      int titleSwordSettingValue = INVALID;
      int plankWeaponSettingValue = INVALID;
      int crowbarWeaponSettingValue = INVALID;
      int stickWeaponSettingValue = INVALID;
      int waterwheelWeaponSettingValue = INVALID;
      int flowerWeaponSettingValue = INVALID;
      int keysSettingValue = INVALID;
      int toolsSettingValue = INVALID;
      int mapsSettingValue = INVALID;
      int ordersSettingValue = INVALID;
      int playAsClawSettingValue = INVALID;
      int playAsHoodSoldierSettingValue = INVALID;
      int playAsHairSoldierSettingValue = INVALID;
      int playAsGunman1SettingValue = INVALID;
      int playAsGunman2SettingValue = INVALID;
      int walkAsRamirezSettingValue = INVALID;

      bigpemu_get_setting_value(&healthSettingValue, sHealthSettingHandle);
      bigpemu_get_setting_value(&freezeSecondsSettingValue, sFreezeSecondsSettingHandle);
      sSleepEnemyCounts = ((uint32_t) (freezeSecondsSettingValue * HL_FRAMES_PER_SECOND));
      bigpemu_get_setting_value(&swordSettingValue, sSwordSettingHandle);
      bigpemu_get_setting_value(&chickenSettingValue, sChickenSettingHandle);
      bigpemu_get_setting_value(&gunSettingValue, sGunSettingHandle);
      bigpemu_get_setting_value(&ammoSettingValue, sAmmoSettingHandle);
      bigpemu_get_setting_value(&titleSwordSettingValue, sTitleSwordSettingHandle);
      bigpemu_get_setting_value(&plankWeaponSettingValue, sPlankWeaponSettingHandle);
      bigpemu_get_setting_value(&crowbarWeaponSettingValue, sCrowbarWeaponSettingHandle);
      bigpemu_get_setting_value(&stickWeaponSettingValue, sStickWeaponSettingHandle);
      bigpemu_get_setting_value(&waterwheelWeaponSettingValue, sWaterwheelWeaponSettingHandle);
      bigpemu_get_setting_value(&flowerWeaponSettingValue, sFlowerWeaponSettingHandle);
      bigpemu_get_setting_value(&keysSettingValue, sKeysSettingHandle);
      bigpemu_get_setting_value(&toolsSettingValue, sToolsSettingHandle);
      bigpemu_get_setting_value(&mapsSettingValue, sMapsSettingHandle);
      bigpemu_get_setting_value(&ordersSettingValue, sOrdersSettingHandle);
      bigpemu_get_setting_value(&playAsClawSettingValue, sPlayAsClawSettingHandle);
      bigpemu_get_setting_value(&playAsHoodSoldierSettingValue, sPlayAsHoodSoldierSettingHandle);
      bigpemu_get_setting_value(&playAsHairSoldierSettingValue, sPlayAsHairSoldierSettingHandle);
      bigpemu_get_setting_value(&playAsGunman1SettingValue, sPlayAsGunman1SettingHandle);
      bigpemu_get_setting_value(&playAsGunman2SettingValue, sPlayAsGunman2SettingHandle);
      bigpemu_get_setting_value(&walkAsRamirezSettingValue, sWalkAsRamirezSettingHandle);

      int modelQuentinAsClawSettingValue = 0;
      int modelQuentinAsHoodSettingValue = 0;
      int modelQuentinAsHairSettingValue = 0;
      int modelQuentinAsRamirezSettingValue = 0;
      int modelQuentinAsArakSettingValue = 0;
      int modelQuentinAsKortanSettingValue = 0;
      int modelQuentinAsMangusSettingValue = 0;
      int modelQuentinAsMaskedWomanSettingValue = 0;
      int modelQuentinAsClydeSettingValue = 0;
      int modelQuentinAsDundeeBSettingValue = 0;
      int modelQuentinAsDundeeDSettingValue = 0;

      bigpemu_get_setting_value(&modelQuentinAsClawSettingValue, sModelQuentinAsClawSettingHandle);
      bigpemu_get_setting_value(&modelQuentinAsHoodSettingValue, sModelQuentinAsHoodSettingHandle);
      bigpemu_get_setting_value(&modelQuentinAsHairSettingValue, sModelQuentinAsHairSettingHandle);
      bigpemu_get_setting_value(&modelQuentinAsRamirezSettingValue, sModelQuentinAsRamirezSettingHandle);
      bigpemu_get_setting_value(&modelQuentinAsArakSettingValue, sModelQuentinAsArakSettingHandle);
      bigpemu_get_setting_value(&modelQuentinAsKortanSettingValue, sModelQuentinAsKortanSettingHandle);
      bigpemu_get_setting_value(&modelQuentinAsMangusSettingValue, sModelQuentinAsMangusSettingHandle);
      bigpemu_get_setting_value(&modelQuentinAsMaskedWomanSettingValue, sModelQuentinAsMaskedWomanSettingHandle);
      bigpemu_get_setting_value(&modelQuentinAsClydeSettingValue, sModelQuentinAsClydeSettingHandle);
      bigpemu_get_setting_value(&modelQuentinAsDundeeBSettingValue, sModelQuentinAsDundeeBSettingHandle);
      bigpemu_get_setting_value(&modelQuentinAsDundeeDSettingValue, sModelQuentinAsDundeeDSettingHandle);

      int pocModelHoodAsRamirezSettingValue = 0;
      int pocModelHoodAsArakSettingValue = 0;
      int pocModelHoodAsKortanSettingValue = 0;
      int pocModelHoodAsMangusSettingValue = 0;
      int pocModelHoodAsMaskedWomanSettingValue = 0;
      int pocModelHoodAsClydeSettingValue = 0;
      int pocModelHoodAsDundeeBSettingValue = 0;
      int pocModelHoodAsDundeeDSettingValue = 0;
      int pocUnusedAnimationSettingValue = 0;

      bigpemu_get_setting_value(&pocModelHoodAsRamirezSettingValue, sPocModelHoodAsRamirezSettingHandle);
      bigpemu_get_setting_value(&pocModelHoodAsArakSettingValue, sPocModelHoodAsArakSettingHandle);
      bigpemu_get_setting_value(&pocModelHoodAsKortanSettingValue, sPocModelHoodAsKortanSettingHandle);
      bigpemu_get_setting_value(&pocModelHoodAsMangusSettingValue, sPocModelHoodAsMangusSettingHandle);
      bigpemu_get_setting_value(&pocModelHoodAsMaskedWomanSettingValue, sPocModelHoodAsMaskedWomanSettingHandle);
      bigpemu_get_setting_value(&pocModelHoodAsClydeSettingValue, sPocModelHoodAsClydeSettingHandle);
      bigpemu_get_setting_value(&pocModelHoodAsDundeeBSettingValue, sPocModelHoodAsDundeeBSettingHandle);
      bigpemu_get_setting_value(&pocModelHoodAsDundeeDSettingValue, sPocModelHoodAsDundeeDSettingHandle);
      bigpemu_get_setting_value(&pocUnusedAnimationSettingValue, sPocUnusedAnimationSettingHandle);

      int wipModelGunAsTurretValue = 0;
      bigpemu_get_setting_value(&wipModelGunAsTurretValue, sWipModelGunAsTurretSettingHandle);

      mFighter = QUENTIN;
      if (modelQuentinAsClawSettingValue > 0)
      {
         mFighter = CLAW;
      }
      else if (modelQuentinAsHoodSettingValue > 0)
      {
         mFighter = HOOD;
      }
      else if (modelQuentinAsHairSettingValue > 0)
      {
         mFighter = HAIR;
      }
      else if (modelQuentinAsRamirezSettingValue > 0)
      {
         mFighter = RAMIREZ;
      }
      else if (modelQuentinAsArakSettingValue > 0)
      {
         mFighter = ARAK;
      }
      else if (modelQuentinAsKortanSettingValue > 0)
      {
         mFighter = KORTAN;
      }
      else if (modelQuentinAsMangusSettingValue > 0)
      {
         mFighter = MANGUS;
      }
      else if (modelQuentinAsMaskedWomanSettingValue > 0)
      {
         mFighter = MASKED_WOMAN;
      }
      else if (modelQuentinAsClydeSettingValue > 0)
      {
         mFighter = CLYDE;
      }
      else if (modelQuentinAsDundeeBSettingValue > 0)
      {
         mFighter = DUNDEE_B;
      }
      else if (modelQuentinAsDundeeDSettingValue > 0)
      {
         mFighter = DUNDEE_D;
      }
      //Turbine
      //Tank
      //Turret

      enum FightersEnum hoodReplacementFighter = HOOD;
      if (pocModelHoodAsRamirezSettingValue > 0)
      {
         hoodReplacementFighter = RAMIREZ;
      }
      else if (pocModelHoodAsArakSettingValue > 0)
      {
         hoodReplacementFighter = ARAK;
      }
      else if (pocModelHoodAsKortanSettingValue > 0)
      {
         hoodReplacementFighter = KORTAN;
      }
      else if (pocModelHoodAsMangusSettingValue > 0)
      {
         hoodReplacementFighter = MANGUS;
      }
      else if (pocModelHoodAsMaskedWomanSettingValue > 0)
      {
         hoodReplacementFighter = MASKED_WOMAN;
      }
      else if (pocModelHoodAsClydeSettingValue > 0)
      {
         hoodReplacementFighter = CLYDE;
      }
      else if (pocModelHoodAsDundeeBSettingValue > 0)
      {
         hoodReplacementFighter = DUNDEE_B;
      }
      else if (pocModelHoodAsDundeeDSettingValue > 0)
      {
         hoodReplacementFighter = DUNDEE_D;
      }
      //Turbine
      //Tank
      //Turret

      // Quentin Item is first Item in Item List.
      const uint32_t pQuentinItemJagAddr = ((uint32_t) HL_ITEM_LIST_ADDR);
      const uint32_t ownerOffset = ((uint32_t) (&(sHlItems[0].mOwner))) - ((uint32_t) (&(sHlItems[0])));
      const uint32_t sheetOffset = ((uint32_t) (&(sHlItems[0].mSheet))) - ((uint32_t) (&(sHlItems[0])));

      if (! sEnemyModelReplacedInRAM)
      {
         // Enemies have 2 less animations then Quentin.
         // Enemy's character sheet is fully populated when last pointer in character sheet is no longer garbage value.
         const uint32_t pLastMiscPtr = HL_HOOD_SOLDIER_SHEET_P_ADDR + HL_SIZE_OF_CHAR_SHEET_HEADER + (HL_NUM_MODELS_PER_FIGHTER*HL_SIZE_OF_POINTER) + ((HL_NUM_ANIMATIONS_QUENTIN-2)*HL_SIZE_OF_POINTER) + ((HL_NUM_MISC_FIGHTER-1)*HL_SIZE_OF_POINTER);
         const uint32_t lastMiscAddr = bigpemu_jag_read32(pLastMiscPtr);
         if (0 != lastMiscAddr && 0xFFFFFFFF != lastMiscAddr)
         {
            // The enemy's character sheet is finally fully populated. 
            sEnemyModelReplacedInRAM = 1;

            // Replace the enemys model data with the selected fighter's model data.
            costumeChange(HL_HOOD_SOLDIER_SHEET_P_ADDR, hoodReplacementFighter);

            // WIP: Probably need to have different height offsets for every target character sheet and source replacement fighter combination.
            //	Better yet would be figure out how to calculate what the offset should be.  For Quentin, was lazy and just trial-and-error eyeballed what height offset to apply.
            int weaponInduced = 0;
            applyHeightOffset(HL_HOOD_SOLDIER_SHEET_P_ADDR, hoodReplacementFighter, weaponInduced);
         }
      }

      // Consider RAM initialized once Quentin character item is initialized.  Quentin
      // Item has unique character sheet address.
      const uint32_t quentinSheet = bigpemu_jag_read32(HL_ITEM_LIST_ADDR + sheetOffset);
      if (HL_QUENTIN_SHEET_P_ADDR == quentinSheet)
      {
         // Initiate a delayed response to sheet creation.
         if (WAIT == sCostumeEnum)
         {
            sCostumeEnum = DELAY;
         }
         if (DELAY == sCostumeEnum)
         {
            if (sCostumeDelayCounter > HL_COSTUME_DELAY_TICKS)
            {
               sCostumeEnum = BEGIN;
            }
            ++sCostumeDelayCounter;
         }
         else if (BEGIN == sCostumeEnum)
         {
            sCostumeEnum = DONE;

            if ((titleSwordSettingValue > 0) && gFighterHackAttributes[mFighter].mSupportTitleScreenSword)
            {
               // Reorient some items so they display as expected, when used as a weapon.
               orientTitleScreenSword();
            }
            orientFlower();

            if (wipModelGunAsTurretValue)
            {
               // Overwrite gas gun model data with turret model data.
               copyTurretToGasgunModel();
            }

            // Copy fighter's model data over Quentin's model data.
            costumeChange(HL_QUENTIN_SHEET_P_ADDR, mFighter);

            // If quentin character sheet being used.
            const uint32_t characterSheet = bigpemu_jag_read32(HL_ITEM_LIST_ADDR + sheetOffset);
            if (HL_QUENTIN_SHEET_P_ADDR == characterSheet)
            {
               // But another fighters model data was copied over Quentin's model data.
               if (QUENTIN != mFighter)
               {
                  // Apply offset for fighter to get the copied model data's feet to touch (near) the ground.
                  int weaponInduced = 0;
                  applyHeightOffset(HL_QUENTIN_SHEET_P_ADDR, mFighter, weaponInduced);
               }
            }

            if (pocUnusedAnimationSettingValue > 0)
            {
               // Overwrite animation17 (0-based) -> Joypad-up + A-button (walking jump forward)
               // with jump-diving-summersault animation.
               const uint32_t blockOffset = 0x000001C0;
               const int sourceAnimNum = 5; // zero based
               const int destAnimNum = 17; // zero based
               readAnimationFromCompactDiscTrack8(blockOffset, sourceAnimNum, HL_QUENTIN_SHEET_P_ADDR, destAnimNum);
            }
         }

         // Found the "Rogues Gallery to be very annoying to navigate when trying to place
         // flower in vase, via joypad.  Repeatedly kept displaying developer faces, and
         // still don't know how to get away from the faces.  And they uglified the faces
         // too.
         //
         // Set the bit within gamestate memory area that specifies whether teleporter
         // doors are active or not.  Don't make this a setting, as low risk, and noone
         // should have to deal with the faces coming up when trying to put flower in
         // vase.  Faces are super annoying.
         const uint32_t stateByteAddr = 0x0000440D;
         uint8_t stateByte = bigpemu_jag_read8(stateByteAddr);
         stateByte |= (1 << 7);
         bigpemu_jag_write8(0x0000440D, stateByte);

         // Store copy of the item list.
         if (! sItemsInitializedInRAM)
         {
            // Read the item list up front, first chance get.
            // Not really taking advantage of this since making settings be respected
            // on per frame basis.
            // But its only one read, f it.  Let it stay.
            readItemList();
            sItemsInitializedInRAM = 1;

            // You can play as:
            //    Claw
            //    Hooded soldier
            //    Hair soldier
            //    Gun Man 1 and Gun Man 2
            //
            // Tried playing as Clyde, and she would initially only rotate.  Then after
            // arming Clyde with weapon, Clyde flipped out, and flew back and hit ground
            // in infinite-loop fashion.  Like a possessed rag doll.  And you could control
            // direction/movement of the possessed rag doll.  Not very useful, but kinda humorous.
            // Stop using the weapon, and the possessed rag doll behavior stops.  Almost
            // like assigning weapon to Quentin Item with that character sheet was injuring
            // the Quentin Item (though lifebar did not go down from this behavior).
            //
            // Tried playing as Ramirez.  Ramirez can walk around, but not run, and not fight.
            // Selecting to use Macleod Sword, Gas Gun, or Chicken causes Ramirez to do the
            // possessed rag doll thing.  Total disappointment, after seeing Ramirez with a
            // sword already drawn.  Not surprising though.
            //
            // Dundee B same as Clyde.
            // Dundee D same as Clyde.
            // Tank displays, but can't even rotate.
            // Turret displays, rotates, and even lets you fire at and kill the Hood Soldier.  Same
            // rag doll experience if you try arming a weapon (though you don't need to arm
            // the turret to shoot.  Someone could probably change the Scene and XYZPos values of
            // a number of enemy fighters to appear in this screen, and then use the turret as part
            // of shooting gallery.  Could.
            // Turbine displays/rotates but nothing much to report.
            //
            //
            //
            // Set Quentin to use BigPEmu character sheet specified
            // by BigPEmu user config setting.
            //
            // Do not allow this to change after gameplay starts.
            // Freezes game if you change in session.
            // Pretty certain the freezing when try to change char
            // sheet mid session/game, occurs because the models
            // and animations associated to the character sheet are
            // dynamically loaded/unloaded on as needed basis, but
            // simply programatically overwriting the character
            // sheet pointer (within Item) without invoking the code
            // to load the associated models/animations is going to
            // cause problems.  Overwrite the character sheet pointer
            // at game startup, and then the logic to load the
            // associated models/animations runs for the overwritten
            // character sheet pointer
            //
            // TODO: test whether this messes up memory track save slots.
            uint32_t characterSheet = HL_QUENTIN_SHEET_P_ADDR;
            if (playAsClawSettingValue > 0)
            {
               // Design Decision: Pretty much give up on mixing and matching animations from fighters,
               //                  or copying full set of fighter animations over quentin animation sets
               // Rationale: There is a seperate set of animations for bare hands, macleod sword, gun, and chicken, that are applied/used to/as Quentins animations.
               //	           Fighters only supply one set of those animations (gun for gunmen, sword for swordmen, bare hands for quentin).
               //            Might think you could use gunmans animations when have gun, and use swordmans animations when have sword,
               //            however, Claw goes from being very hunched over, to being upright like the gunmen, which doesn't look right and seems very off
               //            Does not appear there is a way to add a file reference to character sheet to overwrite animations to an entry point equal to animation pointers indices, as appears the models prior to animations on compact disc track would get copied too.
               //            Source code for how models/animations are stored on compact disc changed between revision of source code and production release of game, as well.
               //            There is also the PIA nature of always having to pay attention to when different weapon is selected and then load and overwrite the current animations with animations the mod requires
               //            And this adds good amount of risk of breaking things.
               //            And adds good amount of risk of going down an unanticipated rabbit hole.
               characterSheet = HL_CLAW_SHEET_P_ADDR;
            }
            else if (playAsHairSoldierSettingValue > 0)
            {
               characterSheet = HL_HAIR_SOLDIER_SHEET_P_ADDR;
            }
            else if (playAsHoodSoldierSettingValue > 0)
            {
               characterSheet = HL_HOOD_SOLDIER_SHEET_P_ADDR;
            }
            else if (playAsGunman1SettingValue > 0)
            {
               characterSheet = HL_GUNMAN1_SHEET_P_ADDR;
            }
            else if (playAsGunman2SettingValue > 0)
            {
               characterSheet = HL_GUNMAN2_SHEET_P_ADDR;
            }
            else if (walkAsRamirezSettingValue > 0)
            {
               characterSheet = HL_RAMIREZ_SHEET_P_ADDR;
            }

            // Track8 Animations Discovery
            //
            // Uncomment and edit blockoffset value to test/view the unused
            // animations provided on Track8(1-based) on compact disc.
            //
            // Joypad controls to initiate the test animations:
            //    A-button
            //    B-button
            //    C-button
            //    up + B-button
            //    up + C-button
            //    double up + B-button
            //    double up + C-button
            //    double down + A-button
            // You need to keep controls pressed, until animation is complete, or else
            // you will not see the complete animation.
            //
            //const uint32_t blockOffset = 0x00000000;
            //const uint32_t blockOffset = 0x00000038;
            //const uint32_t blockOffset = 0x00000070;
            //const uint32_t blockOffset = 0x000000A8;
            //const uint32_t blockOffset = 0x000000E0;
            //const uint32_t blockOffset = 0x00000118;
            //const uint32_t blockOffset = 0x00000150;
            //const uint32_t blockOffset = 0x00000188;
            //const uint32_t blockOffset = 0x000001C0;
            //const uint32_t blockOffset = 0x000001F8;
            //characterSheet = createTestSheet(blockOffset);

            bigpemu_jag_write32(HL_ITEM_LIST_ADDR + sheetOffset, characterSheet);

            // If playing as non-Quentin fighter.
            if (HL_QUENTIN_SHEET_P_ADDR != characterSheet)
            {
               // Deactivate all food/drink items, as non-Quentin fighters do not have animations
               // to support use of non food/drink items.  Without disabling food/drink items,
               // Non-Quentin fighters attempting to use food/drink items results in player disappearing
               // from screen and inability to move to different screen.
               //
               // Recommend enabling Highlander Misc - Infinite Health when playing as non-Quentin fighter.
               const uint32_t flagsOffset = ((uint32_t) (&(sHlItems[0].mFlags))) - ((uint32_t) (&(sHlItems[0])));
               for (uint8_t itemIdx = 0; itemIdx < HL_NUM_ITEMS; ++itemIdx)
               {
                  const HlItem* const pItem = &(sHlItems[itemIdx]);
                  if (0x32 == sHlItems[itemIdx].mRadius)
                  {
                     const uint32_t sheet = sHlItems[itemIdx].mSheet;
                     if ((HL_WINE_SHEET_P_ADDR == sheet) ||
                         (HL_CHEESE_SHEET_P_ADDR == sheet) ||
                         (HL_LOAF_SHEET_P_ADDR == sheet))
                     {
                        bigpemu_jag_write8(HL_ITEM_LIST_ADDR + (itemIdx * HL_ITEM_SIZE) + flagsOffset, 1<<HL_WSTDeactivated);
                     }
                  }
               }
            }
         }
      }

      // Don't do anything that results in a read/write to item list, until
      // RAM has been initialized.
      if (sItemsInitializedInRAM)
      {
         // Calculate offsets to radius, life, flags fields.
         const uint32_t radiusOffset = ((uint32_t) (&(sHlItems[0].mRadius))) - ((uint32_t) (&(sHlItems[0])));
         const uint32_t lifeOffset = ((uint32_t) (&(sHlItems[0].mLife))) - ((uint32_t) (&(sHlItems[0])));
         const uint32_t flagsOffset = ((uint32_t) (&(sHlItems[0].mFlags))) - ((uint32_t) (&(sHlItems[0])));

         if (0 == sAwakenEnemiesCountdown)
         { // If there is no active countdown.
            // no op
         }
         else if ((sAwakenEnemiesCountdown > 1) && (sAwakenEnemiesCountdown < sSleepEnemyCounts))
         { // If in the middle of a countdown.
            // Continue countdown.
            --sAwakenEnemiesCountdown;
         }
         else if (1 == sAwakenEnemiesCountdown)
         { // If at end of countdown (0-value is considered no-countdown, 1-value is considered end-of-countdown)

            // For all items in item list, except first item (first item is Quentin item)
            for (uint8_t itemIdx = 1; itemIdx < HL_NUM_ITEMS; ++itemIdx)
            {
               // Determine if item is enmey fighter item.
               const HlItem* const pItem = &sHlItems[itemIdx];
               int isSoldier = 0;
               if (HL_PERSON_RADIUS == pItem->mRadius)
               {
                  isSoldier = 1;
               }

               // If soldier item.
               if (isSoldier)
               {
                  // Obvious: revert life back to prior value.
                  const uint8_t oldLifeValue = sHlItems[itemIdx].mLife;

                  // Not so obvious: Revert radius back to prior value.
                  //
                  // Every enemy fighter that was "engaged" with Quentin at
                  // start of sleep, has his radius set to 0 by game
                  // logic, when game logic detects life value being
                  // set to 0.
                  //
                  // When radius is set to zero, and enemy fighter reawakens,
                  // enemy fighter approaches/fights Quentin like normal, however
                  // all hit detection between enemy fighter and Quentin seems
                  // to be turned off.
                  const uint8_t oldRadiusValue = sHlItems[itemIdx].mRadius;

                  bigpemu_jag_write8(HL_ITEM_LIST_ADDR + (itemIdx * HL_ITEM_SIZE) + lifeOffset, oldLifeValue);
                  bigpemu_jag_write16(HL_ITEM_LIST_ADDR + (itemIdx * HL_ITEM_SIZE) + radiusOffset, oldRadiusValue);
               }
            }

            sAwakenEnemiesCountdown = 0;
         }
         else if ((sAwakenEnemiesCountdown > 0) && (sSleepEnemyCounts == sAwakenEnemiesCountdown))
         { // If at start of countdown.

            // Refresh copy of the item list.
            readItemList();

            // For all items in item list, except first item (first item is Quentin item)
            for (uint8_t itemIdx = 1; itemIdx < HL_NUM_ITEMS; ++itemIdx)
            {
               // Determine if item is enemy fighter item.
               const HlItem* const pItem = &sHlItems[itemIdx];
               int isSoldier = 0;
               if (HL_PERSON_RADIUS == pItem->mRadius)
               {
                  isSoldier = 1;
               }

               // If soldier item.
               if (isSoldier)
               {
                  // Mark enemy fighter as dead.
                  // Really don't feel like byte swapping again, and only need to write one field of Item record anyways.
                  bigpemu_jag_write8(HL_ITEM_LIST_ADDR + (itemIdx * HL_ITEM_SIZE) + lifeOffset, 0);
               }
            }

            --sAwakenEnemiesCountdown;
         }

         if (healthSettingValue > 0)
         {
            bigpemu_jag_write8(HL_HEALTH_ADDR, HL_HEALTH_VALUE);
         }

         // Don't really need to refresh the sHlItems list for below, since
         // only changing owner to Quentin.
         if (swordSettingValue > 0)
         {
            for (uint32_t itemIdx = 0; itemIdx < HL_NUM_ITEMS; ++itemIdx)
            {
               const uint32_t sheet = sHlItems[itemIdx].mSheet;
               if (HL_SWORD_SHEET_P_ADDR == sheet)
               {
                  // Update script copy of Item List.
                  sHlItems[itemIdx].mOwner = pQuentinItemJagAddr;

                  // Update Item List in Jaguar RAM.
                  bigpemu_jag_write32(HL_ITEM_LIST_ADDR + (itemIdx * HL_ITEM_SIZE) + ownerOffset, sHlItems[itemIdx].mOwner);

                  // There is only one to find.
                  break;
               }
            }
         }
         if (chickenSettingValue > 0)
         {
            for (uint32_t itemIdx = 0; itemIdx < HL_NUM_ITEMS; ++itemIdx)
            {
               // Only one of each of these items are available.
               const uint32_t sheet = sHlItems[itemIdx].mSheet;
               if (HL_CHICKEN_SHEET_P_ADDR == sheet)
               {
                  // Update script copy of Item List.
                  sHlItems[itemIdx].mOwner = pQuentinItemJagAddr;

                  // Update Item List in Jaguar RAM.
                  bigpemu_jag_write32(HL_ITEM_LIST_ADDR + (itemIdx * HL_ITEM_SIZE) + ownerOffset, sHlItems[itemIdx].mOwner);
                  uint32_t sheetAddr = HL_CHICKEN_SHEET_P_ADDR;

                  // Select model different then the rubber chicken's model.
                  if (titleSwordSettingValue > 0)
                  {
                     if (gFighterHackAttributes[mFighter].mSupportTitleScreenSword)
                     {
                        sheetAddr = HL_TITLE_SCREEN_SWORD_SHEET_P_ADDR;
                     }
                  }
                  if (plankWeaponSettingValue > 0)
                  {
                     sheetAddr = HL_PLANK_SHEET_P_ADDR;
                  }
                  if (crowbarWeaponSettingValue > 0)
                  {
                     sheetAddr = HL_CROWBAR_SHEET_P_ADDR;
                  }
                  if (stickWeaponSettingValue > 0)
                  {
                     sheetAddr = HL_STICK_SHEET_P_ADDR;
                  }
                  if (waterwheelWeaponSettingValue > 0)
                  {
                     sheetAddr = HL_WATERWHEEL_SHEET_P_ADDR;
                  }
                  if (flowerWeaponSettingValue > 0)
                  {
                     sheetAddr = HL_FLOWER_SHEET_P_ADDR;
                  }

                  // Overwrite pointer to first model (weapon character sheets only have one model to point to).
                  const uint32_t altModelAddr = bigpemu_jag_read32(sheetAddr + HL_SIZE_OF_CHAR_SHEET_HEADER);
                  bigpemu_jag_write32(HL_CHICKEN_SHEET_P_ADDR + HL_SIZE_OF_CHAR_SHEET_HEADER, altModelAddr);

                  // There is only one to find.
                  break;
               }
            }
         }
         if (gunSettingValue > 0)
         {
            for (uint32_t itemIdx = 0; itemIdx < HL_NUM_ITEMS; ++itemIdx)
            {
               // Only one of each of these items are available.
               const uint32_t sheet = sHlItems[itemIdx].mSheet;
               if (HL_GAS_GUN_SHEET_P_ADDR == sheet)
               {
                  // Update script copy of Item List.
                  sHlItems[itemIdx].mOwner = pQuentinItemJagAddr;

                  // Update Item List in Jaguar RAM.
                  bigpemu_jag_write32(HL_ITEM_LIST_ADDR + (itemIdx * HL_ITEM_SIZE) + ownerOffset, sHlItems[itemIdx].mOwner);

                  // There are multiple to find, break after finding first one.
                  break;
               }
            }
         }
         if (ammoSettingValue > 0)
         {
            for (uint32_t itemIdx = 0; itemIdx < HL_NUM_ITEMS; ++itemIdx)
            {
               const uint32_t sheet = sHlItems[itemIdx].mSheet;

               if ((HL_GAS_GUN_SHEET_P_ADDR == sheet))
               {
                  // Update script copy of Item List.
                  sHlItems[itemIdx].mLife = 8;

                  // Update Item List in Jaguar RAM.
                  bigpemu_jag_write8(HL_ITEM_LIST_ADDR + (itemIdx * HL_ITEM_SIZE) + lifeOffset, sHlItems[itemIdx].mLife);

                  // There are multiple Gun Items.  Update all of them to infinite.
               }
            }
         }
         // TODO short circuit below and above, so doesn't set sheet and/or set model pointer for every enabled weapon
         //      should just set/enable for first or last encountered
         // TODO methodize the loop

         if (keysSettingValue > 0)
         {
            // There are multiple keys.  Assign ownership of all to Quentin Item.
            for (uint32_t itemIdx = 0; itemIdx < HL_NUM_ITEMS; ++itemIdx)
            {
               const uint32_t sheet = sHlItems[itemIdx].mSheet;
               if (HL_KEY_SHEET_P_ADDR == sheet)
               {
                  // Update script copy of Item List.
                  sHlItems[itemIdx].mOwner = pQuentinItemJagAddr;

                  // Update Item List in Jaguar RAM.
                  bigpemu_jag_write32(HL_ITEM_LIST_ADDR + (itemIdx * HL_ITEM_SIZE) + ownerOffset, sHlItems[itemIdx].mOwner);
               }
            }
         }
         if (toolsSettingValue > 0)
         {
            // There are multiple tools.  Assign ownership of all to Quentin Item.
            for (uint32_t itemIdx = 0; itemIdx < HL_NUM_ITEMS; ++itemIdx)
            {
               const uint32_t sheet = sHlItems[itemIdx].mSheet;
               if (HL_FLOWER_SHEET_P_ADDR == sheet ||
                   HL_CROWBAR_SHEET_P_ADDR == sheet ||
                   HL_LEVER_SHEET_P_ADDR == sheet ||
                   HL_WATERWHEEL_SHEET_P_ADDR == sheet ||
                   HL_PLANK_SHEET_P_ADDR == sheet ||
                   HL_STOPWATCH_SHEET_P_ADDR == sheet ||
                   HL_LOCKET_SHEET_P_ADDR == sheet ||
                   HL_UNIFORM_SHEET_P_ADDR == sheet ||
                   HL_STICK_SHEET_P_ADDR == sheet)
               {
                  // Update script copy of Item List.
                  sHlItems[itemIdx].mOwner = pQuentinItemJagAddr;

                  // Update Item List in Jaguar RAM.
                  bigpemu_jag_write32(HL_ITEM_LIST_ADDR + (itemIdx * HL_ITEM_SIZE) + ownerOffset, sHlItems[itemIdx].mOwner);
               }
            }
         }
         if (mapsSettingValue > 0)
         {
            // There are multiple maps.  Assign ownership of all to Quentin Item.
            for (uint32_t itemIdx = 0; itemIdx < HL_NUM_ITEMS; ++itemIdx)
            {
               // Only one of each of these items are available.
               const uint32_t sheet = sHlItems[itemIdx].mSheet;
               if (HL_MAP_SHEET_P_ADDR == sheet)
               {
                  // Update script copy of Item List.
                  sHlItems[itemIdx].mOwner = pQuentinItemJagAddr;

                  // Update Item List in Jaguar RAM.
                  bigpemu_jag_write32(HL_ITEM_LIST_ADDR + (itemIdx * HL_ITEM_SIZE) + ownerOffset, sHlItems[itemIdx].mOwner);
               }
            }
         }
         if (ordersSettingValue > 0)
         {
            // There are multiple orders.  Assign ownership of all to Quentin Item.
            for (uint32_t itemIdx = 0; itemIdx < HL_NUM_ITEMS; ++itemIdx)
            {
               // Only one of each of these items are available.
               const uint32_t sheet = sHlItems[itemIdx].mSheet;
               if (HL_ORDERS_SHEET_P_ADDR == sheet)
               {
                  // Update script copy of Item List.
                  sHlItems[itemIdx].mOwner = pQuentinItemJagAddr;

                  // Update Item List in Jaguar RAM.
                  bigpemu_jag_write32(HL_ITEM_LIST_ADDR + (itemIdx * HL_ITEM_SIZE) + ownerOffset, sHlItems[itemIdx].mOwner);
               }
            }
         }

         uint32_t lastAnimPtrOffset = HL_SIZE_OF_CHAR_SHEET_HEADER + (HL_NUM_MODELS_PER_WEAPON * HL_SIZE_OF_POINTER) + ((HL_NUM_ANIMATIONS_QUENTIN - 2 - 1) * HL_SIZE_OF_POINTER);
         for (int wIdx = 0; wIdx < 3; ++wIdx)
         {
            const uint32_t lastAnimPtr = weaponCharSheets[wIdx] + lastAnimPtrOffset;
            const uint32_t newLastAnimAddr = bigpemu_jag_read32(lastAnimPtr);
            if ((newLastAnimAddr != weaponLastAnimAddrs[wIdx]) && (0 != newLastAnimAddr))
            {
               // Apply offset for fighter to get the copied model data's feet to touch (near) the ground.
               const int weaponInduced = 1;
               applyHeightOffset(weaponCharSheets[wIdx], mFighter, weaponInduced);
            }
            weaponLastAnimAddrs[wIdx] = newLastAnimAddr;
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
   const int catHandle = bigpemu_register_setting_category(pMod, "Highlander Misc");
   sHealthSettingHandle = bigpemu_register_setting_bool(catHandle, "Infinite Health", 1);

   // Settings for weapons.  User might want some fidelity in which weapons are unlocked.
   // Eg.  Unlocking the sword, may take away from game journey and possibly even prevent
   // FMV clip from playing.  Infinite ammo is also something user may or may not want.
   const int weaponCatHandle = bigpemu_register_setting_category(pMod, "Highlander Weapons");
   sSwordSettingHandle = bigpemu_register_setting_bool(weaponCatHandle, "Unlock Sword", 1);
   sChickenSettingHandle = bigpemu_register_setting_bool(weaponCatHandle, "Unlock Chicken", 1);
   sGunSettingHandle = bigpemu_register_setting_bool(weaponCatHandle, "Unlock Gun", 1);
   sAmmoSettingHandle = bigpemu_register_setting_bool(weaponCatHandle, "Infinite Ammo", 1);
   sTitleSwordSettingHandle = bigpemu_register_setting_bool(weaponCatHandle, "Title Sword", 0);
   sPlankWeaponSettingHandle = bigpemu_register_setting_bool(weaponCatHandle, "Plank", 0);
   sCrowbarWeaponSettingHandle = bigpemu_register_setting_bool(weaponCatHandle, "Crowbar", 0);
   sStickWeaponSettingHandle = bigpemu_register_setting_bool(weaponCatHandle, "Stick", 0);
   sWaterwheelWeaponSettingHandle = bigpemu_register_setting_bool(weaponCatHandle, "Waterwheel", 0);
   sFlowerWeaponSettingHandle = bigpemu_register_setting_bool(weaponCatHandle, "Flower", 0);

   // This setting when enabled, and associated button is pressed, causes all enemies fighters
   // to basically go to sleep for the setting-specified amount of seconds.
   //
   // This is intended to help with BS frustration-loop situations in game such as:
   // a) enemy fighter has cornered Quentin, and enemty fighter keeps repeatedly attacking Quentin,
   //    while Quentin does not have time to counter attack.
   // b) enemy fighter keeps repeatedly pushing Quentin offscreen as soon as Quentin comes back on screen
   // c) enemy fighter catches Quentin with back turned, and keeps repeatedly attacking Quentin,
   //    preventing Quentin from having time to turnaround and counter attack.
   //
   // If player invokes this multiple times (with setting value maxed out), player can probably get through
   // game without ever having to fight a single enemy.
   //
   // Probably could modify the implementation to only impact enemy fighters within a certain radius of,
   // Quentin, but do not see the reward being worth the effort at this time.
   //
   sFreezeSecondsSettingHandle = bigpemu_register_setting_int_full(catHandle, "Freeze Length (S)", 0, 0, HL_MAX_FREEZE_LENGTH, 1);

   sKeysSettingHandle = bigpemu_register_setting_bool(catHandle, "Unlock All Keys", 1);
   sToolsSettingHandle = bigpemu_register_setting_bool(catHandle, "Unlock All Tools", 1);
   sMapsSettingHandle = bigpemu_register_setting_bool(catHandle, "Unlock All Maps", 1);
   sOrdersSettingHandle = bigpemu_register_setting_bool(catHandle, "Unlock All Orders", 1);

   // Don't see any need to have settings for food/life items (BOTTLE, BREAD, CHEESE), given
   // an infinite health setting is available and less annoying to use.

   const int playAsCatHandle = bigpemu_register_setting_category(pMod, "Highlander Play As");
   sPlayAsClawSettingHandle = bigpemu_register_setting_bool(playAsCatHandle, "Claw", 0);
   sPlayAsHoodSoldierSettingHandle = bigpemu_register_setting_bool(playAsCatHandle, "Hood Soldier", 0);
   sPlayAsHairSoldierSettingHandle = bigpemu_register_setting_bool(playAsCatHandle, "Hair Soldier", 0);
   sPlayAsGunman1SettingHandle = bigpemu_register_setting_bool(playAsCatHandle, "Gun Man 1", 0);
   sPlayAsGunman2SettingHandle = bigpemu_register_setting_bool(playAsCatHandle, "Gun Man 2", 0);
   sWalkAsRamirezSettingHandle = bigpemu_register_setting_bool(playAsCatHandle, "Ramirez Walk Only", 0);

   // Create settings to allow user to select Quentin to look like other characters.
   // 
   // No reason to supply settings to display Quentin to look like Gunman1 or Gunman2.
   // While there are separate character sheets for Gunman1 and Gunman2, Gunman1 and Gunman2
   // models appear to be duplicate or very close to duplicate of Hood Soldier model, once
   // the gun is taken away from Gunman1 and Gunman2's right hand models.
   const int modelAsCatHandle = bigpemu_register_setting_category(pMod, "Highlander Model Quentin As");
   sModelQuentinAsClawSettingHandle = bigpemu_register_setting_bool(modelAsCatHandle, "Claw", 0);
   sModelQuentinAsHoodSettingHandle = bigpemu_register_setting_bool(modelAsCatHandle, "Hood Soldier", 0);
   sModelQuentinAsHairSettingHandle = bigpemu_register_setting_bool(modelAsCatHandle, "Hair Soldier", 0);
   sModelQuentinAsRamirezSettingHandle = bigpemu_register_setting_bool(modelAsCatHandle, "Ramirez", 0);
   sModelQuentinAsArakSettingHandle = bigpemu_register_setting_bool(modelAsCatHandle, "Arak", 0);
   sModelQuentinAsKortanSettingHandle = bigpemu_register_setting_bool(modelAsCatHandle, "Kortan", 0);
   sModelQuentinAsMangusSettingHandle = bigpemu_register_setting_bool(modelAsCatHandle, "Mangus", 0);
   sModelQuentinAsMaskedWomanSettingHandle = bigpemu_register_setting_bool(modelAsCatHandle, "Masked Woman", 0);
   sModelQuentinAsClydeSettingHandle = bigpemu_register_setting_bool(modelAsCatHandle, "Clyde", 0);
   sModelQuentinAsDundeeBSettingHandle = bigpemu_register_setting_bool(modelAsCatHandle, "Dundee B", 0);
   sModelQuentinAsDundeeDSettingHandle = bigpemu_register_setting_bool(modelAsCatHandle, "Dundee D", 0);

   // No great reason to supply settings to display Hood as Hood, Hair, Gunman1, Gunman2, Claw, as can
   // already play against those fighters.
   const int pocModelHoodAsCatHandle = bigpemu_register_setting_category(pMod, "Highlander POC Model Hood As");
   sPocModelHoodAsRamirezSettingHandle = bigpemu_register_setting_bool(pocModelHoodAsCatHandle, "Ramirez", 0);
   sPocModelHoodAsArakSettingHandle = bigpemu_register_setting_bool(pocModelHoodAsCatHandle, "Arak", 0);
   sPocModelHoodAsKortanSettingHandle = bigpemu_register_setting_bool(pocModelHoodAsCatHandle, "Kortan", 0);
   sPocModelHoodAsMangusSettingHandle = bigpemu_register_setting_bool(pocModelHoodAsCatHandle, "Mangus", 0);
   sPocModelHoodAsMaskedWomanSettingHandle = bigpemu_register_setting_bool(pocModelHoodAsCatHandle, "Masked Woman", 0);
   sPocModelHoodAsClydeSettingHandle = bigpemu_register_setting_bool(pocModelHoodAsCatHandle, "Clyde", 0);
   sPocModelHoodAsDundeeBSettingHandle = bigpemu_register_setting_bool(pocModelHoodAsCatHandle, "Dundee B", 0);
   sPocModelHoodAsDundeeDSettingHandle = bigpemu_register_setting_bool(pocModelHoodAsCatHandle, "Dundee D", 0);

   const int pocUnusedAnimationHandle = bigpemu_register_setting_category(pMod, "Highlander POC Unused Animation");
   sPocUnusedAnimationSettingHandle = bigpemu_register_setting_bool(pocUnusedAnimationHandle, "Up + A", 0);

   const int wipModelGunAsCatHandle = bigpemu_register_setting_category(pMod, "Highlander WIP Model Gun As");
   sWipModelGunAsTurretSettingHandle = bigpemu_register_setting_bool(wipModelGunAsCatHandle, "Turret", 0);

   sOnInputFrameEvent = bigpemu_register_event_input_frame(pMod, on_input_frame);
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

   sItemsInitializedInRAM = INVALID;
   sEnemyModelReplacedInRAM = INVALID;
   sAwakenEnemiesCountdown = 0;
   sHealthSettingHandle = INVALID;
   sWeaponsLabelSettingHandle = 0;
   sFreezeSecondsSettingHandle = INVALID;
   sSleepEnemyCounts = 0;

   sSwordSettingHandle = INVALID;
   sChickenSettingHandle = INVALID;
   sGunSettingHandle = INVALID;
   sAmmoSettingHandle = INVALID;
   sTitleSwordSettingHandle = INVALID;
   sPlankWeaponSettingHandle = INVALID;
   sCrowbarWeaponSettingHandle = INVALID;
   sStickWeaponSettingHandle = INVALID;
   sWaterwheelWeaponSettingHandle = INVALID;
   sFlowerWeaponSettingHandle = INVALID;

   sKeysSettingHandle = INVALID;
   sToolsSettingHandle = INVALID;
   sMapsSettingHandle = INVALID;
   sOrdersSettingHandle = INVALID;

   sPlayAsClawSettingHandle = INVALID;
   sPlayAsHoodSoldierSettingHandle = INVALID;
   sPlayAsHairSoldierSettingHandle = INVALID;
   sPlayAsGunman1SettingHandle = INVALID;
   sPlayAsGunman2SettingHandle = INVALID;
   sWalkAsRamirezSettingHandle = INVALID;

   sModelQuentinAsClawSettingHandle = INVALID;
   sModelQuentinAsHoodSettingHandle = INVALID;
   sModelQuentinAsHairSettingHandle = INVALID;
   sModelQuentinAsRamirezSettingHandle = INVALID;
   sModelQuentinAsArakSettingHandle = INVALID;
   sModelQuentinAsKortanSettingHandle = INVALID;
   sModelQuentinAsMangusSettingHandle = INVALID;
   sModelQuentinAsMaskedWomanSettingHandle = INVALID;
   sModelQuentinAsClydeSettingHandle = INVALID;
   sModelQuentinAsDundeeBSettingHandle = INVALID;
   sModelQuentinAsDundeeDSettingHandle = INVALID;

   sPocModelHoodAsRamirezSettingHandle = INVALID;
   sPocModelHoodAsArakSettingHandle = INVALID;
   sPocModelHoodAsKortanSettingHandle = INVALID;
   sPocModelHoodAsMangusSettingHandle = INVALID;
   sPocModelHoodAsMaskedWomanSettingHandle = INVALID;
   sPocModelHoodAsClydeSettingHandle = INVALID;
   sPocModelHoodAsDundeeBSettingHandle = INVALID;
   sPocModelHoodAsDundeeDSettingHandle = INVALID;
   sPocUnusedAnimationSettingHandle = INVALID;

   sWipModelGunAsTurretSettingHandle = INVALID;

   sCharacterModelsInitialized = 0;
   sCostumeEnum = DONE;
   sCostumeDelayCounter = -1;
}


// IDEA: Someone could possibly make a multiplayer internet mod, similar to what Whitehouse did for AVP.
//       Not sure this game is worth that treatment.  If game was faster, and less clunky, it would be
//       more interesting.  Would be limited in how many models/animations could be loaded at any time,
//       but not a big deal if multiple players used just a few sets of models/animations.
//
// IDEA: Reencrypt modified compact disc files, so some of this could run on real hardware.  Script would
//       be used as reference as to what to change on compact disc files (edit or add animations for
//       current 3-4 animation characters.)
//
// IDEA: Modify compact disc files, and run on Game Drive with real hardware, if Game drive bypasses need to decrypt.
//
// IDEA: Create a cartridge ROM that runs on game drive with real hardware, but accesses contents of compact disc to reuse compact disc provided models, animations, sounds, sets/scenes, code, etc.
//       Cartridge EPROM for saving state.  Game Drive should support EPROM saves.
//
// IDEA: Replace World State to create a second mission.
//
// IDEA: Replace World State and code to create a tournament style game using the sets/scenes and models/animations/etc provided by compact disc.
//       Could warp to teleporter cave after every match ends, to let player select a different teleporter door for different enemy to fight.
//
// IDEA: See if can successfully modify character sheets, in world state entries, stored to memory track save slot.
//       Hex edited world state entry for quentin, which was stored to memory track, to different character sheet, and upon loading that save slot, game restarted (back to start screen menus) and then the save slot was deleted as well.
//       Hex edited world state entry for start-of-game Hood-Soldier, which was stored to memory track, to different character sheet, and upon loading that save slot, game restarted (back to start screen menus) and then the save slot was deleted as well.
//       Perhaps a hash was added after revision of source code.
//       If you could change character sheet for quentin world state entry, in memory track save slot, and successfully load that modified save slot, then could play game as Hair, Hood, Claw, Gunman1, Gunman2 (as long as you don't cause the eat/drink animations to be executed).
//
// IDEA: If support an end game or tournament mode, make opponent stay alive no matter how many times hit, until a kill move is applied to take head
//       The running jump and 360 degree chicken strike animation (but applied to sword model) would make a good candidate for kill move.
//       Probably unneccesary, probably not worth the effort.
//       Probably a good idea that the double up + c-button take the head though, regardless of animation.  Quentin's is a jumping overhead strike, which isn't too bad.
//
// IDEA: Fix the text in the Options Menu for the title screen sword model, in lieu of the rubber chicken model.
//       There is probably no place for text id and 3 multilingual text pointers immediately after title screen sword model, before the macleod sword model begins.
//       Fix would be to
//          1) copy the title screen sword model to somewhere free in memory, that will not get clobbered by dynamic loading/freeing of other game resources
//          2) immediatly after Step1 add text index of 0 and 3 pointers to SWORD~ text
//          3) update the title screen sword character sheet to point to the new copy of title screen sword model from Step1
//       Can live with it the way it is now (garbled text), as long as doesn't crash game.  Thusly marking this as an IDEA, not a TODO.
//
// IDEA: If Hair, Hood, Claw, Gunman1/2 have different animations then Quentin and each other, could possibly
//       update Ramirez, Mangus, Arak, Kortan, Masked Woman, Clyde, Dundee B, DundeeD to use a mix of animations
//       from Quentin, Hair, Hood, Claw, Gunman1/2.  Probably mush easier to do this, if can find some way to have
//       emulator or GameDrive to recognize a modified Track5 compact disc file.
//
// IDEA: Write and initiate a script that plays all movies, back-to-back
//
// IDEA: Locate/document where location of the zbuffer images (where retrieved/calculated from)?  Currently no need to know, so not looking into this.
//       Likely location is interleaved with the frame buffers
//
// IDEA: If ever need to visualize/edit Highlander provided models
//       python trimesh and opengl
//          import trimesh
//          import numpy as np
//          # Load or define your mesh
//          mesh = trimesh.Trimesh(vertices=vertices, faces=faces)
//          # Set each facet to a random color
//          for facet in mesh.facets:
//              mesh.visual.face_colors[facet] = trimesh.visual.random_color()
//          # Show the colored mesh
//          mesh.show()
//
// IDEA: Generic 3d model importer that accepts vertices and facets.  maybe fight for life, iron soldier, lara croft, others?
//       pycollada or similiar could possibly help convert from external model format to highlander model format
//
//       Suspect any alternative model that used texturing or gourad shading of fighter in source game will be pointless to convert to highlander.
//       - Tried a lara croft model, just importing a model/body-feature 1 at a time over the gas gun model and displaying in the Options Screen.
//         Without texturing or gourad, looked really bad.  E.g. texturing/gourad allow source game to make shin appear to have normal curved taping of body part, but without texturing/gourad just looked like 4 long rectangle polygons and some end of limb polys.
//         Possible the source model had polygon count greatly reduced from playstation 1 implementation.
//         But source model appeared to have had polygon counts, for body parts, similar to quentin polygon counts.
//       - Would expect alternative models would need to have the same number of models as highlander fighters, and model the same body parts.
//         For instance, had concerns lara croft ponytail might count as a seperate model, and throw off symmetry between games, causing the highlander animations to be incompatible with the lara croft models.
//         Saw model of head have ponytail go down to about shoulders and stop.
//         Never saw a model with rest/bottom of ponytail.  Perhaps that is all further ponytail goes down for lara croft models.
//       - Would have to assign colors to the facets/polygons of model for models which used texture mapping in their original game.
//       - Might have to determine which vertices should be treated as origins, to connect one limb to another limb.
//         The attempted model did not make reference to origins or how to connect limb to limb.
//         The attempted model likely had all limbs use the same origin in regards to the x/y/z values for their vertices.
//         The attempted model odel was likely generated years after tomb raider release, by someone wanting to just create a static model of lara croft.
//         Somehow managed to lose the link to the model which attempted with.
//         Attempted model did have 15 models for lara croft.
//         Based on vertice count, seemed to see legs and arms were handled similar/same as highlander.
//         Saw seperate model for head, torso, guessing pelvis as well.
//         Weirdly enough, not certain ever saw the feet or hands.
//             Likely did not recognize hands/feet, as source game probably relied on texturing, and display of model in highlander Options Screen normalizes size of model view to size of Options Screen.
//
// IDEA: If implement an end game or tournament mode, at end of quickening, increase y position to lift character into air.   stretch goal if possible: somehow add some lightning bolts or similar (probably only possible by utilizing BigPEmu interface to create bolts outside the emulated Jaguar).
//
// IDEA: Restore *fightable* kortan and arak to throne room right before vault.  This will take more then just world state updates.
//       The proof-of-concept hijaks an existing character sheet, so all enemies sharing that character sheet get alternative models applied.
//       Someone could add a new character sheet, apply alternative model to new character sheet, and then assign new character sheet to one world state fighter, and then only one opponent fighter would appear with alternative model.
//       Doing this for one of the opponent fighters in throne room to make appear as Kortan, and the other one in throne room could be made to look like Arak, assuming fits into the different memory regions.
//
// IDEA: Free any no-longer used blocks, after overwrite models referenced by character sheet.  Freeing may require nothing more then writing a zero to the numBlocks field of RamMetadata stored after every 448 bytes, in the animation/models/logics section of memory.
//       Not sure anyone is going to use this script, or use the script enough to make any difference within a gaming session, so downgrading this to just an idea.
//
// IDEA: Find better alternative to get away from being wailed on, then the current put-all-enemies-to-sleep-by-lowering-their-life-to-zero approach.
//       This approach works, but causes them to drop the items they have on them.
//       Tried setting wstFlags bits in World State entry, and didn't care for the results.
//       Tried setting cshBehavior value in character sheet entry, and didn't care for the results.
//       Had one trial where kept getting attacked until player pushed onto different screen and then enemy disappeared.
//       Had one trial where enemy just runs away, and then you never see another enemy again which results in not being able to even transition from outside dundee village tank to canyon (as are required to defeat at least one Hood Soldier to make that transition).
//       FSAShield is possibly viable, but wasn't all that thrilled with results.
//          Fighter opponent kept following player, and player wasn't able to run away, turn around, and still have much distance between player and fighter opponent, before time runs out (regardless of length of time).
//             uint32_t taddr = HL_CHARACTER_INSTANCE_TABLE_START_ADDR + 16
//             uint8_t stance = bigpemu_jag_read8(taddr);
//             stance = stance | (1<<6); // FSAShield bit
//             bigpemu_jag_write8(taddr, stance);
//
// IDEA: Improve the put-enemies-to-sleep-by-lowering-their-life-to-zero approach, to only only impact those enemies within the same set and/or scene as player.
//
// IDEA: Alternative approach to displaying non-weapons as a weapon.
//       Possibly no advantage and likely disadvantages.
//       Setting WSTWeapon bit of world state flags field allows item to be placed in players hand, and defaults to unarmed animations.
//          uint32_t taddr = 0x00032840 + 31;//HL_FLOWER_SHEET_P_ADDR + 31;
//          uint8_t flags = bigpemu_jag_read8(taddr);
//          flags |= (1<<4); // WSTWeapon
//          bigpemu_jag_write8(taddr, flags);
//
// IDEA: Consider swapping some of the byte-wise read/write loops with sysmemread and sysmemwrite
//       calls.
//       Believe those calls swap bytes.
//       - If just reading and then writing to another location, switching to those calls
//         might make sense.
//       - If reading, manipulating, and then writing to another location, those calls may
//         provide zero advantage.
//       - If reading from compact disc track file, and then writing to Jaguar RAM, those
//         calls might not make sense, as compact disc has bytes swapped at word level,
//         whereas those calls swap at long word level.
//       - As it stands now, at least everything is using one consistent approach to
//         read/write to Jaguar memory.
//
// WIP: Orient turret model to something reasonable, when turret model data is copied over gas gun model data.  It will look wonky when player rotating, but could orient so it points straight forward when player not rotating.  May have to write some code to rotate/recalculate vertices.  Really don't care enough about this to bother now, or possibly ever.  Might as well at least include the start of this mini-effort though.  Suspect any further effort on turret is really a case of 'juice not worth the squeeze'.
//
// WIP: If exit a set with someone frozen, everyone in set and prior set (that were prior frozen), die.  Future sets not affected.   
//
// WIP: Probably need to apply different height offsets for the POC Model Hood As setting.
//
// TODO: Consider swapping the read of entire world state data into array of structures, to
//       just reading in the relevant fields from world state data.  Current read approach was
//       done early in development of script, and that approach seems to be inconsistent with
//       approach subsequently taken elsewhere in the script.










// Revision History:
//    01. Health cheat
//    02. Cheat to auto enable/hold all weapons
//    03. Unlimited gun ammunition
//    04. Hackiest possible cheat to programatically enter the joypad cheat to warp to teleportor room
//    05. To be cute, made all enemy fighters dead, via world state
//
//    Things escalated from there...
//    06. Auto enable/hold all items, and saw title screen sword (thinking ramirez sword) and saw references to kortan, ramirez, arak, dundee, clyde in a blotch of text when searched for ramirez text in RAM
//    07. Found enemy fighters worksheets
//    08. Assigned enemy fighters' character sheets to quentin world state item
//    09. Created settings for play game using claw, hood, hair, gunman1, gunman2 character sheets
//    10. Created setting to freeze players for set period of time, to escape situation where enemy wails on you from behind and you get no chance to turn around
//    11. Assigned flower/plank as weapons to quentin
//    12. Found ramirez and others worksheets
//    13. Set quentin world state to use clyde character sheet, which allowed standing around and turning only.  until equip weapon, and then clyde turned into possessed rag doll
//    14. Set quentin world state to use ramirez character sheet and could stand, turn, and walk around only.
//    15. Assigned nearby enemy to have ramirez character sheet and enemy just stands around
//
//    At this point, obtain a copy/revision of highlander source code.  Now using source code as a reference.
//
//    16. Assigned nearby enemy to have ramirez character sheet (to force load of ramirez model/animation data) and then copied enemy chracter sheet model pointers to quentin character sheet model ptrs, and able to fight as ramirez.  of course as soon as enemy gone, game crashes
//
//    Following is part that took most time, as need understanding of model layout, and other structures that point to models character sheets, world state items, etc
//    17. Then deep copy model data for costumes that fit over quentins block perfect, and can fight as stander/walker, but no weapons will display (but weapons function otherwise)
//    18. Then deep copy model data for other costumes that don't fit perfect and make adjustments to fit work, etc
//
//    19. Slowly start to realize how to enable display of weapon for standers/walkers
//    20. Removed display of weapon from hand model of Kortan, Arak, Ramirez.
//    21. Copied weapon from hand model of Kortan, Arak, Rameriz over the Macleod sword, when Quenton's model data is copied over by Kortan, Arak, Ramirez model data.
//    22. Updated right hand model to allow display of weapons that are selected.  Now costumed quinten no longer shoots with fingers or uses hand as a sword.
//    23. Updated flower and title screen sword models to be oriented/displayed correctly when used as weapon
//    24. Got grasp of scripting system.
//    25. Created memory map.
//    26. Created cheat to have the teleporter doors always active in teleporter room.  Getting to the vase to place flower is really obnoxious with the developer uglified portraits continually coming up, no matter how hard you try to avoid.
//    27. Documented memory structures that utilized.
//    28. Verified there are no unused models/animations/etc on Track5(1-based).
//    29. Verified there may be 1 unused WAVE on Track6(1-based).
//    30. Verified there are no unused videos on Track7(1-based).
//    31. Reviewed/tested/analyzed/documented the animations provided on apparently unused Track8(1-based) of compact disc.
//    32. Created compact disc map.
//    33. Refactored to read model data from compact disc track file rips, instead of from model data previously exported from RAM.
//    34. Added proof-of-concept of applying different models to opponent fighters.
//        So can have Clyde, Kortan fight you.
//        Proof of concept hijaks an existing character sheet, so all enemies sharing that character sheet get alternative models applied.
//        Someone could add a new character sheet, apply alternative model to new character sheet, and then assign new character sheet to one world state fighter, and then only one opponent fighter would appear with alternative model.  Doing this for one of the opponent fighters in throne room to make appear as Kortan, and the other one in throne room could be made to look like Arak, assuming fits into the different memory regions.
//    35. Updated height of animations when non-Quentin model is used to model Quentin.










// Below is extensive documentation of memory structures/layout of game.
// Might look better in a spreadsheet, but if put in spreadsheet, noone will
// ever open/view the spreadsheet.  Placing directly in this script, though
// ugly and less maintainable, will prevent the documentation from never being
// seen again.









//Memory Map - Quick and dirty, first pass attempt of mapping memory and providing descriptions.
// 0x00000000-0x00000002 0x0C8B08
// 0x00000003-000000000F zeros
// 0x00000010-0000000017 0x00DFFEFC 0x00DFFEFA
// 0x00000018-00000000B7 zeros
// 0x000000B8-0x000000BB 0x00DFFEFA
// 0x000000BC-0x000000FF zeros
// 0x00000100-0x00000103 pointer to INTSERV.S interrupt handler. - 0x00005E72
// 0x00000104-0x000023FF 0xFFs
// 0x00002400-0x00002403 "_NVM" checked by startup (actually 10 bytes present: 0x5F 0x4E 0x56 0x4D 0x4E 0x71 0x4E 0x71 0x4E 0x75)
// 0x00002404-0x00002BFF 0xFFs
// 0x00002C00-0x00002C4F ?????? 80 bytes???????
// 0x00002C50-0x00002FFF 0's
// 0x00003000-0x00003071 table of 17 68k-branch-to-subroutine calls.
// 0x00003072-0x00003077 dunno.  the code direct below overwrites 0x00003074-0x00003077.  74 thru 77 may be value matching one of 17 pointers above.
// 0x00003078-0x000030E9 68k code for line direct below ?only.  seems to get hit when going from cinepak to start screen and cinepak to actual gameplay.
// 0x000030EA-0x000030F5 zeros that may get copied into gpu ram.
// 0x000030F6-0x000031D9 CDINIT.GAS
// 0x000031DA-0x0000323B 68k code that copies 0x00003248 gpu code to gpu ram.  seems to get hit when about to play cinepak video.
// 0x0000323C-0x00003247 zeros that may get copied into gpu ram.
// 0x00003248-0x0000331F CDINITF.GAS (cinepak?)
// 0x00003320-0x00003323 ??????
// 0x00003324-0x00003381 copies 0x00003324 thru somewhere before 0x000034F9, if read code correctly, which seems wrong.  then again not sure this particular bit of code ever gets called anyways.
// 0x00003382-0x00003395 zeros
// 0x00003396-0x000034F9 CDINITM.GAS - didn't see this get loaded up with breakpoints attached from hard start through in actual gameplay.
// 0x000034FA-0x00003505 ???
// 0x00003506-0x000039E9 68k methods pointed to by the table at 0x00003000-0x00003071.  most/all subroutines/exceptions seem to reference 0x00DFFF00, which presumably is hardware cd related.
// 0x000035EA-0x000035EB unsure.  2 bytes.
// 0x000034EC-0x000039FF 0xFFs
// 0x00003A00-0x00003B23 more 68k code that has possibly been used and overwritten in places.
// 0x00003B24-0x00003DFF 0xFFs
// 0x00003E00-0x00003E01 0x0001
// 0x00003E02-0x00003FBF 0xFFs
// 0x00003FC0-0x00003FC5 0x80 0x02 0x30 0x17 0x00 0x05
// 0x00003FC6-0x00003FFF 0xFFs
// 0x00004000-0x0000401F looks like first 4 lines of HIGH1.S 68000 code.  suspect the remainder of HIGH1.S 68000 code was initially present, executed once, and then overwritten.
// 0x00004020-0x0000409F listbuf - object processor list related.  HIGH1.S reserved 128 bytes.  Last 54 bytes don't match listcpy, but stop object proceeds the last 54 bytes.  listbuf overwrote startup 68k code.
// 0x000040A0-0x0000411F listcpy - object processor list related.  HIGH1.S reserved 128 bytes.  Last 54 bytes don't match listbuf, but stop object proceeds the last 54 bytes.  listcpy overwrote startup 68k code.
//    0x000040A0 Object Processor List start
//    0x000040E0-0x000040E7 Object Processor Stop Object
// 0x00004120-0x00004127 pad_now; bmyvars(begin main vars) - joypad related.
// 0x00004128-0x0000412F pad_shot - joypad related.
// 0x00004130-0x00004133 clrptr
// 0x00004134-0x00004137 disp_toggle - causes V16G 45 in one line, then line of scene/set ids, then line of x position data, then line of z position data.  first line likely revision id of the game/source-code along with frame rate.
// 0x00004138-0x00004237 single_obj - supposed to be 64 longs.
//      0x00004138-0x0000413B next in draw list (according to read breakpoint and FORMMAT.GAS).
//      0x0000413C-0x0000413F status
//      0x00004140-0x00004143 IDP (according to read breakpoint and FORMMAT.GAS)
//      0x00004144-0x00004147 pointer to model of object in hand (e.g. sword).
//      0x00004148-0x0000415F IDmatrix "next 6 longs = 4x3 IDmatrix" according to breakpoint and 3DENGINE.GAS.  thought identity matrices were square, but whatever.
//      0x00004160-0x00004161 xaxis according to breakpoint and FORMMAT.GAS.
//      0x00004162-0x00004163 zaxis according to breakpoint and FORMMAT.GAS.
//      0x00004164-0x00004165 yaxis according to breakpoint and FORMMAT.GAS.
//      0x00004166-0x00004167 velocity - according to breakpoint and FORMMAT.GAS.
//      0x00004168-0x00004237 all zeros when walking or cycling through options screen item selection.  maybe never used.
// 0x00004238-0x00004249 unit_matrix some matrix definitely starts here as 3dengine begins reading this and lighting matrix info in item select options dialog, but not when just walking about.  18 word reads from both addresses.
// 0x0000424A-0x0000424F unit_pos? see above
// 0x00004250-0x00004261 view_matrix 18 words read starting at 0x00004250 and at lighting matrix, when walking around.  maybe they are just lazy copying past the non-lighting matrix while simulataneously copying the entire lighting matrix.  dunno.  there are some comments bout 4x3 matrix in 3d engine, but main.s declarations are for 3x3 matrices, but with arguably 1 by 3 position matrix after it, so maybe thats what 3d engine means by 4x3.  even then that is only 12 words, not 18 words.
// 0x00004262-0x00004263 view_pos definitely view_pos, given view_ht address definitively verified).
// 0x00004264-0x00004265 view_ht - definitely view_ht.  breakpoint on this when changing screens verified written to.
// 0x00004266-0x00004267 view_z - definitely view_z, given view_ht address definitely verified.
// 0x00004268-0x00004273 view_unused (don't understand this.  3dengine.gas reads 18 words, with this being the last 6 words.  and if unit_matrix/pos read is reading 6 words past useful, don't understand why they bother with last 6 words.  must be missing something.  or the ds statements in main.s are way outdated.) (noesis memory window shows zeros for this too, like it really is unused.)
// 0x00004274-0x00004275 scaling
// 0x00004276-0x00004277 zeros for phrase alignment.
// 0x00004278-0x000042B7 play_area
// 0x000042B8-0x000042B9 lonoff - light_data
// 0x000042BA-0x000042BB lambient
// 0x000042BC-0x000042BD l1rgb
// 0x000042BE-0x000042BF l1x
// 0x000042C0-0x000042C1 l1y
// 0x000042C2-0x000042C3 l1z
// 0x000042C4-0x000042C5 l2rgb
// 0x000042C6-0x000042C7 l2x
// 0x000042C8-0x000042C9 l2y
// 0x000042CA-0x000042CB l2z
// 0x000042CC-0x000042CD l3rgb
// 0x000042CE-0x000042CF l3x
// 0x000042D0-0x000042D1 l3y
// 0x000042D2-0x000042D3 l3z
// 0x000042D4-0x000042D5 l4rgb
// 0x000042D6-0x000042D7 l4x
// 0x000042D8-0x000042D9 l4y
// 0x000042DA-0x000042DB l4z
// 0x000042DC-0x000042DF tempbuff - revision of source makes it appear this is not used.
// 0x000042E0-0x000042E3 ctrlview - revision of source makes it appear this is not used.
// 0x000042E4-0x000042E7 ctrlmodel - revision of source makes it appear this is not used.
// 0x000042E8-0x000042EB ctrlmod2 - revision of source makes it appear this is not used.
// 0x000042EC-0x000042EF thismodel - revision of source makes it appear this is not used.
// 0x000042F0-0x000042F3 endptr
// 0x000042F4-0x00004323 long words for ctrltest, ctrltes2, stance, testp1, testp2, testp3, testp4, tempvar.  no code seems to reference these by name.
//      0x00004308-0x0000430B suspect stance.  code does not directly reference.  temporarily goes from 0 to 2 when select to use item (whether usable or not).
//      0x0000430C-0x0000430F read on every frame, by few places, but doesn't appear in revision of code have.
// 0x00004324-0x00004327 = digit1 - MAIN.S says this is first digit of frame rate or frame counter or place holder value.  On my machine, BigPEmu always displays '4'.
// 0x00004328-0x0000432B = digit2 - MAIN.S says this is first digit of frame rate or frame counter or placeholder value.  On my machine, BigPEmu always displays '5'.
// 0x0000432C-0x000043CB digits - main declares 40 longs.  only 24 longs are populated (at least when disp_toggle is set to 0x00000001).
//      0x0000432C-0x0000438B - populated with pointers to data draw areas for alphanumerics, corresponding to 8 digit line for set/scene ids, 8 digit line for x pos, 8 digit line for z pos.
//      0x0000438C-0x000043CB - zeros when walking around and cycling through Options item selection dialog.  maybe never used.
//      nothing present here or before digit1 to account for the suspected version info constant alphanumerics, which are probably one bitmap object.
//      code indicates it is version X14H, while game is displaying on screen V16G.
//      suspect x meant "version:" and v means "verson:", and x and v are essentially same thing.  if not, code may be newer version then the production release.
// 0x000043CC-0x000043CF csrc - should be this and only referenced in CLEARJAG.S, but never got breakpoint to hit while walking or browsing items in Options dialog.
// 0x000043D0-0x000043D3 cdst - should be this and only referenced in CLEARJAG.S, but never got breakpoint to hit while walking or browsing items in Options dialog.
// 0x000043D4-0x000043D5 ht - should be this and only referenced in CLEARJAG.S AND commented out.  never got breakpoint to hit while walking or browsing items in Options dialog.
// 0x000043D6-0x000043D7 wd - should be this and only referenced in CLEARJAG.S AND commented out.  never got breakpoint to hit while walking or browsing items in Options dialog.
// 0x000043D8-0x000043DB nwd - should be this and only referenced in CLEARJAG.S, but never got breakpoint to hit while walking or browsing items in Options dialog.
// 0x000043DC-0x000043DF swd - should be this and only referenced in CLEARJAG.S, but never got breakpoint to hit while walking or browsing items in Options dialog.
// 0x000043E0-0x000043E3 forctr - never got breakpoint to hit while walking or browsing items in Options dialog.
// 0x000043E4-0x000043E7 formkr - never got breakpoint to hit while walking or browsing items in Options dialog.
// 0x000043E8-0x000043EB waitctr - got breakpoint to hit while walking or browsing items in Options dialog.
// 0x000043EC-0x000043EF currcmd - never got breakpoint to hit while walking or browsing items in Options dialog.
// 0x000043F0-0x000043F3 playerws - looks like index of player world state, according to AICTRL.GAS ControlCode.
// 0x000043F4-0x000043F7 tinhand - address of world state of Item Temporarily put into Hand and displayed as such (e.g. eating food/drink).
// 0x000043F8-0x000043FB modinhand - pointer to model of weapon in hand.
// 0x000043FC-0x000043FF inhand - pointer to world state of weapon in hand, null for bare hands.
// 0x00004400-0x00004403 pickup - pointer to world state of picked up item. populated when deciding whether to inspect, use, accept, etc.
// 0x00004404-0x00004407 instpick - pointer to character instance of picked up item.  populated when deciding whether to inspect, use, accept, etc.
// 0x00004408-0x00004487 gamestate - 1024 bits of game state.  saved/loaded to/from save slots.  Referenced by NVRAM.S, COLLECT.GAS, and EVENT.GAS.  Using a wine bottle marked a bit.  Activating teleporter set a bit.  Bit# is divided by 32 to find the correct longword, then the remainder is used to access bit within that long word (so you see bit31, bit30,...bit1, bit0, bit 63, bit62, ... bit33, bit32, bit 95, ..., etc, etc).
// 0x00004488-0x0000448B TableIndex - not referenced by source code, other then to reserve memory.
// 0x0000448C-0x0000448F SlideStart - not referenced by source code, other then to reserve memory.
// 0x00004490-0x00004493 using - should be here.  possibly this is something else.  (at one point, thought this was related to joypad cheats, probably based on outdated source code.)
// 0x00004494-0x00004497 timecode - related to bufstart.
// 0x00004498-0x0000449B bufstart - this and other buffer/cd-read below about has to be as described.
// 0x0000449C-0x0000449F bufend
// 0x000044A0-0x000044A3 lastload - gets set to bufend value.
// 0x000044A4-0x000044A4 cdwait
// 0x000044A5-0x000044A5 loadstop
// 0x000044A6-0x000044A6 setfacing - source code doesn't seem to reference this, other then to reserve memory for it.
// 0x000044A7-0x000044A7 flushq
// 0x000044A8-0x000044A8 redbookmode
// 0x000044A9-0x000044A9 psprite - set to 0xFF when hit pause button, but stays 0xFF when unpause.  dunno.
// 0x000044AA-0x000034AA allow_cheats - looks like this is outdated from source code and now setting this to non-zero value only allows enabling the the disp_toggle word to get set to value of 0x00000001, causing debug info to be displayed in upper left hand of game screen, by pressing the 5 button after enabling this.
// 0x000044AB-0x000044AB slowtowalk - according to outdated source.  possibly should have different name for production release.  setting this to 1, will warp player to teleporter cave.
// 0x000044AC-0x000044AD slowtowalk (or lifeflags) (may actually be slowtowalk, per ANIM.GAS and AICTRL.GAS) - Unsure.  This still relates to life bar cheat.  Read breakpoint hit on every frame while player stands still makes youn wonder about its purpose.  Looked like gets set to 0 on every frame while standing still as well.
// 0x000044AE-0x000044AE unsure - probably zero-key and/or music mute related.  when music is muted from zero-key, has value of 0xFF.  otherwise has value of 0x00.  appears to be accessed by post-revision version of JOY.S.
// 0x000044AF-0x000044AF looks like used during CD access, when changing screens.  post revision CDLOADER.S reads this.
// 0x000044B0-0x000044B0 unsure - cleared every frame by MAIN.S.  Snuck in to MAIN.S::Gameloop between setvars and input subroutine calls, post revision.
// 0x000044B1-0x000044B1 didn't notice this getting read or written.  didn't try very hard to determine purpose.
// 0x000044B2-0x000044B3 lifeflags (likely, according to AICTRL.GAS.  0 value should make immortal (always 0x80 life value).  read every frame.
// 0x000044B4-0x000044B7 finloop
// 0x000044B8-0x000044BC redblock
// 0x000044BC-0x000044BF cineblock
// 0x000044C0-0x000044C3 currcd
// 0x000044C4-0x000044C7 entercd
// 0x000044C8-0x000044CB datalong (according to revision documentation, may hold PICT or DATA) for datatype of load.  failed to get read or write breakpoint hit by walking around or selecting different item.
// 0x000044CC-0x000044CF CurrScene - same pointer value as value in Scenea,b,c which is current.  write breakpoint hit when changing screens.
// 0x000044D0-0x000044D3 CurrZ - same pointer value as value in ZBuffa,b,c.
// 0x000044D4-0x000044D7 Boffa - block offset a
// 0x000044D8-0x000044DB Boffb - block offset b
// 0x000044DC-0x000044DF Boffc presumably - never got breakpoint to hit.  possibly not used in production revision of game, or not used during normal gameplay.
// 0x000044E0-0x000044E3 currws - appears to be world state of item in hand (bread, sword, etc, including when first pickup from enemy fighter and review item for use/rejection/inspection).
// 0x000044E4-0x000044E7 currmod - appears to be model of item described directly above.
// 0x000044E8-0x000044EB gameover
// 0x000044EC-0x000044EF fincine (likely) - read and write breakpoints never hit, while walking or selecting item.  fincine is declared, but not read or written to in the code.
// 0x000044F0-0x000044F3 cde
// 0x000044F4-0x000044F7 possibly padding - no read or write breakpoint hit while walking or selecting item.
// 0x000044F8-0x000044FB possibly padding, but seems odd to padd into middle of phrase - no read or write breakpoint hit while walking or selecting item.
// 0x000044FC-0x000044FF Buffa (; this and next to are pointers to the beginning of the three scenes for buff 1,2,3.
// 0x00004500-0x00004503 Buffb
// 0x00004504-0x00004507 Buffc presumably - c variant never hit read or write breakpoint while walking or selecting item.  possibly not used by production version of game.
// 0x00004508-0x0000450B ZBuffa
// 0x0000450C-0x0000450F ZBuffb
// 0x00004510-0x00004513 ZBuffc presumably - c variant never hit read or write breakpoint while walking or selecting item.  possibly not used by production version of game.
// 0x00004514-0x00004517 Scenea
// 0x00004518-0x0000451B Sceneb
// 0x0000451C-0x0000451F Scenec presumably - c variant never hit read or write breakpoint while walking or selecting item.  possibly not used by production version of game.
// 0x00004520-0x00004523 lowy - according to documentation of old revision.  that revision writes 10,000decimal every frame, but never reads lowy.
// 0x00004524-0x00004527 animsheet - null for barehands, otherwise points to character sheet for sword, gas gun, chicken as equipped.
// 0x00004528-0x0000452B demomode - something along lines of  0x00000001 to play before game (and possibly end game) cinepak videos. 0x00000000 otherwise.
// 0x0000452C-0x0000452F unsure - looks like top of post-revision-EVENT.GAS writes to it.  cleared every frame. didn't see read when walking around or selecting item.
// 0x00004530-0x00004533 colptr - colour pointer for color cycling of effects/music bars when paused.
// 0x00004534-0x00004537 volmode - while paused, 0x00000000 when first pause, 0x00000003 when display GAME LOAD/SAVE screen, 0x00000002 when display EFFECTS VOLUME screen, 0x00000001 when display MUSIC VOLUME screen.
// 0x00004538-0x0000453B cdvol - MUSIC VOLUME ranging from value of 0x00000000 to 0x00007FFF.
// 0x0000453C-0x0000453F sfxvol - EFFECTS VOLUME ranging from value of 0x00000000 to 0x00007FFF.
// 0x00004540-0x00004543 unsure - looks to keep a constant value when walking, selecting item, changing volume of effects/music.  noted value of 0x000048FF during one session and noted value of 0x0001FFFF during another session.
// 0x00004544-0x00004553 text1 - unsure when populated. txtPtr, x, y, display status.
// 0x00004554-0x00004563 text2 - unsure when populated.
// 0x00004564-0x00004573 text3 - description of item in Options/Inventory screen; also game/load save screen.  always see value of 0x00000000 for display status for this and 2 direct below.
// 0x00004574-0x00004583 text4 - all the static text in Options/Inventory screen; also game/load save screen; also music/fx volume screens.
// 0x00004584-0x00004593 text5 - name of item in Options/Inventory screen.
// 0x00004594-0x0000459B zero padding or tcadd/txx.
// 0x0000459C-0x0000459F tcadd - here or within couple long words to left.  don't see used in source code.
// 0x000045A0-0x000045A3 txx - here or within couple long words to left.  don't see used in source code.
// 0x000045A4-0x000045A7 txy - here or within couple longs words to left.  don't see used in source code.
// 0x000045A8-0x000045AB creditstate - here or within couple long words to left.  read in source code, but don't see written.  displaying credits screen made no difference.
// 0x000045AC-0x000045AF stopload - values of 0x00000000 and 0x00000001.
// 0x000045B0-0x000045B3 loadone
// 0x000045B4-0x000045B7 loadtwo
// 0x000045B8-0x000045BB cacheoff
// 0x000045BC-0x000045BF eventmode
// 0x000045C0-0x000045C3 scriptblock 
// 0x000045C4-0x000045DB
//    0x000045C4 something to do with CDLOADER.S reads and writes to this.
//               this may be changing every screen, and changing more then once (incrementing?) first second or two of screen had to wait on.
//               suspect this is a pointer to current progress of prospective load of scene, as increments several times in 16k-19k increments, and occurs within like second or so of switching to different screen.
//               weirdly when use cheat to go to teleporter room, this increments by an amount of only 0x00000020, a ridiculous number of times.
//                       orders of magnitude less of an increment then natural change of screen.
//               further analysis makes this look like (at least sometimes) a progress cursor for population of buff1 and buff2.
//    0x000045C8 possibly scriptblock, debuginfo, frametime
//    0x000045CC possibly scriptblock, debuginfo, frametime
//    0x000045D0 possibly scriptblock, debuginfo, frametime
//    0x000045D4 possibly scriptblock, debuginfo, frametime
//    0x000045D8-0x000045DB frametime - no hit on read breakpoint, hit every frame on write breakpoint seen values of (0x000000010, 0x00000012), (0x0000000C, 0x0000000E).
// 0x000045DC-0x000045DD Snuma - this and 1 below are occupied and look like scene/set numbers, 2nd below (3rd total) is 0s.
// 0x000045DE-0x000045DF Snumb
// 0x000045E0-0x000034E1 Snumc supposedly - don't think the 3rd is used in production release.
// 0x000045E2-0x000045E3 newscene
// 0x000045E4-0x000045E5 OS old scene number
// 0x000045E6-0x000045E7 CUS new scene number (presumably CU=current)
// 0x000045E8-0x000045E9 CurrSet
// 0x000045EA-0x000045EB buffused
// 0x000045EC-0x000045ED ???????? are there now two buffused? buffused 2?
// 0x000045EE-0x000045EF this thing increments by 1 for at least 1000 count possibly 65k count when went outside dundee village to back inside dundee village.  went 0,1,2,0,1,2, repeatedly while 0x000045C4 was incremnting by 0x00000020 when changed screen to screen about to leave dundee village.
// 0x000045F0-0x00004DEF statevectors - 2k of state that gets saved/loaded to/from save slots.  not a lot of hits in source code.  suspect the scripts do bulk of modifying.
// 0x00004DF0-0x00004DF1 scriptscene
// 0x00004DF2-0x00004DF3 scriptevent
// 0x00004DF4-0x00004DF7 ? 0x00004DFC indexes from here.
// 0x00004DF8-0x00004DFB ? 0x00004DFC indexes over this
// 0x00004DFC-0x00004DFF ? breakpoint triggered when first move from outside dundee village to canyon.  looks like in script.gas.  seen values of 0x00000000 as default, temporarily 0x00000001 and 0x00000003 when hit breakpoint.  suspect 0x00000002 is possible.  is indexed from 0x00004DF0.
// 0x00004E00-0x00004E13 ? dummy/padding?
// 0x00004E14-0x00004E15 ? added to MAIN.S scene event handler, but added after revision of source code.  seems conditionally allows execution to flow to cineblock section of code directly below it. (slightly possible nverror1.)
// 0x00004E16-0x00004E17 ? read breakpoint hit when transitioning from outside dundee village to canyon, before cinepak video displays.  68k code.  hit repeatedly while cinepak video plays.  write breakpoint hit when cinepak video ends.  gpu code.  SCRIPT.GAS tablewrite.
// 0x00004E18-0x00004E1B ? INTSERV.S adds 1 to this.  acts like more of a frame count then the supposed frameno and framecount variables below.
// 0x00004E1C-0x00004E2F ? unsure. possibly just padding.  read/write breakpoints not hit walking around or selecting item.
// 0x00004E30-0x00004E33 backbuff
// 0x00004E34-0x00004E37 drawbuff
// 0x00004E38-0x00004E3B showbuff
// 0x00004E3C-0x00004E3F listp1
// 0x00004E40-0x00004E43 listp2
// 0x00004E44-0x00004E47 listp3
// 0x00004E48-0x00004E4B listp4
// 0x00004E4C-0x00004E4F framesync - identified this one prior to identifing one 16 bytes to right.
// 0x00004E50-0x00004E53 frameno    - frameno, framerate, framecount not in order specified by MAIN.S.  Breakpoints and INTSERV.S led to this order.
// 0x00004E54-0x00004E57 framerate
// 0x00004E58-0x00004E5B framecount
// 0x00004E5C-0x00004E5F framesync - according to FORMMAT.GAS disassembly.
// 0x00004E60-0x00004E63 unsure - occasionally updated to match frameno.   0x000057E6 instructions suggest these might be new or old values, as copies 0x00004E60-0x00004E6B to 0x00004E50-0x00004E5B.
// 0x00004E64-0x00004E67 unsure - seems to match framerate.
// 0x00004E68-0x00004E6B unsure - occasionally updated to match framecount, and next breakpoint framecount increments by one.
// 0x00004E6C-0x00004E6F unsure - possibly padding.  never hit breakpoint walking around or selecting item.
//     barlife should have been somewhere in last 32 bytes. expected at 0x00004E58 0x00004E5C based on MAIN.S, but old revision of source code.
//     emyvars=0x00004E70 (end main vars)
// 0x00004E70-0x00004E71 height - according to start selection screen disassembly.
// 0x00004E72-0x00004E73 a_vdb - according to start selection screen disassembly.
// 0x00004E74-0x00004E75 a_vde
// 0x00004E76-0x00004E77 width - according to start selection screen disassembly.
// 0x00004E78-0x00004E79 a_hdb
// 0x00004E7A-0x00004E7B a_hde
// 0x00004E7C-0x00004E7F timing
// 0x00004E80-0x00004E83 syposlo
// 0x00004E84-0x00004E87 syposhi
// 0x00004E88-0x00004E8B sxpos
// 0x00004E8C-0x00004E8F unsure non null value(s).  never see write breakpoint hit, even on startup.  don't see read breakpoint hit.
// 0x00004E90-0x00004E90 joypad configuration mapping related.  first couple seconds of game startup (initial menu with lore design limited red square) these are following 16 bytes are initialized with 0x04.
// 0x00004E91-0x00004E91 joypad configuration mapping related
// 0x00004E92-0x00004E92 joypad configuration mapping related
// 0x00004E93-0x00004E93 joypad configuration mapping related
// 0x00004E94-0x00004E94 joypad configuration mapping related
// 0x00004E95-0x00004E95 joypad configuration mapping related
// 0x00004E96-0x00004E96 joypad configuration mapping related
// 0x00004E97-0x00004E97 joypad configuration mapping related.  write breakpoint hit when Pause, Option.
// 0x00004E98-0x00004E98 unsure - cleared by 68k every frame.
// 0x00004E99-0x00004EA7 unsure - initialized to 0x04.  for a time thought saw every other byte initialized to 0x0F.
// 0x00004EA8-0x00004EA9 nvram_check
// 0x00004EEA-0x00004EF1 4 of these 6 bytes are supposed to be nvram_size.
// 0x00004EF2-0x00004F01 nvram_applic
// 0x00004F02-0x00004F0B nvram_filenm
// 0x00004F0C-0x00004F0C nvram_ok
// 0x00004F0D-0x00004F0D nvram_fnum
// 0x00004F0E-0x00004F0E nvram_opnum
// 0x00004F0F-0x00004F0F nvram_mode
// 0x00004F10-0x00004F10 nvram_exit
// 0x00004F11-0x00004F17 ?nvram related?
// 0x00004F18-0x00004F1B ScriptSet
// 0x00004F1C-0x00004F1F zero padding
// 0x00004F20-0x00004F25 VideoCount (CINESTUB.S refers to 0x00004F22)
// 0x00004F26-0x00004F27 likely padding to next long word
// 0x00004F28-0x00004F2C VideoRate
// 0x00004F2C-0x00004F2F DelayRate
// 0x00004F30-0x00003F33 CDRate
// 0x00004F34-0x00004F37 MovieOffset
// 0x00004F38-0x00004F39 MovieType
// 0x00004F3A-0x00004F3B PlayState
// 0x00004F3C-0x00004F3D CDState
// 0x00004F3E-0x00004F3F ScreenState
// 0x00004F40-0x00004F43 ScreenOffset
// 0x00004F44-0x00004F47 ScreenDraw
// 0x00004F48-0x00004F4B ScreenShow
// 0x00004F4C-0x00004F4F FilmIndex
// 0x00004F50-0x00004F53 FilmCount
// 0x00004F54-0x00004F57 BufferIndex
// 0x00004F58-0x00004F5B BufferCount
// 0x00004F5C-0x00004F5F GPUOffset
// 0x00004F60-0x00004F62 semaphore
// 0x00004F63-0x00004FBB unsure - looks like padding that oddly ends 4 bytes earlier then might think.  no breakpoint hit restarting game, walking, selecting item, traveling to canyon.
// 0x00004FBC-0x00004FEB unsure - 48 bytes of nonzero data, but breakpoints never hit.
// 0x00004FEC-0x00004FFF unsure - zeros, but didn't really bother attempting breakpoints.
// 0x00005000-0x0000508F HIGH1.S code
// 0x00005090-0x00005E2F MAIN.S code
//      0x000058CA-0x00005A23 seems to hard code the version info to V16G101.
// 0x00005E30-0x00005E69 INTSERV.S::IntInit code.
// 0x00005E70-0x00005ED5 INTSERV.S::Frame interrupt handler code.
// 0x00005ED6-0x00005EF5 INTSERV.S more code - believe this got split from INTSERVE::Frame.
// 0x00005EF6-0x00005FDD VIDINIT.S code
// 0x00005FDE-0x00006255 JLISTER.S code
// 0x00006256-0x00006607 CLEARJAG.S code
// 0x00006608-0x000067E3 JOY.S::chngscn code
// 0x000057E4-0x00007029 JOY.S remainder of JOY.S code
// 0x0000702A-0x00007BA3 NVRAM.S code
// 0x00007BA4-0x00007C7F GPU.S code
// 0x00007C80-0x00007F35 LIBRARY.S code
// 0x00007437-0x000081F7 CDLOADER.S code
// 0x000081F8-0x0000841D likely extended CDLOADER.S code.  definitely 68K code.
// 0x0000841E-0x00008E03 CINEPAK.S code
// 0x00008E04-0x00008F81 OBJECT.S code
// 0x00008F82-0x0000923B belive this is subset of CINE68K.LIB.
// 0x0000923C-0x00009717 CINELIST.GAS
// 0x00009718-0x0000975B zeros - copied from Track2 (1 based) (and the code above was probably directly above this in Track2 (1 based)).
// 0x0000975C-0x00009853 0x0000001, 0x00000001, 0x00000002, 0x000000002, 0x0000003, 0x00000003, ..., 0x0000001F, 0x0000001F - copied directly from Track2 (1-based) and was directly below the zeros in Track2.
// 0x00009854-0x00009897 0x0000001Fs - copied contiguous from above Track2 (1-based).
// 0x00009898-0x000098DB 0x00s - copied contiguous from above Track2 (1-based).
// 0x000098DC-0x000099D7 0x00000001, 0x00000002, 0x00000003, ..., 0x00000003F - copied contiguous from above Track2 (1-based).
// 0x000099D8-0x00009A17 0x0000003Fs - copied contiguous from above Track2 (1-based).
// 0x00009A18-0x00009A8F 0x00s - copied contiguous from above Track2 (1-based).
// 0x00009A90-0x00009A93 0x00000025 - (copied contiguous from above Track2 (1-based))
// 0x00009A94-0x00009B4B looks like pointers to GPU local RAM (presumably methods, but only 46 of them (SCRIPT.GAS provides 70 methods) - copied contiguous from above Track2 (1-based).
// 0x00009B4C-0x00009B4F 0x00s - copied contiguous from above Track2 (1-based).
// 0x00009B50-0x00009C8F highlife - green lifebar bitmap (they only provide pixels for first phrase of each line of green lifebar, then setup OP to repeat that phrase for remainder of line).
// 0x00009C90-0x0000B08F ???? pattern of 16 bytes zeros, then 64 bytes non zero, with that 80 byte layout repeated 64 times.  Copied from Track2 (1-based), pretty much contiguous from above (think it was contiguous copy, but then highlife contents got blitted over that area).
//                       would make sense for this to be MAIN.S highlifebar, which includes highlife.rgb.  highlife.rfb is 5200 bytes long and 80x64=5200.
//                       however, never saw randomly placed breakpoints, in this memory block, hit.  even on hard boot.
//                       and verified highlander title bar is actually at 0x0001C188-0x0001F387, via breakpoints and overwriting memory to verify displayed as such.
//                       dunno.
// 0x0000B090-0x0000B114 sintbl - (MAIN.s, ANDY.GAS, ANIM.GAS, FORMAT.GAS)  Copied from Track2(1-based), pretty much contiguous from above.
// 0x0000B115-0x0000B117 zero padding
// 0x0000B118-0x0000B15F dummymod - definitely a model related to hands/holding-weapon-with-hands.  believe bare hands model.  Copied from Track2(1-based), pretty much contiguous from above.
// 0x0000B160-0x0000B17F text index and pointers to begin/end of "BARE HANDS" texts for display in Options/Inventory screen.
//   0x0000B160-0x0000B163 0x00000000 believe index of the following 3 lines.
//   0x0000B164-0x0000B167 pointer to blob of english text (0x00019760).
//   0x0000B168-0x0000B16B pointer to blob of french text (0x000019F3F).
//   0x0000B16C-0x0000B16F pointer to blob of german text (0x0001A83A).
//   0x0000B170-0x0000B173 0x00000001 believe index of the following 3 lines.
//   0x0000B174-0x0000B177 pointer to blob of english text (0x0001976B).
//   0x0000B178-0x0000B17B pointer to blob of french text (0x000019F4A).
//   0x0000B17C-0x0000B17F pointer to blob of german text (0x0001A84D).
// 0x0000B180-0x0000B184 0x7Es - ascii tildes.  Copied from Track2(1-based), pretty much contiguous from above.
// 0x0000B185-0x0000B1C5 "       INVENTORY	|A REJECT  B EXAMINE  C USE|      OPTION ACCEPT~"  Copied from Track2(1-based), pretty much contiguous from above.
// 0x0000B1C6-0x0000B203 "        INVENTORY|A DROP  B EXAMINE  C USE|      OPTION EXITS~  "   Copied from Track2(1-based), pretty much contiguous from above.
// 0x0000B204-0x0000B361 <french and german versions of the above 2 lines>   Copied from Track2(1-based), pretty much contiguous from above.
// 0x0000B362-0x0000B363 zero padding   Copied from Track2(1-based), pretty much contiguous from above.
// 0x0000B364-0x0000B387 pointers to the start characters of the english, french, and german texts defined in 0x0000B185-0x0000B4F3 (text immediately above and text immediately below these pointers)  Copied from Track2(1-based), pretty much contiguous from above.
// 0x0000B388-0x0000B3FA "PLEASE REMOVE THE DISK,|ENSURE IT IS CLEAN, AND|REPLACE IT.  THE GAME|WILL CONTINUE WHEN THE|DRIVE DOOR IS CLOSED.~"  Copied from Track2(1-based), pretty much contiguous from above.
// 0x0000B3FB-0x0000B4F3 <french and german versions of the above 1 line>  Copied from Track2(1-based), pretty much contiguous from above.
// 0x0000B4F4-0x0000B597 zeros
// 0x0000B4F8-0x0000B597 bitmap image for ' '  This and following bitmap images defined in FONT4.RGB.  Copied from Track2(1-based), pretty much contiguous from above.
// 0x0000B598-0x0000B637 bitmap image for '!'
// 0x0000B638-0x0000B6D7 bitmap image for '"'
// 0x0000B6D8-0x0000B777 bitmap image for '#'
// 0x0000B778-0x0000B817 bitmap image for '$'
// 0x0000B818-0x0000B8B7 bitmap image for '%'
// 0x0000B8B8-0x0000B957 bitmap image for '&'
// 0x0000B958-0x0000B9F7 bitmap image for '''
// 0x0000B9F8-0x0000BA97 bitmap image for '('
// 0x0000BA98-0x0000BB37 bitmap image for ')'
// 0x0000BB38-0x0000BBD7 bitmap image for '*'
// 0x0000BBD8-0x0000BC77 bitmap image for '+'
// 0x0000BC78-0x0000BD17 bitmap image for ','
// 0x0000BD18-0x0000BDB7 bitmap image for '-' (subtract sign)
// 0x0000BDB8-0x0000BE57 bitmap image for '.'
// 0x0000BE58-0x0000BEF7 bitmap image for '/'
// 0x0000BEF8-0x0000BF97 bitmap image for '0'
// 0x0000BF98-0x0000C037 bitmap image for '1'
// 0x0000C038-0x0000C0D7 bitmap image for '2'
// 0x0000C0D8-0x0000C177 bitmap image for '3'
// 0x0000C178-0x0000C217 bitmap image for '4'
// 0x0000C218-0x0000C2B7 bitmap image for '5'
// 0x0000C2B8-0x0000C357 bitmap image for '6'
// 0x0000C358-0x0000C3F7 bitmap image for '7'
// 0x0000C3F8-0x0000C497 bitmap image for '8'
// 0x0000C498-0x0000C537 bitmap image for '9'
// 0x0000C538-0x0000C5D7 bitmap image for ':'
// 0x0000C5D8-0x0000C677 bitmap image for ';'
// 0x0000C678-0x0000C717 bitmap image for '<'
// 0x0000C718-0x0000C7B7 bitmap image for '='
// 0x0000C7B8-0x0000C857 bitmap image for '>'
// 0x0000C858-0x0000C8F7 bitmap image for '?'
// 0x0000C8F8-0x0000C997 bitmap image for '@'
// 0x0000C998-0x0000CA37 bitmap image for 'A'
// 0x0000CA38-0x0000CAD7 bitmap image for 'B'
// 0x0000CAD8-0x0000CB77 bitmap image for 'C'
// 0x0000CB78-0x0000CC17 bitmap image for 'D'
// 0x0000CC18-0x0000CCB7 bitmap image for 'E'
// 0x0000CCB8-0x0000CD57 bitmap image for 'F'
// 0x0000CD58 0x0000CDF7 bitmap image for 'G'
// 0x0000CDF8-0x0000CE97 bitmap image for 'H'
// 0x0000CE98-0x0000CF37 bitmap image for 'I'
// 0x0000CF38-0x0000CFD7 bitmap image for 'J'
// 0x0000CFD8-0x0000D077 bitmap image for 'K'
// 0x0000D078-0x0000D117 bitmap image for 'L'
// 0x0000D118-0x0000D1B7 bitmap image for 'M'
// 0x0000D1B8-0x0000D257 bitmap image for 'N'
// 0x0000D258-0x0000D2F7 bitmap image for 'O'
// 0x0000D2F8-0x0000D397 bitmap image for 'P'
// 0x0000D398-0x0000D437 bitmap image for 'Q'
// 0x0000D438-0x0000D4D7 bitmap image for 'R'
// 0x0000D4D8-0x0000D577 bitmap image for 'S'
// 0x0000D578-0x0000D617 bitmap image for 'T'
// 0x0000D618-0x0000D6B7 bitmap image for 'U'
// 0x0000D6B8-0x0000D757 bitmap image for 'V'
// 0x0000D758-0x0000D7F7 bitmap image for 'W'
// 0x0000D7F8-0x0000D897 bitmap image for 'X'
// 0x0000D898-0x0000D937 bitmap image for 'Y'
// 0x0000D938-0x0000D9D7 bitmap image for 'Z'
// 0x0000D9D8-0x0000DA77 bitmap image for '['
// 0x0000DA78-0x0000DB17 bitmap image for '\'
// 0x0000DB18-0x0000DBB7 bitmap image for ']'
// 0x0000DBB8-0x0000DC57 bitmap image for '^'
// 0x0000DC58-0x0000DCF7 bitmap image for '_'
// 0x0000DCF8-0x0000DD97 bitmap image for '|'
// 0x0000DD98-0x0000DE37 bitmap image for non-english A
// 0x0000DE38-0x0000DED7 bitmap image for non-english A
// 0x0000DED8-0x0000DF77 bitmap image for non-english character
// 0x0000DF78-0x0000E017 bitmap image for non-english character
// 0x0000E018-0x0000E0BA bitmap image for non-english character
// 0x0000E0BB-0x0000E157 bitmap image for non-english character
// 0x0000E158-0x0000E1F7 bitmap image for non-english character
// 0x0000E1F8-0x0000EB9F title screen sword model  - this and below models, below text numbers and below pointers copied from Track2(1-based), pretty much contiguous from above.
// 0x0000EBA0-0x0000EE7F macleod sword model.
//somewhere models and text pointers are not copied contiguously from Track2(1-based).  Not certain they are even all from Track2.
// 0x0000EE80-0x0000EE8F text number and pointers to text for macleod sword model (english: 0x00019791, + 2 pointers for french and german).
//                       format (for this and following models is <text number starting at zero and incrementing by one><pointer to english text><pointer to french text><pointer to german text> repeat for any other texts.
// 0x0000EE90-0x0000EE97 likely track number and block offset for macleod sword cinepak video (0x00000002 0x00016DB9).
// 0x0000EE98-0x0000F777 gasgun model.
// 0x0000F778-0x0000F797 text numbers and pointers to texts for gas gun model (0x000197AB).
// 0x0000F798-0x0000F848 ?????? 0x04s for 176 bytes (like all the other models immediately below and above, contigously copied from Track2(1-based).
// 0x0000F848-0x0001011F rubber chicken model.
// 0x00010120-0x0001013F text numbers and pointers to texts for rubber chicken model (0x000197FA).
// 0x00010140-0x000109EF cheese model.
// 0x000109F0-0x00010A0F text numbers and pointers to texts for cheese model (0x0001989A).
// 0x00010A10-0x0001116F bread/loaf model.
// 0x00011170-0x0001118F text numbers and pointers to texts for bread/loaf model (0x00019869).
// 0x00011190-0x000116A7 merlot79/bottle model.
// 0x000116A8-0x000116C7 text numbers and pointers to texts for merlot79/bottle model (0x000198B7).
// 0x000116C8-0x00011CF7 key model.
// 0x00011CF8-0x00011D07 text number and pointers to text for key model (0x0001B288) (only has text number 1, with no text number 0).
// 0x00011D08-0x000121BF locket model.
// 0x000121C0-0x000121C7 text number and pointers to text for locket model (0x0001991B).
// 0x000121D0-0x000121D7 track number and blockoffset to locket cinepak video.  0x00000002 0x0000C242.
// 0x000121D8-0x0001255F waterwheel model.
// 0x00012560-0x0001256F text number and pointers to text for waterwheel model (0x0001992B).
// 0x00012570-0x00012577 likely track number and block offset for waterwheel cinepak video  0x00000002 0x00012C6D.
// 0x00012578-0x000127CF crowbar model.
// 0x000127D0-0x000127EF text numbers and pointers to texts for crowbar model (0x0001993E).
// 0x000127F0-0x00012E47 lever model.
// 0x00012E48-0x00012E67 text numbers and pointers to texts for lever model (0x000199C5).
// 0x00012E68-0x0001332F map model.
// 0x00013330-0x0001333F text number and pointers to text for map model (0x0001B2D8) (only has text number 1, with no text number 0).  and middle pointer looks invalid).
// 0x00013340-0x00013667 orders model.
// 0x00013668-0x00013677 text number and pointers to text for map model (0x0001B338) (only has text number 1, with no text number 0).  and middle pointer looks invalid).
// 0x00013678-0x00013C27 stopwatch model.
// 0x00013C28-0x00013C47 text numbers and pointers to texts for stopwatch model (0x000199EF).
// 0x00013C48-0x00013D5F wooden stick model.
// 0x00013D60-0x00013D7F text numbers and pointers to texts for wooden stick model (0x00019A2E).
// 0x00013D80-0x00013E97 wooden plank model.
// 0x00013E98-0x00013EB7 text numbers and pointers to texts for wooden plank model (0x00019A52).
// 0x00013EB8-0x00014E4F uniform model.
// 0x00014E50-0x00014E6F text numbers and pointers to texts for uniform model (0x000199A4).
// 0x00014E70-0x00015427 flower model.
// 0x00015428-0x00015447 text numbers and pointers to texts for flower model (0x00019980).
// 0x00015448-0x00015457 0x00000000 0x00000000 0x0000007E 0x00000000 - this was found earlier, in Track2(1-based), than expected if all models were pulled contiguously from Track2).
// 0x00015458-0x00016CF7 wscopy - copy of world state data as read from Track2(1-based), with non final pointer values (based on comparing nonpointer values for first 5 32-byte entries).  6304 bytes.  197 32 byte instances.  sheet pointers are 0x0001xxxxs.  pointers look valid, but to each other and to copy of character sheets that are lower in memory then the character sheets associated to 0x00032660 copy of world state.  looks like this gets copied (and pointers adjusted in copy) to 0x00032660.  possibly due to initial design thought of saving differences in state to memory track and/or to make soft boot of game not have to read this from CD.  walked around, fought, and saved to slot with read breakpoint on the quentin item at top of this and no breakpoint hit.
// 0x00016CF8-0x0001745B zeros - MAIN.S::wscopy suggests world state is 8k long, so this would be zero padding at end of world state memory.  Copied from Track2(1-based), contiguously from above line.
// 0x00017458-0x0001745B cscopy - pointer to 0x0001775C.  Copied from Track2(1-based), contiguously from above line.
// 0x0001745C-0x0001775B zeros.  Copied from Track2(1-based), contiguously from above line.
// 0x0001775C-0x000184F7 copy of character sheets - as read from Track2(1-based), contigously from above line. pointers to models and animations are null.  determined by matching block offset values for people characters, but did not verify block offsets of item block offsets.
// 0x000184F8-0x0001975F zeros - to pad out character worksheets to 8k.  Copied from Track2(1-based), contiguously from above line.
// 0x00019760-0x00019790 BARE HANDS etc text -  This text and all the texts directly below copied from Track2(1-based), contiguously from above line.
// 0x00019791-0x000197AA THE SWORD OF THE MACLEODS etc text
// 0x000197AB-0x000197F9 GAS GUN etc text
// 0x000197FA-0x00019868 RUBBER CHICKEN etc text
// 0x00019869-0x00019899 BREAD etc text
// 0x0001989A-0x000198B6 CHEESE etc text
// 0x000198B7-0x000198EE BOTTLE etc text
// 0x000198EF-0x0001991A KEY etc text
// 0x0001991B-0x0001992A MOTHER'S LOCKET etc text
// 0x0001992B-0x0001993D CLYDE'S WATERWHEEL etc text
// 0x0001993E-0x0001997F CROWBAR etc text
// 0x00019980-0x000199A3 FLOWER etc text
// 0x000199A4-0x000199C4 UNIFORM etc text
// 0x000199C5-0x000199EE LEVER etc text
// 0x000199EF-0x00019A2D STOPWATCH etc text
// 0x00019A2E-0x00019A51 WOODEN STICK etc text
// 0x00019A52-0x00019A7C WOODEN PLANK etc text
// 0x00019A7D-0x00019A80 MAP etc text
// 0x00019A81-0x00019A90 HUNTER'S ORDERS etc text
// 0x00019A91-0x00019AAF KEY FOUND AT THE|CANYON GATE etc text
// 0x00019AB0-0x00019AD3 A KEY FOUND IN THE|HUNTERS BARRACKS etc text
// 0x00019AD4-0x00019AFC A KEY FOUND IN THE|HUNTERS TRAINING AREA etc text
// 0x00019AFD-0x00019B1D A KEY FOUND IN THE|SECURITY ROOM etc text
// 0x00019B1E-0x00019B27 HEAVY KEY etc text
// 0x00019B28-0x00019B39 MAINTENANCE KEY 1 etc text
// 0x00019B3A-0x00019B4B MAINTENANCE KEY 2 etc text
// 0x00019B4C-0x00019B58 SECURITY KEY etc text
// 0x00019B59-0x00019B61 DOOR KEY etc text
// 0x00019B62-0x00019B72 MAP OF HIGHLANDS etc text
// 0x00019B73-0x00019B81 MAP OF CANYONS etc text
// 0x00019B82-0x00019B91 MAP OF FAVELLAS etc text
// 0x00019B92-0x00019B0F MAP OF SEWERS etc text
// 0x00019BA0-0x00019BB5 MAP OF ENERGY CHAMBER etc text
// 0x00019BB6-0x00019BC4 MAP OF VILLAGE etc text
// 0x00019BC5-0x00019BDC LOOK-OUT HUNTERS ORDERS etc text
// 0x00019BDD-0x00019BF3 VILLAGE HUNTERS ORDERS etc text
// 0x00019BF4-0x00019C09 CANYON HUNTERS ORDERS etc text
// 0x00019C0A-0x00019C25 ENERGY DOME OFFICERS ORDERS etc text
// 0x00019C26-0x00019C3F CANYON GATEKEEPERS ORDERS etc text
// 0x00019C40-0x00019C54 TANK OFFICERS ORDERS etc text
// 0x00019C55-0x00019C6E FRONT GATE HUNTERS ORDERS etc text
// 0x00019C6F-0x00019CB1 MAKE A MAP OF THE VILLAGE|OF THE DUNDEES AND THE|SURROUNDING AREA. etc text
// 0x00019CB2-0x00019E4D other text (possibly orders) (english text) etc text
// 0x00019E4E-0x0001B124 (french and german text equivalents of direct above).
// 0x0001B125-0x0001B127 zeros - probably padding.  Copied from Track2(1-based), contiguously from above line.
// 0x0001B128-0x0001B16F six 12 byte entries in format of <ptr to [real] map world state entry><0x00000003><0x0000xxxx ptr?>  Copied from Track2(1-based), contiguously from above line.
// 0x0001B170-0x0001B287 fourteen 20 byte entries of format <ptr to key|map|order [real] world state entry><0x00000001><ptr to english text><ptr to french text><ptr to german text>.  probably here, cause text is not 1-to-1 with model for keys, maps, orders.  Copied from Track2(1-based), contiguously from above line.
// 0x0001B288-0x0001B3C7 twenty 16 byte entries of format <ptr to key|map|order [real] world state entry><ptr to english text><ptr to french text><ptr to german text>.  probably here, cause text is not 1-to-1 with model for keys, maps, orders.  Copied from Track2(1-based), contiguously from above line.
// 0x0001B3C8-0x0001C187 320 0x39s, then 0x02s, 0x0A, 0x35, 0x0D, 0x49, 0x09, 0x16, 0x1E, 0x24, 0x34.  11 rows of 320 pixels/bytes wide.  Copied from Track2(1-based), contiguously from above line.  read breakpoint not hit, even when go from soft reset to actual gameplay.
// 0x0001C188-0x0001F387 highlander title bar 40 rows 320 pixels/bytes wide (this is before in game, so subject to change for game).  Copied from Track2(1-based), contiguously from above line.
// 0x0001F388-0x00020DC7 320 0x6Es, then 0x35s, 0xB3, 0x45, 0x0A, 0xB1, 0x02, 0x39, 0xB2, 0xB2, 0xB2, 0xB2, 0x38, 0x38, 0x38, 0x38, 0x58, 0x58, 0x58, 0x58.  Copied from Track2(1-based), contiguously from above line.
// 0x00020DC8-0x00023FC7 blue/purple lifebar/mask image.  40 rows 320 pixels/bytes wide.  Copied from Track2(1-based), contiguously from above line.
// 0x00023FC8-0x00024C47 320 0x39s, then 0x39s, 0x39, 0x39, 0x39, 0xB2, 0xB2, 0xB2, 0xB2, 0xB2.  10 rows of 320 pixels/bytes wide.  Possibly used for color cycling or maybe the lifebar mask image is taller outside of normal gameplay.  Copied from Track2(1-based), contiguously from above line.  Read breakpoint not hit, even when go from soft reset to actual gameplay.  Copied from Track2(1-based), contiguously from above line.
// 0x00024C48-0x00024C49 length, in 2-byte-words, of following color lookup table (CLUT) values.  Copied from Track2(1-based), contiguously from above line.
// 0x00024C4A-0x00024DB1 color lookup table (CLUT) values - Copied from Track2(1-based), contiguously from above line.
// 0x00024DB2-0x00024F69 "JOYPAD RECONFIGURE. <BLAH BLAH>".  Copied from Track2(1-based), contiguously from above line.
// 0x00024F70-0x00024F7B pointers to english, french, german texts.  Copied from Track2(1-based), contiguously from above line.
// 0x00024F7C-0x00024FD0 "YOU CAN'T USE THAT HERE.  <BLAH BLAH>".  Copied from Track2(1-based), contiguously from above line.
// 0x00024FFC-0x000251F7 "MUSIC VOLUME PRESS UP OR DOWN TO ALTER <BLAH BLAH>" then <same for effects volume>.  Copied from Track2(1-based), contiguously from above line.
// 0x000251F8-0x00026D6B pause2 (PAUSE2.RGB) - Copied from Track2(1-based), contiguously from above line.
// 0x00026D78-0x00026E77 barol (CBAR.S) - Copied from Track2(1-based), contiguously from above line.
// 0x00026D78-0x00026E97 endbcl (CBAR.S) - Copied from Track2(1-based), contiguously from above line.
// 0x00026E98-0x00026F97 arctan_table (JOY.S) - Copied from Track2(1-based), contiguously from above line.
// 0x00026F98-0x00026F9F ?????? 0x80000000 0x00000000 - Copied from Track2(1-based), contiguously from above line.
// 0x00026FA0-0x00026FAF app_name (NVRAM.S) - Copied from Track2(1-based), contiguously from above line.
// 0x00026FB0-0x00026FB9 filename (NVRAM.S) - Copied from Track2(1-based), contiguously from above line.
// 0x00026FBA-0x00026FEB nvram_files (NVRAM.S) - Copied from Track2(1-based), contiguously from above line.
// 0x00026FEC-0x000271FF save_text, full1, instr (NVRAM.S) - slightly different and now multilingual.  Copied from Track2(1-based), contiguously from above line.
// 0x00027200-0x0002728F ?????? presumably NVRAM.S related.  Copied from Track2(1-based), contiguously from above line.
// 0x00027290-0x00027558 nverror1 through nverror9 - (One time thought saw MAIN.S recognize 0x00027218 as nverror1, based on breakpoint near top of MAIN.S).  Copied from Track2(1-based), contiguously from above line.
// 0x00027559-0x00027561 nvopstar (NVRAM.S) - Copied from Track2(1-based), contiguously from above line.
// 0x00027562-0x00027562 nvfistar (NVRAM.S) - Copied from Track2(1-based), contiguously from above line.
// 0x00027563-0x00027617 ?????? looks like gpu code with no rts/rte.  reference to quentin world state item's character sheet.  instruction breakpoint not hit on hard start, walking around, selecting weapon, combating enemy fighter.  Copied from Track2(1-based), contiguously from above line.
// 0x00027618-0x00027667 likely KERNEL.GAS::StartGPU.  nop instructions for interrupts.  Copied from Track2(1-based), contiguously from above line.
// 0x00027668-0x00027675 likely KERNEL.GAS jump into GPU RAM (continuing from prior line).  Copied from Track2(1-based), contiguously from above line.
// 0x00027676-0x0002850B possibly KERNEL.GAS nops after the jump into GPU RAM, but with WAY more nop statements then KERNEL.GAS specifies.  1,867 nop statements?  Copied from Track2(1-based), contiguously from above line.
// 0x0002850C-0x000285D5 KERNEL.GAS::kerncode
// 0x000285D6-0x000285D7 likely padding for KERNEL.GAS
// 0x000285D8-0x000285EF memspace KERNEL.GAS likely
// 0x000285F0-0x00028617 returnadds / endspace KERNEL.GAS likely
// 0x00028618-0x00028767 3DENGINE.GAS STAGE1 code
// 0x00028768-0x000287F7 lights (3DENGINE.GAS) presumably, but twice as many long words as source file (each one long word? (onoff, ambient, rgb1, rgb2, rgb3, rgb4, x1, y1, z1, x2, y2, z2, x3, y3, z3, x4, y4, z4)
// 0x000287F8-0x000287FF MODELLER (3DENGINE.GAS)
// 0x00028800-0x00029291 remainder of 3DENGINE.GAS code
// 0x00029292-0x00029297 zero padding per 3DENGINE.GAS
// 0x00029298-0x0002934B internal storage for ilist (16 longs), fvlst (12 longs), fheader (2 longs), wcvtx (2 longs), idmatrix (9 longs), idpos (3 longs), for 3DENGINE.GAS.
// 0x0002934C-0x0002935B jump table pointers to clipit, cliprt, clip3, clipdn subroutines, for 3DENGINE.GAS.
// 0x0002935C-0x0002938B lastsavor (3DENGINE.GAS)
// 0x0002938C-0x0002938F zero padding, per 3DENGINE.GAS
// 0x00029390-0x00029397 BITMAP (3DBITMAP.GAS)
// 0x00029398-0x00029521 StartGPU 3DBITMAP.GAS code
// 0x00029522-0x00029527 zero padding, per 3DBITMAP.GAS
// 0x00029528-0x0002952F GETMAT (FORMMAT.GAS)
// 0x00029530-0x00029731 FORMMAT.GAS code
// 0x00029732-0x00029737 zero padding, per FORMMAT.GAS
// 0x00029738-0x0002975B idmatrix (FORMMAT.GAS internal)
// 0x0002975C-0x00029767 idpos (FORMMAT.GAS internal)
// 0x00029768-0x00029773 rotations (FORMMAT.GAS internal)
// 0x00029774-0x00029877 sintbl2 (words stored in longs) (FORMMAT.GAS)
// 0x00029878-0x0002988F endspace (FORMMAT.GAS)
// 0x00029890-0x00029897 CDCONTROL (CDCONTRO.GAS)
// 0x00029898-0x0002A1AB StartGPU through freeupmem routines (CDCONTRO.GAS).  routine the middle of this (referencing SAMPLEDATA?)
// 0x0002A1AC-0x0002A1B9 CDCONTRO.GAS nomor_entry subroutine
// 0x0002A1BA-0x0002A413 SETLOGIC.GAS code (SETLOGIC.GAS is included in the middle of CDCONTRO.GAS...)
// 0x0002A414-0x0002A465 unsure.  GPU code, presumably from extended SETLOGIC.GAS or CDCONTRO.GAS.  Makes reference to chartbl
// 0x0002A466-0x0002A497 CDCONTRO.GAS::freeupmem subroutine
// 0x0002A498-0x0002A51B CDCONTRO.GAS::GetTrack subroutine
// 0x0002A51C-0x0002A56D CDCONTRO.GAS::memspace (20 longs initialized to zero) (nested return addresses)
// 0x0002A56E-0x0002A56F CDCONTRO.GAS zero padding
// 0x0002A570-0X0002a577 EVENT (EVENT.GAS)
// 0X0002A578-0x0002B1ED EVENT.GAS::StartGPU code
// 0x0002B1EC-0x0002B1EF zero padding, per EVENT.GAS
// 0x0002B1F0-0x0002B247 EVENT.GAS::ememspace - 24 long words
// 0x0002B248-0x0002B287 EVENT.GAS::ereturnadds - 16 long words
// 0x0002B288-0x0002B28F zero padding 8 bytes, and don't see why its there, unless phrase alignment goes ahead and adds a phrase if already phrase aligned.
// 0x0002B290-0x0002B297 EVENT.GAS::NEWSCENE
// 0x0002B298-0x0002B3E3 EVENT.GAS::StartGPU2
// 0x0002B3E4-0x0002B3E7 zero paddingl, per EVENT.GAS
// 0x0002B3E8-0x0002B447 EVENT.GAS::nmespace - 24 long words
// 0x0002B448-0x0002B487 EVENT.GAS::nreturnadds - 16 long words
// 0x0002B488-0x0002B48F EVENT.GAS::EV2
// 0x0002B490-0x0002B537 EVENT.GAS::StartGPU3
// 0x0002B538-0x0002B597 EVENT.GAS::e2memspace - 24 long words
// 0x0002B598-0x0002B5D7 EVENT.GAS::e2returnadds - 16 long words
// 0x0002B5D8-0x0002B5DF ANIM.GAS::ANIMCODE
// 0x0002B5E0-0x0002BA91 ANIM.GAS::Start.GPU line1 through line963 of ANIM.GAS
// 0x0002BA92-0x0002BE7B COLLIDE.GAS::initialize (they included COLLID.GAS in middle of ANIM.GAS)
// 0x0002BE7C-0x0002BFD7 FINDTRI.GAS
// 0x0002BFD8-0x0002BFF9 COLLIDE.GAS::<last 16 lines>
// 0x0002BFFA-0x0002C1A5 ANIM.GAS::colskip to end of ANIM.GAS code
// 0x0002C1A6-0x0002C1AB zero padding, per ANIM.GAS
// 0x0002C1AC-0x0002C2AB ANIM.GAS::sintbl
// 0x0002C2AC-0x0002C2BB ANIM.GAS::stancelist
// 0x0002C2BC-0x0002C2BF zero padding
// 0x0002C2C0-0x0002C2CF WAVE.DAS::WaveHeader
// 0x0002C2D0-0x0002C65F WAVE.DAS code (looking heavily modified (in at least places the places that reviewed))
// 0x0002C660-0x0002C8EF WAVE.DAS playing, redbookdetect, WaveInterface, WaveCodeTable, WaveSoundTable, WaveSoundBuffer.  Possibly WaveCodeTable has 3 more pointers then revision of source code.  playing, redbookdetect, WaveInterface have 6 longs according to revision of source code, but Noesis Memory Window shows only 4 longs (possibly playing/redbookdetect moved below WaveCodeTable.  Someone else can figure out by setting breakpoints and comparing disassembly to revision of source code.
// 0x0002C8F0-0x0002C8F7 SCNLOGIC.GAS::CHARNEWSCN
// 0x0002C8F8-0x0002CC11 SCNLOGIC.GAS code
// 0x0002CC12-0x0002CC17 zero padding, per SCNLOGIC.GAS
// 0x0002CC18-0x0002CC1F AICTRL.GAS::AIControlHeader
// 0x0002CC20-0x0002D72F AICTRL.GAS::AIControlStart to ControlCodeTable (code)
// 0x0002D730-x00002D76B AICTRL.GAS::ControlCodeTable (appears an additional routine was added to AICTRL.GAS)
// 0x0002D76C-0x0002D76F zero padding, per AICTRL.GAS
// 0x0002D770-0x0002D777 COMBAT.GAS::PPCOLL
// 0x0002D778-0x0002DD3D COMBAT.GAS code
// 0x0002DD3E-0x0002DD3F zero padding, per COMBAT.GAS
// 0x0002DD40-0x0002DD47 COLLECT.GAS::COLLECT
// 0x0002DD48-0x0002E867 COLLECT.GAS::StartGPU
// 0x0002E868-0x0002E887 8 longs initialized to 0, per COLLECT.GAS
// 0x0002E888-0x0002E88F
// 0x0002E890-0x0002EF01 ??????
// 0x0002EF02-0x0002EFE3 CSHCODE.GAS source
// 0x0002EFE4-
// 0x0002F028-0x0002F02F SHOWT.GAS::TEXTS
// 0x0002F030-0x0002F249 SHOWT.GAS::StartGPU
// 0x0002F04A-0x0002F24F zero padding, per SHOWT.GAS
// 0x0002F250-0x0002F26F SHOWT.GAS::8 long words initialized to zero
// 0x0002F270-0x0002F30F long words initialized to zero by ???
// 0x0002F310-0x0002F317 SCRIPT.GAS::SCRIPTCODE
// 0x0002F318-0x0002FD8B SCRIPT.GAS
// 0x0002FD8C-0x0002FD9F SCRIPT.GAS::condtable
// 0x0002FDA0-0x0002FEB7 SCRIPT.GAS::scripttable - pointers to complex script commands/subroutines.  See "Scripts And Script Commands" documetentation/comments near the bottom of this script.
// 0x0002FEB8-0x000300BB probably a script.  would have been SAMPLE.SCT::MAINSCRIPT, but unsure if a "sample" script or other script would be linked into production release.  MAINSCRIPT may spawn cheat code
// 0x000300BC-0x0003012B zeros (SAMPLE.SCT specified 128 long words be here, but this is significantly fewer then 128 long words.  Perhaps a different, used script that is longer then SAMPLE.SCT, or else SAMPLE.SCT was updated to be longer.
// 0x0003012C-0x0003012F zeros? (SAMPLE.SCT::ScriptLimit followed the 128 long words)
// 0x00030130-0x00030137 SCENEPR.GAS::SCENEPR
// 0x00030138-0x0003032D SCENEPR.GAS code
// 0x0003032E-0x0003032F zero padding, per SCENEPR.GAS
// 0x00030330-0x0003034F SCENEPR.GAS 8 long words
// 0x00030350-0x00030357 GPUCTRL.GAS::GPUControlHeader
// 0x00030358-0x00030385 GPUCTRL.GAS code
// 0x00030386-0x00030387 zero padding, per GPUCTROL.GAS
// 0x00030388-0x00030397 CINESTUB.GAS::CineStubHeader
// 0x00030398-0x000304DB CINESTUB.GAS code
// 0x000304DC-0x000304DF zero padding, per CINESTUB.GAS
// 0x000304E0-0x000304EF CINEDSP.DAS::CineAudioHeader
// 0x000304F0-0x000305FF CINEDSP.DAS code
// 0x00030600-0x00030603 CINEDSP.DAS CineAudioVolume
// 0x00030604-0x00030607 CINEDSP.DAS CineAudioInterface
// 0x00030608-0x0003060B CINEDSP.DAS CineAudioState
// 0x0003060C-0x0003060F CINEDSP.DAS <zero padding>
// 0x00030610-0x0003260F 8k image deobfuscation data block, contained on Track2(1-based)
// 0x00032610-0x0003265F zeros for 80 bytes (read breakpoint not hit when soft boot, start game, select weapon, walk around, fight.)
// 0x00032660-0x00033EFF ws - world state items/characters.  32 byte entries.  197 defined.  sheet pointers are 0x0003xxxx's.  VIDSTUFF.INC and possibly other locations say entire world state is 16k, MAIN.S which copies says only 8k, and clearly 8K when view memory in Noesis Memory Window.
// 0x00033F00-0x0003465F world state continued, but empty.  59 empty entries.  seems like this would be a place to perform additive hacks (e.g. extra characters/items).  AICTRL refers to wstEntries=0x0100, so 256 entries at 32 bytes a piece, = 8k of world state.
// 0x00034660-0x0003475F 256 zeros - read breakpoint not hit when soft boot, start game, select weapon, walk around, fight, with exception of first 2 long words.  Read breakpoint hit first two long words likely as end-of-loop-bug in loop over world state items to correct pointer values to character sheets.
// 0x00034760-0x0003580F cs - character sheets and set sheet references which are defined by game.  character sheets are variable length.  Vast majority of character sheets are not fully populated, at any snapshot in time.  VIDSTUFF.INC and possibly other locations say entire world state is 16k, MAIN.S which copies says only 8k, and clearly 8K when view memory in Noesis Memory Window.
//      0x00034760-0x00034763 pointer to first character sheet
//      0x00034764-0x00034A63 set list - 192 long word references, so room for 64 12-byte-set-sheets.  Very sparsely populated.
// 0x00034A64-0x00034B4B HL_QUENTIN_SHEET_P_ADDR
// 0x00034B4C-0x00034C2B HL_HOOD_SOLDIER_SHEET_P_ADDR
// 0x00034C2C-0x00034D0B HL_HAIR_SOLDIER_SHEET_P_ADDR
// 0x00034D0C-0x00034DEB HL_GUNMAN1_SHEET_P_ADDR
// 0x00034DEC-0x00034ECB HL_GUNMAN2_SHEET_P_ADDR
// 0x00034ECC-0x00034FAB HL_CLAW_SHEET_P_ADDR
// 0x00034FAC-0x00035023 HL_RAMIREZ_SHEET_P_ADDR
// 0x00035024-0x0003509B HL_MASKED_WOMAN_SHEET_P_ADDR
// 0x0003509C-0x00035113 HL_MANGUS_SHEET_P_ADDR
// 0x00035114-0x0003518B HL_ARAK_SHEET_P_ADDR
// 0x0003518C-0x00035203 HL_KORTAN_SHEET_P_ADDR
// 0x00035204-0x00035277 HL_CLYDE_SHEET_P_ADDR
// 0x00035278-0x000352EB HL_DUNDEE_B_SHEET_P_ADDR
// 0x000352EC-0x0003535F HL_DUNDEE_D_SHEET_P_ADDR
// 0x00035360-0x0003539B HL_TURBINE_SHEET_P_ADDR
// 0x0003539C-0x000353CF HL_TANK_SHEET_P_ADDR
// 0x000353D0-0x00035403 HL_TURRET_SHEET_P_ADDR
// 0x00035404-0x00035433 HL_HAND_SHEET_P_ADDR
// 0x00035434-0x00035463 HL_RED_WATER_SHEET_P_ADDR
// 0x00035464-0x00035493 HL_GREEN_WATER_SHEET_P_ADDR
// 0x00035494-0x000354C3 HL_BLUE_WATER_SHEET_P_ADDR
// 0x000354C4-0x000354EB HL_TITLE_SCREEN_SWORD_SHEET_P_ADDR
// 0x000354EC-0x00035593 MACLEOD_SWORD_SHEET_P_ADDR
// 0x00035594-0x0003563B HL_GAS_GUN_SHEET_P_ADDR
// 0x0003563C-0x000356E3 HL_CHICKEN_SHEET_P_ADDR
// 0x000356E4-0x000356F7 HL_WINE_SHEET_P_ADDR
// 0x000356F8-0x0003570B HL_CHEESE_SHEET_P_ADDR
// 0x0003570C-0x0003571F HL_LOAF_SHEET_P_ADDR
// 0x00035720-0x00035733 HL_KEY_SHEET_P_ADDR
// 0x00035734-0x00035747 HL_LOCKET_SHEET_P_ADDR
// 0x00035748-0x0003575B HL_WATERWHEEL_SHEET_P_ADDR
// 0x0003575C-0x0003576F HL_CROWBAR_SHEET_P_ADDR
// 0x00035770-0x00035783 HL_LEVER_SHEET_P_ADDR
// 0x00035784-0x00035397 HL_MAP_SHEET_P_ADDR
// 0x00035798-0x000357AB HL_ORDERS_SHEET_P_ADDR
// 0x000357AC-0x000357BF HL_STOPWATCH_SHEET_P_ADDR
// 0x000357C0-0x000357D3 HL_STICK_SHEET_P_ADDR
// 0x000357D4-0x000357E7 HL_PLANK_SHEET_P_ADDR
// 0x000357E8-0x000357FB HL_UNIFORM_SHEET_P_ADDR
// 0x000357FC-0x0003580F HL_FLOWER_SHEET_P_ADDR
// 0x00035810-0x0003675F character sheets continued, but no character sheets actually defined here by game.  Probably another place person could perform additive hacks.
// 0x00036760-0x0003885F zeros
// 0x00038860-0x000388AF regsave
// 0x000388B0-0x00038??? collidework (COLLIDE.GAS references and asigns to list_ptr).  Definitely here, but unsure how many bytes it takes up.
// 0x000388B0-0x00038CBF VIDSTUFF.INC claims collidework = WaveTable, and test3 and debugarea are in this block of memory, but VIDSTUFF.INCs memory sizes do not add up.  Didn't see much happening in debugger either, but didn't try very hard either.
// 0x00038CC0-0x0003CCBF instance_data_area - 16k of instance data.  instance data structure appears to be 32 bytes, but the entries seem to be 64 bytes with a trailing 32 bytes of zero data.  counted 84 world state entries with radius=0x64 (fighters), but 79 instance data entries at game startup.  assumed there was instance data entry for each fighter (since they change xyz and facing), but not sure now.  there is room for 256 entries.
// 0x0003CCC0 draw_data_area - 4k of draw data area data.
//	0x0003CEA0 HL_QUENTIN_DRAW_DATA_AREA_START_ADDR
//	0x0003CF90 SWORD_DRAW_DATA_AREA_START_ADDR
//      other entries above, and below.  just listing a couple interesting ones.
// 0x0003DCC0-0x0003FCFF chartbl - character instance table
// 0x0003FD00-0x00041CFF activchar - active character area - associated world state, character sheets, flags, AI attributes for active characters.
// 0x00041D00-0x000421FF cdq - CD Queue
// 0x00042200-0x000422FF zeros presumably.  VIDSTUFF comment states there was 256 bytes reserved betwen Set Data and end of CD Queue.
// 0x00042300-? setdata
//      0x00042314 setdata + SceneOffset is likely the offset from setdata in bytes to the scene/block-offset pairs INFO (info includes a long word stating number of long word pairs).
//      +42334 looks to be number of 8 byte scene number to block offset pairs.
//      the offsets described in structdef.inc are in bytes from start of setdata.
// 0x0004B300-0x000C72FF = memchunks
// 0x000C7300-0x000C76FF svlist - svlist save area (topcode=0x000C7300; endmem = 0x000C7300 (probably end memchunks)).
// 0x000C7700-0x000C7AFF origsave - origin save area
// 0x000C7B00-0x000C82FF vlist2 - vertex list 2 area
// 0x000C8300-0x000C8AFF vlist - vertex list area
// 0x000C8000-0x001F803F cinepak populates/updates while playing cinepak video.
//    0x000C8008 scaled bitmap object pointed to this address when playing back cinepak video.
// 0x000C8B00-0x001266FF buff4, buff5, and zbuff phrase interleaved - 3d frame buffer 1, 3d frame buffer 2, 3d z buffer.  each is 320x200x2 bytes.
// 0x000C8B00 buff4
// 0x000C8B08 buff5
// 0x000C8B10 zbuff
// 0x00100000-0x0010E100 during first 1-2 seconds after hard boot, is 320x180x1 bitmap that displays red LORE rectangle.
// 0x00126700-0x00126AFF ?????? suspect nothing significant.
// 0x00126B00-0x00012AAFF nvworkspace = scratch2 - there is single reference to nvworkspace and single reference to scratch2 in source files.
// 0x0012AB00-0x000132AFF scratch - believe scratch is purely notional.
// 0x00132B00-0x000186BFF buff2 - backbuffer2+Z+data+leadin
// (0x001362A0 frame buffer image source)
// 0x00186C00-0x001DACFF buff1 - backbuffer1+Z+data+leadin
// 0x001DAD00-0x001FFCFF cdbuff - believesaw this in debugger one time, but might be mistaken.
//    0x001E7D00 endnbuff - just calculated address based on VIDSTUFF.INC.  could be wrong.
//    0x001FFD00 endcdbuff - just calculated address based on VIDSTUFF.INC.  could be wrong.
// 0x001FFD00-0x001FFDFF
//    0x001FFA00 object_buffer - for cinepak.  should be there, if CINEPAK.INC equate is accurate.
//    0x001FF810 object_list - for cinepak.  should be there, if CINEPAK.INC equate is accurate.
// 0x001FFE00-0x00200000 512 byte stack.  verified address, via disassembly of CLEARJAG.S.
//
//
//
//
//
//
//
//
//
//
// 0x00F1B398 waveinterface + 12
// 0x00F1B390 ?waveplay
// 0x00F1B618 waitredbook
// 0x00F1B618 redbookdetect











// Layout of significant memory structures.
//
// The revision of source code explains in 6+ files the layout of memory structures,
// but none of the descriptions matches the final production version of game, and
// none of the descriptions match each other.  While helpful, it got confusing,
// so documented memory structures below.
//
// The conflicting descriptions came from, but not limited to:
//     LOGICS.S
//     STRUCDEF.INC
//     SHEET.S
//     VIDSTUFF.INC
//     3DENGINE.GAS(Line898 version 2 description, not the top-of-3DENGINE.GAS description))
//     CDCONTRO.GAS
//
//
//
//
//
//
//
//
//
//
// World State
//  Description
//      Describes the layout/state of all collectables/fighters in the game.
//      Saved/loaded to/from save slots.
//      World State is always present in memory, and fully populated.
//      Comprised of 256 32-byte World State Entries.
//      First 197 entries are populated, while trailing 59 entries are not populated.
//      Noesis Memory Window verifies/shows World State only taking up 8K.
//
//  Layout:
//      wstSet    - 2 bytes  ; set id
//      wstRadius - 2 bytes  ; character radius
//      wstSheet  - 4 bytes  ; pointer to character sheet [0 = null entry]
//      wstParent - 4 bytes  ; pointer to parent WST record
//      wstXpos   - 4 bytes  ; x position
//      wstYpos   - 4 bytes  ; y position
//      wstZpos   - 4 bytes  ; z position
//      wstXface  - 1 byte   ; x facing
//      wstYface  - 1 byte   ; y facing
//      wstZface  - 1 byte   ; z facing
//      wstSanity - 1 byte   ; sanity [0-100]
//      wstPerson - 1 byte   ; personality [0-100]
//      wstStr    - 1 byte   ; strength byte
//      wstLife   - 1 byte   ; life points [0-100]
//      wstFlags  - 1 byte
//
//  Notes:
//      The 59 empty world state entries seems like the place to begin implementing
//      additive hacks (e.g. extra characters/items).
//
//      mScene seems to represent the scene/screen the character currently resides in.
//      (Screen as in background image).
//
//      mRadius affects hit detection, whether enemies engange with Quentin, and whether
//      item can be picked up.  Quentin and other fighters' world state entries all seem to have
//      radius set to 0x64, collectibles have radius set to 0x32, Tank and
//      Turret have radius set to 0, 1.  Forget what Control Panel radius's are set.
//
//      mSheet is address for character sheet.  (Character sheet is table of pointers to model,
//      animations, etc for character.  there are different sheets for different types of enemy fighters,
//      Quentin, each collectable items.  Anything that is displayed that you can fight or collect.)
//
//      mOwner points to address of world state entry which owns this world state entry.  Quentin
//      world state entry and enemy fighter world state entry always have zeros here.  A collectable will
//      have a 0-value if not owned by Quentin or an enemy fighter.  A collectible on a enemy fighter,
//      will point to the enemy fighter's world state entry.  Once the collectable is owned by Quentin,
//      the collectable's world state entry points to Quentin world state entry.
//
//      m<>pos and m<>face deal with the location and direction the world state entry is facing.
//
//      Can't tell that mSanity/mPersonality are really used anywhere.  There may have been
//      plans to make this more of a role playing game, where player attributes are leveled up
//      overtime.  Certainly was evidence of initial plans of things not getting implemented in
//      production release of game.  (Eg. The 3 megabyte version of the obtained source code, had
//      references to character sheets for Ramirez, Clyde, Woman.  But the character sheets/models/etc
//      are not utilized in production release of game.)
//      Actually, wstUsage and wstDword are equated to mSanity and mPersonality and are referenced
//      in revision of source code.  Did not get a read breakpoint to get invoked.
//
//      mStrength is referenced/used in revision of combat source code.  Presumably to influence
//      the strength of a fighter's attack and/or resistance from a fighter's attack.  Did not get
//      a read breakpoint to get invoked.
//
//      mLife is fighter world state entry's remaining health.  This value is reflected in health
//      bar for Quentin world state entry, and Quentin dies when mLife goes down to zero.
//
//      mFlags is used for various state conditions.
//          According to LOGICS.INC
//          WSTRegistered  equ   0
//          WSTInanimate   equ   1
//          WSTAmmo        equ   3
//          WSTWeapon      equ   4
//          WSTCollectable equ   5
//          WSTDeactivated equ   6
//          WSTCollidable  equ   7
//
//
//
//
//
//
//
//
//
// Character Sheets
//  Description:
//      Contiguously stored, linked list of character sheets.
//      Character sheets basically provide definition for fighers/collectables, and internals don't really
//      change state in meaningful way. (Pointers can go from null, to non-null, back to null, etc, but
//      when pointers are present, they point to the same data (though data may be in different location
//      in memory).
//
//      Character sheets provide pointers to models, animations, logics, and sounds associated to fighters.
//      Character sheets provide same/similar for collectibles, as well.
//      One character sheet is pointed to by each world state entry.
//      Character sheet model/animation/miscellaneous pointers are null, until a world state entry
//      using the character sheet becomes active.  Once world state entry becomes active, its character
//      sheet is fully populated by loading models/animations/sounds/logics into memory, and then setting
//      the character sheets models/animations/miscellaneous pointers appropriately.
//
//      Character sheets are always present in memory, in same address.  However, character sheets may not always be fully populated.
//      The internal state is not saved/restored to/from save slots.
//
//  Layout
//      cshNext     - 4 byte pointer to next character sheet (character sheets are in linked list)
//      cshFlags    - 2 byte status flags
//      cshModelOff - 1 byte offset in long words, from beginning of character sheet, to first model pointer.
//      cshModelNum - 1 byte number of model pointers.  load of models via file references seems to ignore this value.
//      cshAnimOff  - 1 byte offset in long words, from beginning of character sheet, to first animation pointer.
//      cshAnimNum  - 1 byte number of animation pointers.  load of animationss via file references seems to ignore this value.
//      cshMiscOff  - 1 byte offset in long words, from beginning of character sheet, to miscellanous data pointers.
//      cshMiscNum  - 1 byte number of miscellaneous data pointers.
//      cshFileOff  - 1 byte offset in long words, from beginning of character sheet, to file references.
//      cshFileNum  - 1 byte number of file references.
//      cshBehavior - 1 byte behavior of fighter character.
//      cshRecord   - 1 byte always seems to be set to 0x00 for fighters, nonzero for some/all collectables.
//      variable length list of 4-byte-pointers to models.
//      variable length list of 4-byte-pointers to animations.
//      variable length list of 4-byte-pointers to miscellaneous data.
//          first miscellaneous appears to point to logic table (believe tying joypad inputs to animations to apply).
//          fighters (Quentin, Hood, Hair, Claw, Gunman1/2) have 5 of these, with the last 4 pointing to WAVE sounds).
//              4 sounds are: soundKIA(killed), soundHIT, soundATT(use weapon to attack), soundPAR (parry).
//      variable length list of 8-byte-file-references
//          2 bytes for entry position
//          2 bytes for track offset
//          4 bytes for block offset
//
//  Notes:
//      LOGICS.INC defines the following values for cshBehavior:
//          ; AI Commands
//          ; ===========
//          aiNop          equ   0
//          aiGotoPosition equ   1
//          aiGotoPerson   equ   2
//          aiFacePosition equ   3
//          aiFacePerson   equ   4
//          aiAttackPerson equ   5
//          aiAttackPlayer equ   6
//          aiFacePlayer   equ   7
//          aiGotoPlayer   equ   8
//          aiFollowPlayer equ   9
//          aiShootPerson  equ   10
//          aiShootPlayer  equ   11
//          aiDefault      equ   12
//          aiFollowPerson equ   13
//
//      File references specify where to load the the models, animations, miscellaneous from.
//
//      Appears models/animations/and-likely-first-miscellaneous are specified by one file reference, and
//      last four miscellanous (WAVE sounds) are specified by second file reference.
//      Would seem to need to be that way for Ramirez, Mangus, Masked Woman, DundeeB, DundeeD, Clyde,
//      Kortan and Arak, based on them only having one file reference and only one miscellaneous populated.
//
//      Entry position appears to be an index into the character sheets long word pointers, indicating
//      where pointers should begin to be updated for this file referenced load.
//
//      Not certain have ever seen 3 fighter character sheets active at same time.  Almost certainly have
//      not seen 4 fighter character sheets active at same time.
//
//      There are memory constraints to how many models/animations/sounds/etc that can be loaded at one time.
//
//      Near the end of this script file are captures of character sheets of the fighters, with newline
//      breaks in between the variable length lists, to help visualize the layout better.
//      This was something done early on in the hacking process, and might as well keep it around for referral
//      in case anyone ever needs to debug some of this script involving the use of other characters models.
//
//
//
//
//
//
//
//
//
// Memory Chunks / Memory Blocks (memchunks)
//  Description:
//      Memory Chunks/Blocks is the format some/all of the data is stored on compact disc,
//      and also how some of that data is managed as dynamically loaded/unloaded
//      from Jaguar main memory.
//
//      From VIDSTUFF.INC
//          ;Notes on Memory Managed area:
//          ;512k area of phrase aligned chunks (448 bytes each = 1170 full chunks)
//          ;one word per chunk in management table
//          ;top bit set if free for reuse, rest indicates 'owner' chunk
//          ;'owner' chunk has info on number of chunks in first long
//          ;and pointer to sheet reference in second long
//          ;first phrase is skipped for pointer info by character/set sheets
//          ;all files must be stored contiguously
//          ;garbage control is done whenever a new set is entered, or explicitly
//          ;by a scene event or character logic
//
//  Layout:
//      First Chunk/Block for model/animation/sound/etc
//          4 byte number of chunks/blocks in the model/animation/sound/etc.
//          4 byte address of character sheet pointer, which points to this chunk/block.
//          440 bytes of data.
//      Subsequent Chunks/Blocks for model/animation/sound/etc
//          448 bytes of data, with trailing zero padding if needed.
//
//  Notes:
//      Dynamically loaded/unloaded data is read and copied to Jaguar main memory
//      in blocks of 448 bytes.  Models, animations, sounds, character logics,
//      fit into this category.  When models, animations, sounds, etc are loaded
//      into main memory, character sheets point 8 bytes into the first memory block
//      of the relevant data, bypassing the 8 byte header for the memory block.
//      Subsequent chunks/blocks for that model/animation/sound/etc are stored
//      contiguous to prior chunks/blocks for that model/animation/sound/etc.
//
//      All chunks/blocks for a model, animation, sound, etc need to be contiguous in memory.
//      Models, animations, sounds, etc do not need to be contiguous in memory, in relation to each other.
//
//      Data for models/animations/sounds/etc is stored on compact disc in the
//      same fashion, but byteswapped at the word level.
//
//      VIDSTUFF.INC documents/comments the following notional addresses for different types of data, though no code references bitmap, charlgcs, animsp, modelspc, snd, and the end address is incorrect
//      The listed types of data are present here, but in interleaved/random fashion.
//      Suspect VIDSTUFF.INC was just showing the math for how much space is needed for the different types of data, not defining strict locations for the data.
//          0x0004B300 bitmaps
//          0x00053300 charlgcs
//          0x0005F300 animspc
//          0x00089300 modelspc
//          0x000A9300 snd
//          0x000C9300 endmem = svlist
//
//      Have verified that one fighter's model data can start at 0x0004B300, with that fighter's animation data immediately following the model data.
//
//      Suspect amount of memory resereved for memblocks/chunks is limiting factor in how many different fighter character sheets can be loaded at once, as the character sheets models, animations, sounds have to be present here.
//
//
//
//
//
//
//
//
//
// Model
//  Description:
//      Dynamically loaded/unloaded, constant data which describes vertices (including origins) and facets
//      of a model.  Models represent weapons, collectibles, and body parts of fighters.
//
//  Layout:
//      2 byte length of model in bytes
//      1 byte origin number
//      1 byte number of origins - Note: Right hand model has value of 1, and left hand model has value of 0, since right hand can hold/articulate a weapon, while left hand cannot hold/articulate a weapon.
//      2 byte number of vertices
//      2 byte number of facets
//      4 byte pointer to variable length list of vertices and origin/vertices (the vertices and origin/vertices are defined in this model).  (8 bytes per vertice / origin-vertice)
//      4 byte pointer to variable length list of facets (the facets are defined in this model)
//      4 byte pointer to "saveout" vertices ("saveout" vertices are "defined" in this model)
//      4 byte pointer to dunno.  Perhaps always null.  One comment in 3DENGINE.GAS referes to CLP.  Never saw this populated.
//      variable length list of vertices (number of entries matches the number of vertices)
//          2 byte x
//          2 byte y
//          2 byte z
//          2 byte 0x0001
//      variable length list of origins (number of entries matches the number of origins)
//          2 byte x
//          2 byte y
//          2 byte z
//          2 byte origin number (128-256) in form of 0x00<origin number from other model that is connected/articulating from this origin/vertice)
//      variable length list of facets (number of entries matches the number of facets)
//          2 byte RGB
//          2 byte x normal (possibly always 0x0000)
//          2 byte y normal (possibly always 0x0000)
//          2 byte z normal (possibly always 0x0000)
//          2 byte number of vertice references (seems to always be 0x0003 for fighters)
//          2 byte code refers to Np.  NP is the ptr offset (from self, in long words) to the next facet entry (seems to always have value of 0x0001 for fighters.)
//          3 bytes 3 vertice references
//          1 byte (seems to always be 0x00 for fighters) (presumably would be a 4th vertice reference if number of vertice references equaled 0x0004)
//      saveouts
//          2 byte reference number of model set (seems to always be 0x0000.  perhaps this is a 'global model set id' (quentin and hood both))
//          2 byte number of "saveouts"
//          variable length list of "saveouts".  length matches the 2 byte number of "saveouts"
//              per "saveout"
//                  2 byte internal (to this model) vertice number
//                  2 byte external (global to all models) vertice number
//          Note: The external vertice numbers seem to increment by 1 within and across models
//          Note: Left hand model has no saveouts.
//          Note: Believe these "saveout" vertices are used where models connect to each other.
//                Foot models had 4 facets, but only 1 vertice defined.  Presumably the 4 facets
//                involve some of the "saveout" vertices from other models.
//
//  Notes
//      Model state is not saved/restored to/from save slots.
//
//      Models used by fighter character sheets look like below, in regards to: order of model, origin number, number of origins, what model depicts
//           1st model  0x00  0x04        torso
//           2nd model  0x87  0x01        left upper arm
//           3rd model  0x88  0x01        left lower arm
//           4th model  0x89  0x00        left hand
//           5th model  0x8A  0x01        right upper arm
//           6th model  0x8B  0x01        right lower arm
//           7th model  0x8C  0x00/0x01   right hand
//           8th model  0x8D  0x00        head
//           9th model  0x80  0x02        pelvis
//          10th model  0x81  0x01        left thigh
//          11th model  0x82  0x01        left shin
//          12th model  0x83  0x00        left foot
//          13th model  0x84  0x01        right thigh
//          14th model  0x85  0x01        right shin
//          15th model  0x86  0x00        right foot
//          Quentin's 7th model (right hand) has number of origins set to 1 so that detachable weapon model
//          can articulate from right hand, while other fighters have a value of 0 for their 7th model (right hand).
//          (Other fighters have the model data for their weapon embedded into their right hand model.)
//
//
//
//
//
//
//
//
//
//
// Animation Layout
//  Description:
//      Dynamically loaded/unloaded, presumably-constant data, which describes how to animate an action for all models of a character.
//
//  Layout:
//      ANIMSIZE:    2 byte - size of animation in bytes
//      OFFSETLOW:   2 byte
//      FRAMESIZE:   2 byte
//      ANIMMODELS:  1 byte - number of models to animate
//      NUMFRAMES:   1 byte - number of animation frames
//      ANIMFPS:     1 byte - frames per second for animation
//      SOUNDSHEET:  1 byte - don't know that this is used in production release.  Read breakpoint hit, but not write breakpoint, when bare hand uppercut.  Same thing for when breakpoint hit for walk forward animation.  Character sheet supplies sounds for attack.  Character sheet does not provide sound for walk forward footsteps.  Twenty minutes spent on this.  Moving on...
//      SOUNDENTRY:  1 word - don't know that this is used in production release.  Read breakpoint hit, but not write breakpoint, when bare hand uppercut.  Same thing for when breakpoint hit for walk forward animation.  Character sheet supplies sounds for attack.  Character sheet does not provide sound for walk forward footsteps.  Twenty minutes spent on this.  Moving on...
//      HEIGHTSTART: 2 byte - this impacts where character model is placed relative to ground.  When game is hacked to have alternative model data copied over Quentin's model data, Quentin appears to have feet sunk into ground up to Quentin's shins, unless this value is modified to correct Quentin's height when using alternative model data.
//      variable length list of animation frames (number of entries matches NUMFRAMES). Taken from STRUCDEF.INC and not verified other then viewing that this seems to fit calculation of NUMFRAMES * FRAMESIZE:
//          animXmove:  1 word   ; movements in x,y,z directions
//          animYmove:  1 word
//          animZmove:  1 word
//          animYturn:  1 byte   ; rotation about Y axis
//          animFlags:  1 byte   ; flags (see below)
//          animSpin:   1 byte   ; rotation rate (not really used)
//          animHit:    1 byte   ; attack/defense value
//          animRange:  1 word   ; attack range
//          animDirAz:  1 byte   ; 
//          animDirEl:  1 byte
//          animSprAz:  1 byte
//          animSprEl:  1 byte
//          animHigh:   1 word   ; highest point of character
//          animLow:    1 word   ; lowest point of character
//          animAngles: variable length list of 3-bytes-per-model rotation values.  Length is 3*ANIMMODELS.
//      4-6 extra, trailing zero bytes that framesize accounts for, but don't add to start-of-animation + (framesize * numframes).  Think this is insignificant.
//
//
//  Notes:
//      Notes taken while tracking down hacks
//
//      Animation state is constant, but dynamically loaded/unloaded.
//      Animation state is not saved/restored to/from save slots.
//
//      Number of animtions per character sheet
//          Quentin character sheets has pointers to 30 animations.
//          Hood Soldier, Hair Soldier, Gunman1, Gunman2, Claw character sheets have pointers to 28 animations.
//          Quentin character sheet has 2 extra animations for eat and drink food.
//          Ramirez, Mangus, Arak, Kortan, Masked Woman character sheets have pointer to 4 animations: stand, turn left, turn right, walk.
//          Clyde, DundeeB, DundeeD character sheets have pointers to 3 animations: stand, turn left, turn right.
//
//      Manual lists 29 unique moves for Quentin
//          walk forward
//          walk backwards
//          turn left
//          turn right
//          run
//          punch
//          uppercut
//          kick
//          walking jump
//          leg sweep
//          kneeling uppercut
//          dodge left
//          dodge right
//          jump back
//          running jump
//          back-handed-slap
//          punch combo
//          leg and head slash
//          neck swipe
//          overhead chop
//          parry to left
//          parry  overhead
//          sword jab
//          hip swipe
//          two-handed overhead chop
//          parry to right
//          shoot behind
//          shoot forward
//          shoot forward from hip
//          <perhaps there is a 30th idle/standing animation>
//
//      Quentin animations, with bare hands weapon
//          animation00 is response - fly backwards and fall down on back        (0x00034AB0)
//          animation01 is response - crumple down with twist                    (0x00034AB4)
//          animation02 is response - smacked flat 45 degrees to right           (0x00034AB8)
//          animation03 is response - smacked flat 45 degrees to left            (0x00034ABC)
//          animation04 is response - stumble backwards                          (0x00034AC0)
//          animation05 is response - stumble forward to left                    (0x00034AC4)
//          animation06 is response - face smacked to left                       (0x00034AC8)
//          animation07 is response - face smacked to right                      (0x00034ACC)
//          animation08 is response - fall down on back                          (0x00034AD0)
//          animation09 is action - stand                                        (0x00034AD4)
//          animation10 is action - single up (walk)                             (0x00034AD8)
//          animation11 is action - double up (run)                              (0x00034ADC)
//          animation12 is action - down (walk backward)                         (0x00034AE0)
//          animation13 is action - double down + C-button (hop backwards)       (0x00034AE4)
//          animation14 is action - down, when running (stop running)            (0x00034AE8)
//          animation15 is action - turn left                                    (0x00034AEC)
//          animation16 is action - turn right                                   (0x00034AF0)
//          animation17 is action - up + A-button (walking jump forward)         (0x00034AF4)
//          animation18 is action - double up + A-button (running jump forward)  (0x00034AF8)
//          animation19 is action - A-button (jab)                               (0x00034AFC)
//          animation20 is action - B-button (uppercut)                          (0x00034B00)
//          animation21 is action - C-button (kick)                              (0x00034B04)
//          animation22 is action - up + B-button (leg sweep)                    (0x00034B08)
//          animation23 is action - up + C-button (kneeling uppercut)            (0x00034B0C)
//          animation24 is action - double up + B-button (forward slaps)         (0x00034B10)
//          animation25 is action - double up + C-button (forward combo punch)   (0x00034B14)
//          animation26 is action - double down + A-button (dodge left)          (0x00034B18)
//          animation27 is action - double down + B-button (dodge right)         (0x00034B1C)
//          animation28 is action - drink wine                                   (0x00034B20)
//          animation29 is action - eat cheese (and probably bread)              (0x00034B24)
//
//      Weapons provide overriding animations:
//          Quentin is animated differently when holding a weapon, versus how animated when bare handed.
//          Weapons provide 28 overriding animations.
//          Weapons do not override the last 2 (eat/drink) of Quentin's animations.  (Watch the weapon disappear when eating bread...)
//
//
//
//
//
//
//
//
// 
// Active Character Table
//  Description:
//      Ephemeral data for active characters, and usually contains zero values for attributes
//      other then actWorld, actInst, and actFlags, actStatus.
//
//  Layout
//      actWorld:       ds.l    1           ; pointer to world state [0 = null entry]
//      actInst:        ds.l    1           ; pointer to character instance
//      actFlags:       ds.w    1           ; flags
//      actStatus:      ds.w    1           ; status
//      actJoypad:      ds.l    1           ; current joypad action
//      actAction:      ds.l    1           ; previous joypad action
//      actCount:       ds.l    1           ; counter
//      actAICommand:   ds.l    1
//      actAIData1:     ds.l    1
//      actAIData2:     ds.l    1
//      actAIData3:     ds.l    1
//      actAIData4:     ds.l    1
//
//  Notes:
//      8,192 bytes total.
//      In theory, 44 bytes per entry, per above layout taken from LOGICS.INC
//      In practice, 64 bytes dedicated per entry.  The remainder, after what is laid out above, must be null or not described above.
//      Room for 128 entries.
//      Never seen more then 10 entries at a time.
//      Haven't placed any breakpoints on above to verify accuracy.
//      Given developers allocated 64 bytes, and below only describes 44 bytes, seems possible there are other undocumented variables.
//      Believe actCounter is a tick counter, and resets to zero every time joypad direction button is touched.
//          Believe fought solider in village and did not see his aidata bytes ever filled.  saw his joypad action bytes change.
//          AIData values may be very ephemeral.
//
//
//
//
//
//
//
//
//
//
// Character Instance Table
//  Description:
//      Associates world state, character sheet, draw data area, animations, models, etc for active characers as well as hold ephemeral state for those active characters.
//
//  Layout
//      citSheet:       ds.l    1           ; pointer to character sheet [0 = null entry]
//      citDraw:        ds.l    1           ; pointer to draw data area
//      citWorld:       ds.l    1           ; pointer to world state
//      citModelNum:    ds.w    1           ; model count
//      citStance:      ds.b    1           ; stance
//      citTween:       ds.b    1           ; tween
//      citAnimate:     ds.l    1           ; pointer to animation
//      citFrame:       ds.w    1           ; frame
//      citOldFrame:    ds.b    1           ; previous frame
//      citFlags:       ds.b    1           ; flag holder
//      citFacing:      ds.b    1           ; facing
//      citMoving:      ds.b    1           ; moving
//      citHeight:      ds.w    1           ; lock height
//      citTriangle:    ds.w    1           ; triangle
//      citGravity:     ds.w    1           ; gravity
//      citSpeed:       ds.w    1           ; speed
//      citStyle:       ds.w    1           ; style
//      citCollision:   ds.w    1           ; collision
//      citXmoveback:   ds.w    1
//      citYmoveback:   ds.w    1
//      citZmoveback:   ds.w    1
//      citTmoveback:   ds.w    1           ; triangle last in
//      citdummy:       ds.w    1
//      citRecord:      ds.w    0
//
//  Notes:
//      8,256 bytes (64 bytes more then 8K for some reason).
//      Each entry is 48 bytes long.
//      Room for 172 entries, but haven't seen more then 10 entries populated.
//      Not saved/restored to/from save slots.
//      This seems to be the structure to access, when you need to locate draw data area pointer associated to a character.
//      citDraw appears to really be the pointer to the draw data area for the character's first model, and the following draw data areas for that character's remaining models follow contiguously.
//      citModelCount has value of 0x10 for Quentin, as this tracks Quentin's weapon as Quentin's 16th model (even when Quentin only using Bare Hands as weapon).
//
//
//
//
//
//
//
//
//
//
// Draw Data Area
//  Description:
//      Very ephemeral linked list which associates model to instance data area, probably for caching/quick-indexing purposes.
//  Layout
//      ddaNext:    ds.l    1           ; pointer to next draw data area
//      ddaFlags:   ds.l    1           ; flags
//      ddaInst:    ds.l    1           ; pointer to instance data area [0 = null entry]
//      ddaModel:   ds.l    1           ; pointer to model
//
//  Notes:
//      Each entry is 16 bytes long.
//      Room for 256 entries.
//      This could conceivably be a limiting factor, if someone want to hack additional characters onto one screen or possibly even within one set.
//      Not saved/restored to/from save slots.
//      Quentin character has 15 contiguous entries (one entry for each model).
//      Draw data area for weapon character held by quentin, immediataly follows the quentin entries.
//      Had to overwrite values in here a few times, when hacking character sheets to use different model pointers and/or data, as this cache was bypassing the character sheet.
//
//
//
//
//
//
//
// Instance Data Area
//  Description:
//      Very ephemeral data containing data used for 3d engine rotates/etc.
//
//  Layout:
//      idaMatrix1: ds.w    1
//      idaMatrix2: ds.w    1
//      idaMatrix3: ds.w    1
//      idaMatrix4: ds.w    1
//      idaMatrix5: ds.w    1
//      idaMatrix6: ds.w    1
//      idaMatrix7: ds.w    1
//      idaMatrix8: ds.w    1
//      idaMatrix9: ds.w    1
//      idaXpos:    ds.w    1
//      idaYpos:    ds.w    1
//      idaZpos:    ds.w    1
//      idaXface:   ds.w    1
//      idaYface:   ds.w    1
//      idaZface:   ds.w    1
//
//  Notes:
//      16K of data
//      30 bytes per entry, according to above, copied from LOGICS.INC.
//      However Noesis Memory Window makes it appear 64 bytes were allocated per entry, and briefly skimming through memory it appears the last 32 bytes of entries is not used.
//      Room for 256 entries, suggesting 1-1 relationship with Draw Data Areas.
//      Not saved/restored to/from save slot.
//      Never messed with this or viewed it.
//
//
//
//
//
//
//
//
//
//
// Set Sheets
//  Description:
//      Describes location of set data: 1) where copied to in Jaguar memory
//      and 2) where resides on compact disc.
//
//  Layout
//      4 byte - internal pointer to data
//      2 byte - track/data num on cd
//      2 byte - dummy word
//      4 byte - block offset on cd
//
//  Notes:
//      Believe there is 192 long words available for set sheets.
//      Each set sheet is 12 bytes long.
//      Thusly there is space for 64 set sheets to be populated, at any given time.
//      There seems to always be 2 set sheets loaded at beginning of set sheet memory.
//          Even in teleporter room, with teleporter doors activated/unblocked, only saw 2 set sheets.
//      At the title screen there is a third set sheet located in middle of set sheet memory.
//      At the start of game, noted that the first two entries pointed to memory that begins with WAVE data.
//
//
//
//
//
//
//
//
//
//
// Set Data (0x00042300)
//  Description
//      Data for current set.
//      (At least looks like only one set in this memory area at at given time,
//      based on limited analysis of memory.)
//
//      Only scratched the surface of Set/Scene data structures and logic.
//      Basically only delved into enough, to get basic understanding of
//      how scripts work, so could reproduce the same state changes that
//      occur when the flower-placed-in-teleporter-room-vase event logic/script
//      is performed.
//
//  Layout
//      Hinum:          ds.l    1
//      Lonum:          ds.l    1
//      EventOffset:    ds.l    1   ; event data
//      CollOffset:     ds.l    1   ; collision data
//      InitOffset:     ds.l    1   ; initialisation point data
//      SceneOffset:    ds.l    1   ; lookup table of scenes (cd block offsets)
//      ScriptOffset:   ds.l    1   ; offset to set level script
//      12 bytes for set sheet track number and block offset (based on inspecting memory displayed by Noesis Memory Window and inspecting compact disc track with hex editor).
//          4 byte - track number
//          4 byte - block offset
//          2 byte - possibly id number within this set sheet, starting at 0, and incrementing by 1 for every set sheet in this set.
//          2 byte - always zero padding?
//      12 bytes for set sheet track number and block offset (based on inspecting memory displayed by Noesis Memory Window.
//
//  Notes:
//      CDCONTRO.GAS and STRUCDEF.INC provide some additional context in comments.
//      Only a quarter of this memory may be actually utilized.
//
//
//
//
//
//
// Event
//  Description:
//      Seems to detect neccesity of and initiate change/load of scenes, playback of cinepak videos, etc.
//
//  Layout:
//      Review comments/documents near bottom of CDCONTRO.GAS (search for "SET PROCESSING").
//
//  Notes:
//      Didn't have need to dig into this, so referring to CDCONTRO.GAS for documentation.
//      If dig into this later, update this section with finding/explanation of description
//      layout, notes.
//
//
//
//
//
//
//
//
//
// Collision Data
//  Description:
//      Handles detection/collision of collision triangles/vertices which are used to
//      mark event occurrence.
//  Layout:
//      Review comments/documents near bottom of CDCONTRO.GAS (search for "SET PROCESSING").
//      Review STRUCDEF.INC.
//
//  Notes:
//      Didn't have need to dig into this, so referring to CDCONTRO.GAS for documentation.
//      If dig into this later, update this section with finding/explanation of description
//      layout, notes.
//
//
//
//
//
//
//
//
//
//
// Set Level Script
//  Description:
//      Contains script commands.
//
//  Layout:
//      See below "Scripts And Script Commands" description.
//
//  Notes:
//      See below "Scripts And Script Commands" description.
//
//
//
//
//
//
//
//
//
//
// Scripts And Script Commands
//  Description:
//      Provides scripting capability for game.  82 different instructions available.
//
//  Layout:
//      In general it appears instructions can be 4, 8 and possibly 2 bytes long.
//      SCRIPT.GAS::decode may be best description, of general 4 byte instruction layout.
//          8 bits: opcode
//          4 bits: addressing mode
//          4 bits: register
//          16 bits: operand
//      8 byte instructions seem to have a second operand in the last 4 bytes (see triangleheight opcode comments below).
//      Not, neg, and abs instructions may only be 2 bytes long.  Haven't checked into it.
//
//  Notes:
//      The following 82 script commands/opcodes are available for use in scripts
//          2instructions per entry (supposedly. have not verified.)
//              not (cmd=0x01)
//              neg (cmd=0x02)
//              abs (cmd=0x03)
//          simple binary operations
//              add (cmd=0x04)
//              sub (cmd=0x05)
//              cmp (cmd=0x06)
//              move (cmd=0x07)
//              div (cmd=0x08)
//          simple operations using one register with an additional value
//              add (cmd=0x09)
//              sub (cmd=0x0A)
//              move (cmd=0x0B)
//              cmp (cmd=0x0C)
//          addresses supplied in this table in memory ([1-based indice into scripttable memory] is prepended for documentation purposes)
//              [01] exchange (cmd=0x0D)     ; swap two registers
//              [02] branch (cmd=0x0E)       ; move program counter to new location
//              [03] quit (cmd=0x0F)         ; cease processing for this go
//              [04] subroutine (cmd=0x10)   ; call a subroutine
//              [05] endsub (cmd=0x11)       ; return from subroutine
//              [06] spawn (cmd=0x12)        ; spawn a sub-process
//              [07] suicide (cmd=0x13)      ; halt processing and delete state vector
//              [08] kill (cmd=0x14)         ; kill a known process
//              [09] pause (cmd=0x15)        ; pause for a count in 8.8 seconds
//              [10] select (cmd=0x16)       ; choose a character to control
//              [11] swapchar (cmd=0x17)
//              [12] chartoreg (cmd=0x18)
//              [13] regtochar (cmd=0x19)
//              [14] animate (cmd=0x1A)      ; start an animation for the above character
//              [15] anim_direct (cmd=0x1B)  ; direct animation from register
//              [16] setanim (cmd=0x1C)      ; start a set animation for the above character
//              [17] chase (cmd=0x1D)        ; request character chases another
//              [18] attack (cmd=0x1E)       ; request character attacks another
//              [19] face (cmd=0x1F)         ; request character faces another
//              [20] walkto (cmd=0x20)       ; request character goes to a x,z point
//              [21] turnto (cmd=0x21)       ; request character turns to a x,z point
//              [22] attplayer (cmd=0x22)    ; instruct hunter to return to normal attack player mode
//              [23] freeze (cmd=0x23)       ; instruct a character to do nothing (doesn't release control)
//              [24] release (cmd=0x24)      ; release player to joypad control
//              [25] waitstop (cmd=0x25)     ; wait for a character to complete a task
//              [26] waitanim (cmd=0x26)     ; wait for animation to complete
//              [27] cinepak (cmd=0x27)      ; play a cinepak (8 byte instruction)
//              [28] redbook (cmd=0x28)      ; play redbook recording
//              [29] sample (cmd=0x29)       ; play an audio sample
//              [30] camera (cmd=0x2A)       ; switch viewpoint
//              [31] charchange (cmd=0x2B)
//              [32] eventcontrol (cmd=0x2C) ; set/clear event mask bits
//              [33] waitevent (cmd=0x2D)    ; wait until event request is accepted
//              [34] wstread (cmd=0x2E)      ; read world state
//              [35] wstwrite (cmd=0x2F)     ; write world state
//              [36] actread (cmd=0x30)      ; read active char
//              [37] actwrite (cmd=0x31)     ; write active char
//              [38] citread (cmd=0x32)      ; read char table
//              [39] citwrite (cmd=0x33)     ; write char table
//              [40] testowner (cmd=0x34)    ; various game state tests
//              [41] testset (cmd=0x35)
//              [42] testscene (cmd=0x36)
//              [43] testbit (cmd=0x37)      (references 0x00004408 gamestate)
//              [44] setbit (cmd=0x38)       (alternative entry point) (references 0x00004408 gamestate)
//              [45] testprox (cmd=0x39)
//              [46] testdist (cmd=0x3A)
//              [47] quit (cmd=0x3B)         (2nd appearance in table)
//              [48] waitredbook (cmd=0x3C)
//              [49] eventmask (cmd=0x3D)
//              [50] keytest (cmd=0x3E)
//              [51] keymask (cmd=0x3F)
//              [52] slideto (cmd=0x40)
//              [53] restore_item (cmd=0x41)
//              [54] saturate (cmd=0x42)
//              [55] patchit (cmd=0x43)      (references 0x00042318, the 7th long word of setdata which is ScriptOffset)
//              [56] activation (cmd=0x44)   ; controls activate/deactivate characters
//              [57] reset (cmd=0x45)
//              [58] fakescene (cmd=0x46)    (references 0x00042314, 6th long word of setdata, which is SceneOffset)
//              [59] force_pickup (cmd=0x47)
//              [60] use_default (cmd=0x48)
//              [61] poker (cmd=0x49)
//              [62] triangleheight (cmd=0x4A) (references 0x0004230C - 4th long word of setdata, which is CollOffset) - 8 byte instruction that sets sets height indexed by 4th byte to value of byte 8 (or more likely value of byte5 thru byte 8).  In case of flower placement this is called for each teleporter door, to modify height from 0 to 1.
//              [63] no source script method (cmd=0x4B) (world state and 0x00004490 referenced - using related?)
//              [64] no source script method (cmd=0x4C) (0x00004E1A and 0x00004DF4 referenced)
//              [65] no source script method (cmd=0x4D) (0x00004DF4 referenced)
//              [66] setbit (cmd=0x4E)
//              [67] no source script method (cmd=0x4F) (0x00F1B390 referenced)
//              [68] no source script method (cmd=0x50)
//              [69] no source script method (cmd=0x51) (0x00004120 (joypad probably) referenced)
//              [70] no source script method (cmd=0x52) (0x00004E14, private:tableread and private:tablewrite addresses referenced)
//
//      Didn't spend much time trying to understand this thoroughly.  Just enough to figure
//      out what state needed to change to activate/unblock the teleporter doors in teleporter
//      room.
//
//  Examples:
//      There are example .SCT script files included with revision of source
//      code that are likely useful as reference as well, if you want to
//      get grasp of how scripting works.
//
//      Below is start of hand dissasembly of script used to activate
//      teleporter doors.  Once figured out script enough to locate
//      the address/bit	which activates the teleporter doors, stopped
//      disassembling the script.
//          (branch (0x0E)
//          (quit (0x0F))
//          0x000437A4
//          if scene with vase (cmd=testscene=0x36, scene=0x06DD) 
//              if using (flower presumably) (cmd=4B) 
//                  setbit (cmd=0x38)
//                  select (cmd=0x16)
//                  testowner (cmd=0x40)
//                  cinepak (0x27)
//                  camera (0x2A)
//                  quit (0x0F)
//            ????somewhere - not sure where in nesting
//            move (0x0B)
//            wstwrite (0x2F)
//            activation (0x44)
//            triangleheight (0x4A)
//            triangleheight (0x4A)
//            triangleheight (0x4A)
//            triangleheight (0x4A)
//            triangleheight (0x4A)
//            triangleheight (0x4A)
//            suicide (0x13)
//
//
//
//
//
//
//
//
//
//
//
// Image Deobfuscation Block
//  Description:
//      When background images for scenes are loaded into memory, this block is XOR'd against the image loaded in memory.
//      Presumably the images are obfuscated on compact disc, and the XOR is applied to deobfuscate.
//      Presumably the images on compact disc were obfuscated, so that way back in 1995 someone couldn't pull the imagary/
//      Intellectual Property (IP) from disc, and do something else with the IP.
//
//  Layout:
//      8K block of data that is XOR'd against the copy of compact disc stored image data, 8K at a time.
//
//  Notes:
//      Contained on Track2(1-based) of the compact disc.  Found when memory mapping.
//
//      Prior to memory mapping, had dumped all background/scene images from Track4(1-based) to hard drive, 
//      to see if any background/scene images were on disc, but not included in game play (worth a shot, given
//      that models are included on disc, but not included in gameplay).
//      It turns out five of the background/scene images (maps) have all black pixels for the first deobfuscated
//      8K, and that obfuscated 8K from one of those background/scene images is thusly the same  as the deobfuscation
//      data which is stored on Track2(1-based).
//
//
//
//
//
//
//
//
//
//
// CD Queue
//  Description:
//      Queue of requests to load data from compact disc.
//
//  Layout, according to LOGICS.INC:
//      cdqState:       ds.w    1
//      cdqTrack:       ds.w    1
//      cdqBlock:       ds.l    1
//      cdqBuffer:      ds.l    1
//      cdqBufferPtr:   ds.w    1
//      cdqEntry:       ds.w    1
//      cdqSheet:       ds.w    1
//      cdqReserved:    ds.w    1
//      cdqRecord:      ds.w    0
//
//  Notes:
//      Space for 64 20-byte long entries.
//
//      Described in LOGICS.INC, CDLOADER.S, and CDCONTRO.GAS and probably a must read if you need to understand how
//      game is loading data from compact disc.
//
//      Never really looked at this, after it became apparent that character sheets
//      referenced models and animations via one disc reference.
//
//      cdqstate
//      cdqTrack - likely the track number of compact disc to read from.
//      cdqBlock - presumably the block offset, within track of compact disc.
//      cdqBuffer - buff1 or buff2, for scene/set data.
//      cdqBufferPtr - 1=buff1=a, 2=buff2=b, 3=c, for scene/set data.  seems like more of an buff index then ptr.
//      cdqEntry - probably related to compact disc track or where stored within sheet within Jaguar memory.
//      cdqSheet - unsure.  possibly a sheet type.  didn't track as index into character sheets and set sheets.
//      cdqReserved - still looks like zero padding in production release of game.
//      cdqRecord - just a label, used by code to calculate offsets from beginning of structure.
//
//      Blocksize is equal to HL_RAM_BLOCK_SIZE according to source revision's CINEPAK.INC file.
//
//      When specifying track number and block offsets, track number appears to be 2 less then the filename.
//      At least for file references in character sheets.
//      Likely this is because developers treat track number as 0-based, starting with second session.
//
//
//
//
//
//
//
//
//
// svlist, origsave, vlist2, vlist
//  Description:
//      Aggregated, cached vertice information, including origins and saveouts?
//      3DENGINE.GAS references, and 3DENGINE.GAS source revision may be very close to production version.
//
//  Layout:
//      svlist - 1k save list area
//      origsave - 1k origin save area
//      vlist2 - 2k vertex list area
//      vlist - 2k vertex list 2 area
//
//      Unsure of layout within those areas.
//  Notes:
//      Analyzing via Noesis Memory Window, don't really see the connection of saveouts from models
//      to this.
//
//      Comment in source refers to Model Set Reference, but don't see anything obvious indicating
//      which model the data is associated to.  Both Quentin and Hood Soldier active/populated character
//      sheets models used the same value for model->saveouts->2-byte-reference-number-of-model-set.
//
//      Stepping through code would probably help clear this up, but what currently want to accomplish
//      doesn't require understanding these data lists.
//
//      Possibly another limiting factor into how many figther character sheets can be active at any given time.
//
//
//
//
//
//
//
//
//
//
// nvworkspace
//  Description:
//      Presumably scratch place for memory track related routines and possibly to save initial version of state for differencing.
//      Purely speculating...
//
//  Layout:
//
//  Notes:
//
//
//
//
//
//
//
//
//
//
// statevectors
//  Description:
//      2k of state that gets saved/loaded to/from save slots.
//  Layout:
//      Likely read/write at the long word level.
//  Notes:
//      Not a lot of search hits in source code.
//      Suspect the scripts do bulk of modifying.
//      Referenced by NVRAM.S, COLLECT.GAS, and EVENT.GAS.
//
//
//
//
//
//
//
//
//
//
// gamestate
//  Description:
//      1024 pieces of state which is saved/loaded to/from save slots.
//
//  Layout:
//      1024 contiguous bits (128 bytes).
//      Bit# is divided by 32 to find the correct longword, then the remainder is used to access bit within that long word (so you see bit31, bit30,...bit1, bit0, bit 63, bit62, ... bit33, bit32, bit 95, ..., etc, etc).  Probably normal way to set bit greater then your widest register.
//
//  Notes:
//      Referenced by NVRAM.S, COLLECT.GAS, and EVENT.GAS.
//      Using a wine bottle marked a bit.
//      Activating teleporter set a bit.
//
//
//
//
//
//
//
//
//
// Compact Disc Maping
//  Track 1 (1 based)
//      Session1, while other tracks are part of Session2.
//      Should be warning audio track and contain no real game data, per http://www.mdgames.de/jaguarcd/CreatingJaguarCDs.html.
//
//  Track 2 (1 based) - Code And Data Copied At Startup
//      0x00000000-0x00000067 header pattern data
//      0x00000068-0x00000069 length of code and image to copy
//      0x0000006A-0x000006DB code that was copied to 0x00004000 in Jaguar memory, at hard boot (in line with http://www.mdgames.de/jaguarcd/CreatingJaguarCDs.html)
//      0x000006DB-0x0000EADB hard-boot 8-bit intro-screen with red LORE DESIGN rectangle likely near beginning of this track (and associated CLUT data likely above that).  only track that came up with search hit for a 10 byte sequence, ripped from memory.  Object Processor List was pointing to 0x00100000 in Jaguar Memory as location was copied to.
//                            followed by likely CLUT for 8-bit intro-screen image (exactly 256*3 bytes long, which should be the correct size for an 8-bit to 16-bit CLUT).
//      0x0000EADC-0x00011661 zeros
//      0x00012662-0x000126A1 byteswapped "CODE" header pattern data
//      0x000126A2-0x0003FCB1 code (and associated reserved memory) copied to 0x00005000-0x00032610 in Jaguar memory.
//                            see memory map documentation to know what all was copied (most/all of remaining code, font bitmaps, models for collectibles, initial copy of world state, initial copy of character sheets, multilingual text, 8k image deobfuscation data block, etc).
//                            99.9% confident this is correct, given the begin and end 32 bytes of this data and locations in Jaguar memory match.
//      0x0003FCB2-0x00044661 zeros
//      0x00044662-0x000446C1 byteswapped "ATARI APPROVED DATA TAILER <other 4 ATRI bytes repeating>.  (apparently trailer pattern data.)
//      0x000446C2-0x001013FF zeros
//
//  Track 3 (1 based) - Sets
//      search for following (copied from lil below 0x00042300 at start of game) 00 00 03 3F 00 00 84 0E 00 00 B4 3E 00 00 99 0F, resulted in 1 hit in Track3(1-based).  but further down in the datablock then would expect, unless other set data above it copied elsewhere.
//      0x002B891A-0x002BABB5 start-of-game set - size matches and first and last 4 phrases match
//      CDLINK.INC defines likely no-longer-valid block-offsets for 45 sets.
//      Presumably there are still at least 45 sets in the production release of game.
//      Concievable 3 sets were added since the revision of source code (e.g. teleporter room/cave could have been added post source code revision).
//      Or there could be data for 3 things that are not set related.
//
//      rapid scrolling through file (not stopping) counted 49 sections
//      48 entries located with following search string "TADATADATADATADATADATADATADATADATADATADATADATADATADATADATADAT"
//      1 trailer entry located with the following search string "TARA IPARPVODED TA AATLIREA RT!ITAIRTAIRTAIRTAIRTAIRTAIRTAIRTAIRTAIRTAIRTAIRTAIRTAIRTAIRTAIRTAIR"
//
//      CDLINK seemed to have 44 to 45 different scene-base-name, and right around 600 scenes defined
//          so perhaps 3 sets and 70 scenes were added.  teleporter room likely has quite a few scenes, for different angles, and then for alternative versions of any scene depicting teleporter and/or vase
//
//  Track 4 (1 based) - Background Images For Scenes
//      Search for: IPTC (PICT byteswapped)
//      Some possibility there is other data on this track as well, but didn't look.
//      Looks like the tailer at end has 2k-ish of data.  unsure what for.
//      Search for what thought was first 14 bytes of that 2k-ish data, byteswapped in Noesis, yielded no search hit.
//              HxD: D3 DB 53 45 D4 91 D3 D5 50 4F C6 93 50 49
//              byteswapped: DB D3 45 53 91 D4 D5 D3 4F 50 93 C6 49 50
//              Formatted for Noesis Memory Window search: 0xDB 0xD3 0x45 0x53 0x91 0xD4 0xD5 0xD3 0x4F 0x50 0x93 0xC6 0x49 0x50
//
//  Track 5 (1 based) - Models, Animations, Logics, Character Specific WAVEs
//      0x00020280 (131,712dec - 640 over 128k) between each of them
//          start 2.66 of these in
//      0x38 block offsets between each of them
//
//      0x000559B6-0x0006490C - block 0x0000 - Quentin models, animations, and logics            - 61,262bytes
//      0x00075C36-0x0007BC5F - block 0x0038 - Quentin WAVEs
//      0x00095EB6-0x000A1009 - block 0x0070 - Macleod Sword animations
//      0x000B6136-0x000BFAC1 - block 0x00A8 - Macleod Sword WAVEs
//      0x000D63B6-0x000E1273 - block 0x00E0 - chicken animations
//      0x000F6636-0x0010107F - block 0x0118 - chicken WAVEs
//      0x001168B6-0x001217BD - block 0x0150 - gun animations
//      0x00136B36-0x0013FD53 - block 0x0188 - gun WAVEs
//      0x00156DB6-0x00163B54 - block 0x01C0 - hood models, animations, and logics
//      0x00177036-0x00184A8D - block 0x01F8 - hood WAVEs
//      0x001972B6-0x001A37C4 - block 0x0230 - hair models, animations, and logics
//      0x001B7536-0x001C4F8D - block 0x0268 - hair WAVEs
//      0x001D77B6-0x001E57E4 - block 0x02A0 - gunman1 models, animations, and logics
//      0x001F7A36-0x00207219 - block 0x02D8 - gunman1 WAVEs
//      0x00217CB6-0x00225CE4 - block 0x0310 - gunman2 models, animations, and logics
//      0x00237F36-0x00247719 - block 0x0348 - gunman2 WAVEs
//      0x002581B6-0x002643BC - block 0x0380 - claw models, animations, and logics
//      0x00278436-0x0027FC97 - block 0x03B8 - claw WAVEs
//      0x002986B6-0x0029BADC - block 0x03F0 - arak models, animations, and logics
//      0x002B8936-0x002BCA54 - block 0x0428 - kortan models, animations, and logics
//      0x002D8BB6-0x002DB7DC - block 0x0460 - mangus models, animations, and logics
//      0x002F8E36-0x002FC01C - block 0x0498 - ramirez models, animations, and logics
//      0x003190B6-0x0031BB5C - block 0x04D0 - masked woman models, animations, and logics
//      0x00339336-0x0033B7F0 - block 0x0508 - clyde models, animations, and logics
//      0x003595B6-0x0035B9B0 - block 0x0540 - dundee b models, animations, and logics
//      0x00379836-0x0037BF40 - block 0x0578 - dundee d models, animations, and logics
//      0x00399AB6-0x0039AAA0 - block 0x05B0 - turbine models, animations, and logics
//      0x003B9D36-0x003BB5ED - block 0x05E8 - tank models 
//      0x003D9FB6-0x003DF0D0 - block 0x0620 - turret model, animation, logics, and WAVEs (sounds may be repeated via some trickery - this is data stretch where the AWEV bytes are 3 in a row, then one trailer near end)
//      0x003FA236-0x003FB2D1 - block 0x0658 - hand? model
//      0x0041A4B6-0x0041A8A1 - block 0x0690 - red water (control panel) model
//      0x0043A736-0x0043AB21 - block 0x06C8 - green water (control panel) model
//      0x0045A9B6-0x0045ADA1 - block 0x0700 - blue water (control panel) model
//
//  Track 6 (1 based) - WAVEs
//      Holds 38 WAVEs  search for (AWEV (WAVE byteswapped)
//
//      Set Sheets seem to reference these via block offsets and specifying track number 4 (0-based starting on session 2, so Track 6).
//      Seem to reference for footsteps and set background muzak.
//
//      Set Sheets defined by Set Data.  Set Data defined in Track 3 (1-based).
//      Scanned Track3 (1-based) and noted the following block offsets, being referred to within Track 6 (1-based).
//          Lkely footstep effects:
//              0x00, 0x38, 0x70, 0xA8
//              DATA.INC references BO_SOUND_FOOT ERTH, SAND, METL
//          Likely background muzak:
//              0xE0, 0x118, 0x150, 0x188, 0x1C0, 0x1F8, 0x268, 0x2A0, 0x2D8, 0x310,
//              0x348, 0x380, 0x3B8, 0x3F0, 0x428, 0x460, 0x498, 0x4D0, 0x508, 0x540,
//              0x578, 0x5B0, 0x5E8, 0x620, 0x658, 0x690, 0x6C8, 0x700, 0x738, 0x770,
//              0x7A8, 0x7E0, 0x818
//          37 of 38 WAVEs, within Track 6 (1-based) referenced
//          Track 6 (1-based), block offset 0x0230 was not referenced
//              // Place the below in on_emu_frame and uncomment, to hear this sound.  Unsure if ever referenced in the game.  Certainly does not appear to be directly referenced by any of the sets.
//              // Sounds bit like chimey bells.
//              // bigpemu_jag_write32(0x0004232C, 0x00000230);  // Test any block offset from track 6 (1-based) with this).
//
//  Track7 (1 based) - Cinepak Block Offsets:
//      Location Approach - Approach taken to locate block offset of video.
//          1 - Searched through Track6(1-based) set data blocks for cinepak script instructions.
//              Jaguar memory: 0x27000000 <long word block offset>
//              Byte swapped, 0x00270000, for hex editor search of just portion that won't vary between instructions: 0x00270000
//              Got about 40 hits, with 36 of those fitting constraints of cinepak script instruction.
//              Copied the long word block offset that followed the search string, and byteswapped.
//          2 - Copied the block offset, from Jaguar memory, that follows text ptrs for the few collectibles that play videos when examined:
//                  0x000121D4
//                  0x00012574
//                  0x0000EE94
//              Additional information in memory map.
//          3 - Breakpoint on MovieOffset 0x00004F34 and then cineblock (0x000044BC) contains block offset when videos played.
//              Two of block offsets, located with this approach, belong to videos played before gameplay starts.
//              The third block offset, located with this approach, is during middle of gameplay.  Unsure
//              why the third block offset wasn't located in any of the Set Data scripts, or if just missed it.
//              Believe WSB.INC/MATTSWSB.INC at least specifies bit in gamestate that decides whether this needs played again or not.
//
//              Could probably get address of each header contained in Track7(1-based) and divide
//              by 2352 to get (rough) block offset values used by game.  CINEPAK.INC equates
//              block_size=#2352.  (Block size calculations were used on Track5(1-based) to locate
//              Track5(1-based) addresses.)
//          4 - Copied block offset, from Jaguar memory, for global main/sample script (0x0003FEB8 (see memory map))
//
//      Verification:
//          Individually:
//              Attach Noesis debugger
//              Fill Memory at locket-block-offset (0x000121D4) with block offset to test
//              Select to examine locket in Options Menu.
//              Should play back video referenced by block offset.
//
//          Overall:
//              Ensure number of identified block offsets matches the number of IFML (FILM byteswapped) headers in Track7(1-based) file.
//
//      Notes:
//          There are 36 videos located on Track7(1-based), per 36 IFML (FILM byteswapped) headers contained on Track7(1-based).
//
//          Located block offsets for all 36 videos contained on Track7(1-based).
//
//          scriptblock 0x000045C0 is updated with the block offset of currently playing
//          cinepak video, when cinepak video playback is initiated by script.
//
//          MovieOffset value does not match blockoffset value.
//
//          Don't believe game-specific CD Queue is written to for Cinepak videos.  Which
//          makes sense, as it appears Cinepak video playback is stubbed out to a
//          game-agnostic/generic library binary.
//
//          Don't believe there is any point in mapping block offsets to specific bytes in
//          Track7(1-based), as each block-offset/video should begin in Track7(1-based)
//          with a IFML header, and the block offsets should be in the same order as the
//          IFML headers.
//
//          All significant videos in Track7(1-based) are used in the production release of game.
//          Possibly less significant 2 videos of screaming-fall and 1 video of unable-to-open-locked-gate
//          video are not used in production release of game.  Couldn't make gate video happen
//          in limited tests, and didn't bother trying to get the the screaming-fall videos
//          to be displayed, but it seems odd that there are two variations (one with person running
//          to player, and another without person running to player).  All other videos were
//          verified as included in production release of game by scrubbing through youtube
//          playthrough: https://www.youtube.com/watch?v=J3QRSANSVqU
//
//          Block Offset (Track6(1-based)   Location Approach   Description
//          ===================================================================
//          0x00000000                      1                   use crowbar to pry open sewer vent/hatch
//          0x0000021F                      1                   use wooden stick to break lock on closed gate in dundee village
//          0x000005F1                      3                   "buzz" - feel prescence of another immortal
//          0x00001070                      1                   use security key at end game
//          0x00001630                      1                   use crowbar to break lock on trunk in dundee village
//          0x000019DD                      1                   credits canyon fly through
//          0x00003043                      1,4                 game over - die
//          0x0000418E                      1                   ramirez opens vault door via control panel, ramirez sets bomb, mangus fights kortan
//          0x000051A1                      1                   use lever to lower bridge
//          0x00005AC9                      3                   between start game and start screen in dundee village.  starts with salamander
//          0x00009606                      1                   win game - bomb explodes, mangus's head taken, clyde/ramirez/quentin
//          0x0000B1B4                      1                   use door key to open door
//          0x0000B5D5                      1                   use uniform
//          0x0000BCE3                      1                   screaming fall, and someone running towards you (in production release?)
//          0x0000BF9F                      1                   screaming fall, and noone running towards you (in production release?)
//          0x0000C242                      2                   locket
//          0x0000DC35                      1                   place flower in vase, in teleporter room
//          0x0000DDB7                      1                   gate in canyon raised, after initiate from control panel
//          0x0000E549                      1                   rameriz computer generated introduction along with cartoon provided background info, and display of macleod sword with ramirez talking, and then back to computer generated ramirez and quentin
//          0x00010381                      1                   use stopwatch to jump through turbine blades
//          0x00010628                      1                   kick trunk in dundee village
//          0x00010AC2                      1                   can't open closed/locked gate in dundee village
//          0x00010CD0                      1                   after defeat claw: mangus, eyepatch guy, ramirez, and quentin discuss entering city
//          0x000118C9                      1                   map transition scene between pass-tank and canyon
//          0x0001207A                      1                   map transition scene after going through raised canyon gate
//          0x000123F9                      1                   knocking on guarded door, while not wearing uniform
//          0x000125FC                      1                   placing board over canyon bridge gap
//          0x00012863                      1                   control panel opening vault door
//          0x00012C6D                      2                   waterwheel
//          0x00015137                      1                   flythrough video, seconds after mangus/eyepatch-guy/ramirez/discussion, to go past sewer waterfall and turbine
//          0x0001594A                      1                   video explaining turbine blades opposite each other every 4 seconds
//          0x00015E2D                      1                   use maintenace key 1 to open a sewer hatch
//          0x000165CC                      1                   use maintenace key 2 to open a sewer hatch
//          0x00016DB9                      2                   macleod sword
//          0x00017084                      3                   intro with lightning and then into blue circle haze cartoon clip
//          0x000189FF                      1                   use heavy key to open canyon door
//
//  Track 8 (1 based) - Unused Animations
//      Contains animations that were not used in production version of game.
//      While interesting that an entire track was included, but not used, in
//      production version of game, the animations don't seem very useful.
//
//      Most of these animations appear to have been designed to be used in place
//      of the video clip exchanges between characters like Quentin and Ramirez,
//      and possibly other characters like Mangus, Kortan, Clyde, etc.  These
//      animations depict talking and hand gestures.
//
//      There are some other animations that depict player getting knocked down,
//      and then getting back up.
//      One animation appears to cause model to give up and prepare to lose head.
//      There is an animation which causes model to look left, right, left, right
//      like trying to sneak past something.
//      Another animation causes model to jump dive/fly horizontal, and then
//      somersault on landing (perhaps dive through blades of turbine?).
//
//      As far as ever being useful:
//          The jump/dive/somersault animation could be useful at any time.
//          The forward thrust from shoulder height could be useful when equipped with sword/chicken.
//          If someone made a tournament fighter mode, then the following
//          could be useful:
//              Prepare to lose head - end of match
//              Fall down and get back up - end of round, but not end of match.
//          The talking/hand-jestures animations are not very useful without
//          backing dialog audio, and are probably OBE given the cinepak videos.
//
//      Scripting the game to use the jump-dive animation and/or shoulder high
//      thrust would seem like the only useful mod at this time.  Scripting the
//      game to use the jump-dive and/or shoulder high thrust animation(s)
//      would require overwriting an animation every time weapon is equipped, if
//      you want to overwrite weapon animation, since weapon equipping causes
//      weapon animations to be be dynamically loaded/replaced.
//      However, Quentin animations seem to stay loaded, even when equipping/changing
//      weapons, so overwriting Quentin animations may be the way to go for proof-of-concept
//      purposes.  Playback of Quentin animations is only accomplished when Quentin
//      is bare hands (has no weapon equipped) however.
//
//      0x0005598E-0x00060ED9=46,411 bytes - 7 animations - block-offset=0x0000
//          +0x0E80=0x0005680E - step forward, hand gesture
//          +0x12A0=0x00057AAE - angle backwards, hand gestures
//          +0x22A0=0x00059D4E - talking, hand gestures, bowing hand gesture
//          +0x19D8=0x0005B726 - talking, hand gesture
//          +0x0958=0x0005C07E - step backwards
//          +0x26C0=0x0005E73E - talking
//          +0x2780=0x00060EBE - talking, hand gestures, maybe something in hand and tossed away
//      0x00075C0E-0x0007EA93=36,485 bytes - 6 animations - block-offset=0x0038
//          +0x1DB8=0x000779C6 - from kneeling to upright, with hand gestures
//          +0x0A60=0x00078426 - forward thrust from shoulder height
//          +0x0640=0x00078A66 - hit, stumble backwards, until laid flat
//          +0x0D78=0x000797DE - get up, after fallen on butt 180 degrees
//          +0x2950=0x0007C12E - get up after having been laid flat, hand gesture, face back like possibly held by neck or possibly just chest out
//          +0x2950=0x0007EA7E - talking and hand guestures
//      0x00095E8F-0x0009A2D7=17,480 bytes - 3 animations - block-offset=0x0070
//          +0x0D78=0x00096C07 - unique one - get up after 180 degree falling on butt, and scamper 45 degrees up into air (recover from fall on steps?)
//          +0x0D78=0x0009797F - similar to direct above, without the air shenanigans
//          +0x2950=0x0009A2CF - get up after having been laid flat, hand gesture, face back like possibly held by neck or possibly just chest out 
//      0x000B610E-0x000B97DB=14,029 bytes - 2 animations - block-offset=0x00A8
//          +0x0D78=0x000B6E86 - get up after falling on butt
//          +0x2950=0x000B97D6 - get up after having been laid flat, hand gesture, face back like possibly held by neck or possibly just chest out
//      0x000D638E-0x000D9995=13,831 bytes - 2 animations - block-offset=0x00EO
//          +0x1CF0=0x000D807E - step forward while looking left, right, left right
//          +0x1910=0x000D998E - multiple steps forward, hand gesture, probably while talking
//      0x000F660E-0x000FD577=28,521 bytes - 4 animations - block-offset=0x0118
//          +0x16C0=0x000F7CCE - talking, hand gesture
//          +0x1F00=0x000F9BCE - talking, steps forward, hand gesture
//          +0x1D30=0x000FB8FE - talking, step forward, hand gesture
//          +0x1C70=0x000FD56E - talking, step backward, hand gesture
//      0x0011688E-0x001191DF=10,577 bytes - 1 animations - block-offset=0x0150
//          +0x2950=0x001191DE - talking, hand gesture, turns 180 degrees
//      0x00136B0E-0x00146BAF=65,697 bytes - 8 animations - block-offset=0x0188
//          +0x1748=0x00138256 - talking, step forward, hand gesture
//          +0x1F00=0x0013A156 - talking, step angle, hand gesture
//          +0x1F00=0x0013C056 - talking, rotate 45 degrees, hand gesture
//          +0x1F00=0x0013dF56 - talking, step forward, hand gestures
//          +0x2428=0x0014037E - talking, step forward, hand gestures
//          +0x2428=0x001427A6 - talking, hand gestures
//          +0x1808=0x00143FAE - talking, step forward, hand gestures
//          +0x2BE8=0x00146B96 - hand gestures, look upwards
//      0x00156D8E-0x001639B3=52,261 bytes - 6 animations - block-offset=0x01C0
//          +0x1430=0x001581BE - possibly examining collectable (map?)
//          +0x1F00=0x0015A0BE - talking and hand gestures
//          +0x33A0=0x0015D45E - standing and possibly talking,
//          +0x33A0=0x001607FE - talking and hand gesture
//          +0x2950=0x0016314E - talking and hand gesture (very similar to direct above)
//          +0x0850=0x0016399E - jump-diving to summersault
//      0x0017700E-0x0017BFF2=20,452 bytes - 3 animations - block-offset=0x01F8 - there was a surrender for beheading in one of these or above
//          +0x1AA0=0x00178AAE - looks like collapsing to knees and preparing to lose head.  possibly just kneeling/bowing to someone
//          +0x2950=0x0017B3FE - sidestep talking hand-gesture
//          +0x0BF0=0x0017BFEE - possibly sword plunged into, and then sword removed to overhead.
//
//  Track 9 - Encryption Data
//      Should be encryption-data, per http://www.mdgames.de/jaguarcd/CreatingJaguarCDs.html
//
//
//
//
//
//
//
//
//
//
// Character sheet exports.
//
// Below are exports of the different fighter character sheets, when assigned to Quentin.  These
// exports were useful when implementing the script functionality to replace Quentin model data
// with other fighters model data.
//
// Could probably delete this, but keeping around in case need to do any debugging of the model
// swapping functionality.
// 
// Ramirez character sheet when assigned to Quentin world state entry
//     0x00034FAC  0x00 0x03 0x50 0x24 0x00 0x01 0x04 0x0F 0x13 0x04 0x17 0x05 0x1C 0x01 0x09 0x00
//
//     0x00034FBC  0x00 0x05 0x8F 0x88 0x00 0x05 0x96 0x88 0x00 0x05 0x98 0x48 0x00 0x05 0x9A 0x08
//     0x00034FCC  0x00 0x05 0x9D 0x88 0x00 0x05 0x9F 0x48 0x00 0x05 0xA1 0x08 0x00 0x05 0xA6 0x48
//     0x00034FDC  0x00 0x05 0xB2 0x88 0x00 0x05 0xB4 0x48 0x00 0x05 0xB6 0x08 0x00 0x05 0xB7 0xC8
//     0x00034FEC  0x00 0x05 0xB9 0x88 0x00 0x05 0xBB 0x48 0x00 0x05 0xBD 0x08
//                                                                             0x00 0x05 0xBE 0xC8
//     0x00034FFC  0x00 0x05 0xC0 0x88 0x00 0x05 0xC7 0x88 0x00 0x05 0xCC 0xC8
//                                                                             0x00 0x05 0xD2 0x08
//     0x0003500C  0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00
//     0x0003501C  0x00 0x00 0x00 0x03 0x00 0x00 0x04 0x98
//
//
// Mangus character sheet when assigned to Quentin world state entry
//     0x0003509C  0x00 0x03 0x51 0x14 0x00 0x01 0x04 0x0F 0x13 0x04 0x17 0x05 0x1C 0x01 0x09 0x00
//
//     0x000350AC  0x00 0x05 0x8F 0x88 0x00 0x05 0x96 0x88 0x00 0x05 0x98 0x48 0x00 0x05 0x9A 0x08
//     0x000350BC  0x00 0x05 0x9D 0x88 0x00 0x05 0x9F 0x48 0x00 0x05 0xA1 0x08 0x00 0x05 0xA4 0x88
//     0x000350CC  0x00 0x05 0xAF 0x08 0x00 0x05 0xB0 0xC8 0x00 0x05 0xB2 0x88 0x00 0x05 0xB4 0x48
//     0x000350DC  0x00 0x05 0xB6 0x08 0x00 0x05 0xB7 0xC8 0x00 0x05 0xB9 0x88
//                                                                             0x00 0x05 0xBB 0x48
//     0x000350EC  0x00 0x05 0xBD 0x08 0x00 0x05 0xC2 0x48 0x00 0x05 0xC7 0x88
//                                                                             0x00 0x05 0xCC 0xC8
//     0x000350FC  0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00
//
//     0x0003510C  0x00 0x00 0x00 0x03
//                                     0x00 0x00 0x04 0x60
//
//
// Masked Woman character sheet when assigned to Quentin
//     0x00035024  0x00 0x03 0x50 0x9C 0x00 0x01 0x04 0x0F 0x13 0x04 0x17 0x05 0x1C 0x01 0x09 0x00
//
//     0x00035034  0x00 0x05 0x8F 0x88 0x00 0x05 0x96 0x88 0x00 0x05 0x98 0x48 0x00 0x05 0x9A 0x08
//     0x00035044  0x00 0x05 0x9D 0x88 0x00 0x05 0x9F 0x48 0x00 0x05 0xA1 0x08 0x00 0x05 0xA4 0x88
//     0x00035054  0x00 0x05 0xAD 0x48 0x00 0x05 0xAF 0x08 0x00 0x05 0xB0 0xC8 0x00 0x05 0xB2 0x88
//     0x00035064  0x00 0x05 0xB4 0x48 0x00 0x05 0xB6 0x08 0x00 0x05 0xB7 0xC8
//                                                                             0x00 0x05 0xB9 0x88
//     0x00035074  0x00 0x05 0xBB 0x48 0x00 0x05 0xC0 0x88 0x00 0x05 0xC5 0xC8 0x00 0x05 0xCB 0x08
//
//     0x00035084  0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00
//     0x00035094  0x00 0x00 0x00 0x03
//                                     0x00 0x00 0x04 0xD0
//
//
// Arak character sheet when assigned to Quentin world state entry
//     0x00035114  0x00 0x03 0x51 0x8C 0x00 0x01 0x04 0x0F 0x13 0x04 0x17 0x05 0x1C 0x01 0x09 0x00
//
//     0x00035124  0x00 0x05 0x8F 0x88 0x00 0x05 0x96 0x88 0x00 0x05 0x98 0x48 0x00 0x05 0x9A 0x08
//     0x00035134  0x00 0x05 0x9D 0x88 0x00 0x05 0x9F 0x48 0x00 0x05 0xA1 0x08 0x00 0x05 0xA6 0x48
//     0x00035144  0x00 0x05 0xB0 0xC8 0x00 0x05 0xB2 0x88 0x00 0x05 0xB4 0x48 0x00 0x05 0xB6 0x08
//     0x00035154  0x00 0x05 0xB7 0xC8 0x00 0x05 0xB9 0x88 0x00 0x05 0xBB 0x48
//                                                                             0x00 0x05 0xBD 0x08
//     0x00035164  0x00 0x05 0xC4 0x08 0x00 0x05 0xCB 0x08 0x00 0x05 0xD0 0x48
//                                                                             0x00 0x05 0xD5 0x88
//     0x00035174  0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00
//
//     0x00035184  0x00 0x00 0x00 0x03 0x00 0x00 0x03 0xF0
//
//
// Kortan character sheet when assigned to Quentin world state entry
//     0x0003518C  0x00 0x03 0x52 0x04 0x00 0x01 0x04 0x0F 0x13 0x04 0x17 0x05 0x1C 0x01 0x07 0x00
//
//     0x0003519C  0x00 0x05 0x8F 0x88 0x00 0x05 0x98 0x48 0x00 0x05 0x9A 0x08 0x00 0x05 0x9B 0xC8
//     0x000351AC  0x00 0x05 0x9F 0x48 0x00 0x05 0xA1 0x08 0x00 0x05 0xA2 0xC8 0x00 0x05 0xA8 0x08
//     0x000351BC  0x00 0x05 0xB2 0x88 0x00 0x05 0xB4 0x48 0x00 0x05 0xB6 0x08 0x00 0x05 0xB7 0xC8
//     0x000351CC  0x00 0x05 0xB9 0x88 0x00 0x05 0xBB 0x48 0x00 0x05 0xBD 0x08
//                                                                             0x00 0x05 0xBE 0xC8
//     0x000351DC  0x00 0x05 0xC5 0xC8 0x00 0x05 0xCC 0xC8 0x00 0x05 0xD9 0x08
//                                                                             0x00 0x05 0xE1 0xC8
//     0x000351EC  0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00
//
//     0x000351FC  0x00 0x00 0x00 0x03 0x00 0x00 0x04 0x28 
//
//
// Clyde character sheet when assigned to Quentin world state entry
//     0x00035204  0x00 0x03 0x52 0x78 0x00 0x01 0x04 0x0F 0x13 0x03 0x16 0x05 0x1B 0x01 0x07 0x00
//
//     0x00035214  0x00 0x05 0x8F 0x88 0x00 0x05 0x96 0x88 0x00 0x05 0x98 0x48 0x00 0x05 0x9A 0x08
//     0x00035224  0x00 0x05 0x9D 0x88 0x00 0x05 0x9F 0x48 0x00 0x05 0xA1 0x08 0x00 0x05 0xA4 0x88
//     0x00035234  0x00 0x05 0xAD 0x48 0x00 0x05 0xAF 0x08 0x00 0x05 0xB0 0xC8 0x00 0x05 0xB2 0x88
//     0x00035244  0x00 0x05 0xB4 0x48 0x00 0x05 0xB6 0x08 0x00 0x05 0xB7 0xC8
//                                                                             0x00 0x05 0xB9 0x88
//     0x00035254  0x00 0x05 0xBB 0x48 0x00 0x05 0xC0 0x88
//                                                         0x00 0x05 0xC5 0xC8 0x00 0x00 0x00 0x00
//     0x00035264  0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00
//                                                                             0x00 0x00 0x00 0x03
//     0x00035274  0x00 0x00 0x05 0x08
//
//
// DundeeB character sheet when assigned to Quentin world state entry
//     0x00035278  0x00 0x03 0x52 0xEC 0x00 0x01 0x04 0x0F 0x13 0x03 0x16 0x05 0x1B 0x01 0x07 0x00
//
//     0x00035288  0x00 0x05 0x8F 0x88 0x00 0x05 0x94 0xC8 0x00 0x05 0x96 0x88 0x00 0x05 0x98 0x48
//     0x00035298  0x00 0x05 0x9B 0xC8 0x00 0x05 0x9D 0x88 0x00 0x05 0x9F 0x48 0x00 0x05 0xA2 0xC8
//     0x000352A8  0x00 0x05 0xAD 0x48 0x00 0x05 0xAF 0x08 0x00 0x05 0xB0 0xC8 0x00 0x05 0xB2 0x88
//     0x000352B8  0x00 0x05 0xB4 0x48 0x00 0x05 0xB6 0x08 0x00 0x05 0xB7 0xC8
//                                                                             0x00 0x05 0xB9 0x88
//     0x000352C8  0x00 0x05 0xBB 0x48 0x00 0x05 0xC0 0x88
//                                                         0x00 0x05 0xC5 0xC8 0x00 0x00 0x00 0x00
//     0x000352D8  0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00
//                                                                             0x00 0x00 0x00 0x03
//     0x000352E8  0x00 0x00 0x05 0x40 
//
//
// DundeeDClyde character sheet when assigned to Quentin world state entry
//     0x000352EC  0x00 0x03 0x53 0x60 0x00 0x01 0x04 0x0F 0x13 0x03 0x16 0x05 0x1B 0x01 0x07 0x00
//
//     0x000352FC  0x00 0x05 0x8F 0x88 0x00 0x05 0x96 0x88 0x00 0x05 0x98 0x48 0x00 0x05 0x9A 0x08
//     0x0003530C  0x00 0x05 0x9D 0x88 0x00 0x05 0x9F 0x48 0x00 0x05 0xA1 0x08 0x00 0x05 0xA4 0x88
//     0x0003531C  0x00 0x05 0xAF 0x08 0x00 0x05 0xB0 0xC8 0x00 0x05 0xB2 0x88 0x00 0x05 0xB4 0x48
//     0x0003532C  0x00 0x05 0xB6 0x08 0x00 0x05 0xB7 0xC8 0x00 0x05 0xB9 0x88
//                                                                             0x00 0x05 0xBB 0x48
//     0x0003533C  0x00 0x05 0xBD 0x08 0x00 0x05 0xC2 0x48
//                                                         0x00 0x05 0xC7 0x88 0x00 0x00 0x00 0x00
//     0x0003534C  0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00
//                                                                             0x00 0x00 0x00 0x03
//     0x0003535C  0x00 0x00 0x05 0x78
//
//
// Hood Soldier character sheet when assigned to Quentin world state entry
//     0x00034B4C  0x00 0x03 0x4C 0x2C 0x00 0x01 0x04 0x0F 0x13 0x1C 0x2F 0x05 0x34 0x02 0x06 0x00
//
//     0x00034B5C  0x00 0x05 0x8F 0x88 0x00 0x05 0x94 0xC8 0x00 0x05 0x96 0x88 0x00 0x05 0x98 0x48
//     0x00034B6C  0x00 0x05 0x9B 0xC8 0x00 0x05 0x9D 0x88 0x00 0x05 0x9F 0x48 0x00 0x05 0xA4 0x88
//     0x00034B7C  0x00 0x05 0xAD 0x48 0x00 0x05 0xAF 0x08 0x00 0x05 0xB0 0xC8 0x00 0x05 0xB2 0x88
//     0x00034B8C  0x00 0x05 0xB4 0x48 0x00 0x05 0xB6 0x08 0x00 0x05 0xB7 0xC8
//                                                                             0x00 0x05 0xB9 0x88
//     0x00034B9C  0x00 0x05 0xC0 0x88 0x00 0x05 0xC7 0x88 0x00 0x05 0xD0 0x48 0x00 0x05 0xD9 0x08
//     0x00034BAC  0x00 0x05 0xE0 0x08 0x00 0x05 0xEA 0x88 0x00 0x05 0xF1 0x88 0x00 0x05 0xF8 0x88
//     0x00034BBC  0x00 0x05 0xFA 0x48 0x00 0x06 0x01 0x48 0x00 0x06 0x08 0x48 0x00 0x06 0x0D 0x88
//     0x00034BCC  0x00 0x06 0x16 0x48 0x00 0x06 0x18 0x08 0x00 0x06 0x1F 0x08 0x00 0x06 0x2B 0x48
//     0x00034BDC  0x00 0x06 0x34 0x08 0x00 0x06 0x3B 0x08 0x00 0x06 0x42 0x08 0x00 0x06 0x4A 0xC8
//     0x00034BEC  0x00 0x06 0x51 0xC8 0x00 0x06 0x57 0x08 0x00 0x06 0x5E 0x08 0x00 0x06 0x65 0x08
//     0x00034BFC  0x00 0x06 0x6D 0xC8 0x00 0x06 0x74 0xC8 0x00 0x06 0x7B 0xC8
//                                                                             0x00 0x06 0x7F 0x48
//     0x00034C0C  0x00 0x06 0x84 0x88 0x00 0x06 0xE4 0xC8 0x00 0x07 0x02 0x88 0x00 0x07 0x23 0xC8
//
//     0x00034C1C  0x00 0x00 0x00 0x03 0x00 0x00 0x01 0xC0 0x00 0x2C 0x00 0x03 0x00 0x00 0x01 0xF8
//
//
// Hair Soldier character sheet when assigned to Quentin world state entry
//     0x00034C2C  0x00 0x03 0x4D 0x0C 0x00 0x01 0x04 0x0F 0x13 0x1C 0x2F 0x05 0x34 0x02 0x06 0x00
//
//     0x00034C3C  0x00 0x05 0x8F 0x88 0x00 0x05 0x94 0xC8 0x00 0x05 0x96 0x88 0x00 0x05 0x98 0x48
//     0x00034C4C  0x00 0x05 0x9B 0xC8 0x00 0x05 0x9D 0x88 0x00 0x05 0x9F 0x48 0x00 0x05 0xA4 0x88
//     0x00034C5C  0x00 0x05 0xAF 0x08 0x00 0x05 0xB0 0xC8 0x00 0x05 0xB2 0x88 0x00 0x05 0xB4 0x48
//     0x00034C6C  0x00 0x05 0xB6 0x08 0x00 0x05 0xB7 0xC8 0x00 0x05 0xB9 0x88
//                                                                             0x00 0x05 0xBB 0x48
//     0x00034C7C  0x00 0x05 0xC2 0x48 0x00 0x05 0xC9 0x48 0x00 0x05 0xD2 0x08 0x00 0x05 0xDA 0xC8
//     0x00034C8C  0x00 0x05 0xE1 0xC8 0x00 0x05 0xEC 0x48 0x00 0x05 0xF3 0x48 0x00 0x05 0xFA 0x48
//     0x00034C9C  0x00 0x05 0xFC 0x08 0x00 0x06 0x03 0x08 0x00 0x06 0x0A 0x08 0x00 0x06 0x0F 0x48
//     0x00034CAC  0x00 0x06 0x18 0x08 0x00 0x06 0x19 0xC8 0x00 0x06 0x20 0xC8 0x00 0x06 0x26 0x08
//     0x00034CBC  0x00 0x06 0x2B 0x48 0x00 0x06 0x32 0x48 0x00 0x06 0x39 0x48 0x00 0x06 0x42 0x08
//     0x00034CCC  0x00 0x06 0x49 0x08 0x00 0x06 0x4E 0x48 0x00 0x06 0x55 0x48 0x00 0x06 0x5C 0x48
//     0x00034CDC  0x00 0x06 0x65 0x08 0x00 0x06 0x6C 0x08 0x00 0x06 0x73 0x08 
//                                                                             0x00 0x06 0x78 0x48
//     0x00034CEC  0x00 0x06 0x7D 0x88 0x00 0x06 0xDD 0xC8 0x00 0x06 0xFB 0x88 0x00 0x07 0x1C 0xC8
//
//     0x00034CFC  0x00 0x00 0x00 0x03 0x00 0x00 0x02 0x30 0x00 0x2C 0x00 0x03 0x00 0x00 0x02 0x68
//
//
// GunMan1 character sheet when assigned to Quentin world state entry
//     0x00034D0C  0x00 0x03 0x4D 0xEC 0x00 0x01 0x04 0x0F 0x13 0x1C 0x2F 0x05 0x34 0x02 0x0B 0x00
//
//     0x00034D1C  0x00 0x05 0x8F 0x88 0x00 0x05 0x94 0xC8 0x00 0x05 0x96 0x88 0x00 0x05 0x98 0x48
//     0x00034D2C  0x00 0x05 0x9B 0xC8 0x00 0x05 0x9D 0x88 0x00 0x05 0x9F 0x48 0x00 0x05 0xAB 0x88
//     0x00034D3C  0x00 0x05 0xB4 0x48 0x00 0x05 0xB6 0x08 0x00 0x05 0xB7 0xC8 0x00 0x05 0xB9 0x88
//     0x00034D4C  0x00 0x05 0xBB 0x48 0x00 0x05 0xBD 0x08 0x00 0x05 0xBE 0xC8
//                                                                             0x00 0x05 0xC0 0x88
//     0x00034D5C  0x00 0x05 0xC7 0x88 0x00 0x05 0xCE 0x88 0x00 0x05 0xD7 0x48 0x00 0x05 0xE0 0x08
//     0x00034D6C  0x00 0x05 0xE7 0x08 0x00 0x05 0xF1 0x88 0x00 0x05 0xF8 0x88 0x00 0x05 0xFF 0x88
//     0x00034D7C  0x00 0x06 0x01 0x48 0x00 0x06 0x03 0x08 0x00 0x06 0x0A 0x08 0x00 0x06 0x0F 0x48
//     0x00034D8C  0x00 0x06 0x18 0x08 0x00 0x06 0x19 0xC8 0x00 0x06 0x20 0xC8 0x00 0x06 0x2D 0x08
//     0x00034D9C  0x00 0x06 0x35 0xC8 0x00 0x06 0x3C 0xC8 0x00 0x06 0x43 0xC8 0x00 0x06 0x4E 0x48
//     0x00034DAC  0x00 0x06 0x55 0x48 0x00 0x06 0x5E 0x08 0x00 0x06 0x68 0x88 0x00 0x06 0x73 0x08
//     0x00034DBC  0x00 0x06 0x7D 0x88 0x00 0x06 0x88 0x08 0x00 0x06 0x8F 0x08
//                                                                             0x00 0x06 0x94 0x48
//     0x00034DCC  0x00 0x06 0x99 0x88 0x00 0x06 0xF9 0xC8 0x00 0x07 0x17 0x88 0x00 0x07 0x56 0x88
//
//     0x00034DDC  0x00 0x00 0x00 0x03 0x00 0x00 0x02 0xA0 0x00 0x2C 0x00 0x03 0x00 0x00 0x02 0xD8
//
//
// GunMan2 character sheet when assigned to Quentin world state entry
//     0x00034DEC  0x00 0x03 0x4E 0xCC 0x00 0x01 0x04 0x0F 0x13 0x1C 0x2F 0x05 0x34 0x02 0x0F 0x00
//
//     0x00034DFC  0x00 0x05 0x8F 0x88 0x00 0x05 0x94 0xC8 0x00 0x05 0x96 0x88 0x00 0x05 0x98 0x48
//     0x00034E0C  0x00 0x05 0x9B 0xC8 0x00 0x05 0x9D 0x88 0x00 0x05 0x9F 0x48 0x00 0x05 0xAB 0x88
//     0x00034E1C  0x00 0x05 0xB4 0x48 0x00 0x05 0xB6 0x08 0x00 0x05 0xB7 0xC8 0x00 0x05 0xB9 0x88
//     0x00034E2C  0x00 0x05 0xBB 0x48 0x00 0x05 0xBD 0x08 0x00 0x05 0xBE 0xC8 
//                                                                             0x00 0x05 0xC0 0x88
//     0x00034E3C  0x00 0x05 0xC7 0x88 0x00 0x05 0xCE 0x88 0x00 0x05 0xD7 0x48 0x00 0x05 0xE0 0x08
//     0x00034E4C  0x00 0x05 0xE7 0x08 0x00 0x05 0xF1 0x88 0x00 0x05 0xF8 0x88 0x00 0x05 0xFF 0x88
//     0x00034E5C  0x00 0x06 0x01 0x48 0x00 0x06 0x03 0x08 0x00 0x06 0x0A 0x08 0x00 0x06 0x0F 0x48
//     0x00034E6C  0x00 0x06 0x18 0x08 0x00 0x06 0x19 0xC8 0x00 0x06 0x20 0xC8 0x00 0x06 0x2D 0x08
//     0x00034E7C  0x00 0x06 0x35 0xC8 0x00 0x06 0x3C 0xC8 0x00 0x06 0x43 0xC8 0x00 0x06 0x4E 0x48
//     0x00034E8C  0x00 0x06 0x55 0x48 0x00 0x06 0x5E 0x08 0x00 0x06 0x68 0x88 0x00 0x06 0x73 0x08
//     0x00034E9C  0x00 0x06 0x7D 0x88 0x00 0x06 0x88 0x08 0x00 0x06 0x8F 0x08 
//                                                                             0x00 0x06 0x94 0x48
//     0x00034EAC  0x00 0x06 0x99 0x88 0x00 0x06 0xF9 0xC8 0x00 0x07 0x17 0x88 0x00 0x07 0x56 0x88
//
//     0x00034EBC  0x00 0x00 0x00 0x03 0x00 0x00 0x03 0x10 0x00 0x2C 0x00 0x03 0x00 0x00 0x03 0x48
//
//
//  Claw character sheet when assigned to Quentin world state entry
//     0x00034ECC  0x00 0x03 0x4F 0xAC 0x00 0x01 0x04 0x0F 0x13 0x1C 0x2F 0x05 0x34 0x02 0x06 0x00
//
//     0x00034EDC  0x00 0x05 0x8F 0x88 0x00 0x05 0x94 0xC8 0x00 0x05 0x96 0x88 0x00 0x05 0x98 0x48
//     0x00034EEC  0x00 0x05 0x9A 0x08 0x00 0x05 0x9B 0xC8 0x00 0x05 0x9D 0x88 0x00 0x05 0xA1 0x08
//     0x00034EFC  0x00 0x05 0xAB 0x88 0x00 0x05 0xAD 0x48 0x00 0x05 0xAF 0x08 0x00 0x05 0xB0 0xC8
//     0x00034F0C  0x00 0x05 0xB2 0x88 0x00 0x05 0xB4 0x48 0x00 0x05 0xB6 0x08 
//                                                                             0x00 0x05 0xB7 0xC8
//     0x00034F1C  0x00 0x05 0xBE 0xC8 0x00 0x05 0xC5 0xC8 0x00 0x05 0xCE 0x88 0x00 0x05 0xD7 0x48
//     0x00034F2C  0x00 0x05 0xE1 0xC8 0x00 0x05 0xEC 0x48 0x00 0x05 0xF3 0x48 0x00 0x05 0xFA 0x48
//     0x00034F3C  0x00 0x05 0xFC 0x08 0x00 0x06 0x03 0x08 0x00 0x06 0x08 0x48 0x00 0x06 0x0D 0x88
//     0x00034F4C  0x00 0x06 0x14 0x88 0x00 0x06 0x16 0x48 0x00 0x06 0x19 0xC8 0x00 0x06 0x1F 0x08
//     0x00034F5C  0x00 0x06 0x24 0x48 0x00 0x06 0x29 0x88 0x00 0x06 0x2E 0xC8 0x00 0x06 0x35 0xC8
//     0x00034F6C  0x00 0x06 0x3B 0x08 0x00 0x06 0x43 0xC8 0x00 0x06 0x4A 0xC8 0x00 0x06 0x51 0xC8
//     0x00034F7C  0x00 0x06 0x5C 0x48 0x00 0x06 0x66 0xC8 0x00 0x06 0x6D 0xC8
//                                                                             0x00 0x06 0x78 0x48
//     0x00034F8C  0x00 0x06 0x7D 0x88 0x00 0x06 0xA7 0x88 0x00 0x06 0xCC 0x48 0x00 0x06 0xF2 0xC8
//
//     0x00034F9C  0x00 0x00 0x00 0x03 0x00 0x00 0x03 0x80 0x00 0x2C 0x00 0x03 0x00 0x00 0x03 0xB8
//
//
// Quentin character sheet when assigned to Quentin world state entry
//     0x00034A64  0x00 0x03 0x4B 0x4C 0x00 0x01 0x04 0x0F 0x13 0x1E 0x31 0x05 0x36 0x02 0x00 0x00
//
//     0x00034A74  0x00 0x05 0x8F 0x88 0x00 0x05 0x96 0x88 0x00 0x05 0x98 0x48 0x00 0x05 0x9A 0x08
//     0x00034A84  0x00 0x05 0x9D 0x88 0x00 0x05 0x9F 0x48 0x00 0x05 0xA1 0x08 0x00 0x05 0xA4 0x88
//     0x00034A94  0x00 0x05 0xAF 0x08 0x00 0x05 0xB0 0xC8 0x00 0x05 0xB2 0x88 0x00 0x05 0xB4 0x48
//     0x00034AA4  0x00 0x05 0xB6 0x08 0x00 0x05 0xB7 0xC8 0x00 0x05 0xB9 0x88
//                                                                             0x00 0x05 0xBB 0x48
//     0x00034AB4  0x00 0x05 0xC2 0x48 0x00 0x05 0xC9 0x48 0x00 0x05 0xD2 0x08 0x00 0x05 0xDA 0xC8
//     0x00034AC4  0x00 0x05 0xE1 0xC8 0x00 0x05 0xEC 0x48 0x00 0x05 0xF3 0x48 0x00 0x05 0xFA 0x48
//     0x00034AD4  0x00 0x05 0xFC 0x08 0x00 0x05 0xFD 0xC8 0x00 0x06 0x03 0x08 0x00 0x06 0x08 0x48
//     0x00034AE4  0x00 0x06 0x0D 0x88 0x00 0x06 0x12 0xC8 0x00 0x06 0x18 0x08 0x00 0x06 0x1D 0x48
//     0x00034AF4  0x00 0x06 0x22 0x88 0x00 0x06 0x2B 0x48 0x00 0x06 0x34 0x08 0x00 0x06 0x3C 0xC8
//     0x00034B04  0x00 0x06 0x45 0x88 0x00 0x06 0x4E 0x48 0x00 0x06 0x58 0xC8 0x00 0x06 0x61 0x88
//     0x00034B14  0x00 0x06 0x68 0x88 0x00 0x06 0x6F 0x88 0x00 0x06 0x76 0x88 0x00 0x06 0x7D 0x88
//     0x00034B24  0x00 0x06 0x90 0xC8 
//                                     0x00 0x06 0xA5 0xC8 0x00 0x06 0xAB 0x08 0x00 0x06 0xD8 0x88
//     0x00034B34  0x00 0x06 0xEA 0x08 0x00 0x06 0xF6 0x48
//                                                         0x00 0x00 0x00 0x03 0x00 0x00 0x00 0x00
//     0x00034B44  0x00 0x2E 0x00 0x03 0x00 0x00 0x00 0x38
//
//
//
//
//
//
//
//
//
//
// Teleporter Room
//     Each scene which displays inactive teleporter doors has a corresponding alternative scene where the teleporter doors are active.
//     Each scene which displays the empty vase has a corresponding alternative scene where the flower is placed in the vase.
//     Teleporter doors are blocked, unblocked by collision triangle data's height set to zero or one respectively.
//     During production version gameplay, the scene script is responsible for detecting the placement of flower, playback of flower placement cinepak video, activation and unblocking of the teleporter doors, along with some other associated housekeeping activites (setting the 0x37th bit of gamestate, setting activation flag of flower world state entry, etc).
//     Teleporter door activation/unblocking is saved in save slots.
//     If the game detects the 0x37th bit of gamestate set, teleporter doors are activated/unblocked and appropriate alternative scenes are displayed.
//
//
//
//
//
//
//
//
//
//
// Assigning different model data to Quentin
//     Doesn't appear copying assigning tank/turret model data over Qunetin model data would work.
//         Tank/Turret only has 1 to 2 models, and believe the animations expect 15 models.
//     Doesn't appear assigning tank/turret is very useful either.
//         Tank/Turret only have 0 to 1 animations, and neither of those animations allow moving of tank/turret.
//         However, you can shoot the first enemy with the turret.
//             Unless someone adds a bunch of world state characters at the first set/scene as a shooting gallery, this is a dead end.  Probably pretty pointless to create a shooting gallery.
//     Possibly can copy turret model data over the gun model data.
//
//
//
//
//
//
//
//
//
//
// Cinepak
//     Believe this bypasses CD Queue.  They stubbed out cinepak playback to a library, so would make sense if custom CD Queue is bypassed.
//     Did not see CD Queue updated when went from playing cinepak videos of locket, sword, waterwheel in succession, via the Options menu.
//     Instead scriptblock seem to contain the block offset for the cinepak video that was being played.
//     Saw cinepak videos initiated in 3 different ways (there could be other ways as well):
//         Via script commands/instructions provided by Set Data's Script section, and blockoffset is set in scriptblock .
//             script command/instruction format is: 0x27000000 <4 byte block offset>
//         Via script commands/instructions provided by main/sample script (0x0003FEB8 (see memory map)).
//         Via Options Menu using block offsets provided after model name text pointers for sword, waterwheel, locket.
//             These block offsets were set in scriptblock as well.
