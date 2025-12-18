//__BIGPEMU_SCRIPT_MODULE__
//__BIGPEMU_META_DESC__     "Infinite health and ammo, all weapons, motion tracker and all security cards, in Aliens Vs Predator."
//__BIGPEMU_META_AUTHOR__   "jguff"

#include "bigpcrt.h"

// Found using BigPEmu System->Load Software screen and highlighting ROM file.
#define AVP2_HASH           0x1BCB3230DB41AA47ULL

// Found using Pro Action Replay like descending search script.
//
// Writing to AVP2_HEALTH_ADDR causes game to copy value to 0x00040A7E.
// Writing to 0x00040A7E does not cause game to copy the value to AVP2_HEALTH_ADDR.
// Quick search of code reveals 0x00030A6A is player_energy, and 0x00040A7E is old_energy.
// Current and old values show up in pairs for other attributes as well.
// Doesn't appear the old value ever really needs written to.
#define AVP2_HEALTH_ADDR    0x00030A6A

// Source code shows max health is 2 bytes to right of AVP2_HEALTH_ADDR.
#define AVP2_MAX_HEALTH_ADDR    AVP2_HEALTH_ADDR + 2

// Found shotgun ammo using Pro Action Replay like descending search script.
// Source code confirmed structure and offsets to other guns' ammo.
#define AVP2_SHOTGUN_ADDR   0x0002F214
#define AVP2_PULSE_ADDR     0x0002F224
#define AVP2_FLAME_ADDR     0x0002F234
#define AVP2_SMART_ADDR     0x0002F244

// Bitmask representing which weapons are available.
//
// Found counting bytes from ammo addresses, with help of source code.
// Samrter person would have would have referred to Whitehouse's AVP
// scripts.  Only thought to look at Whitehouse's scripts afterwards.
#define AVP2_CUR_WEP_ADDR   0x0002F30A

// Bits representing which weapons are available.
#define AVP2_SHOTGUN_BITMASK_VALUE  2
#define AVP2_PULSE_BITMASK_VALUE    4
#define AVP2_FLAME_BITMASK_VALUE    8
#define AVP2_SMART_BITMASK_VALUE    16

// As defined in game source code.
#define AVP2_MAX_AMMO_VALUE         44

// Number of weapons available to player.
// Also, use this field to determine which creature is playing the game.
#define AVP2_NUM_WEPS_ADDR          0x0002F363
#define AVP2_NUM_HUMAN_WEPS         4
#define AVP2_NUM_ALIEN_WEPS         3
#define AVP2_NUM_PREDATOR_WEPS      6

// Max health for player, based on which creature the player selected.
#define AVP2_MAX_HUMAN_HEALTH       0x03E8
#define AVP2_MAX_ALIEN_HEALTH       0x0465
#define AVP2_MAX_PREDATOR_HEALTH    0x0BB8

// Taken from Whitehouse avp_common.c script.
#define AVP2_ACCESS_LEVEL_ADDR      0x00030AD0
#define AVP2_CUR_ACCESS_ADDR        0x00030AD2

#define AVP2_ACCESS_LEVEL_VALUE     10

// Address and value to display motion tracker.
//
// 01. Found this by tracking HUD_MSG.S:cm10, cm11, cm2 to HUD_MSG.S:clm_mtrack
//       cm10: 'MOTION'
//       cm11: 'TRACKER'
//       cm2: 'COLLECTED'
// 02. Then searched RAM for cm10, cm11, and cm2 to get their addresses
//       cm10: 0x000252B9
//       cm11: 0x000252C0
//       cm2:  0x00025279
//
// 03. Noted the following:
//       HUD_MSG.S
//          clm_mtrack::    dc.l    $FFFFFF00|MSG_NEW
//                          dc.l    cm10,cm11,cm2
//     Where MSG_NEW = 0xFF
// 04. Then searched through RAM for 0xFFFFFFFF 0x000252B9 0x000252C0 0x00025279
// 05. Got no hits.
// 06. Then realized the above addresses are off by one
// 07. So searched through RAM for 0xFFFFFFFF 0x000252B8 0x000252BF 0x00025278,
//     resulting in only one hit: 0x000251EE
// 08. Then searched through RAM for 0x000251EE, getting only one hit: 0x0000FB00
//     Believe this is part of 10 byte 68K instruction that moves 4 byte immediate value to 4 byte memory address.
//      PLAYER.S
//          move.w  #-1,show_mt
//          move.l  #clm_mtrack,new_message
// All looks good, as show_mt (show motion tracker) is what is being searched for.
// 09. Placed read breakpoint onto 0x0000FB00 (instruction breakpoint may have been better choice, but whatever).
// 10. Viewed youtube video showing how player can get to motion tracker within less then 10 minutes of gameplay.
// 11. Played the 10 minutes, got upto motion tracker, and performed Save State.
// 12. Picked up motion tracker, and breakpoint was hit.
// 13. Disassembly shows
//          move.w  #$ffff, $3106c.l
//          move.l  #$251ee, $39834.l
//
#define AVP2_MTRACKER_ADDR          0x0003106C
#define AVP2_MTRACKER_ENABLE_VALUE  0xFFFF

#define INVALID                     -1

// Boilerplate.
static int32_t sOnLoadEvent = INVALID;
static int32_t sOnEmuFrame = INVALID;
static int32_t sMcFlags = 0;
static const int32_t skMcFlag_Loaded = (1 << 0);

// Setting handles.
static int sHealthSettingHandle = INVALID;
static int sAmmoSettingHandle = INVALID;
static int sUnlockWeaponsSettingHandle = INVALID;
static int sMaxSecuritySettingHandle = INVALID;
static int sMotionTrackerSettingHandle = INVALID;

static uint32_t on_sw_loaded(const int eventHandle, void* pEventData)
{
    const uint64_t hash = bigpemu_get_loaded_fnv1a64();

    sMcFlags = 0;

    if (AVP2_HASH == hash)
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
        int healthSettingValue = INVALID;
        int ammoSettingValue = INVALID;
        int unlockWeaponsSettingValue = INVALID;
        int maxSecuritySettingValue = INVALID;
        int motionTrackerSettingValue = INVALID;
        bigpemu_get_setting_value(&healthSettingValue, sHealthSettingHandle);
        bigpemu_get_setting_value(&ammoSettingValue, sAmmoSettingHandle);
        bigpemu_get_setting_value(&unlockWeaponsSettingValue, sUnlockWeaponsSettingHandle);
        bigpemu_get_setting_value(&maxSecuritySettingValue, sMaxSecuritySettingHandle);
        bigpemu_get_setting_value(&motionTrackerSettingValue, sMotionTrackerSettingHandle);

        if (healthSettingValue > 0)
        {
            // Infinite health.  Copy from constant max health to variable health.

            // This line seems to muck things up.  Again it seems like a jag_read16 to
            // uint16_t clobbers a couple bytes of memory or something.
            //uint16_t maxHealthValue = bigpemu_jag_read16(AVP2_MAX_HEALTH_ADDR);

            // Dance around above issue by casting and recasting.
            const uint32_t maxHealthValue = (uint32_t) bigpemu_jag_read16(AVP2_MAX_HEALTH_ADDR);
            bigpemu_jag_write16(AVP2_HEALTH_ADDR, (uint16_t) maxHealthValue);
        }

        if (ammoSettingValue > 0)
        {
            // Only human needs to have infinite ammo.  Alien and predator
            // are always charging back up.
            const uint8_t numberWeaponsAvailable = bigpemu_jag_read8(AVP2_NUM_WEPS_ADDR);
            if (((uint8_t) AVP2_NUM_HUMAN_WEPS) == numberWeaponsAvailable)
            {
                bigpemu_jag_write16(AVP2_SHOTGUN_ADDR, AVP2_MAX_AMMO_VALUE);
                bigpemu_jag_write16(AVP2_PULSE_ADDR, AVP2_MAX_AMMO_VALUE*6);
                bigpemu_jag_write16(AVP2_FLAME_ADDR, AVP2_MAX_AMMO_VALUE*2);
                bigpemu_jag_write16(AVP2_SMART_ADDR, AVP2_MAX_AMMO_VALUE*2);
            }
        }

        if (unlockWeaponsSettingValue > 0)
        {
            // Enable all weapons for human.
            // Also enables 4 attack weapons for predator.  Predator has a 5th 'weapon'
            // medipack, but with infinite health option, medipack seems OBE.
            // Alien seems to have claw, mouth, tail attacks, no matter what.
            bigpemu_jag_write8(AVP2_CUR_WEP_ADDR, ((uint8_t) AVP2_SHOTGUN_BITMASK_VALUE | AVP2_PULSE_BITMASK_VALUE | AVP2_FLAME_BITMASK_VALUE | AVP2_SMART_BITMASK_VALUE)); // 0x1E
        }

        if (maxSecuritySettingValue > 0)
        {
            // Game manual says Security Clearance 10 needed to activate self destruct.
            bigpemu_jag_write8(AVP2_ACCESS_LEVEL_ADDR, ((uint8_t) AVP2_ACCESS_LEVEL_VALUE));
        }

        if (motionTrackerSettingValue > 0)
        {
            bigpemu_jag_write16(AVP2_MTRACKER_ADDR, AVP2_MTRACKER_ENABLE_VALUE);
        }
    }
    return 0;
}

void bigp_init()
{
    void* pMod = bigpemu_get_module_handle();
	
    bigpemu_set_module_usage_flags(pMod, BIGPEMU_MODUSAGE_DETERMINISMWARNING);

    // Register cheat settings with BigPEmu GUI interface.
    const int catHandle = bigpemu_register_setting_category(pMod, "Aliens Vs Predator");
    sHealthSettingHandle = bigpemu_register_setting_bool(catHandle, "Infinite Health", 1);
    sAmmoSettingHandle = bigpemu_register_setting_bool(catHandle, "Infinite Ammo", 1);
    sUnlockWeaponsSettingHandle = bigpemu_register_setting_bool(catHandle, "Unlock Weapons", 1);
    sMaxSecuritySettingHandle = bigpemu_register_setting_bool(catHandle, "Max Security Clearance", 1);
    sMotionTrackerSettingHandle = bigpemu_register_setting_bool(catHandle, "Unlock Motion Tracker", 1);

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

    sHealthSettingHandle = INVALID;
    sAmmoSettingHandle = INVALID;
    sUnlockWeaponsSettingHandle = INVALID;
    sMaxSecuritySettingHandle = INVALID;
    sMotionTrackerSettingHandle = INVALID;
}