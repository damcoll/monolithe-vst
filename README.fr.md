# MONOLITHE

[English](README.md) · **Français**

Une boîte à rythmes lo-fi à 8 voix, en plugin, jouée par le MIDI de votre logiciel de musique.
Gratuite et open source (AGPLv3). VST3 pour Windows et macOS, AU pour macOS.

![Façade de MONOLITHE](docs/facade.png)

MONOLITHE est né comme une boîte à rythmes analogique matérielle. Ce plugin en est la version logicielle :
huit voix librement inspirées des circuits du projet matériel (résonateurs en T ponté, métal façon 808, bruit),
une façade sans menus, et une section MACHINE qui lui donne le son d'une vieille machine un peu fatiguée.

## Les voix

Chaque voix répond à une note MIDI, à partir de **C1 (note MIDI 36)**, comme un Drum Rack d'Ableton.

| Note | Voix | Son | ACCORD | DURÉE |
|---|---|---|---|---|
| C1 (36) | **01 GRAVE** | Résonateur grave frappé, avec une chute de hauteur, saturé | 36 – 87 Hz | 0,15 – 1,6 s |
| C#1 (37) | **02 PEAU** | Peau de caisse : deux modes de corps et un claquement de bruit | 120 – 260 Hz | 50 – 250 ms |
| D1 (38) | **03 BRUIT** | Bruit dans un filtre accordé (shaker, charleston) | 500 Hz – 9 kHz | 30 – 600 ms |
| D#1 (39) | **04 MÉTAL** | Six ondes carrées aux rapports de la cymbale 808 | une octave en dessous / au-dessus | 30 – 900 ms |
| E1 (40) | **05 BOIS** | « Toc » de bois, court | 400 Hz – 2,5 kHz | 20 – 250 ms |
| F1 (41) | **06 SOUFFLE** | Souffle filtré qui monte puis s'éteint | sourd – brillant | 0,2 – 3 s |
| F#1 (42) | **07 CLOCHE** | Deux oscillateurs à la quinte, façon cloche de 808 | 270 Hz – 1,1 kHz | 80 ms – 1,2 s |
| G1 (43) | **08 DRONE** | Deux oscillateurs qui battent lentement, tenus tant que la note est tenue | 40 – 160 Hz | relâchement 0,1 – 4 s |

Certains logiciels appellent la note MIDI 36 « C2 » au lieu de « C1 » : c'est le numéro de note qui compte.

## Les commandes

**Par voix**
- **NIVEAU** : volume de la voix.
- **ACCORD** : hauteur (voir le tableau ci-dessus).
- **DURÉE** : longueur du son (temps de relâchement pour DRONE).
- **MUTE** : coupe la voix.
- **Pad** : cliquez dessus pour écouter la voix. Les pads s'allument en ambre quand leur voix sonne.

**MACHINE**
- **DÉRIVE** : l'instabilité analogique. Chaque coup sort un peu différent (hauteur, longueur, force), et chaque voix se désaccorde lentement puis revient, comme des circuits qui chauffent. 0 = parfaitement régulier.
- **POUSSIÈRE** : le lo-fi. Moins de bits (jusqu'à 8), fréquence d'échantillonnage réduite (jusqu'à 6 kHz), son plus sombre, souffle de bande et craquements de vinyle. 0 = son propre.
- **ACCENT** : l'importance de la vélocité MIDI. 0 = tous les coups à pleine force, 1 = pleinement dynamique.

**Sortie**
- **MASTER** : volume de sortie. Une saturation douce évite que la somme des voix écrête.
- Le **vu-mètre** suit la sortie et affiche la dernière voix jouée.

Toutes les commandes sont automatisables. Boutons : glisser vers le haut ou le bas, ou molette ; double-clic pour revenir
à la valeur par défaut. La fenêtre se redimensionne de 75 % à 200 % (coin en bas à droite).

## Installation

Téléchargez la dernière version sur la page [**Releases**](../../releases).

**Windows** : décompressez et copiez `MONOLITHE.vst3` dans `C:\Program Files\Common Files\VST3\`
(créez le dossier s'il n'existe pas ; Windows demandera les droits d'administrateur).

**macOS** : décompressez et copiez :
- `MONOLITHE.vst3` dans `~/Library/Audio/Plug-Ins/VST3/`
- `MONOLITHE.component` (AU) dans `~/Library/Audio/Plug-Ins/Components/`

Le plugin n'est pas signé par Apple. Si macOS le bloque, ouvrez le Terminal et lancez :

```
xattr -dr com.apple.quarantine ~/Library/Audio/Plug-Ins/VST3/MONOLITHE.vst3
xattr -dr com.apple.quarantine ~/Library/Audio/Plug-Ins/Components/MONOLITHE.component
```

> ⚠️ **La version macOS n'a pas encore été testée sur un vrai Mac.** Elle est compilée et vérifiée automatiquement
> (pluginval et auval d'Apple), mais personne ne l'a encore jouée dans un logiciel de musique. Vos retours sont les
> bienvenus : ouvrez une [issue](../../issues).

Relancez ensuite l'analyse des plugins dans votre logiciel. MONOLITHE apparaît comme un instrument (catégorie *Drum*).

## Astuce pour Ableton Live 11 : des noms de percussions au lieu d'un piano

Ableton n'affiche des noms de percussions dans l'éditeur MIDI que pour son propre Drum Rack. Pour voir « 01 GRAVE »,
« 02 PEAU »… au lieu des touches de piano :

1. Créez une piste MIDI nommée par exemple *MONOLITHE notes*, avec un **Drum Rack** vide.
2. Sur les pads C1 à G1 : déposez n'importe quel son, renommez la chaîne (01 GRAVE, 02 PEAU, 03 BRUIT, 04 MÉTAL,
   05 BOIS, 06 SOUFFLE, 07 CLOCHE, 08 DRONE), puis supprimez le son. La chaîne vide garde son nom.
3. Enregistrez ce Drum Rack comme preset pour le réutiliser.
4. Sur la piste de MONOLITHE, réglez **MIDI From** sur *MONOLITHE notes*, puis **Pre FX**, et le monitoring sur **In**.
5. Écrivez vos clips sur la piste *MONOLITHE notes*, avec **Fold** activé.

Les logiciels qui lisent les noms de notes VST3 (FL Studio, Cubase…) affichent directement le nom des voix.

## Compiler depuis le code source

Prérequis : CMake 3.22 ou plus, et Visual Studio 2022 (Windows) ou Xcode (macOS).

```
git clone --recursive https://github.com/damcoll/monolithe-vst.git
cd monolithe-vst
cmake -B build
cmake --build build --config Release
```

Les plugins sont dans `build/Monolithe_artefacts/Release/`. Trois petits outils sont aussi compilés :
- `MonolitheRender` : un .wav par voix, pour régler les sons sans logiciel de musique ;
- `MonolitheDemo` : un motif joué par tout le plugin à plusieurs réglages de DÉRIVE et POUSSIÈRE ;
- `MonolitheSnapshot` : une image PNG de la fenêtre du plugin.

## Limites connues

- Son mono (le même signal sur les deux sorties), pas de panoramique par voix.
- Pas de presets ni de séquenceur intégré : le rythme vient de votre logiciel de musique.

## Licence et crédits

MONOLITHE est publié sous licence **GNU AGPLv3**, voir [LICENSE](LICENSE).

- Construit avec [JUCE](https://juce.com) (AGPLv3) et le SDK VST3 de Steinberg (MIT).
- Polices : [Big Shoulders](https://github.com/xotypeco/big_shoulders) et [IBM Plex Mono](https://github.com/IBM/plex),
  sous SIL Open Font License (voir `assets/fonts`).
- Plugins vérifiés avec [pluginval](https://github.com/Tracktion/pluginval) de Tracktion.
