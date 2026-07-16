# Minecraft Bedrock cheat client (movement pass)

A DLL-injection cheat client for **Minecraft Bedrock for Windows**, built for use on your
own server / singleplayer worlds. First pass covers the movement category: Flight, Speed,
NoFall, NoClip, Spider. Combat, visuals/ESP, and misc/utility modules can be added the same
way once this pass is validated.

Ships as a **single executable**, `BedrockCheat.exe` — the cheat DLL and a default config
are embedded as resources and self-extract at runtime. Nothing else needs to be distributed.

## How it's structured

- `injector/` — the loader. Embeds `cheat.dll` and `config/offsets.json` (built by `cheat/`)
  as Win32 resources at build time (see `injector/CMakeLists.txt`). At runtime it extracts
  the DLL to a per-run temp folder, grants that folder's DLL AppContainer read/execute
  access, and injects it into `Minecraft.Windows.exe` via `CreateRemoteThread` +
  `LoadLibraryW`.
- `cheat/` — the DLL, unchanged by the single-exe packaging:
  - `src/core/` — MinHook wrapper (`Hook`), AOB pattern scanner (`PatternScanner`),
    JSON-backed offset table (`Offsets`), SEH-guarded memory read/write (`GameMemory`),
    writable-memory region enumeration (`MemoryRegions`), module base class + registry
    (`Module`, `ModuleManager`), keybind polling (`InputManager`).
  - `src/gui/` — generic D3D11 swapchain `Present` hook (`Overlay`) that draws an ImGui
    click-GUI (`ClickGui`) listing every module with a checkbox. Insert toggles the GUI.
  - `src/modules/movement/` — Flight (F1), Speed (F2), NoFall (GUI-only), NoClip (F3),
    Spider (F4).
  - `src/tools/` — the offset auto-finder (below): `IncrementalScanner` (Cheat-Engine-style
    value narrowing), `Vec3Utils` (groups surviving floats into position/velocity-shaped
    triplets), `PointerScanner` (BFS pointer-chain search + cross-respawn validation),
    `OffsetWizard` (the guided panel tying it together, F5).
- `config/offsets.json` — the only place game-version-specific addresses live, and the
  template embedded into the exe as the first-run default.

## Finding offsets: the in-game auto-finder (F5)

Press **F5** in-game to open the Offset Finder. It can't run unattended — there's no way for
code to know which memory address is "position" without watching what changes while you
physically move — but it automates the rest of what you'd otherwise do by hand in Cheat
Engine:

1. **Position**: it snapshots all plausible float values in the game's writable memory, then
   you walk for a second and click Narrow, repeating a few times until only a handful of
   candidates remain. You then click the one matching your F3 coordinates.
2. **Velocity** / **on-ground**: same idea, scoped to a small window around the position
   address you just confirmed — move again for velocity, jump a few times for on-ground.
3. **Pointer chain**: it searches the game module's static memory for a chain of pointers
   that resolves to your position address (an automated version of Cheat Engine's pointer
   scan), so the offset keeps working across relaunches rather than just this session.
4. If more than one chain candidate survives, it asks you to die/respawn (or relog) and type
   your new coordinates in, then keeps only the chains that still resolve correctly —
   coincidental matches won't.
5. On success it writes straight into `offsets.json` next to `BedrockCheat.exe` and applies
   immediately, no restart needed.

This is the most complex part of the codebase and the part I could least verify without a
live Windows + Minecraft process to test against — if a step misbehaves (finds zero
candidates, times out, etc.), the manual path below still works as a fallback.

### Manual fallback

1. Attach a disassembler/debugger (x64dbg, IDA, Ghidra) to a running `Minecraft.Windows.exe`.
2. Find the local player object — typically via a static/global pointer chain from the
   client instance. `core::PatternScanner::Find("48 8B 05 ? ? ? ?")`-style AOB scans (see
   `cheat/src/core/PatternScanner.h`) are the tool for relocating a known signature after an
   update, once you've identified one.
3. Within the player object, locate the position `Vec3<float>`, velocity `Vec3<float>`,
   on-ground bool, and (if you want NoClip) the collision flag.
4. Cross-referencing a public offset dump for your exact version (e.g. the Horion project's
   published offsets) is the fastest way to get oriented, even though the addresses
   themselves won't match between forks/versions.
5. Put the resulting values (hex strings or plain ints both work) into `offsets.json` next to
   `BedrockCheat.exe`; no rebuild needed.

## Building (Windows only)

Requires Visual Studio 2022 (Desktop C++ workload) and
[vcpkg](https://github.com/microsoft/vcpkg) with `VCPKG_ROOT` set.

```powershell
cmake --preset windows-x64
cmake --build --preset windows-x64
```

Or open the folder directly in Visual Studio (File > Open > Folder) — it picks up
`CMakePresets.json` automatically.

Output is `out/build/windows-x64/injector/BedrockCheat.exe` — that single file is the whole
build artifact; `cheat.dll` is compiled along the way but only exists embedded inside it.

## Running

1. Enable **Windows Developer Mode** (Settings > Privacy & security > For developers) —
   required to attach to/inject into the sandboxed (AppContainer) Bedrock process at all.
2. Launch Minecraft, get into a world.
3. Run `BedrockCheat.exe` **as Administrator** (it needs elevated rights to grant the
   extracted DLL AppContainer ACLs and to open the target process).
4. Insert toggles the click-GUI; F5 opens the Offset Finder (see above) if you haven't
   populated `offsets.json` yet; each movement module also has its own keybind (see above).

## Scope note

This is built for your own server/singleplayer use, per your setup. Using it against
players or servers you don't control would violate the Minecraft EULA and most servers'
ToS, and isn't what this scaffold is aimed at.
