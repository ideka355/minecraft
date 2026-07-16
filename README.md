# Minecraft Bedrock cheat client (movement pass)

A DLL-injection cheat client for **Minecraft Bedrock for Windows**, built for use on your
own server / singleplayer worlds. First pass covers the movement category: Flight, Speed,
NoFall, NoClip, Spider. Combat, visuals/ESP, and misc/utility modules can be added the same
way once this pass is validated.

## How it's structured

- `injector/` — standalone .exe that finds `Minecraft.Windows.exe`, grants the DLL
  AppContainer read/execute access, and injects `cheat.dll` via `CreateRemoteThread` +
  `LoadLibraryW`.
- `cheat/` — the DLL itself:
  - `src/core/` — MinHook wrapper (`Hook`), AOB pattern scanner (`PatternScanner`),
    JSON-backed offset table (`Offsets`), SEH-guarded memory read/write (`GameMemory`),
    module base class + registry (`Module`, `ModuleManager`), keybind polling
    (`InputManager`).
  - `src/gui/` — generic D3D11 swapchain `Present` hook (`Overlay`) that draws an ImGui
    click-GUI (`ClickGui`) listing every module with a checkbox. Insert toggles the GUI.
  - `src/modules/movement/` — Flight (F1), Speed (F2), NoFall (GUI-only), NoClip (F3),
    Spider (F4).
- `config/offsets.json` — the only place game-version-specific addresses live.

## The one thing this scaffold can't do for you

`Offsets`, `PatternScanner`, and the hook/GUI framework are all generic and functional as
written. What's genuinely missing is **your build's actual memory layout** — the address of
the local player, and the byte offsets to position/velocity/on-ground/collision within it.
Those change with every Minecraft.Windows.exe update and can't be guessed or hardcoded
correctly ahead of time. Every movement module checks its required offsets and silently
no-ops if they're zero, rather than writing to a garbage address and crashing the game.

To fill in `config/offsets.json`:

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
5. Put the resulting values in `config/offsets.json` (hex strings or plain ints both work).
   No rebuild needed — it's loaded at DLL attach.

## Building (Windows only)

Requires Visual Studio 2022 (Desktop C++ workload) and
[vcpkg](https://github.com/microsoft/vcpkg) with `VCPKG_ROOT` set.

```powershell
cmake --preset windows-x64
cmake --build --preset windows-x64
```

Or open the folder directly in Visual Studio (File > Open > Folder) — it picks up
`CMakePresets.json` automatically.

Output lands in `out/build/windows-x64/injector/` and `.../cheat/`, with `offsets.json`
copied alongside both.

## Running

1. Enable **Windows Developer Mode** (Settings > Privacy & security > For developers) —
   required to attach to/inject into the sandboxed (AppContainer) Bedrock process at all.
2. Fill in `config/offsets.json` (see above) — modules no-op against an unset/zeroed table.
3. Launch Minecraft, get into a world.
4. Run `injector.exe` **as Administrator** from its output directory (it needs elevated
   rights to grant the DLL AppContainer ACLs and to open the target process).
5. Insert toggles the click-GUI; each module also has its own keybind (see above).

## Scope note

This is built for your own server/singleplayer use, per your setup. Using it against
players or servers you don't control would violate the Minecraft EULA and most servers'
ToS, and isn't what this scaffold is aimed at.
