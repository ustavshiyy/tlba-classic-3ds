# LBA Classic 3DS

A Nintendo 3DS port of **Little Big Adventure / Relentless: Twinsen's Adventure**, built on top of the ScummVM **TwinE** engine and adapted specifically for the dual-screen 3DS interface.

> **Current stable milestone: v0.1.0**

The goal is not just to make LBA run on a 3DS, but to make it feel like a native dual-screen version of the game: gameplay on the upper screen, useful game UI on the touch screen, 3DS-friendly controls, save profiles, inventory, Holomap and proper pause behaviour.

---

## Highlights

- Standalone Little Big Adventure 3DS build
- Custom dual-screen UI
- Persistent lower-screen HUD
- Touch-controlled behaviour selector
- Inventory on the lower screen
- Holomap on the lower screen
- Custom 3-slot save/profile selector
- Copy and delete save profiles
- LBA-style fonts and UI
- Life, magic, money, keys and clovers on the touch screen
- Magic bar grows with Twinsen's actual magic level
- `PAUSE` overlay on the upper screen while menus, inventory or Holomap are open
- Touch / button movie skipping
- Improved 3DS HOME / application-exit handling

---

## Controls

### Gameplay

| 3DS control | Action |
|---|---|
| **Circle Pad** | Move / turn Twinsen |
| **D-Pad** | Move / turn Twinsen |
| **A** | Current behaviour action — interact / jump / attack / hide depending on Twinsen's behaviour |
| **Y** | Open / change behaviour |
| **L** | Context / special action, such as talking, searching or interacting |
| **R** | Hold to use the Magic Ball / current weapon |
| **X** | Center the camera on Twinsen |
| **SELECT** | Open inventory |
| **START** | Open the in-game menu |
| **Touch screen** | Use the lower-screen HUD, behaviour selector, inventory, Holomap and save UI |

### Menus and dialogue

| 3DS control | Action |
|---|---|
| **A** | Confirm / select |
| **B** | Back / cancel |
| **D-Pad** | Navigate |
| **D-Pad Right** | Advance dialogue text |
| **Touch screen** | Select touch-screen UI elements |

### Movies

| 3DS control | Action |
|---|---|
| **B** | Skip movie |
| **Touch screen** | Skip movie |

### Holomap

The Holomap is displayed on the lower screen while the upper screen keeps the current game scene paused.

Use the **D-Pad** and **touch screen** to navigate it. Press **B** to go back.

---

## Installing on a Nintendo 3DS

A homebrew-capable Nintendo 3DS is required.

### 1. Copy the 3DS application

Create this folder on the SD card:

```text
/3ds/lba-classic-3ds/
```

Copy the release build into it:

```text
/3ds/lba-classic-3ds/lba-classic-3ds.3dsx
```

Recommended layout:

```text
SD:/
└── 3ds/
    └── lba-classic-3ds/
        ├── lba-classic-3ds.3dsx
        └── game/
```

---

## Steam game data

This project does **not** contain the original Little Big Adventure game assets. You must own the game and copy the data from your own installation.

The current stable build is primarily developed and tested with the **Classic Steam version** detected by `TLBA1C.exe`.

### Finding the Steam files

In Steam:

1. Open your Library.
2. Right-click **Little Big Adventure / Twinsen's Little Big Adventure Classic**.
3. Choose **Manage → Browse local files**.
4. Find the Classic game directory containing:

```text
TLBA1C.exe
```

### Recommended method

Copy the **entire contents of the directory containing `TLBA1C.exe`** to:

```text
SD:/3ds/lba-classic-3ds/game/
```

This is strongly recommended. It avoids missing music, speech or movie resources when Steam changes the package layout.

The resulting directory should look roughly like this:

```text
SD:/3ds/lba-classic-3ds/game/
├── TLBA1C.exe
├── TLBA1C_640_480_256.bmp
├── RESS.HQR
├── TEXT.HQR
├── SAMPLES.HQR
├── LBA_GRI.HQR
├── LBA_BLL.HQR
├── LBA_BRK.HQR
├── SCENE.HQR
├── SPRITES.HQR
├── FILE3D.HQR
├── BODY.HQR
├── ANIM.HQR
├── ANIM3DS.HQR
├── INVOBJ.HQR
├── FLA_GIF.HQR
├── FLASAMP.HQR
├── ...voice files...
└── ...music files...
```

Depending on the Steam release, filename case and the exact set of auxiliary files may differ.

### Important audio files

For full music and speech, also keep the original Steam audio files.

Music is loaded from files whose base names look like:

```text
lba1-01
lba1-02
...
lba1-32
```

Keep the extensions supplied by Steam.

Speech uses the original language voice banks, for example English files based on names such as:

```text
ensys.vox
encre.vox
engam.vox
en000.vox
en001.vox
...
en011.vox
```

Other voice languages use the corresponding language prefix.

**The safest rule is simple: copy the whole Classic game directory, not a hand-picked subset.**

---

## First launch

1. Insert the SD card into the 3DS.
2. Start **Homebrew Launcher**.
3. Launch **LBA Classic 3DS**.

On the first launch, if the game has not been configured yet, the ScummVM launcher will appear.

Choose **Add Game** and browse to:

```text
sdmc:/3ds/lba-classic-3ds/game/
```

The game should be detected as a Little Big Adventure Classic / Steam version.

Add it and start the game.

After the game target has been configured once, LBA Classic 3DS automatically launches the LBA target on subsequent starts.

### Force the launcher

To open the ScummVM launcher again instead of auto-starting LBA:

**Hold SELECT while launching the `.3dsx` application.**

Once the game is running, SELECT returns to its normal LBA Classic 3DS function: **Inventory**.

---

## Saves and configuration

The 3DS backend currently stores configuration here:

```text
SD:/3ds/scummvm/scummvm.ini
```

Save files are stored here:

```text
SD:/3ds/scummvm/saves/
```

Do not delete those directories when replacing the `.3dsx` with a newer build if you want to keep your configuration and saves.

---

## Dual-screen interface

The lower screen is used as an actual LBA interface rather than simply mirroring the game.

During normal gameplay it shows:

- Life
- Magic
- Money
- Keys
- Clover leaves
- Twinsen's four behaviour modes
- Inventory / Holomap shortcuts

The magic meter reflects the current magic level and grows as Twinsen acquires stronger magic.

---

## Save profiles

The standalone 3DS frontend includes a custom LBA-style save selector with:

- Three player profiles
- Player-name entry
- Copy save
- Delete save
- Original-style animated plasma selection

---

## Movies

Movies are shown in their original **4:3 aspect ratio**, centered on the 3DS upper display instead of being stretched to the full 5:3 panel.

Movies may be skipped using **B** or the touch screen.

---

## Old 3DS / 2DS compatibility

New Nintendo 3DS is the primary development and testing platform.

The control scheme intentionally avoids requiring **ZL/ZR**, so the current layout is suitable for the original 3DS / 3DS XL / 2DS family as well.

However, performance and stability on older hardware still need broader testing. Treat Old 3DS / 2DS support as **experimental** for now.

---

## Repository layout

This repository contains the parts of ScummVM modified for LBA Classic 3DS rather than unrelated ScummVM engines.

```text
backends/
└── platform/
    └── 3ds/

base/
└── main.cpp

engines/
├── engine.cpp
└── twine/
```

Other ScummVM engines are intentionally not included.

---

## Building

This repository is intended to be used with a compatible ScummVM source tree and a Nintendo 3DS development environment based on **devkitPro / devkitARM / libctru**.

Overlay the files from this repository onto the corresponding paths in the ScummVM source tree.

For the configured standalone tree:

```bash
make -j4 lba-classic-3ds.3dsx
```

The resulting application is:

```text
lba-classic-3ds.3dsx
```

For clean rebuilds after backend/header changes:

```bash
make clean
make -j4 lba-classic-3ds.3dsx
```

---

## Game data and copyright

Original Little Big Adventure game data is **not included** in this repository or in release source archives.

You must provide the data from a legally owned copy of the game.

Little Big Adventure and its original assets belong to their respective copyright holders.

---

## Credits

- **Adeline Software International** — original Little Big Adventure
- **ScummVM project**
- **TwinE engine contributors**
- **2point21 / lba1-classic** — reference source for original LBA Classic behaviour
- **devkitPro / libctru** — Nintendo 3DS homebrew toolchain

---

## License

Code derived from ScummVM is distributed under the applicable ScummVM license.

See [`COPYING`](COPYING) for details.

---

## Status

### v0.1.0 Stable

First stable checkpoint of the standalone 3DS port.

The current baseline includes the custom dual-screen HUD, touch interface, save/profile system, inventory, Holomap, 4:3 movies, pause overlay, 3DS controls and stable scene-transition behaviour working together as one release.
