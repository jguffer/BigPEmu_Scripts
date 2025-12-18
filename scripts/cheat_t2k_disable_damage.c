//__BIGPEMU_SCRIPT_MODULE__
//__BIGPEMU_META_DESC__    "Disable damage, in 2000 Mode of Tempest 2000."
//__BIGPEMU_META_AUTHOR__  "jguff"

#include "bigpcrt.h"

// Found using BigPEmu System->Load Software screen and highlighting ROM file.
#define T2K_HASH              0x0030407D0269CE03ULL

// Found via stepping through disassembly.
#define T2K_HIT_DETECT_ADDR   0x0080A53E

#define INVALID               -1

// Boilerplate.
static int32_t sOnLoadEvent = INVALID;
static int32_t sOnEmuFrame = INVALID;
static int32_t sMcFlags = 0;
static const int32_t skMcFlag_Loaded = (1 << 0);

static uint32_t on_sw_loaded(const int eventHandle, void* pEventData)
{
   const uint64_t hash = bigpemu_get_loaded_fnv1a64();

   sMcFlags = 0;

   if (T2K_HASH == hash)
   {
      sMcFlags |= skMcFlag_Loaded;
   }
   return 0;
}

static uint32_t on_emu_frame(const int eventHandle, void* pEventData)
{
   if (sMcFlags & skMcFlag_Loaded)
   {
      // Not adding a setting to control whether below is applied or not.
      // Since below is overwriting instructions, just let it always run.
      // If user doesn't want to have this run, user needs to restart without
      // this script enabled.

      // TODO: Consider moving below into on_sw_loaded method, since it
      //       should only need to run once, not every emulation frame.

      // Try writing bytes for RTS from script
      //    How want to look in memory viewer: 0x4E 0x75
      //    bigpemu_jag_write does not seem to work for these addresses
      //       likely cause the addresses are in ROM space.
      //bigpemu_jag_write8(T2K_HIT_DETECT_ADDR, 0x4E);
      //bigpemu_jag_write8(T2K_HIT_DETECT_ADDR + 1, 0x75);

      // bigpemu_jag_sysmemwrite seems to successfully overwrite ROM memory.
      // At least with BigPEmu v 1.19.
      //
      // And first level enemy bullets passed right through player, inflicting no damage.
      // Enemy can still touch/grab to kill player.
      // Spikes still kill player.
      // Demonhead horns still kill player.
      // Possibly only one type of enemy's bullets have been bypassed (bullhorns may still do me in for example. dunno)
      const uint8_t rtsInstruction[2] = {0x4E, 0x75};
      //Lines below this line seem to make this line OBE.
      //More importantly, commenting out this line, with lines further down present,
      //seemed to make superzapper behavior go back to normal.
      //TODO: Have not tested this script past level 5, since commenting out this line.
      //      See how well this script works for other people.
      //bigpemu_jag_sysmemwrite(rtsInstruction, T2K_HIT_DETECT_ADDR, 2);

      // Prevent at least first level enemies from touch/grab killing you.
      bigpemu_jag_sysmemwrite(rtsInstruction, 0x0080B1A4, 2);

      // Prevent getting spiked on end of level fly out.
      bigpemu_jag_sysmemwrite(rtsInstruction, 0x0080B922, 2);

      // Prevent fuseball from electrocuting player.  Some chance there is a scenario where this will cause crash.
      bigpemu_jag_sysmemwrite(rtsInstruction, 0x0080C11E, 2);

      // Prevent Demonhead's shot horn from killing player.
      bigpemu_jag_sysmemwrite(rtsInstruction, 0x0080A56E, 2);

      // Mirror return fire, pulsar electrocution, and UFO zapping seems to not cause damage at this point.
      // Played through all 100 levels this way.

      // One of the injected return-to-subroutine instructions seemed to have side affect
      // that superzapping after some point (likely getting shot or touched) causes the
      // superzapping to never stop:  infinite superzapper, with one button press, for
      // remainder of level.

      // ******** Using superzapper at some point after beginning of level, prevents enemies
      // from releasing powerups.  Making for a very visually BORING level experience.
      // Maybe zapping any but first enemy who releases blaster upgrade, causes this.
      // ******** Suggest not using superzapper with this cheat enabled.
      // TODO: Consider looking into this further.
      //       2025-12-10 update: commenting out instruction overwrite for T2K_HIT_DETECT_ADDR
      //                          may have resolved this.

      // Believe i noticed shot spikes sometimes were not removed from the playfield, but were no longer
      // being hit by bullets either.  Looked like the spikes were drawn, but hit detection didn't think
      // the spikes were there.  Sometimes spikes like this appeared to be drawn in blue, instead of green.
      // Didn't seem to hurt anything.

      // Believe this works for 2000, Plus, Traditional modes.
      // Unsure of impact on Duel mode, and don't care about impact on Duel mode.
   }
   return 0;
}

void bigp_init()
{
   void* pMod = bigpemu_get_module_handle();

   bigpemu_set_module_usage_flags(pMod, BIGPEMU_MODUSAGE_DETERMINISMWARNING);

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
}