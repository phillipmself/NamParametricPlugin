# NamParametricPlugin

JUCE-based `VST3 + Standalone` plugin for loading and running Neural Amp Modeler (`.nam`) models,
including parametric NAM models via `nam_core_parametric`.

## Scope (v1)

- Mono input/output processing path.
- Host-automatable parameters:
  - `Input Gain` (`-12 dB` to `+12 dB`)
  - `Output Gain` (`-12 dB` to `+12 dB`)
- Model load via file chooser (`.nam`).
- Dynamic model parameters are generated in the plugin UI and applied at runtime.
- Dynamic model parameters are **not** exposed as host automation parameters in v1.
- Plugin state persists:
  - APVTS state
  - `modelPath` (absolute path)
  - dynamic model parameter values by name
- Sample-rate mismatch between DAW and model is handled via AudioDSPTools resampling.

## Repository Layout

- `Source/`: plugin processor/editor and NAM integration layer.
- `external/JUCE`: JUCE submodule.
- `external/nam_core_parametric`: NAM core with parametric support.
- `external/AudioDSPTools`: resampling container dependency.

## Prerequisites

- macOS toolchain (Xcode command line tools)
- CMake `>= 3.22`
- Ninja
- `clang-format` (for local formatting consistency)

## Setup

Initialize submodules (required):

```bash
git submodule update --init --recursive
```

## Build

Debug build:

```bash
cmake --preset mac-debug
cmake --build --preset mac-debug -j 8
```

Release build:

```bash
cmake --preset mac-release
cmake --build --preset mac-release -j 8
```

## Run (Standalone)

After a debug build, launch:

```bash
./build/mac-debug/NamParametricPlugin_artefacts/Debug/Standalone/NAM\ Parametric\ Plugin.app/Contents/MacOS/NAM\ Parametric\ Plugin
```

VST3 output path (debug):

```text
build/mac-debug/NamParametricPlugin_artefacts/Debug/VST3/NAM Parametric Plugin.vst3
```

## Usage Notes

1. Open plugin.
2. Click `Load .nam Model`.
3. Choose a `.nam` file.
4. If the model is parametric, dynamic controls appear under `Model Parameters`.
5. Adjust `Input Gain` / `Output Gain` as needed (bounded to `-12/+12 dB`).

## Manual Validation Checklist

Run these checks before merging major changes:

1. Build succeeds with `cmake --preset mac-debug` and `cmake --build --preset mac-debug`.
2. Plugin loads with no model and processes audio safely (pass-through + gains).
3. Gains are bounded to `-12/+12 dB` and audible as expected.
4. Non-parametric model loads; dynamic section shows no controls.
5. Parametric model loads; sliders/checkboxes appear and audibly affect output.
6. Model sample-rate mismatch still processes audio without failure (resampling active).
7. Save/reload session restores gains, `modelPath`, and dynamic values by name.
8. Missing/broken stored `modelPath` does not crash and plugin remains usable.

## Known Limitations (v1)

- Dynamic model parameters are UI-only (no host automation IDs).
- Mono-only internal processing.
- Model load is chooser-only (no drag/drop).
- AU format is currently out of scope.
- macOS-first validated path.
