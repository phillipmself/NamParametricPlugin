# NamParametricPlugin

JUCE-based `VST3 + AU + Standalone` plugin for loading and running Neural Amp Modeler (`.nam`) models,
including parametric NAM models via `nam_core_parametric`.

## Related Projects

This plugin builds on two companion repositories:

- [neural-amp-modeler-parametric](https://github.com/phillipmself/neural-amp-modeler-parametric) —
  trains and generates the parametric `.nam` models this plugin loads.
- [NeuralAmpModelerCoreParametric](https://github.com/phillipmself/NeuralAmpModelerCoreParametric) —
  the core inference code (vendored here as `nam_core_parametric`) that allows the plugin to run
  those parametric models.

## Download

Prebuilt builds are published on the
[Releases page](https://github.com/phillipmself/NamParametricPlugin/releases).

### macOS (universal: `arm64` + `x86_64`)

1. Download `NAM-Parametric-Plugin-<version>-macOS-VST3.zip`, `...-macOS-AU.zip`, and/or
   `...-macOS-Standalone.zip` (standalone app).
2. Unzip, then move the plugin into the matching folder (or the `.app` wherever you like):
   - VST3: `~/Library/Audio/Plug-Ins/VST3/`
   - AU: `~/Library/Audio/Plug-Ins/Components/`
3. **These builds are unsigned/unnotarized**, so macOS Gatekeeper will refuse to open them the
   first time. To allow it:
   - Right-click (or Control-click) the `.app`/`.vst3`/`.component` and choose **Open**, then
     confirm in the dialog that appears, **or**
   - Run in Terminal:
     ```bash
     xattr -cr "/path/to/NAM Parametric Plugin.app"
     xattr -cr ~/Library/Audio/Plug-Ins/VST3/"NAM Parametric Plugin.vst3"
     xattr -cr ~/Library/Audio/Plug-Ins/Components/"NAM Parametric Plugin.component"
     ```
   You only need to do this once per download.

### Windows (x64)

1. Download `NAM-Parametric-Plugin-<version>-Windows-VST3.zip` and/or
   `...-Windows-Standalone.zip`.
2. Unzip, then move the `.vst3` folder into `C:\Program Files\Common Files\VST3\` (or run the
   standalone `.exe` wherever you like).
3. **This build is unsigned**, so Windows SmartScreen may warn that it's from an unrecognized
   publisher. Click **More info**, then **Run anyway**. You only need to do this once per download.

## Models

Parametric `.nam` models are published as their own releases, separate from the plugin builds, on the
[Releases page](https://github.com/phillipmself/NamParametricPlugin/releases). Model releases use a
`models-*` tag so they version independently of the plugin.

**Latest:**
[5153 100W (Blue) — Parametric Models v1](https://github.com/phillipmself/NamParametricPlugin/releases/tag/models-5153-v1)

- `5153_100w_Blue_ConcatWaveNet_BoostInput+4dB.nam` — ConcatWaveNet capture.
- `5153_100w_Blue_HyperWaveNet_BoostInput+4dB.nam` — HyperWaveNet capture.
- `models-5153-v1.zip` — both of the above.

To use a model, download a `.nam` file and load it in the plugin (see [Usage Notes](#usage-notes)).

> **Input calibration:** These models run a little quiet on the input. Add roughly **4–6 dB of input
> gain** for them to respond like a real 5153. Future model releases will be more accurately
> input-calibrated, so this extra input gain won't be needed.

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

macOS:
- Xcode command line tools
- CMake `>= 3.22`
- Ninja
- `clang-format` (for local formatting consistency)

Windows:
- Visual Studio (Desktop development with C++ workload) or the standalone MSVC Build Tools
- Ninja
- CMake `>= 3.22`
- Run from a "Developer PowerShell/Command Prompt for VS" (or run `vcvarsall.bat`/`Enter-VsDevShell` first) so `cl.exe` is on `PATH`

## Setup

Initialize submodules (required):

```bash
git submodule update --init --recursive
```

## Build

macOS debug build:

```bash
cmake --preset mac-debug
cmake --build --preset mac-debug -j 8
```

macOS release build:

```bash
cmake --preset mac-release
cmake --build --preset mac-release -j 8
```

Windows release build:

```bash
cmake --preset windows-release
cmake --build --preset windows-release
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

VST3 / AU output paths (debug):

```text
build/mac-debug/NamParametricPlugin_artefacts/Debug/VST3/NAM Parametric Plugin.vst3
build/mac-debug/NamParametricPlugin_artefacts/Debug/AU/NAM Parametric Plugin.component
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
- Windows build is CI-built but less battle-tested than the macOS path (macOS is the primary dev environment).
