# Changelog

## 1.0.0

The first release of DtBlkFix: Darrell Tam's DtBlkFx with the fixes below. The engine is the same, and the audio is bit-identical to the original except where Sync is on.

### Fixed

- **Timing.** The output was delayed by one block while the host was told the latency was zero, so at the default settings everything came out a beat late. The delay is now reported, so the host compensates for it. The Delay knob shows "auto".
- **Song sync.** With Sync on, the tempo and song position were read about once a beat, so blocks drifted off the beat after loops, jumps and tempo changes, and where they landed depended on the host's buffer size. They're now read on every buffer, and a block starts on every beat, including the first beat after a loop or jump, at any buffer size. A beat that falls less than a 16th of a beat after a block start is no longer skipped. While the transport is stopped, the beat grid keeps running from where it stopped.
- **Window size.** The window scales to 75, 100, 125, 150 or 200%, with text and lines drawn at that size and the images smoothly scaled. Pick it from a spectrogram's menu. It's saved with the plugin's state.
- **Skins.** Original, Dark and Warm, from a spectrogram's menu, saved with the plugin's state. A skin is a folder of the same images the plugin always loaded, so you can add your own in `dtblkfx\skins`.

### Changed

- 64-bit VST3 and standalone app, built with JUCE 8, with the original editor inside. The VST2 SDK isn't needed to build it.
- Own plugin ID and name, DtBlkFix, so it installs next to the original.
- Windows installer, and a zip for installing by hand.
- The plugin's 44 parameters, the factory presets and the format of the saved settings are the original's.
