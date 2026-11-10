<h1><img src="docs/banner.png" alt="eidos"></h1>

# Eidos Danim Fork (Polskie wydanie + Wygładzanie sprite'ów DeSmuME-Style)

Fork natywnego portu PC **Eidos** (zsynchronizowany z upstream **v0.2.5** z [Project-Holon/eidos](https://github.com/Project-Holon/eidos)), stworzony specjalnie dla polskiego projektu tłumaczenia **Pokémon Czarna Wersja (Pokemon JetBlack)**.

---

### Nowości i usprawnienia w tym fork-u:

1. **Pełne spolszczenie interfejsu One-Screen UI:**
   * Wszystkie ekrany natywnego interfejsu jednoekranowego Eidosa (Magazyn Pokémon / PC Box, Drużyna, Plecak, Menu, Dialogi, Karta Trenera, Pasek skrótów klawiszy) zostały w 100% spolszczone zgodnie z oficjalną polską terminologią Pokémon (zgodność ze standardem *Brilliant Diamond / Shining Pearl*).
   * Spolszczenie nazw typów Pokémonów (`OGNISTY`, `WODNY`, `MROCZNY`, `WALCZĄCY` itd.), statusów oraz komunikatów akcji.
   * Automatyczna aktywacja polskiego interfejsu dla polskiego ROM-u (`IRBO`).

2. **Zaawansowane wygładzanie sprite'ów i tekstur (DeSmuME-Style):**
   * Eliminacja twardych, kwadratowych pikseli DS-a na sprite'ach postaci, NPC, Pokémonów oraz interfejsu.
   * **Tryb `light`:** interpolacja Hermite / Smooth Bilinear (dokładnie jak filtr *Smooth / Bilinear* w emulatorze DeSmuME) – czyste wygładzenie krawędzi bez utraty ostrości i bez rozmycia.
   * **Tryb `strong`:** rekonstrukcja Scale3x z 4-krotnym antyaliasingiem krawędzi (efekt czystej grafiki wektorowej).
   * **Świat 3D:** pełna obsługa zapętlania współrzędnych tekstur 3D (`pixel_sample_wrap`), dzięki czemu filtry działają na całym otoczeniu gry (ściany, podłogi, drogi, modele).
   * Przełączanie w locie w menu: **Settings > Display > Pixel art filter** (*none* / *light* / *strong*).

3. **Synchronizacja z Upstream v0.2.5:**
   * Zawiera wszystkie najnowsze poprawki stabilności, renderer OpenGL, obsługę motywów One-screen (Remaster, Remix battle layout), poprawki nawigacji kontrolera/klawiatury, poprawki dla Steam Decka oraz optymalizacje pamięci z głównego repozytorium.

---

A native port of Pokémon Black and White built on the [aletheia](https://github.com/Project-Holon/aletheia) decompilation. Rather than emulating the Nintendo DS, eidos compiles the decompiled C source directly to native code and loads assets from a user-supplied ROM.

Supports Windows, macOS, Linux, Android, and iOS, with experimental builds for Nintendo Switch and PlayStation Vita.

[![Discord](https://img.shields.io/badge/Discord-Join-5865F2?style=for-the-badge&logo=discord&logoColor=white)](https://discord.gg/Fpyemx4qp)

[![Windows](https://img.shields.io/badge/Windows-Download-0078D4?style=for-the-badge&logo=windows&logoColor=white)](https://github.com/Project-Holon/eidos/releases/latest/download/eidos-windows.zip)
[![macOS](https://img.shields.io/badge/macOS-Download-000000?style=for-the-badge&logo=apple&logoColor=white)](https://github.com/Project-Holon/eidos/releases/latest/download/eidos-macos.dmg)
[![Linux](https://img.shields.io/badge/Linux-Download-FCC624?style=for-the-badge&logo=linux&logoColor=black)](https://github.com/Project-Holon/eidos/releases/latest/download/eidos-linux.AppImage)
[![Android](https://img.shields.io/badge/Android-Download-3DDC84?style=for-the-badge&logo=android&logoColor=white)](https://github.com/Project-Holon/eidos/releases/latest/download/eidos-android.apk)
[![iOS](https://img.shields.io/badge/iOS-Download-147EFB?style=for-the-badge&logo=apple&logoColor=white)](https://github.com/Project-Holon/eidos/releases/latest/download/eidos-ios.ipa)

### Desktop (Windows, macOS, Linux)

The desktop launcher installs and updates eidos, manages your ROMs, and installs mods. Because builds are unsigned:

- **Windows:** Click **More info** → **Run anyway** if SmartScreen prompts you.
- **macOS (Apple Silicon):** Follow the single Terminal command in `READ ME FIRST.txt` inside the disk image to clear the quarantine attribute before opening.
- **Linux:** Mark the `.AppImage` as executable (`chmod +x`) and run it.
- **Steam Deck:** Add the launcher to Steam as a non-Steam game and start it in Game Mode, with the **Gamepad** controller layout. Desktop Mode also works: Steam then sends the controller as keyboard keys too, and eidos reads one of them at a time.

### Mobile (Android, iOS)

- **Android (8.0+):** Install the APK directly and allow installation from unknown sources if prompted.
- **iOS / iPadOS (15.0+):** Sideload the unsigned IPA using AltStore, Sideloadly, or a similar tool with your Apple ID.

### Console Homebrew (Switch, PS Vita)

Experimental builds are available on the [Releases page](https://github.com/Project-Holon/eidos/releases) for consoles running custom firmware (currently tested in emulators; see `docs/consoles.md`):

- **Nintendo Switch:** Extract `eidos-<version>-switch.zip` to the root of your SD card, place your `.nds` ROM in `sdmc:/switch/eidos/`, and launch eidos from the Homebrew Menu using title takeover (hold **R** while launching a game) for full memory access.
- **PlayStation Vita:** Install `eidos-<version>-vita.vpk` via VitaShell and copy your `.nds` ROM to `ux0:data/eidos/`. Hardware-accelerated rendering requires `libshacccg.suprx` in `ur0:data/`.

On first launch, eidos verifies the ROM in that directory and extracts the required game data.

## Supported ROMs

Pokémon Black and Pokémon White (English, French, German, Italian, and Spanish) are supported and verified against known-good dumps during setup. Japanese and Korean releases are not yet supported.

## Status & Features

The main story is playable, though crashes and visual glitches can still occur. Please report bugs via the issue tracker or Discord so they can be fixed here or upstream in aletheia.

- **Single-Screen & Dual-Screen Modes:** Play in a unified widescreen window with adapted UI layouts for the overworld, battles, menus, and C-Gear/Pokédex apps, or use the classic dual-screen DS layout.
- **Remaster Battles:** Battles take place directly in the surrounding overworld map, using hand-tuned camera placements across ~150 locations.
- **Hardware-Accelerated 3D:** High-resolution internal rendering with fog, edge marking, and anti-aliasing. Uses OpenGL by default, with native backends for Direct3D 12, Metal, and Vulkan.
- **Display & Speed Controls:** Native refresh-rate VSync, instant text, and fast-forward (`Tab` for 2× speed, `Left Shift + Tab` for uncapped).
- **Pixel Art Filtering:** Optional magnification filter (None, Light / Smooth Bilinear, Strong / Scale3x) in Settings > Display.
- **Input & Settings:** Remappable keyboard, gamepad, and touch controls, with port settings integrated directly into the in-game Options menu.
- **Mod Support:** Built-in mod loader supporting asset overrides, rule tweaks, and code mods (`docs/modding.md`), browsable from the launcher's mod index.
- **Multiplayer & WFC (Experimental):** Co-op overworld presence, online Union Room trades and battles, and Mystery Gift distribution via a custom relay server.
- **Spot Editor:** Tooling for authoring and previewing battle camera placements (`tools/spot-editor`).

## Milestones

See the [milestones page](https://github.com/Project-Holon/eidos/milestones) for what is planned for each version and what is in progress.

## Controls

Default keyboard controls match the DS layout:

| DS | Key | DS | Key |
|---|---|---|---|
| D-Pad Up | Up arrow | D-Pad Down | Down arrow |
| D-Pad Left | Left arrow | D-Pad Right | Right arrow |
| A | X | B | Z |
| X | S | Y | A |
| L | Q | R | E |
| START | Enter | SELECT | Right Shift |
| Touch | Left click | | |

**Shortcuts:** `Space` or `Tab` (fast forward, pad's left trigger does the same), `Alt + Return` (fullscreen), `F12` (screenshot).

Save files are stored in:
- **Windows:** `%LOCALAPPDATA%\eidos\saves`
- **macOS:** `~/Library/Application Support/eidos/saves`
- **Linux:** `~/.local/share/eidos/saves`
- **Android:** Internal app storage (`files/saves`)
- **iOS:** App Documents directory
- **Switch:** `sdmc:/switch/eidos/saves`
- **PS Vita:** `ux0:data/eidos/saves`

## Community

- **Discord:** [Join the server](https://discord.gg/Fpyemx4qp)
- **Translations:** [Translate eidos](https://crowdin.com/project/eidos)
