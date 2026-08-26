# CLAUDE.md — FOR ATLANTIS (UE5 Project)

## Session Context
**Last Updated:** August 26, 2026
**Engine:** Unreal Engine 5.8 (`EngineAssociation: "5.8"`, `IncludeOrderVersion: Unreal5_8`, RHI: DX12)
**Phase:** Combat system design (pre-implementation). No combat code written yet.

**Current state:** The project is still the stock UE5 Third Person template plus its three
official variants (Combat / Platforming / SideScrolling), unmodified. All design work to date
lives in `docs/combat_decisions.md` — Decisions #1–#3, two of them `OPEN`.

**Next milestone:** Build a faithful BN3-style combat core first and get it fully playable.
Atlantis-specific modifications (starting with elevation, Decision #3) are layered on **after**,
never designed simultaneously. See `docs/combat_decisions.md` → Open Questions → "Core BN3 Loop".

---

## What This Project Is

`FOR ATLANTIS` is a real-time, grid-based combat game in Unreal Engine 5.8.

- **Combat mechanics** — modelled on Mega Man Battle Network 3: real-time, grid tiles,
  buster + chip-style attacks, tile modifiers.
- **Structure/transitions** — modelled on Clair Obscur: Expedition 33. Enemies exist in the
  explorable world; engaging one transitions into a contained battle arena. No random encounters.
- **Camera** — isometric 2.5D (Decision #1). Grid logic stays flat/2D underneath; the camera and
  art are angled on top. Elevation is a tile property, not a change to the grid data model.
- **PHIS** — the narrative/world-state belief engine — is **fully decoupled** from combat
  (Decision #2). No shared dependency in either direction. Do not introduce one without a new
  decision entry.

---

## Project Layout

```
FOR ATLANTIS GAME\
├── CLAUDE.md                       ← this file
├── .mcp.json                       ← MCP server config — do not delete or move
├── docs\
│   ├── AGENTS.md                   ← canonical Safety Ruleset — read before any work
│   └── combat_decisions.md         ← authoritative design-decision log
└── ProjectAtlantis\                ← the UE5 project
    ├── ProjectAtlantis.uproject
    ├── ProjectAtlantis.sln / .slnx
    ├── Automation_ProjectAtlantis.sln / .slnx
    ├── Config\                     ← DefaultEngine/Game/Input/Editor .ini
    ├── Source\ProjectAtlantis\     ← 90 C++ files, single module
    └── Content\                    ← 718 assets (incl. 521 __External*)
```

**Git-tracked as of August 26, 2026** — see Version Control below for the remote and initial
commit. Before this date the project had no version control; that history is preserved in
`docs/AGENTS.md` Rule 3's addendum. Still exercise care before bulk edits, mass renames, or
deletions — a remote push doesn't undo a bad local commit by itself.

### Source module map — `ProjectAtlantis/Source/ProjectAtlantis/`

| Area | Contents |
|---|---|
| Core | `ProjectAtlantisCharacter`, `ProjectAtlantisGameMode`, `ProjectAtlantisPlayerController` |
| `Variant_Combat/` | Character/GameMode/PlayerController; `AI/` (CombatAIController, CombatEnemy, CombatEnemySpawner, CombatStateTreeUtility, EnvQueryContext_Danger/_Player); `Animation/` (AnimNotify_CheckChargedAttack, CheckCombo, DoAttackTrace); `Gameplay/` (ActivationVolume, CheckpointVolume, DamageableBox, Dummy, LavaFloor); `Interfaces/` (CombatActivatable, CombatAttacker, CombatDamageable); `UI/CombatLifeBar` |
| `Variant_Platforming/` | Character/GameMode/PlayerController; `Animation/AnimNotify_EndDash` |
| `Variant_SideScrolling/` | Character/GameMode/PlayerController/CameraManager; `AI/`; `Gameplay/` (JumpPad, MovingPlatform, Pickup, SoftPlatform); `Interfaces/`; `UI/` |

Four maps: `Lvl_ThirdPerson` (startup + default), `Lvl_Combat`, `Lvl_Platforming`, `Lvl_SideScrolling`.
Default game mode: `BP_ThirdPersonGameMode`.

**These variants are Epic template code, not Atlantis code.** `Variant_Combat` is a melee
combo/AI demo — it is **not** the BN3 grid system and is not the intended foundation for it.
Treat it as reference material to read, not a base class to inherit from, unless a decision
entry says otherwise. Say which you are proposing before writing code against it.

### Module dependencies (`ProjectAtlantis.Build.cs`)
`Core`, `CoreUObject`, `Engine`, `InputCore`, `EnhancedInput`, `AIModule`, `StateTreeModule`,
`GameplayStateTreeModule`, `UMG`, `Slate` — all public. `PrivateDependencyModuleNames` is empty.
Adding a new source subfolder requires adding it to `PublicIncludePaths` in the same file.

### Enabled plugins
`ModelingToolsEditorMode` (Editor only), `StateTree`, `GameplayStateTree`,
`ModelContextProtocol`, `EditorToolset`.

---

## Version Control

**Repo:** https://github.com/cviper7332/FOR-ATLANTIS-GAME.git
**Branch:** `main`
**Initialized:** August 26, 2026. First commit: "Initial commit: stock UE5.8 Third Person
template + variants, pre-combat-implementation baseline" (856 files, ~135 MB tracked; largest
tracked file ~21 MB, under GitHub's 50 MB soft / 100 MB hard thresholds — no LFS needed yet).

`.gitignore` excludes `Binaries/`, `Intermediate/`, `.vs/`, `DerivedDataCache/`, `Saved/`, and
standard VS/Rider/OS artifacts. `ProjectAtlantis/Content/__ExternalActors__/` and
`__ExternalObjects__/` **are tracked** — they are real per-actor data, not cache, despite the
naming (see Do Not Touch above regarding hand-editing them).

Re-check the 50 MB/100 MB thresholds before committing new binary content — Content assets
(meshes, textures, animations) are the files most likely to cross them as work continues.

---

## Safety Ruleset

**Canonical: [`docs/AGENTS.md`](docs/AGENTS.md).** Ten mandatory rules — four on agent conduct
(in force now), six on architecture (binding on combat code as it is written) — plus the
Recurring Failure Modes checklist and the audit table.

Read it before any work. Read the Recurring Failure Modes section specifically before writing
any combat system, any balance constant, or any test oracle.

The four conduct rules in force today:

| Rule | Short form |
|---|---|
| 1 — Research/Apply Separation | Research questions get text answers. No diff in the same turn. Every write needs its own explicit go-ahead at the time it happens. |
| 2 — Live-Document Verification | Never state project/decision status without reading the governing file live, in that turn. A prior AI's claim is not a substitute. |
| 3 — No Undo | No version control exists. Confirm before any deletion, overwrite, or bulk edit. |
| 4 — Append, Don't Rewrite | Decision-log entries are never silently edited — corrections are dated addenda. |

---

## Design Decisions — Process

`docs/combat_decisions.md` is the authoritative log. **Read it before proposing any combat
design or writing any combat code.** Its conventions are binding:

- Numbered decisions, each with `Date`, `Phase`, `Author`, `Status` in that order under the heading.
- `Status` is set at creation, not backfilled.
- **Append, don't rewrite.** Once an entry exists, corrections are added as dated addenda —
  never silent edits to the original text.
- Controlled `Status` vocabulary: `CLOSED — fixed in <commit>`, `CLOSED — enacted in <commit>`,
  `PARTIAL — <done>; OUTSTANDING: <remains>`, `OPEN`,
  `N/A — design rationale, no action implied`, `CLOSED — superseded by Decision #N`
  (citing the superseding number is mandatory).

CC writes decision entries when asked, and should **propose** one whenever a conversation
settles a design question that isn't yet logged. Do not treat a chat conclusion as decided
until it is in the file.

---

## How to Build

**CC cannot trigger UBT. Omar triggers all builds.**

**Reliable path — always use this:**
```
1. Close UE5
2. Open Visual Studio (ProjectAtlantis.sln)
3. Right-click ProjectAtlantis → Build (Development Editor x64)
4. Open UE5 by double-clicking ProjectAtlantis.uproject
5. Restart Claude Code / Cursor to re-establish MCP
```

**Why close UE5 first:** UE5 + VS + UBA running simultaneously on 32GB RAM peaks around
35–38GB committed, which kills UBA compile jobs. Closing the editor frees ~8GB.
*(Carried over from the PRS_TestingPlatform project on this machine — same hardware, same
failure mode expected here; not yet independently measured on this project.)*

**Live Coding:** do not use. It fails consistently with `0xC0000005` under memory pressure.

**Build verification — a "Build Succeeded" does not by itself prove your change compiled in.**
UBT incremental dependency tracking can miss edits. Confirm the relinked
`ProjectAtlantis/Binaries/Win64/UnrealEditor-ProjectAtlantis.dll` timestamp postdates the source
edit before trusting any build or test result. This bit the PRS project twice.

**Header changes trigger a much larger rebuild than `.cpp` changes.** If a change can be made
entirely in a `.cpp`, prefer that during iteration.

---

## MCP

**Endpoint:** `http://127.0.0.1:8000/mcp` (server name `unreal-mcp`, defined in `.mcp.json`)

This is **UE 5.8's built-in Experimental `ModelContextProtocol` plugin** — Epic's own MCP server
running inside the editor process, not an external Python server. Verified at
`C:\Program Files\Epic Games\UE_5.8\Engine\Plugins\Experimental\ModelContextProtocol\`
(`DefaultServerPort = 8000`, `DefaultServerUrlPath = "/mcp"`).

**UE5 must be open and the MCP server running before CC connects.** If tools are missing,
the editor is closed or the server didn't start.

Console commands (in the editor console, `ECVF_Cheat`):

| Command | Effect |
|---|---|
| `ModelContextProtocol.StartServer [port]` | Start the server (defaults to 8000) |
| `ModelContextProtocol.StopServer` | Stop it |
| `ModelContextProtocol.RefreshTools` | Rebuild the tool list, dropping cached schemas — run after a rebuild that changes reflected types |

**MCP is used for:**
- Reading the editor output log during PIE
- Blueprint graph inspection and editing
- Inspecting actors, components, assets, and material/render-target state in the editor

**MCP file-read rule:** always re-read a file immediately before any find-and-replace. The
editor's project panel lags CC edits by one round-trip, and acting on stale content silently
clobbers the newer version.

**CC cannot trigger builds via MCP.** Omar triggers all builds.

**After any full rebuild, restart CC** to re-establish MCP tool access, and consider
`ModelContextProtocol.RefreshTools` if tool schemas look stale.

---

## UE5 API Reference — Ground Truth

Local engine source is installed at:
`C:\Program Files\Epic Games\UE_5.8\Engine\Source\`
(UE_5.6 and UE_5.7 are also installed — **make sure you are reading 5.8**.)

For any question about an exact UE5.8 API signature, enum value, or class behavior, prefer
grepping/reading this local source directly over training data or web search. Web results are
frequently for a different engine version and are wrong for 5.8-specific behavior; training data
has the same version-drift risk. The local 5.8 source is ground truth for what this project
actually builds against.

Epic API docs, when a page is genuinely needed:
`https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/[ModuleName]/[ClassName]`

---

## Pre-Code Checklist (Mandatory)

Before writing any C++ that touches UE5 APIs:

1. **Read `docs/combat_decisions.md`** if the work touches combat design at all — the answer
   may already be decided, deferred, or explicitly rejected there.
2. **Verify the API against local 5.8 source** before writing. Do not assume a method exists,
   and do not assume a signature. Grep `C:\Program Files\Epic Games\UE_5.8\Engine\Source\`.
3. **Verify `UActorComponent` / `AActor` lifecycle signatures** (`BeginPlay`, `EndPlay`,
   `TickComponent`) against 5.8 — parameter names and types have changed across versions.
4. **New source subfolder → add it to `PublicIncludePaths`** in `ProjectAtlantis.Build.cs`,
   or the build fails on includes that look correct.
5. **Say what you're about to change before changing it**, given there is no git safety net.

---

## UE5.8 API Gotchas (Hard-Won — Do Not Repeat)

Carried over from PRS_TestingPlatform (same engine version, same machine). These caused real
build failures there; they will cause them here too.

**UPROPERTY:**
- `BlueprintReadWrite` on a private `UPROPERTY` requires `meta=(AllowPrivateAccess="true")`
- `EditAnywhere` on a level-actor reference won't work on class defaults — use
  `EditInstanceOnly` and set it on the placed instance, or set it via Blueprint at runtime
- Level actor references cannot be set in the Blueprint class editor — only on placed
  instances in the Details panel, or via a Blueprint node at runtime

**pimpl under UHT:**
- Forward declarations must be at file scope, not nested inside the class body
- Special member functions requiring the complete type (destructor, move) must be **declared**
  in the header but **defined out-of-line** in the `.cpp`

**Render targets** (relevant once combat VFX/grid rendering starts):
- `UTextureRenderTarget2D::CreateTransient` does not exist — use `NewObject` +
  `InitAutoFormat` + `UpdateResourceImmediate`
- `RHILockTexture2D` is unsafe under DX12 (this project is DX12) — use
  `RHICmdList.UpdateTexture2D` with `FUpdateTextureRegion2D`

**Post process:**
- `RemoveBlendable` does not exist on `APostProcessVolume` — use
  `AddOrUpdateBlendable(instance, 0.0f)` to disable
- `BL_BeforeTonemapping` was removed in UE5.5+ — use `BL_SceneColorBeforeBloom`

---

## Blueprint Wiring

Blueprint graphs are invisible to grep and easy to break silently. Any wiring CC creates or
changes via MCP **must be written down here**, with enough detail to reconstruct it: node
names, which pins connect, and which are deliberately left unconnected.

A specific trap from the PRS project: a `FlipFlop` macro's next output depends on how many
times it has already fired, so a key bound through one has **no knowable resulting state**
without querying it, and is not scriptable for measurement. If a toggle needs to be driven
reliably, expose a console command or an explicit `Set` entry point rather than relying on
the key.

Nothing Atlantis-specific is wired yet — the template Blueprints are stock.

---

## Do Not Touch

- **`.mcp.json` at project root** — do not delete, move, or change the endpoint path.
- **`docs/combat_decisions.md` existing entries** — append addenda; never silently edit an
  entry that already exists.
- **`Config/DefaultEngine.ini` RHI setting** (`DefaultGraphicsRHI_DX12`) — the DX12 gotchas
  above assume it.
- **`ProjectAtlantis/Content/__ExternalActors__/` and `__ExternalObjects__/`** — 521
  machine-generated one-file-per-actor entries for the four maps. Never hand-edit or
  bulk-delete; they are edited only through the editor.
- **`Binaries/`, `Intermediate/`, `.vs/`, `DerivedDataCache/`, `Saved/`** — build and IDE
  caches (~4.6 GB of the project's 4.7 GB). Never read them for information about the project,
  and never list them in full.

---

## Known Template Leftovers

Cosmetic, but flag before shipping anything: `Config/DefaultGame.ini` still has
`ProjectName=Third Person Game Template`, and `Content/Developers/oramo/` and
`Content/Collections/` are empty.

---

*Last Updated: August 26, 2026*
*Phase: Combat system design (pre-implementation) — Decisions #1–#3 logged, #1 and #3 OPEN*
