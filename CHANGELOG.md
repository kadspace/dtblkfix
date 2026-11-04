# Changelog

## 1.0.0

The first release of DtBlkFix: Darrell Tam's DtBlkFx with the fixes below. The engine is the same, and the audio is bit-identical to the original except for Sync, which is now always on.

### Fixed

- **Timing.** The output was delayed by one block while the host was told the latency was zero, so at the default settings everything came out a beat late. The delay is now always one block and reported, so the host compensates for it. The Delay control is gone.
- **Song sync.** Sync is always on: whenever the host gives a song position, a block starts on every beat, including the first beat after a loop or jump, at any buffer size. The original only synced with the Sync switch on, read the tempo and song position about once a beat (so blocks drifted off the beat after loops, jumps and tempo changes, and where they landed depended on the buffer size), and skipped a beat that fell less than a 16th of a beat after a block start. While the transport is stopped, the beat grid keeps running from where it stopped. With no song position there's no beat, and blocks aren't synced (the original used an arbitrary grid). The Sync control is gone.
- **Window size.** Drag the window's bottom right corner to any size from 75 to 300%, or pick 75, 100, 125, 150 or 200% from a spectrogram's menu. Text and lines are drawn at that size and the images smoothly scaled. It's saved with the plugin's state.
- **Skins.** Original, Dark and Warm, from a spectrogram's menu, saved with the plugin's state. A skin is a folder of the same images the plugin always loaded, so you can add your own in `dtblkfx\skins`.
- **Top strip.** One line: MixBack, Power, Overlap and BlkLen, each with its value beside it. The block length reads in ms and samples, e.g. "92.9 ms · 4096".
- **Style.** Boxes puts each value in a box, in the top strip and the effect rows, which is easier to read; Original keeps the outlined text. Pick it from a spectrogram's menu. It's saved with the plugin's state.
- **Effect rows.** Only the effects in use get a row. **+** adds one (a Filter over the whole range at 0 dB, which changes nothing until you edit it) after the last effect, and the **×** on a row removes that effect and moves the ones below up, so the chain stays in the order shown.

### Changed

- 64-bit VST3 and standalone app, built with JUCE 8, with the original editor inside. The VST2 SDK isn't needed to build it.
- Own plugin ID and name, DtBlkFix, so it installs next to the original.
- Windows installer, and a zip for installing by hand.
- The plugin's 44 parameters, the factory presets and the format of the saved settings are the original's.
