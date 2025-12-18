//__BIGPEMU_SCRIPT_MODULE__
//__BIGPEMU_META_DESC__     "Immediate club, infinite bombs, infinite use of club upgrades, infinite penguin-attack lives and unlock levels,  in Attack Of The Mutant Penguins."
//__BIGPEMU_META_AUTHOR__   "jguff"

#include "bigpcrt.h"

// Found using BigPEmu System->Load Software screen and highlighting ROM file.
#define MPENGUINS_HASH	0xC1E15384328CDB32ULL

// Found using Pro Action Replay like script.
#define MPENGUINS_GREM_HELD_ADDR                0x00038403
#define MPENGUINS_PEN_ATTACK_NUM_LIVES_ADDR     0x00038489
#define MPENGUINS_PEN_ATTACK_NUM_LIVES_VALUE    6

// Found by Noesis Memory Window inspection, after having located MUTANT_PENGUINS_GREM_HELD memory address.
#define MPENGUINS_BOMB_ADDR                     0x00038521
#define MPENGUINS_CLUB_ADDR                     0x00038522
#define MPENGUINS_SUPER_CLUB_ADDR               0x0003852E
#define MPENGUINS_BOMB_VALUE                    6
#define MPENGUINS_WEAPON_UPGRADE1_NUM_USES_ADDR 0x00038530
#define MPENGUINS_WEAPON_UPGRADE2_NUM_USES_ADDR 0x00038531
#define MPENGUINS_WEAPON_UPGRADE1_NUM_USES_VAL  5
#define MPENGUINS_WEAPON_UPGRADE2_NUM_USES_VAL  5
#define MPENGUINS_WEAPON_TIME_CNTDWN_ADDR       0x00038408
#define MPENGUINS_WEAPON_TIME_CNTDWN_VALUE      0x0BF
#define MPENGUINS_NUM_ORBS_COLLECTED_ADDR       0x00038405

// The following addresses seemed to represent which unlocked level is
// selected, when game is at Level Select Screen and user cycles through
// unlocked levels:
//     0x0002E921
//     0x0002E923
// The following address seemed to control how many levels are unlocked,
// when game is at Level Select Screen:
//     0x0002EA73
// Traced 0x0002EA73 to below address, which seems to be the global
// representation of max unlocked level.
// This is 0-based value.
#define MPENGUINS_MAX_UNLOCKED_LEVEL_ADDR   0x00052183

// Youtube playthrough shows 20 levels.  Appeared youtube playthrough was
// on Normal difficulty.  There is some chance other difficulties introduce
// additional levels.
#define MPENGUINS_MAX_NUM_LEVELS    20

#define MPENGUINS_NEG1_2BYTE        0xFFFF
#define MPENGUINS_NEG1_1BYTE        0xFF
#define MPENGUINS_DEFAULT_NUM_ORBS  9
#define MPENGUINS_MIN_NUM_ORBS      0
#define MPENGUINS_MAX_NUM_ORBS      10

#define INVALID                     -1

// Boilerplate.
static int32_t sOnLoadEvent = INVALID;
static int32_t sOnEmuFrame = INVALID;
static int32_t sMcFlags = 0;
static const int32_t skMcFlag_Loaded = (1 << 0);

// Setting handles.
static int sBombsSettingHandle = INVALID;
static int sStartWithClubSettingHandle = INVALID;
static int sMinimumNumberOrbsSettingHandle = 0;
static int sInfiniteUseClubUpgradeSettingHandle = INVALID;
static int sShipsPenguinAttackSettingHandle = INVALID;
static int sUnlockLevelsSettingHandle = INVALID;

static uint32_t on_sw_loaded(const int eventHandle, void* pEventData)
{
    const uint64_t hash = bigpemu_get_loaded_fnv1a64();

    sMcFlags = 0;

    if (MPENGUINS_HASH == hash)
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
        int bombsSettingValue = INVALID;
        int startWithClubSettingValue = INVALID;
        int minimumNumberOrbsSettingValue = 0;
        int infiniteUseClubUpgradeSettingValue = INVALID;
        int shipsPenguinAttackSettingValue = INVALID;
        int unlockLevelsSettingValue = INVALID;
        bigpemu_get_setting_value(&bombsSettingValue, sBombsSettingHandle);
        bigpemu_get_setting_value(&startWithClubSettingValue, sStartWithClubSettingHandle);
        bigpemu_get_setting_value(&minimumNumberOrbsSettingValue, sMinimumNumberOrbsSettingHandle);
        bigpemu_get_setting_value(&infiniteUseClubUpgradeSettingValue, sInfiniteUseClubUpgradeSettingHandle);
        bigpemu_get_setting_value(&shipsPenguinAttackSettingValue, sShipsPenguinAttackSettingHandle);
        bigpemu_get_setting_value(&unlockLevelsSettingValue, sUnlockLevelsSettingHandle);

        // There are two characters to choose from, which i'll refer
        // to as Pan Man and Bat Man.  Pan Man uses a pan club.  Bat
        // Man uses a bat club.
        //
        // During normal gameplay:
        //    Characters start out unarmed: no club, no bombs, no bonus attacks.
        //
        //    Club:
        //       The characters obtain their club by collecting letters
        //       that spell 'BAT' or 'PAN'.  Once character obtains a club,
        //       the character keeps the club for remainder of the level.
        //       Letters are available in treasure chests.
        //    Club Upgrade 1:
        //       Bat Man's Upgrade1 is large bat and gets to use 10 times,
        //       before reverting to normal club.
        //       Pan Man's Upgrade1 is a flashing pan and gets to use 5 times,
        //       before reverting to normal club.
        //    Club Upgrade 2:
        //       Bat Man's Upgrade2 is short range, projectile fire and its
        //       use is limited to around 7 seconds.
        //       Pan Man's Upgrade2 is long range, boomerang like projectile
        //   (flashing pan) and is limited to 5 uses.
        //   Dynamite/bombs:
        //       Found in gremlin activated things.  Stage one was found in
        //          propeller like thing.  Maybe treasure chests too.
        //       Short range attack.
        //       May only be able to hold one bomb.
        //       First stage treasure chest only gives character one bomb.
        //    Glue:
        //       Referenced in manual.
        //       Haven't noticed that yet, but only opened chests in Level1.
        //    Samurai Bonus:
        //       Found in treasure chests.
        //       Gives Pan Man a Samurai Whirlwind attack, in lieu of club.
        //       Gives Bat Man a fire attack that appears to be same as
        //       Bat Man's Club Upgrade2, in lieu of club.
        //       For either character lasts around 12 seconds (manual says 10).
        //    Orbs:
        //       Manual says:"If you collect five powers orbs in a row, without
        //          using your weapon<club>, you will receive a weapon<club> upgrade. If you
        //          collect ten power orbs it will get a double upgrade. Once a
        //          weapon<club> has been upgraded, the powerup usually lasts for a set
        //          number of uses."
        //       Generated by hitting penguins with normal club, 5 per penguin.
        //       Not generated by hitting penguins with super club.
        //       Not generated using Samurai Whirlwind or Bombs
        //       Collect 5 orbs without using club, and you get Club Upgrade1
        //       Collect 10 orbs without using club, and you get Club Upgrade2.
        //
        //  The in-between-stage bonus-rounds provide 3 lives for Penguin Attack.

        // 0x00038403 - The address of num-gremlins-held.
        //
        //      This is the first address found, by using Pro Action Replay like script
        //      Located a number of other addresses (below) nearby num-gremlins-held,
        //          via Noesis Memory Window.
        //          Thought about overwriting this value, but suspect performance issues
        //          or game crash could result.

        if (bombsSettingValue)
        {
            // Overwrite every emulation frame, for infinite bombs. 6 is the value noted
            // in Noesis Memory Window, while script was not applied.
            bigpemu_jag_write8(MPENGUINS_BOMB_ADDR, MPENGUINS_BOMB_VALUE);
        }

        if (startWithClubSettingValue)
        {
            // Set to use normal club
            //
            // Saw 2 groups of 3 bytes, with sets 8 bytes offset from each other.
            // The sets seemed to be in sync, but didn't pay very close
            // attention.  Maybe one set per player/character.
            //
            // One byte, per letter, of the following types of club: 'PAN' or 'BAT'.
            //
            // All bytes initialized to 0x00, when game starts and script not
            // applied.  As player collects letter of club-type, one of the bytes
            // is set to 0xFF.  After 3 letters are collected, the entire group
            // of bytes is set to 0xFF, the remaining byte is set to 0xFF,
            // presumably indicating that club is now available.
            bigpemu_jag_write16(MPENGUINS_CLUB_ADDR, MPENGUINS_NEG1_2BYTE);
            bigpemu_jag_write16(MPENGUINS_CLUB_ADDR+2, MPENGUINS_NEG1_2BYTE);
            bigpemu_jag_write8(0x00038529, MPENGUINS_NEG1_1BYTE);
            bigpemu_jag_write8(0x0003852A, MPENGUINS_NEG1_1BYTE);
            bigpemu_jag_write8(0x0003852B, MPENGUINS_NEG1_1BYTE);
        }

        // This may have only been needed when started player off with club Upgrade1 or club Upgrade2
        // Don't delete, but keep commented out, in case need to revisit this.
        //
        //// Believe this is neccessary for club upgrades.  Not going to 
        //// spend more time analyzing memory and doing test runs to confirm.
        //bigpemu_jag_write8(0x0003852F, 0x09);

        if (minimumNumberOrbsSettingValue > 0)
        {
            // Controls the num-orbs-collected and num-orbs-collected power bar.
            // Setting to 1-orb < max, max, 1-orb over max, every frame, doesn't help,
            // as the code needs to see a change in orbs/value to award the club
            // upgrade.
            //
            // Solution is to conditionally set num orbs to 1 orb below the number
            // of orbs needed for club upgrade.  When character picks up the final
            // orb, the conditional code will not overwrite.  And character will have
            // club upgrade after collecting only 1 orb.
            //
            // (Would have liked to have just found the bits that turned the upgrades
            // on, but was having diffuclty finding those bits.  So changed
            // approach to setting number of orbs.  Collect one orb, and get the
            // club upgrade.)
            //
            // If: you set infiniteClubUpgradeUses=1
            //     and have orbsValue set below number of orbs required for Club Upgrade1
            //     and during gameplay smack penguin(s) with starting club
            //     and pick up enough orbs to upgrade to Club Upgrade1,
            //     but not enough orbs to upgrade to Club Upgrade2,
            //     before the orbs disappear,
            //   character may be stuck with Club Upgrade1, the remainder of the level.
            //   Treasure chest bonus and treasure chest letters _may_ get you to
            //   Club Upgrade2 in some cases... For instance Bat Man can pick up
            //   Samurai Bonus and i think will get upgraded to Club Upgrade2.
            const uint8_t oneBelowUpgrade1 = (5*4)-(1*4);
            const uint8_t oneBelowUpgrade2 = (10*4)-(1*4);
            const uint8_t readByte = bigpemu_jag_read8(MPENGUINS_NUM_ORBS_COLLECTED_ADDR);
            if (readByte <= minimumNumberOrbsSettingValue)
            {
                // Address that controls the number-of-orbs-collected and/or
                // num-orbs-collected power bar.
                //
                // 0 orbs collected equals 0.
                // 10 orbs collected equals 40.
                // 10 is the maximum number of orbs.
                // Each orb adds 4.
                // 5 orbs (value of 20) gets character to Club Upgrade1
                // 10 orbs (value of 40) gets character to Club Upgrade2
                const uint8_t orbsValue = 4 * minimumNumberOrbsSettingValue;
                bigpemu_jag_write8(MPENGUINS_NUM_ORBS_COLLECTED_ADDR, orbsValue);
            }
        }

        if (infiniteUseClubUpgradeSettingValue)
        {
            // This is the address for time-limited-attacks countdown.
            // Value probably just needs to be greater then 0x0001.
            // Overwrite, so no timeout.
            // Noticed a starting value of 0x00BF for Bat Man's Club Upgrade2.
            // Noticed a starting value of 0x01<xx> for Pan Man's Samurai Bonus.
            // Value decrements with time normally, until reaching 0, marking
            // end of access.
            //
            // Setting this seems to be making Pan Man Samurai Bonus, Bat Man
            // Club Upgrade2, and Bat Man Samurai Bonus cause the character to
            // nonstop flash/flicker (flashing like when they are about to lose
            // time limited weapon when script is not applied).
            // Going to call it good enough, for now.
            // 
            // Caveats:
            //    This prevents further club upgrade via orbs, until next level.
            bigpemu_jag_write8(MPENGUINS_WEAPON_TIME_CNTDWN_ADDR, MPENGUINS_WEAPON_TIME_CNTDWN_VALUE);

            // During normal gameplay some club upgrades have limited number
            // of uses.
            // Overwrite so have unlimited number of uses.
            // Caveat: This locks character into that club upgrade for
            // remainder of level.
            // (Changing 'Infinite Use Club' setting in BigPEmu Settings GUI
            // may allow user to change midway through level, but that's pretty
            // clunky.)
            bigpemu_jag_write8(MPENGUINS_WEAPON_UPGRADE1_NUM_USES_ADDR, MPENGUINS_WEAPON_UPGRADE1_NUM_USES_VAL);
            bigpemu_jag_write8(MPENGUINS_WEAPON_UPGRADE2_NUM_USES_ADDR, MPENGUINS_WEAPON_UPGRADE2_NUM_USES_VAL);
        }

        if (shipsPenguinAttackSettingValue)
        {
            // Infinite number of lives for in-between-stage Penguin Attack bonus-round.
            bigpemu_jag_write8(MPENGUINS_PEN_ATTACK_NUM_LIVES_ADDR, MPENGUINS_PEN_ATTACK_NUM_LIVES_VALUE);
        }

        if (unlockLevelsSettingValue > 1) // Level 1 is always unlocked.
        {
            // Unlock if requested value is greater then what EEPROM already unlocks.
            const uint8_t epromLevelsUnlocked = bigpemu_jag_read8(MPENGUINS_MAX_UNLOCKED_LEVEL_ADDR);
            if (unlockLevelsSettingValue > epromLevelsUnlocked)
            {
                // Need to write 0-based value, while user looked at 1-based value, so subtract 1.
                bigpemu_jag_write8(MPENGUINS_MAX_UNLOCKED_LEVEL_ADDR, ((uint8_t) unlockLevelsSettingValue - 1));
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
    const int catHandle = bigpemu_register_setting_category(pMod, "Attack Penguins");
    sBombsSettingHandle = bigpemu_register_setting_bool(catHandle, "Infinite Bombs", 1);
    sStartWithClubSettingHandle = bigpemu_register_setting_bool(catHandle, "Start With Club", 1);
    sMinimumNumberOrbsSettingHandle = bigpemu_register_setting_int_full(catHandle, "Minimum Number Orbs", MPENGUINS_DEFAULT_NUM_ORBS, MPENGUINS_MIN_NUM_ORBS, MPENGUINS_MAX_NUM_ORBS, 1);
    sInfiniteUseClubUpgradeSettingHandle = bigpemu_register_setting_bool(catHandle, "Infinite Use Of Club", 1);
    sShipsPenguinAttackSettingHandle = bigpemu_register_setting_bool(catHandle, "Infinite Ships For Penguin Attack", 1);
    sUnlockLevelsSettingHandle = bigpemu_register_setting_int_full(catHandle, "Unlock Levels", INVALID, INVALID, MPENGUINS_MAX_NUM_LEVELS, 1);

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

    sBombsSettingHandle = INVALID;
    sStartWithClubSettingHandle = INVALID;
    sMinimumNumberOrbsSettingHandle = INVALID;
    sInfiniteUseClubUpgradeSettingHandle = INVALID;
    sShipsPenguinAttackSettingHandle = INVALID;
    sUnlockLevelsSettingHandle = INVALID;
}