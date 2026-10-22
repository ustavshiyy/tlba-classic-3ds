# LBA Classic 3DS

A Nintendo 3DS port of **Little Big Adventure / Relentless: Twinsen's Adventure**, built on top of the ScummVM **TwinE** engine and adapted specifically for the dual-screen Nintendo 3DS interface.

> **Current stable release: v0.2.0**

LBA Classic 3DS v0.2.0 can be installed directly to the Nintendo 3DS HOME Menu as a **CIA**, or launched through the Homebrew Launcher as a **3DSX**.

The ScummVM launcher is completely hidden: when valid game data is present, LBA Classic 3DS starts directly into its own save/profile screen.

---

# Installation

## Recommended: CIA installation

### Requirements

You need:

- a homebrew-enabled Nintendo 3DS
- FBI or another compatible CIA installer
- a legally owned copy of **Little Big Adventure Classic**
- either the **Steam Classic** or **GOG Classic** version

The original game data is **not included** with LBA Classic 3DS.

---

### Step 1 — Install the CIA

Download:

```text
lba-classic-3ds-v0.2.0.cia
```

Copy it anywhere convenient on your SD card, for example:

```text
SD:/cias/lba-classic-3ds-v0.2.0.cia
```

Open **FBI** and install the CIA.

For example:

```text
FBI
→ SD
→ cias
→ lba-classic-3ds-v0.2.0.cia
→ Install and delete CIA
```

After installation, **LBA Classic 3DS** will appear directly on the Nintendo 3DS HOME Menu.

---

### Step 2 — Create the game-data folder

On the SD card create:

```text
SD:/3ds/lba-classic-3ds/game/
```

The final directory structure should look like this:

```text
SD:/
├── cias/
│   └── lba-classic-3ds-v0.2.0.cia
│
└── 3ds/
    └── lba-classic-3ds/
        └── game/
            ├── TLBA1C.exe
            ├── Common/
            │   └── ...
            └── CommonClassic/
                └── ...
```

The installed CIA itself does **not** need to be inside `/3ds/lba-classic-3ds/`.

Only the original LBA game files go there.

---

### Step 3 — Copy the original LBA Classic files

Copy the contents of your legally owned **Steam Classic** or **GOG Classic** installation into:

```text
SD:/3ds/lba-classic-3ds/game/
```

The important point is that this file must exist directly inside that folder:

```text
SD:/3ds/lba-classic-3ds/game/TLBA1C.exe
```

Do **not** add another unnecessary directory level.

Correct:

```text
game/
├── TLBA1C.exe
├── Common/
└── CommonClassic/
```

Wrong:

```text
game/
└── Little Big Adventure/
    ├── TLBA1C.exe
    ├── Common/
    └── CommonClassic/
```

The safest method is to copy the **entire Classic game directory** containing `TLBA1C.exe`.

This avoids missing movies, music, speech or other resources.

---

### Steam version

In Steam:

1. Open your Library.
2. Find **Little Big Adventure / Twinsen's Little Big Adventure Classic**.
3. Right-click the game.
4. Choose **Manage → Browse local files**.
5. Find the Classic game directory containing:

```text
TLBA1C.exe
```

Copy the complete contents of that directory to:

```text
SD:/3ds/lba-classic-3ds/game/
```

A current Steam Classic installation commonly looks roughly like:

```text
game/
├── TLBA1C.exe
├── Common/
└── CommonClassic/
```

The exact auxiliary files may vary between releases.

---

### GOG version

The GOG Classic version is also supported.

Find the GOG game directory containing:

```text
TLBA1C.exe
```

and copy the complete Classic game directory contents to:

```text
SD:/3ds/lba-classic-3ds/game/
```

Again, `TLBA1C.exe` must be directly inside the `game` folder.

---

### Step 4 — Launch the game

Start **LBA Classic 3DS** directly from the Nintendo 3DS HOME Menu.

If supported game data is present:

```text
HOME Menu
→ LBA Classic 3DS
→ LBA save/profile selector
→ Game
```

There is **no visible ScummVM launcher**.

If the game files cannot be found, LBA Classic 3DS displays its own:

```text
GAME DATA NOT FOUND
```

screen with the expected SD-card path.

---

## Updating from an older CIA build

LBA Classic 3DS v0.2.0 uses:

```text
Title:         LBA Classic 3DS
Publisher:     ltlo
Product Code:  CTR-P-LBAC
Title Version: 32 (0.2.0)
```

The project keeps the same internal title identity across the v0.2.0 test/stable builds.

Normally, install the newer CIA using FBI.

If FBI refuses to install the stable build over an older test CIA because the test build has a higher internal version number, uninstall the old **LBA Classic 3DS** CIA first and then install v0.2.0.

Your LBA save files are stored separately under:

```text
SD:/3ds/scummvm/saves/
```

so uninstalling the CIA does not normally remove those save files.

Still, making a backup before updating is recommended.

---

# 3DSX installation

If you prefer the Homebrew Launcher version, download:

```text
lba-classic-3ds-v0.2.0.3dsx
```

Create:

```text
SD:/3ds/lba-classic-3ds/
```

Copy the `.3dsx` as:

```text
SD:/3ds/lba-classic-3ds/lba-classic-3ds.3dsx
```

The complete layout should be:

```text
SD:/
└── 3ds/
    └── lba-classic-3ds/
        ├── lba-classic-3ds.3dsx
        └── game/
            ├── TLBA1C.exe
            ├── Common/
            └── CommonClassic/
```

Launch it from the Homebrew Launcher.

Just like the CIA build, the 3DSX version starts LBA directly without showing the ScummVM launcher.

---

# Updating from v0.1.x

Existing **v0.1.x** saves remain compatible with **v0.2.0**.

Configuration is currently stored at:

```text
SD:/3ds/scummvm/scummvm.ini
```

Save files are stored at:

```text
SD:/3ds/scummvm/saves/
```

Do not delete those directories if you want to preserve your saves.

---

# Features

- Native installable **CIA**
- `.3dsx` Homebrew Launcher build
- Direct LBA startup with no visible ScummVM launcher
- Automatic Steam / GOG Classic detection
- Custom `GAME DATA NOT FOUND` screen
- Full Circle Pad movement instead of the original tank-style controls
- Custom dual-screen interface
- Persistent lower-screen HUD
- Touch-controlled behaviour selector
- Inventory on the lower screen
- Holomap on the lower screen
- Custom 3-slot player/save selector
- Copy and delete save profiles
- Original-style animated plasma selection
- LBA-style fonts and UI
- Life, magic, money, keys and clovers on the lower screen
- Magic bar grows with Twinsen's actual magic level
- `PAUSE` overlay on the upper screen
- Original 4:3 movie presentation
- Touch / button movie skipping
- Correct lower-screen restoration after HOME Menu resume
- Correct lower-screen restoration after sleep / wake
- Improved HOME / application-exit handling
- No dependency on New 3DS-only ZL/ZR buttons
- ScummVM achievement popups hidden during gameplay
- Final sequence adapted for the dual-screen layout
- Full-game completion confirmed on real New Nintendo 3DS hardware

---

# Controls

## Gameplay

| 3DS control | Action |
|---|---|
| **Circle Pad** | Direct analog movement instead of the original tank-style controls |
| **D-Pad** | Move / turn Twinsen |
| **A** | Current behaviour action — interact / jump / attack / hide depending on Twinsen's behaviour |
| **Y** | Open / change behaviour |
| **L** | Context / special action, such as talking, searching or interacting |
| **R** | Hold to use the Magic Ball / current weapon |
| **X** | Center the camera on Twinsen |
| **SELECT** | Open inventory |
| **START** | Open the in-game menu |
| **Touch screen** | Lower-screen HUD, behaviour selector, inventory, Holomap and save UI |

## Menus and dialogue

| 3DS control | Action |
|---|---|
| **A** | Confirm / select |
| **B** | Back / cancel |
| **D-Pad** | Navigate |
| **D-Pad Right** | Advance dialogue text |
| **Touch screen** | Select lower-screen UI elements |

## Movies

| 3DS control | Action |
|---|---|
| **B** | Skip movie |
| **Touch screen** | Skip movie |

## Holomap

The Holomap is displayed on the lower screen while the current game scene remains paused on the upper screen.

Use the **D-Pad** and **touch screen** to navigate it.

Press **B** to return.

---

# Dual-screen interface

During gameplay the lower screen displays:

- Life
- Magic
- Money
- Keys
- Clover leaves
- Twinsen's four behaviour modes
- Inventory / Holomap shortcuts

The magic meter reflects Twinsen's actual current magic level and grows as stronger magic is acquired.

When Inventory, Holomap or the game menu is open, the upper game view is darkened and displays:

```text
PAUSE
```

The active lower-screen interface keeps its normal palette and brightness.

The HUD is restored automatically after returning from the HOME Menu or waking the console from sleep.

---

# Save profiles

The 3DS frontend includes a custom LBA-style save/profile selector with:

- Three player profiles
- Player-name entry
- Copy save
- Delete save
- Original-style animated plasma selection

---

# Movies

Movies are displayed in their original **4:3 aspect ratio**, centered on the upper 400×240 screen rather than stretched to 5:3.

Movies can be skipped using **B** or the touch screen.

The lower screen stays clean during the adapted loading/movie transitions.

---

# Final sequence

The lower screen is intentionally disabled after normal gameplay ends and the final non-interactive sequence begins.

Menus, inventory and saving are blocked during this sequence to prevent invalid end-game save states.

The last valid normal-gameplay save remains available.

---

# HOME Menu presentation

The v0.2.0 CIA includes:

- custom application icon
- custom HOME Menu banner
- short banner audio jingle

Metadata:

```text
Title:         LBA Classic 3DS
Description:   Little Big Adventure Classic
Publisher:     ltlo
Product Code:  CTR-P-LBAC
Title Version: 32 (0.2.0)
```

---

# Old 3DS / 2DS compatibility

**New Nintendo 3DS** is the primary development and testing platform.

The controls intentionally avoid requiring **ZL/ZR**, so the layout is suitable for the original Nintendo 3DS / Nintendo 3DS XL / Nintendo 2DS family.

However, performance and stability on older hardware still need broader testing.

Old 3DS / 2DS support should currently be considered **experimental**.

---

# Repository layout

This repository contains only the parts of ScummVM modified for LBA Classic 3DS rather than unrelated ScummVM engines.

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

# Building

The project is intended to be used with a compatible ScummVM source tree and a Nintendo 3DS development environment based on:

- devkitPro
- devkitARM
- libctru

For CIA creation you also need:

- `makerom`
- `bannertool`

Overlay the modified repository files onto the corresponding paths of the compatible ScummVM source tree.

## Build 3DSX

```bash
make -j4 lba-classic-3ds.3dsx
```

## Build CIA

```bash
make -j4 lba-classic-3ds.cia
```

## Clean build

After backend/header changes:

```bash
make clean
make -j4 lba-classic-3ds.cia lba-classic-3ds.3dsx
```

---

# Game data and copyright

Original Little Big Adventure game data is **not included** in this repository or in the source release archives.

You must provide the game data from a legally owned copy of **Little Big Adventure Classic**.

Little Big Adventure and its original assets belong to their respective copyright holders.

---

# Credits

- **Adeline Software International** — original Little Big Adventure
- **ScummVM project**
- **TwinE engine contributors**
- **2point21 / lba1-classic** — reference source for original LBA Classic behaviour
- **devkitPro / libctru** — Nintendo 3DS homebrew toolchain
- **ltlo** — LBA Classic 3DS port

---

# License

Code derived from ScummVM is distributed under the applicable ScummVM license.

See [`COPYING`](COPYING) for details.

---

# Status

## v0.2.0 Stable

✅ Installable CIA  
✅ 3DSX build  
✅ Direct standalone startup  
✅ No visible ScummVM launcher  
✅ Steam / GOG Classic detection  
✅ Custom missing-data screen  
✅ Dual-screen HUD  
✅ Circle Pad direct movement  
✅ Inventory and Holomap integration  
✅ Custom save/profile system  
✅ 4:3 movies  
✅ HOME Menu / sleep resume fixes  
✅ Full game completed from beginning to end on real New Nintendo 3DS hardware  

⚠️ Old Nintendo 3DS / 2DS compatibility remains experimental.
