# MONOLITHE

Boîte à rythmes logicielle à 8 voix, au son lo-fi, pilotée par le MIDI de votre logiciel de musique.
Version plugin du projet matériel MONOLITHE. Gratuit et open source (AGPLv3).

> **État : squelette (v0.1 en cours).** Une note C1 joue un son de test. Les vraies voix arrivent.

## Les voix

| Note | Voix |
|---|---|
| C1 | 01 GRAVE |
| C#1 | 02 PEAU |
| D1 | 03 BRUIT |
| D#1 | 04 MÉTAL |
| E1 | 05 BOIS |
| F1 | 06 SOUFFLE |
| F#1 | 07 CLOCHE |
| G1 | 08 DRONE |

La disposition est celle d'un Drum Rack d'Ableton : un pad par voix, à partir de C1.

## Installation

Téléchargez la dernière version dans l'onglet **Releases** (ou dans les artefacts de l'onglet **Actions**).

- **Windows** : décompressez `MONOLITHE.vst3` dans `C:\Program Files\Common Files\VST3\`.
- **macOS** : décompressez `MONOLITHE.vst3` dans `~/Library/Audio/Plug-Ins/VST3/`
  (ou `MONOLITHE.component` dans `~/Library/Audio/Plug-Ins/Components/`).
  Le plugin n'est pas signé par Apple : si macOS le bloque, ouvrez un Terminal et lancez
  `xattr -dr com.apple.quarantine ~/Library/Audio/Plug-Ins/VST3/MONOLITHE.vst3`.

Relancez ensuite l'analyse des plugins dans votre logiciel.

## Compiler

Prérequis : CMake 3.22+, et Visual Studio 2022 (Windows) ou Xcode (macOS).

```
git clone --recursive https://github.com/<compte>/monolithe-vst.git
cd monolithe-vst
cmake -B build
cmake --build build --config Release
```

Les plugins sont dans `build/Monolithe_artefacts/Release/`.

## Licence

AGPLv3, voir [LICENSE](LICENSE). Construit avec [JUCE](https://juce.com) (AGPLv3) et le SDK VST3 de Steinberg (MIT).
