//__BIGPEMU_SCRIPT_MODULE__
//__BIGPEMU_META_DESC__    "Modify number of laps raced for Tournament Mode tracks, in Checkered Flag."
//__BIGPEMU_META_AUTHOR__  "jguff"

#include "bigpcrt.h"

// Found using BigPEmu System->Load Software screen and highlighting ROM file.
#define CHECKERED_FLAG_HASH            0xF9DDA93597C567F7ULL

#define CHECKERED_FLAG_TRACK_INFO_ADDR 0x00804BEE

#define CHECKERED_FLAG_MIN_NUM_LAPS    3
#define CHECKERED_FLAG_MAX_NUM_LAPS    15

#define INVALID                        -1

// Boilerplate.
static int32_t sOnLoadEvent = INVALID;
static int32_t sOnEmuFrame = INVALID;
static int32_t sMcFlags = 0;
static const int32_t skMcFlag_Loaded = (1 << 0);

// Setting handles.
static int32_t sLapsSettingHandle = INVALID;

static uint32_t on_sw_loaded(const int eventHandle, void* pEventData)
{
   const uint64_t hash = bigpemu_get_loaded_fnv1a64();

   sMcFlags = 0;

   if (CHECKERED_FLAG_HASH == hash)
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
      int32_t lapsSettingValue = INVALID;
      bigpemu_get_setting_value(&lapsSettingValue, sLapsSettingHandle);

      if (lapsSettingValue > 0)
      {
         // Modify the number of laps that need completed, in Tournament Mode.
         // Without cheat, Tournament Mode requires racing 10+ laps on each of 10
         // different tracks.  Youtube playthrough videos show this takes 50+ minutes
         // even with emulator's better controls.
         // 30 years after Checkered Flag's release, most people are not going to
         // have the patience to spend 50+ minutes to experience all the tracks and
         // the ending.  Especially given the controls.
         //
         // Sometimes get a crash between menu selection and start of tournament race,
         // Sometimes don't.
         // Sometimes get a crash between tournament race results and start of next
         // tournament race, and somtimes don't.
         // Sometimes crashes came with messages to BigPEmu console window about;
         //    'Masking dangerous RISC PC <address>'
         // Crashes sometimes were everytime after a script change, other times
         // more random after a script change.  Like memory is getting clobbered.
         // (As an example, defined an array to be uint32_t[20] instead of intended
         // uint8_t[20], and wrote that to ROM, and got crashes, as was clobbering
         // memory due to my own fault.  Fixed that spot, and an every-run-crash
         // went away.)
         // Even gone so far as to remove all other scripts (enabled or disabled)
         // from the scripts folder to ensure one isn't set to unintentionally overwrite/
         // clobber memory.
         // Maybe something wrong with my BigPEmu install, machine, Checkered Flag
         // ROM, or BigPEmu Checkered Flag compatability.
         //
         // Note: Considered making a cheat to always finish in top position of race,
         // or to at least gain more points per race then other opponents, to ensure
         // can view the ending screens and credits.
         // Actually first tried to implement the always-finish-in-first cheat on
         // Checkered Flag like other racing games that works on (Atari Karts,
         // Supercross 3D, Super Burnout, World Tour Racing), but turns out
         // Checkered Flag's determination of race winner is more complicated then
         // just one variable.  There are at least 2 variables and then there are some
         // tables of times and points, and between more then one byte to set and
         // crashes encountered, started rethinking the whole always-finish-in-first
         // approach.  That is when switched to reducing the number-of-laps cheat.
         // And then realized there really is no point in a cheat for always-finish-in-first.
         // And the code refers to cars by color instead of number...
         // And it isn't entirely clear which car is red, given cars are bi-color...
         // Kinda wish noone had made the source code available...
         // (Did see there is an 11th oval track defined, but don't think that track
         // got linked into the ROM.)
         // Hacking this game has been as bad of an experience as the games controls.
         //
         // It turns out the ending is: entering player name once for store to EPROM
         // and one ending screen.  And player sees the ending screen, whether player
         // wins or lose the tournament.  And you progress through the tournament even
         // when you take last place on every race.
         // And the credits roll if you just start up the emulator and wait.
         //
         // Caveats: There may be issues with saved EPROM as well.  When crashed
         // a bunch, i got into habit of deleting EPROM between exiting BigPEmu
         // and starting another instance of BigPEmu.

         // Overwrite, in ROM, the number of laps required for each track.
         // The skipped bytes are weather conditions.
         //
         // This seems to successfully overwrite ROM memory, with BigPEmu v 1.19.
         // (jag_write calls did not seem to successfully overwrite ROM.)
         //
         // A copy of this data showed up at address 0x002D8BEE, per Noesis
         // Memory Window Find capability.  Not sure what 0x002D8BEE
         // maps to.  Thought RAM was between 0x00000000 and 0x00200000,
         // and ROM at 0x00800000 and upwards to 0x00A000000, 0x00B00000,
         // or 0x00D00000 depending on size of cart ROM.  Think a flag
         // can be set to switch start addresses of RAM and ROM, but even
         // then 0x002D8BEE is outside the 2 megabyte Checkered Flag ROM.
         // So what is 0x002D8BEE pointing to?  Or has the crashing fried
         // my ability to think straight?
         // Seen Noesis Memory Window show what looks like mirrors of
         // RAM (or maybe ROM) throughout unused/mapped memory space, but
         // the mirror is always 0x00<even digit> greater then what it
         // mirrors.  And that doesn't appear to be the case here.
         // Thought EPROM was not mapped to address space, and instead
         // EPROM was written in serial fashion (like a port).
         // (Had seen in code where selected number of laps for single
         // race game are saved to EPROM).
         // Must be missing something...
         //
         // *********************************************************************
         // 1 hour later update: There is some chance that making change
         // to BigPEmu Settings fixes the crashes that seem random or like
         // clobbered memory:
         //    Settings->Lockstep Mode=Close
         // After changing from default to Close, started tournament mode race
         // successfully (no crash), after emulatar start, 6 times in a row.
         // (Also have Noesis closed, while Noesis was open but not connected
         // while crashing was occurring.)
         // *********************************************************************

         // Track description is table of 10 tracks, each track 2 bytes long.
         // Byte0 is weather (0-normal; 1-rain; 2-fog).
         // Byte1 is number of laps.
         const uint8_t numLapsToComplete = (uint8_t)lapsSettingValue;
         uint8_t trackInfo[20];
         bigpemu_jag_sysmemread(trackInfo, CHECKERED_FLAG_TRACK_INFO_ADDR, 20);
         trackInfo[1] = numLapsToComplete;
         trackInfo[3] = trackInfo[1];
         trackInfo[5] = trackInfo[1];
         trackInfo[7] = trackInfo[1];
         trackInfo[9] = trackInfo[1];
         trackInfo[11] = trackInfo[1];
         trackInfo[13] = trackInfo[1];
         trackInfo[15] = trackInfo[1];
         trackInfo[17] = trackInfo[1];
         trackInfo[19] = trackInfo[1];
         bigpemu_jag_sysmemwrite(trackInfo, CHECKERED_FLAG_TRACK_INFO_ADDR, 20);

         // Since was down the rabbit hole of looking at source code,
         // viewing memory in Noesis Memory Window, sometimes placing
         // breakpoints, etc, here are some addresses that were found.
         //
         // These addresses might be a start for any fool who wants to
         // further hack Checkered Flag.
         //
         // Way less organized below.
         //
         //
         // To locate where in ROM the tournament tracks were defined,
         // located source code that defined the data structure detailing
         // number of laps/weather for each track.
         // Then flattened that data structure to the following:
         //    0 10 1 13 0 10 2 13 2 15 0 12 1 15 1 12 0 14 0 10 (decimal)
         // Then used Noesis Memory Window Find capability to search for
         // that flattened data, to find the address(es) that match:
         //    0x00804BEE (the correct address)
         //    0x002D8BEE (unsure what this maps to)
         //
         // Start.S
         //    Tournament_tracks:: defined likely where number of laps defined for each track in tournament
         //    Tournament_setup:: shows some of the setup and duplicate value population

         //    tourn_qty_laps::  ds.b  1  ; depends on track
         //    collon::          ds.w  1  ; to prevent 'collisions' with track.
         //        Might be interesting to turn this off.  Expect bad things will happen.
         // grid_position
         // 0x41E0 - race_timer according to my offset calculation
         //          and believe it is number of seconds times 60 + fraction of 60 (kindof a tv/object-processor frame count)
         //          seems to count a bit faster then 60fps, but think that is the idea of it.
         //          somewhere saw reference to race_timer as master clock
         //          you can't verify it from HUD after complete first lap
         // 0x43D0 - very likely engine revs
         // 0x43D2 - likely num_players
         // 0x43E4 - music_volume
         // 0x43E6 - effects_volume
         // 0x43E8 - old_eff_vol
         // 0x43EB - displayed gear?
         //          possibly gear_1: or gear: from vardefs.s, but not like documentation of 1-6 and automatic having top bit set to 1
         //
         // supposedly winner: defined right before and might be interesting to fiddle around with on 1 lap race
         // 0x4420 likely winner
         // 0x442C last_result (very likely).  result from last race (or just raced race in results screen)
         // 0x442E maybe winner (or best lap) 
         // 0x4430 grid_colour0 probably.
         // 0x444B likely car_colour
         // 0x444E pretty sure qty_laps
         // 0x444F pretty sure track_no
         //
         // ai.inc ai.s look interesting
         //
         // endgame.s: get_posit
         //    .nx:  cmp.b    race_won.w,d0        ; check for this position having been filled
         //          move.b   grid_position.w,d1   ; this is the players car
         // 
         // vardefs.s:
         //    god::             ds.b  1
         //    gear_1::          ds.b  1  ; gear (0=N, 1-6) plus top bit set for Automatics
         //    race_timer::      ds.l  1
         //    time_stopped::    ds.l  1  ; so that can record time at end of race 
         //    lap_start::       ds.l  1
         //    race_won::        ds.b  1  ; so only the first person across is declared winner 
         //    grid_position::   ds.b  1  ; always 0 for ordinary races and free practice, but changes in tournaments 
         //    winner::          ds.b  1  ; holds car number for working out stuff for end game
         //    pos1_car::        ds.b  1  ; these hold the car numbers as they cross the line - they are grid positions
         //    pos2_car::        ds.b  1  ; and so will be used in conjunction with the grid colours to determine the
         //    pos3_car::        ds.b  1  ; following races grid positions when in tournament mode
         //    pos4_car::        ds.b  1
         //    pos5_car::        ds.b  1
         //    pos6_car::        ds.b  1
         //    pos1_colour::     ds.b  1  ; these hold the car colours which relate to the above, to properly work out the
         //    pos2_colour::     ds.b  1  ; following races grid positions when in tournament mode
         //    pos3_colour::     ds.b  1
         //    pos4_colour::     ds.b  1
         //    pos5_colour::     ds.b  1
         //    pos6_colour::     ds.b  1
         //    carsad::          ds.l  1  ; address of your car (as an object) for moving through 3d world
         //    dfct::            ds.l  1  ; displayed frames counter
         //
         // 0x00004420 through 0x0000443F may hold position or end of race position info
         // Believe updating these 2 addresses impacted the displayed position in Heads Up Display:
         //    0x4f51
         //    0x5551
         // 
         // At some point, noted following values for Heads Up Display position,
         // or was it the end of race results position?
         // Positions seem to be specified by the following
         //     0x30 = 1st
         //     0x31 = 2nd
         //     0x32 = 3rd
         //     0x33 = 4th
         //     0x34 = 5th
         //     0x35 = 6th
         // ASCII values for the position numbers regardless.
      }
   }
   return 0;
}

void bigp_init()
{
   void* pMod = bigpemu_get_module_handle();

   bigpemu_set_module_usage_flags(pMod, BIGPEMU_MODUSAGE_DETERMINISMWARNING);

   // Register cheat settings with BigPEmu GUI interface.
   const int32_t catHandle = bigpemu_register_setting_category(pMod, "Checkered Flag");
   sLapsSettingHandle = bigpemu_register_setting_int_full(catHandle, "Num Tournament Laps", CHECKERED_FLAG_MIN_NUM_LAPS, 0, CHECKERED_FLAG_MAX_NUM_LAPS, 1);

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

   sLapsSettingHandle = INVALID;
}