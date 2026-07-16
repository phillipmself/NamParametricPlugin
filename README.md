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
  - dynamic model parameter values by ordered index and name
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

## Model Smoke Test

The smoke-test executable loads one `ConcatWaveNet` and one `HyperWaveNet` model, renders at a
mismatched host sample rate with a non-power-of-two block size, applies alternate full parameter
vectors, and checks for finite, non-silent, parameter-dependent output:

```bash
./build/mac-debug/nam_model_smoke_test /path/to/concat.nam /path/to/hyper.nam
```

To register the same check with CTest, configure with model paths and then run CTest:

```bash
cmake --preset mac-debug \
  -DNAM_CONCAT_TEST_MODEL=/path/to/concat.nam \
  -DNAM_HYPER_TEST_MODEL=/path/to/hyper.nam
ctest --test-dir build/mac-debug --output-on-failure
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
5. Parametric model loads; continuous sliders and switch menus appear and audibly affect output.
6. Model sample-rate mismatch still processes audio without failure (resampling active).
7. Save/reload session restores gains, `modelPath`, and dynamic values by name.
8. Missing/broken stored `modelPath` does not crash and plugin remains usable.

## Known Limitations (v1)

- Dynamic model parameters are UI-only (no host automation IDs).
- Mono-only internal processing.
- Model load is chooser-only (no drag/drop).
- AU format is currently out of scope.
- macOS-first validated path.
