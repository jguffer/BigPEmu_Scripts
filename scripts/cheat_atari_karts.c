//__BIGPEMU_SCRIPT_MODULE__
//__BIGPEMU_META_DESC__     "Infinite cars/lifes, always be deemed first place, unlock all challenges/cups/racers, in Atari Karts."
//__BIGPEMU_META_AUTHOR__   "jguff"

#include "bigpcrt.h"

// Found using BigPEmu System->Load Software screen and highlighting ROM file.
#define ATARI_KARTS_HASH            0xD6821157BFC4A8B0ULL

// Found using Pro-Action Replay like counter script.
#define ATARI_KARTS_POSITION_ADDRA  0x00000203
#define ATARI_KARTS_POSITION_ADDRB  0x000E96c3

// Pro Action Replay Script came up with these 3 addresses for number of cars/lives,
// but don't seem to be it.  Not too surprising to find addresses with values
// that _happen_ to match, given that only compared 4 values (3,2,1,0 cars).
//     0x000CC8CB
//     0x000CC8EB
//     0x000CE76B
//
// Ended up finding by inspecting memory around the addresses identified
// for race position, and then noting that the value is one above the value
// displayed in Heads Up Display of game.  Re-ran Pro Action Replay like script
// with values of 1,2,3,4, and script  identified this address, and
// identified this address only.
#define ATARI_KARTS_CARS_ADDR   0x000E96CB
#define ATARI_KARTS_CARS_VALUE  7

// Don't delete this.  Keep for reference.  Believe these were applied to 0x00004840.  Unlocking the cups makes this OBE though.
//#define ATARI_KARTS_HARATARI_BITVALUE     (1<<27)
//#define ATARI_KARTS_PUM_KING_BITVALUE     (1<<19)
//#define ATARI_KARTS_FIREBUG_BITVALUE      (1<<11)
//#define ATARI_KARTS_MIRACLE_MAN_BITVALUE  (1<<3)

// There are 4 challenges.
enum {BEGINNER_CHALLENGE, WARRIOR_CHALLENGE, MIRACLE_CHALLENGE, ACES_CHALLENGE, NUM_CHALLENGES} challengeBytePosEnum;

// Each challenge is composed of 4 cups.  The same 4 cups are used for each challenge.
enum {BEGINNERS_CUP_BIT, CARLETON_CUP_BIT, TEMPEST_CUP_BIT, MIRACLE_RACE_BIT, NUM_CUPS} cupsEnum;

// Addresses for unlocking challenges and cups.
#define CHALLENGE_UNLOCK_ADDR   0x0000483B
#define CUPRACE_UNLOCK_ADDR     0x00004840

#define INVALID                 -1

// Boilerplate.
static int32_t sOnLoadEvent = INVALID;
static int32_t sOnEmuFrame = INVALID;
static int32_t sMcFlags = 0;
static const int32_t skMcFlag_Loaded = (1 << 0);

// Setting handles.
static int sFinishFirstSettingHandle = INVALID;
static int sContinuesSettingHandle = INVALID;
static int sUnlockSettingHandle = INVALID;

static uint32_t on_sw_loaded(const int eventHandle, void* pEventData)
{
    const uint64_t hash = bigpemu_get_loaded_fnv1a64();

    sMcFlags = 0;

    if (ATARI_KARTS_HASH == hash)
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
        int finishFirstSettingValue = INVALID;
        int continuesSettingValue = INVALID;
        int unlockSettingValue = INVALID;
        bigpemu_get_setting_value(&finishFirstSettingValue, sFinishFirstSettingHandle);
        bigpemu_get_setting_value(&continuesSettingValue, sContinuesSettingHandle);
        bigpemu_get_setting_value(&unlockSettingValue, sUnlockSettingHandle);

        if (continuesSettingValue)
        {
            // Infinite cars/tries/continues.
            bigpemu_jag_write8(ATARI_KARTS_CARS_ADDR, ATARI_KARTS_CARS_VALUE);
        }

        if (finishFirstSettingValue)
        {
            // Always be marked as position one (the winner), in spite
            // of your actual position or time.
            const uint8_t position = 1;
            bigpemu_jag_write8(ATARI_KARTS_POSITION_ADDRA,position);
            bigpemu_jag_write8(ATARI_KARTS_POSITION_ADDRB,position);
        }

        if (unlockSettingValue > 0)
        {
            // Unlock all challenges.  Values of 2, 3, 4 in order of challengeBytePosEnum.
            // This value is written every emulation frame, so OR with existing value, so
            // user can progress if user didn't unlock everything.
            const uint8_t oldValue1 = bigpemu_jag_read8(CHALLENGE_UNLOCK_ADDR);
            const uint8_t unlockValue1 = 4; // 4 unlocks all challenges.
            const uint8_t newValue1 = oldValue1 | unlockValue1;
            bigpemu_jag_write8(CHALLENGE_UNLOCK_ADDR, newValue1);

            // Unlock all cups, for each challenge.  Each challenge's cups status is
            // in seperate byte.
            // This value is written every emulation frame, so OR with existing value, so
            // user can progress if user didn't unlock everything.
            //
            // Additionally, unlocking the Miracle cup, unlocks that Challenge's associated
            // additional driver character.
            const uint8_t unlockValue2 = (1 << BEGINNERS_CUP_BIT) | (1 << CARLETON_CUP_BIT) |
                                         (1 <<   TEMPEST_CUP_BIT) | (1 << MIRACLE_RACE_BIT);
            for (uint32_t challengeIdx = 0; challengeIdx < NUM_CHALLENGES; ++challengeIdx)
            {
                const uint8_t oldValue2 = bigpemu_jag_read8(CUPRACE_UNLOCK_ADDR + challengeIdx);
                const uint8_t newValue = oldValue2 | unlockValue2;
                bigpemu_jag_write8(CUPRACE_UNLOCK_ADDR + challengeIdx, newValue);
            }

            // CAVEAT: This state will get saved to EEPROM.
        }
    }
    return 0;
}

void bigp_init()
{
    void* pMod = bigpemu_get_module_handle();

    bigpemu_set_module_usage_flags(pMod, BIGPEMU_MODUSAGE_DETERMINISMWARNING);

    // Register cheat settings with BigPEmu GUI interface.
    const int catHandle = bigpemu_register_setting_category(pMod, "Atari Karts");
    sFinishFirstSettingHandle = bigpemu_register_setting_bool(catHandle, "Always Deemed Winner", 1);
    sContinuesSettingHandle = bigpemu_register_setting_bool(catHandle, "Infinite Continues", 1);
    sUnlockSettingHandle = bigpemu_register_setting_bool(catHandle, "Unlock All Challenges, Cups, Racers", 1);

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

    sFinishFirstSettingHandle = INVALID;
    sContinuesSettingHandle = INVALID;
    sUnlockSettingHandle = INVALID;
}