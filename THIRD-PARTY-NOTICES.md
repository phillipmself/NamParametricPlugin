# Third-Party Notices

This project (NAM Parametric Plugin) is licensed under the MIT License (see
[LICENSE](LICENSE)). It bundles or links against the third-party software
listed below, each under its own license.

## JUCE (framework)

- Location: `external/JUCE`
- Version: 8.0.11
- License: Dual-licensed [AGPLv3](https://www.gnu.org/licenses/agpl-3.0.en.html)
  or [commercial JUCE licence](https://juce.com/legal/juce-8-licence/)
- Copyright: JUCE / Raw Material Software Limited

This project is built against JUCE without a commercial JUCE licence, so the
compiled plugin as a whole is bound by the terms of the AGPLv3 for the
JUCE-derived portions, in addition to this project's own MIT licence for its
original code.

## VST3 SDK (bundled within JUCE)

- Location: `external/JUCE/modules/juce_audio_processors_headless/format_types/VST3_SDK`
- License: MIT License
- Copyright (c) 2025, Steinberg Media Technologies GmbH

## AudioDSPTools

- Location: `external/AudioDSPTools`
- License: MIT License
- Copyright (c) 2023 Steven Atkinson

## NeuralAmpModelerCore (parametric fork)

- Location: `external/nam_core_parametric`
- Source: https://github.com/phillipmself/NeuralAmpModelerCoreParametric (branch `feature/parametric-main`)
- License: MIT License
- Copyright (c) 2023 Steven Atkinson
- Copyright (c) 2026 Phillip Self (parametric fork)

## Eigen

- Location: `external/nam_core_parametric/Dependencies/eigen`
- License: [Mozilla Public License 2.0](https://www.mozilla.org/en-US/MPL/2.0/)
- This project is built with `EIGEN_MPL2_ONLY` defined, restricting usage to
  the MPL2-licensed subset of Eigen (excluding LGPL-only modules).
- Copyright: Eigen contributors

## nlohmann/json

- Location: `external/nam_core_parametric/Dependencies/nlohmann`
- License: MIT License
- Copyright (c) 2013-2025 Niels Lohmann
