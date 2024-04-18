# Changelog

## Unreleased

### Added
 - [#20] Drag n drop modules to load them.
 - Insert row, pressing `Ins` will insert an empty row at the cursor,
   shifting all rows below it down by 1.
 - Grow pattern, `Ctrl+G` or Edit > Grow pattern, grows a selection in
   a pattern by adding an empty row in between each row.
 - Shrink pattern, `Ctrl+H` or Edit > Shrink pattern, inverse of grow pattern
   by removing spaces in between rows of a selection.

[#20]: https://github.com/stoneface86/trackerboy/issues/20

## [0.6.3] - 2023-01-26

### Added
 - [#13] - Example modules to be included with each release.

   Thank you to the following authors who provided these modules:
    - forple
    - gazanmei
    - sttng
    - omega_bagel
 - Effects List dialog (Help > Effects List...)
 - Menu action in Help to open the online user manual (Help > User Manual...)
 - Shortcuts for Effects List and User manual, defaults to F1 and F2, respectively

### Changed
 - Ported from Qt 5 to Qt 6
 - i386/32-bit builds are no longer supported
 - Miniaudio library updated, v0.10.42 -> v0.11.11
 - RtMidi library updated, 4.0.0 -> 5.0.0

### Fixed
 - Bug when hitting enter in the Wave Editor sets the waveform to 50% duty.
 - Elapsed counter in the Audio Diagnostic Dialog updates again.

[#13]: https://github.com/stoneface86/trackerboy/issues/13

## [0.6.2] - 2022-08-22

### Fixed
 - Crash when deleting the last pattern of a song while it is playing. Tracker
   will now jump to pattern 0 instead when this happens.

## [0.6.1] - 2022-03-15
### Added
 - Backspace operation for pattern editor.
 - [#7] - Add icon for MacOS app (thanks [@jtakakura])

### Changed
 - Erasing a note will also erase the instrument column
 - Moved Song order editor toolbar to the left of the order grid,
   also added Song menu actions to the toolbar.

### Fixed
 - [#5] - Pattern Crash When Switching Songs

[#5]: https://github.com/stoneface86/trackerboy/issues/5
[#7]: https://github.com/stoneface86/trackerboy/issues/7
[@jtakakura]: https://github.com/jtakakura

## [0.6.0] - 2022-03-03
### Added
 - Command line argument parsing, you can now open a module on application startup
 - General tab in Config dialog for changing basic settings.
 - Appearance tab in Config dialog for changing fonts, colors and other settings
 - Keyboard tab in Config dialog for changing keyboard layout and editing keybinds,
   as well as shortcut key sequences.
 - Colors for the audio scope and graph editors are now customizable
 - Autosave. Disabled by default, enable in the General tab in Config.
 - Jxy effect for setting the global volume scale.
 - Menu action in Tracker for reseting the global volume (Tracker|Reset global volume)
 - Context menu for pattern editor
 - Action for replacing all selected instrument columns with the current instrument
   (Edit|Replace instrument)

### Changed
 - Combined the Sound and Midi tabs in the Configuration dialog (Sound + MIDI)
 - AZERTY keyboard layout uses French AZERTY
 - QWERTZ keyboard layout uses German T3
 - CascadiaMono is no longer the default font
 - (Windows) Consolas is now the default font, falling back to Courier New if not available
 - No longer using dock widgets, Instruments + Waveforms are now located to
   the right of the pattern editor.
 - Pattern editor row numbers can be displayed in decimal (default is hexadecimal)
 - Cursor wrap can now be disabled in General settings
 - Hxx effect (set sweep register) now works.
 - Crash save filename no longer using colons in the timestamp (this resulted
   in using an illegal filename on windows).
 - Follow-mode is now accessible from the Tracker menu
 - Selecting an instrument/waveform will switch to that item in its editor if it is open.
 - Module properties dialog can now be opened via Ctrl+P

### Fixed
 - Issue with Instrument Editor not properly updating when changing instruments
 - Hitting enter in the sequence editor line edit opens the Wave editor
 - Bug when saving a new module overwrites the last opened one instead of prompting
   the user for a new location.
 - Song editor not updating on song change.

### Removed
 - Synthesizer quality setting in Sound configuration

## [0.5.0] - 2021-09-10
### Added
 - Instrument + Waveform list views for selecting/adding instruments/waveforms
 - Effect columns can now be hidden/added for each track
 - Multiple song support, modules can now store 1-256 songs
 - Backwards compatibilty for 0.0 (Rev A) modules
 - Tempo calculator dialog (Song > Tempo calculator...)
 - Crash handler. Attempts to save a copy of the module after a fatal error
 - Module properties dialog for editing the song list, module information and
   framerate/system.
 - Song next/previous actions for changing the current song
 - Song selector in Sidebar for selecting the current song
 - Comments dialog for editing module comments
 - File menu now shows a list of recently opened/saved modules

### Changed
 - Back to using miniaudio instead of RtAudio for audio playback
 - Trackerboy is now SDI instead of MDI (only one module can be open at a time).
 - Initial window size is set to a maximum of 1280x720
 - Module file format is now at 1.0 (Rev B)
 - Config file is now more human friendly
 - Redesigned header for the pattern editor
 - Reduced spacing between columns in pattern editor
 - Rewrote OrderEditor using a custom widget instead of QTableView + OrderModel
 - Instrument and Waveform editors are now dialogs instead of docks.
 - Octave decrease/increase shortcut changed from numpad / * to Ctrl+[ and Ctrl+]

### Fixed
 - issue #2 - Exx not taking immediate effect (also fixes V0x and I0x not
              taking immediate effect as well)
 - issue #3 - PulseAudio hang

### Removed
 - Module tree view (replaced by instrument and waveform docks)
 - Instrument select toolbar (functionality replaced by selecting an instrument
   in the instruments dock)
 - Patterns and Tempo spinboxes in Song settings editor


## [0.4.0] - 2021-07-07
### Added
 - MIDI support. You can now use a MIDI input device to input notes / preview instruments
 - Order editor has increment/decrement and set operations again
 - Oscilloscope visualizer
 - Icons for cut/copy/paste

### Changed
 - 32-bit builds are now supported
 - Using RtAudio instead of miniaudio for audio playback
 - Some improvements to the audio diagnostics dialog
 - Fxx effect changes the speed immediately now (previously it only took effect on the next row)
 - Status bar is now updated with speed/tempo, time elapsed and position of
   current song being played.
 - Cursor row is locked when playing in follow-mode
 - Selecting an order while playing in follow-mode jumps to it

### Fixed
 - Crash when creating new module on linux
 - Copy/Paste bugs when copying effect columns


## [0.3.1] - 2021-06-23
### Changed
 - Play and stop shortcut only triggers when pattern editor has focus

### Fixed
 - Note previews not working right on CH4
 - Current track not updating in order editor


## [0.3.0] - 2021-06-22
### Added
 - Note previews when inputting notes in the pattern editor
 - Pattern repeat mode
 - Step mode playback
 - Input toolbar (set the octave and edit step)
 - Instrument toolbar (combobox for the current instrument)
 - Song menu, contains actions for adding/removing orders
 - Sidebar widget, combines song settings + order editor
 - Icon for configuration
 - Keybinds for tracker actions (play, stop, etc)
 - Add "play and stop" shortcut (pressing enter plays, pressing it again stops)

### Changed
 - No longer need to have record mode on when deleting, transposing, etc
   (record mode now only applies to setting notes, instruments and effects)
 - Arpeggio editor graph is now centered at 0
 - Moved stuff from PatternEditor to separate toolbars
 - Renamed Delete action in Edit menu to Erase
 - View menu is now between Tracker and Window
 - Fix "set volume on trigger" checkbox not updating when changing instruments
 - Fix pattern previews not showing up on module load
 - Fix order move up/down actions not disabling/enabling when user changes patterns
 - Setting a note cut does not set the instrument column

### Removed
 - PatternEditor toolbar (now is part of the main application toolbar)
 - Order increment/decrement and set controls


## [0.2.0] - 2021-06-15
The project has changed dramatically since the last release so changes will
not be listed for this version.
Here's a quick list of features:
 * Open/save modules, can have multiple modules open at once
 * Edit pattern data via keyboard (OpenMPT style keybindings, will be configurable in later versions)
 * Transpose selection by note/octave or by a custom semitone offset (Ctrl+T)
 * Reverse selected rows (Ctrl+R)
 * Cut/Copy/Paste pattern data using system clipboard
 * Mix paste (only the unset columns get set to the pasted data)
 * Move pattern data via drag n drop
 * Music playback, pattern cursor follows the current playing row
 * Edit waveforms visually via a graph control, by selecting a preset or by manually entering waveram data
 * Edit instrument sequences visually
 * Instruments and Waveforms can be previewed using the piano control (or by keyboard).
 * Undo/Redo functionality for song order and pattern editing.
 * Audio diagnostics dialog (Help > Audio Diagnostics) for debugging sound output / buffer issues.


## [0.1.0] - 2020-09-20
### Added
 - The main UI has been started
 - Wave editor dialog, user can edit waveforms and preview them using a piano control
 - Configuration dialog, user can select an audio device, set buffer size and volume
 - Instrument editor dialog
 - DeviceManager and DeviceTable classes in the audio library
 - demo_device_manager demo lists all available host api and output devices

### Changed
 - PlaybackQueue can use any portaudio device instead of the default one
 - Reorganized library header folders
 - MusicRuntime only updates "locked" channels
 - Rewrote Engine class to use MusicRuntime class

### Removed
 - File class
 - Old ui code (instedit)


## [0.0.6] - 2020-08-09
### Added
 - song_demo, demonstrates playback of a Song
 - Engine class, handles playback for Songs and sound effects
 - FrequencyControl, frequency effect logic for channels 1, 2 and 3
 - Note lookup table for channel 4, NOTE_NOISE_TABLE
 - MusicRuntime, handles song playback (replaces SongRuntime + PatternRutime + TrackRuntime)
 - NoteControl, handles note triggers and cuts with an optional delay.
 - PatternCursor, utility class for random access/iteration of a Pattern
 - RuntimeContext struct, utility struct containing Synth and Table references
 - Timer class, used for tempo/speed

### Changed
 - Fixed warnings for ubuntu build
 - Mixer no longer uses sigma approximation when calculating the STEP_TABLE, this was done as
   an experiment and also worked as a low pass filter. Filtering will be done elsewhere in the future.
 - added utility method Synth::setFrequency
 - added several new effects. Effects are now categorized by pattern, track and frequency.

### Removed
 - InstrumentRuntime, TrackRuntime, PatternRuntime and SongRuntime classes. These are being replaced
   with the newly added classes in the engine folder.
 - Q53.hpp, replaced by Speed.hpp

## [0.0.5] - 2020-04-24
### Added
 - PatternMaster class, container for all tracks in a Song
 - Synth::reset() and for all APU components also

### Changed
 - Instruments are now a sequence of commands, 1 command per frame
 - All runtime classes have been rewritten to use the new pattern format
 - Song serialization has been rewritten to use new pattern format
 - File format signature changed so that it starts with a non-printable character
 - TrackRuntime and InstrumentRuntime are now template classes
 - Patterns function like FamiTracker's, a pattern is a combination of resuable track data
 - Pattern is now a tuple of Track references
 - Order is now a POD struct of track ids, one for each channel
 - Track no longer behaves like an iterator and now contains data
 - pattern demo now plays a recreation of national park from pokemon G/S/C
 - implemented writeAll method in PlaybackQueue
 - PlaybackQueue uses standard library for sleeping instead of using Pa_Sleep
 - Synth methods now have noexcept and const qualifiers where applicable
 - Fix issue in Wav writer, where the sample count in the fact chunk was
   incorrect for multichannel data.
 - The Mixer and Synth sampling rates can now be changed
 - SongRuntime is now single-use, or, you can no longer set a new song

### Removed
 - Instruction struct, no longer needed
 - PatternRuntime::reset()

## [0.0.4] - 2020-04-01
### Added
 - Wav class, for writing wav files (only supports 32-bit float format)
 - Generator class, base class for PulseGen, WaveGen and NoiseGen
 - PulseGen class, similar to the previously removed PulseChannel
 - WaveGen class, similar to the previously removed WaveChannel
 - Mixer class, adds bandlimited steps to left/right channels
 - Sequencer class, similar to the previously removed one

### Changed
 - Minimum required C++ standard is now C++17
 - PlaybackQueue uses float samples instead of int16_t
 - Synth was rewritten to use emulation based synthesis, generates samples on
   a per frame basis.
 - Synth can now read and write APU registers
 - reset() renamed to restart() in synth APU components
 - setRegister renamed to writeRegister
 - Sweep has its own shadow register and now longer uses the channel's on trigger
   (this behavior now matches the hardware)
 - Sweep and Envelope register contents get applied on restart

### Removed
 - include/trackerboy/synth/utils.hpp, no longer being used
 - libtrackerboy/synth/test_Osc.cpp, was unused
 - Osc, PulseOsc and WaveOsc, since we are now using emulation based synthesis

## [0.0.3] - 2020-03-09
### Added
 - More test cases for File
 - tests for `Table<T>`
 - misc/genstep.py, similar to gensinc.py, generates bandlimited step sets for Osc
 - implementation for WaveOsc
 - NoiseGen, buffered version of NoiseChannel
 - Envelope, moved code from EnvChannel here (will replace EnvChannel)
 - Sweep class, replaces SweepPulseChannel
 - HardwareFile struct (replaces ChannelFile)

### Changed
 - Fix exception being thrown when attempting to insert into table of size one less than maximum
 - Rewrote Osc class. Subclasses must edit the delta buffer manually instead of
   using `deltaSet`.
 - Osc no longer uses sinc sets. It uses bandlimited steps instead, similiar to
   blargg's http://www.slack.net/~ant/bl-synth/11.implementation.html. `Osc::generate`
   uses floats for samples instead of int16_t. Samples range from -1.0-1.0, exceeding both
   limits due to overshoot (so volume will need to be adjusted or clipping will occur).
 - Mixer no longer emulates terminal volume/panning (tracker doesn't use it)
 - Mixer works with sample buffers as opposed to individual samples from each channel
 - Length counters are no longer emulated by the Synth
 - Mixer and Sequencer were moved into Synth, as only the synth used these internally.

### Removed
  - `Osc::deltaSet` as only WaveOsc needed this method
  - `Osc::setMute` and `Osc::muted`, replaced by `Osc::disable` and `Osc::disabled`
  - misc/gensinc.py no longer needed, replaced by genstep.py
  - Channel, EnvChannel, PulseChannel, SweepPulseChannel, WaveChannel, NoiseChannel,
    ChannelFile, Mixer and Sequencer classes

## [0.0.2] - 2020-02-05
### Added
 - `File::loadTable<T>` method
 - uint16_t overload for correctEndian
 - Destructor and copy constructor to `Table<T>`
 - `Table<T>::insert` methods
 - `Table<T>::clear`
 - Version struct + operator overloads

### Changed
 - Rewrote implementation for `Table<T>`, no longer uses `std::unordered_map`
   Uses a vector for the item data, and a 256 uint8_t array that maps an id
   to an index in the vector. Allowing for faster lookups but a slight
   performance loss when removing items.
 - Refactored pattern_demo.cpp and File.cpp to use new Table implementation
 - table size is now a 2 byte field when saving/loading
 - add setSpeed() overload to calculate speed from tempo/rpb settings. Song
   no longer calculates speed when tempo or rpb is set.

### Removed
 - `Table<T>::add`, `Table<T>::set`, as these methods are no longer needed as the
   insert methods should be used instead.

## [0.0.1] - 2020-02-01
### Added
 - This CHANGELOG.md file to serve as a changelog for any new features, removals
   deprecations and so on for the project as a whole.

### Changed
 - README.md, added a roadmap section to list planned features and the order
   in which they are worked on.
 - ORGANIZATION.md, added a guideline to use unix style newlines in all files
 - CMakeLists.txt, updated project version to v0.0.1
