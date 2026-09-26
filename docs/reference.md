# reference.md — Verified UE5.8 API Notes

This doc records UE5.8 API facts verified directly — against local engine source
(`C:\Program Files\Epic Games\UE_5.8\Engine\Source\`, per `CLAUDE.md`'s "UE5 API Reference —
Ground Truth" section) or via live MCP queries against the running editor — rather than assumed
from training data or general web search. Training data and web results carry the same risk here:
both frequently reflect a different engine version and are silently wrong for 5.8-specific
behavior (`CLAUDE.md`'s own words).

**Every entry below cites how it was verified** — a source file path and line number, or the
specific MCP query run — so a future reader can re-verify or extend it rather than trust it
blindly. An entry with no citation is not a verified fact and doesn't belong in this file.

This doc extends `CLAUDE.md`'s ground-truth principle rather than replacing it; `CLAUDE.md`
remains the canonical statement of *why* local source is authoritative and *where* it lives. This
file is where the actual verified findings accumulate as more APIs get checked this way, one
section per API area, so nothing has to be re-derived from scratch the next time it's needed.

---

## Automation Testing (UE5.8)

Verified while designing RTAC's first UE Automation Test (Phase 0 Part B). Citations are from
`Runtime/Core/Public/Misc/AutomationTest.h` (declarations) and
`Runtime/Core/Private/Misc/AutomationTest.cpp` (behaviour), as noted per entry.

### `IMPLEMENT_SIMPLE_AUTOMATION_TEST` — self-registering, no manual wiring

Confirmed current and unchanged in 5.8 (`AutomationTest.h:4297`, and its Program-target variant at
`:4367`). Tests do **not** need to be registered anywhere by hand — the engine's own doc comment
above the macro block states this directly:

> "Builds supporting automation tests will automatically create and register an instance of the
> automation test within the automation test framework as a result of the macro."
> — `AutomationTest.h:4117`

Practically: writing `IMPLEMENT_SIMPLE_AUTOMATION_TEST(...)` plus a `RunTest()` body in a `.cpp`
file compiled into a loaded module is the entire registration step. Nothing else makes a test
discoverable from the Session Frontend.

### How a test actually passes or fails — `RunTest`'s return value is not the whole story

This is the single most misremembered detail in this API, and getting it wrong produces a test
that silently *cannot fail* — exactly the Failure Mode 8 trap. Verified in
`AutomationTest.cpp`:

```cpp
bTestSuccessful = CurrentTest->RunTest(Parameters);                    // :1316
// ...
bTestSuccessful = bTestSuccessful && !CurrentTest->HasAnyErrors()
                                  && CurrentTest->HasMetExpectedMessages();  // :1376
```

Success is the **conjunction** of three things, per the engine's own comment at `:1372-1375`:
the value `RunTest` returned, *and* that no errors were logged during execution, *and* that any
expected messages were met.

Practically: **`return true;` at the end of `RunTest` is correct and idiomatic.** A failing
assertion helper calls `AddError()` internally, which sets `HasAnyErrors()`, which forces the
test to fail regardless of the returned value. A test body that runs assertions and returns `true`
unconditionally can still fail — the return value signals "the test body itself completed without
bailing out," not "the test passed."

### `EAutomationTestFlags` — an `enum class`, and the Filter-mask constraint

Defined as a proper C++11 `enum class` at `AutomationTest.h:88` — combined with `|`, not treated
as a raw `uint32` bitmask.

The macro enforces a `static_assert` (`:4128`) requiring **exactly one** flag from the Filter mask
to be set:

```
SmokeFilter | EngineFilter | ProductFilter | PerfFilter | StressFilter
```

Practically: passing two Filter-mask flags together (e.g. `SmokeFilter | ProductFilter`) is a
**compile error**, not a silently-merged or silently-ignored combination. Exactly one must be
chosen per test.

### Flag values relevant to this project

| Flag | Engine's own comment | Line |
|---|---|---|
| `EditorContext` | "Test is suitable for running within the editor" | 93 |
| `SmokeFilter` | "Super Fast Filter" | 129 |
| `ProductFilter` | "Product Level Test" | 133 |

**Judgment call, recorded so future tests stay consistent with it:** RTAC's first test
(`RTAC.Simulation.Grid.BasicLifecycle`) is small, fast, and has zero engine dependency — the kind
of test that *reads* like a "smoke test" informally. It was still given `ProductFilter`, not
`SmokeFilter`, because the engine's filter categories classify tests by **kind**, not by
**speed**: `SmokeFilter` ("Super Fast Filter") is conventionally the category for engine/build-wide
sanity suites, while `ProductFilter` ("Product Level Test") is what actually describes a
project-specific gameplay-logic test on RTAC's own types. Picking `SmokeFilter` here would have
been choosing the flag by vibe (it's fast) rather than by the category's documented meaning (what
kind of thing is being tested). This reasoning carries forward to future RTAC tests unless a
specific test is genuinely engine/build-sanity in nature rather than product logic.

`EditorContext` alone (no `ClientContext`/`ServerContext`/`CommandletContext`) was chosen for the
same test not because the code under test needs an editor — it has no engine dependency at all —
but because Phase 0 Part B's own Definition of Done is scoped to "requires UE5.8 open," and
Decision #7 ties this project's whole workflow to a live 5.8 editor. The flag should describe what
has actually been verified to run, not everywhere the code could theoretically run.

### `WITH_AUTOMATION_TESTS` — keeping test code out of Shipping

```cpp
#ifndef WITH_AUTOMATION_TESTS
	#define WITH_AUTOMATION_TESTS (WITH_DEV_AUTOMATION_TESTS || WITH_PERF_AUTOMATION_TESTS)
#endif
```
— `AutomationTest.h:63-64`

Convention (confirmed by how the engine's own macro block is gated at `:4086`): wrap an entire
test `.cpp` file in `#if WITH_AUTOMATION_TESTS ... #endif`. This compiles the test out of Shipping
builds automatically, with no separate build configuration or target needed.

### Assertion helpers — confirmed present, with actual signatures

All confirmed live in `AutomationTest.h`:

- `CORE_API bool TestTrue(const TCHAR* What, bool Value);` — `:2603`
- `CORE_API bool TestFalse(const TCHAR* What, bool Value);` — `:2367`
- `CORE_API bool TestEqual(const TCHAR* What, const int32 Actual, const int32 Expected);` — `:1985`
  (overloads also exist for `int64`, `SIZE_T`, `float`/`double` with a `Tolerance` defaulting to
  `UE_KINDA_SMALL_NUMBER`, `FVector`, `FTransform`, `FRotator`, `FColor`, `FLinearColor`, `TCHAR*`,
  `FStringView`, `FString`, `FUtf8StringView`, `FText`, `FName`)
- `template<typename ValueType> inline bool TestNotNull(const TCHAR* What, const ValueType* Pointer);`
  — `:2449`
- `CORE_API bool TestNull(const TCHAR* What, const void* Pointer);` — `:2530`

Overload note: with two `int32` arguments the `int32` overload is an exact match and wins
unambiguously over the `int64`/`SIZE_T` ones, which would require conversions. And `TestNull`
taking `const void*` means any object pointer (e.g. `FRTACTile*`) converts implicitly — no cast
needed at the call site.

**`TestNotNull`'s actual pass/fail semantics — transcribed verbatim from `:2448-2457`, since this
is exactly the kind of detail that is easy to misremember:**

```cpp
template<typename ValueType>
inline bool TestNotNull(const TCHAR* What, const ValueType* Pointer)
{
	if (Pointer == nullptr)
	{
		AddError(FString::Printf(TEXT("Expected '%s' to be not null."), What));
		return false;
	}
	return true;
}
```

Returns `true` (pass) when the pointer is non-null; returns `false` **and logs an `AddError`**
when the pointer is null. It does not throw and does not abort the test on failure — the caller
is expected to check the return value if a null result would make a subsequent dereference unsafe:

```cpp
if (TestNotNull(TEXT("..."), Ptr))
{
	// safe to dereference Ptr here
}
```

A crashing test is worse evidence than a cleanly failing one, so guard the dereference rather than
letting a legitimate assertion failure take the whole run down.

### The framework intercepts `UE_LOG` during a test run — verbosity is load-bearing

Not obvious, and it changes how you instrument a test. For the duration of a test, the framework
installs `FAutomationTestFramework::FAutomationTestOutputDevice`, which intercepts log output
(`AutomationTest.cpp:218`). Its capture predicate (`:233`):

```cpp
bool CaptureLog = !LocalCurTest->SuppressLogs()
    && (Verbosity == ELogVerbosity::Error || Verbosity == ELogVerbosity::Warning || Verbosity == ELogVerbosity::Display)
    && LocalCurTest->ShouldCaptureLogCategory(Category);
```

A captured entry becomes an automation event (`:243-254`): `Error` → `EAutomationEventType::Error`,
`Warning` → `EAutomationEventType::Warning`.

Consequences worth knowing before writing any test logging:

- **`Log` verbosity is not intercepted at all.** It is absent from the predicate, so
  `UE_LOG(Cat, Log, ...)` inside a test provably cannot affect the result, however much of it
  there is.
- **`UE_LOG(Cat, Error, ...)` inside a test fails that test.** It becomes an Error event, which
  feeds `HasAnyErrors()`, which is one of the three conjuncts at `:1376`. This is a way to fail a
  test without ever calling an assertion helper — and a way to fail one *accidentally*, if code
  under test logs an error on a path the test deliberately exercises.
- **Warnings do not fail a test by default**, but this is config-dependent:
  `bElevateLogWarningsToErrors` defaults to `false` (`:181`) and is read from `GEngineIni` under
  `[/Script/AutomationController.AutomationControllerSettings]` (`:2055`). This project does not
  set it — verified by grepping `ProjectAtlantis/Config/`. If it is ever enabled, warnings logged
  during tests become failures.

### Pattern: mirror assertion outcomes into the project's own log category (MCP-queryable)

Reusable pattern, established with `FRTACGridBasicLifecycleTest` — future RTAC tests should follow
it rather than reinventing it.

The Session Frontend shows per-assertion detail in its own UI, but that detail is not reachable
from `EditorToolset.LogsToolset`'s `GetLogEntries`. Since MCP log reading is this project's main
programmatic window into the editor, tests additionally mirror each assertion's outcome into
`LogRTAC` (Rule 9's dedicated category — do not add a second category for tests), bookended by a
start line and an `N/N assertions passed` summary. A single
`GetLogEntries(category="LogRTAC", pattern=".*")` then shows the whole run.

Three constraints make this safe rather than a source of false results:

1. **Additive only.** The logging wraps assertion helpers, it does not replace them. The helpers
   still drive the real pass/fail state via `AddError()` — per the `:1376` conjunction above,
   `HasAnyErrors()` is what actually fails a test, never a log line.
2. **Passes log at `Log`, failures at `Warning` — not `Error`.** Given the interception rules
   above, logging a failure at `Error` would register a *second* error event for one already-failed
   assertion, inflating the reported error count and making one failure look like two. `Warning`
   echoes the failure for a human or an MCP query without touching the error count.
3. **Wrap, don't duplicate the description string.** Thin `CheckTrue`/`CheckEqual`/… lambdas that
   call the assertion and record its result keep one description per call site, so the assertion's
   message and its log line cannot drift apart (Failure Mode 7).

### Module and build implications

- Automation-test support (`Misc/AutomationTest.h`) lives in the **`Core`** module. `Core` is
  already a `PublicDependencyModuleNames` entry in `RTAC.Build.cs` — **no `Build.cs` change is
  needed** to write automation tests inside RTAC.
- UBT compiles every `.cpp` under a module's `Private/` tree automatically, regardless of
  subfolder. A new `Private/Tests/` subfolder needs **no `PublicIncludePaths` entry** — that
  requirement (`CLAUDE.md`'s Pre-Code Checklist item 4) is about the *main project*
  (`ProjectAtlantis.Build.cs`) referencing subfolders inside *RTAC's own* module from outside it;
  it does not apply to a module referencing its own internal folders, which UBT already includes
  automatically.
- `AutomationTest.h` is self-sufficient in its own includes (`CoreTypes.h`, the container headers,
  etc. — `:5-30`), so it can safely be the first include in a test file.

### Known gap in current MCP tooling (honest, not resolved)

`EditorToolset.EditorAppToolset` — the toolset exposing PIE control, console-variable search, and
viewport/actor/asset queries — does **not** currently expose a tool to run an arbitrary console
command or to query the Session Frontend's registered-test list directly. Verified by calling
`describe_toolset` on it live and reading the full returned tool list.

This means test *discovery* (confirming a test actually appears in the Session Frontend tree, as
opposed to confirming that the module defining it loaded and logged) could not be independently
verified via a live MCP query — unlike the plugin-load and log-category verification done through
`EditorToolset.LogsToolset` elsewhere in this project's history. **This is a recorded gap in
current MCP tooling, not a settled or closed question** — a future session with a console-command
or test-listing tool available should re-attempt this verification rather than assume the gap
still holds.

### Known unexplained intermittent: editor crash during transient-world creation in automation tests

**Status: open, contained, not diagnosed.** Recorded here so a future occurrence is recognised
rather than re-investigated from scratch. First observed September 25, 2026.

**Symptom.** An automation test that creates its own transient `UWorld` intermittently kills the
editor. Observed roughly 1 run in 5. The signature is specific enough to identify on sight:

- The test logs its `— starting ===` line and then **zero `[PASS]` lines** — it dies before its
  first assertion.
- `Saved/Crashes/` gains two entries: an **Ensure** (`RendererScene.cpp:4337`,
  `!ActorComponent->IsRegistered() || ActorComponent->GetScene() != this`, naming a
  GameplayDebugger `InputComponent` in `/Engine/Transient.World_N`) and, ~2s later, a fatal
  **Assert** (`RendererScene.cpp:1279`, `Primitives.Num() == 0`).
- The crash callstack's only symbolised project frame is the test's own `UWorld::CreateWorld`
  line — world **creation**, not teardown.
- The session log ends mid-callstack with no `LogExit` lines.

**Practical mitigation: restart the editor and re-run.** It does not reproduce reliably and it
cannot produce a false green — a killed editor logs no `complete: N/N` line at all, so there is
no silent-wrong-result risk. Do not re-derive the diagnosis below.

**What is confirmed** (each verified live, not inferred):

- `AActor::RouteEndPlay` unregisters **no** components (`Actor.cpp:3221` — it calls `EndPlay()`
  and touches components only via `ClearComponentOverlaps()`, gated on
  `EEndPlayReason::RemovedFromWorld`). Demonstrated at runtime by a two-point component census
  showing byte-identical before/after sets on a green run.
- A test world legitimately carries ~11 scene-bound components at teardown, 6 of them
  `UPrimitiveComponent` (4 world `LineBatchComponent`s, 2 `BrushComponent`s from `Brush_0` and
  `DefaultPhysicsVolume_0`). This is **normal** — `UWorld::ClearWorldComponents`
  (`World.cpp:2923`) unregisters exactly these, and every green run has them too.
- `FScene::~FScene` is **not** called by `DestroyWorld`. `UWorld::DestroyWorld` (`World.cpp:2770`)
  never releases the scene; `Scene->Release()` happens in `UWorld::FinishDestroy`
  (`World.cpp:1604`) during **garbage collection**.
- The ensure and the assert concern **different objects** — the ensure's `InputComponent` is not
  a primitive.

**What is ruled out** (five hypotheses, each falsified):

1. *Missing `CollectGarbage()` in teardown* — Epic's own reference (`FActorTestSpawner`) does not
   do this, and GC would not run between `RouteEndPlay` and `DestroyWorld` anyway.
2. *Missing `DestroySpawnedActors`* — the at-risk primitives are world-owned and world-default,
   not test-spawned, so destroying test actors would not unregister any of them.
3. *A teardown-sequencing defect* — the crash occurs during world **creation**, before the first
   assertion; no teardown change can reach it.
4. *A leaked `UWorld`* — directly falsified. A test world survives ~26 minutes only because an
   idle editor runs no GC; a forced collection (PIE start/stop, which triggers
   `UEditorEngine::EndPlayMap`'s `CollectGarbage` at `PlayLevel.cpp:502`) purges it cleanly,
   along with its actors. `DestroyWorldContext` (`UnrealEngine.cpp:17330`) and the root-set
   handling are both correct.
5. *Object-name reuse from omitting `EUniqueObjectNameOptions::GloballyUnique`* — with a null
   `Parent`, `MakeUniqueObjectName` uses the monotonic `Class->ClassUnique` counter
   (`UObjectGlobals.cpp:2705`) and the reuse path bails at `:2542`. No collision is possible
   within a session.

**What remains unknown:** which scene was being destroyed, from what call path, and why it is
intermittent.

**What would be needed to resolve it:** a symbolised callstack from the minidump in
`Saved/Crashes/`. This requires the Epic Launcher's "Editor symbols for debugging" component —
`UnrealEditor-Renderer.pdb` is not part of a default install, which is why the existing crash
logs show `UnrealEditor-Renderer.dll!UnknownFunction []`. Judged not worth the cost at the
observed frequency, given the failure is loud rather than silent.

**Reusable technique.** The two-point component census that produced the `RouteEndPlay` finding is
a port of the engine's own diagnostic at `RendererScene.cpp:1266-1276`, which Epic ships behind
`#if 0` (unreachable in an installed binary engine — the Renderer module cannot be recompiled).
Every API it needs is public: `FThreadSafeObjectIterator` (`UObjectIterator.h`),
`UActorComponent::GetScene` (`ActorComponent.h:1195`), `UActorComponent::IsRegistered` (`:1316`),
`UWorld::Scene` (`World.h:1504`). Taking the census at two points rather than one is what made
the finding provable on a passing run, with no reproduction required.

### Non-identity transform coverage: yaw 90 is a best case, not an adversarial one

`FTransform::TransformPosition` composed with `InverseTransformPosition` is mathematically an
identity, but **not** in floating point. A rotation is stored as a quaternion built from
`sin/cos` of half the angle, which are generally not representable, so a round trip through a
rotated transform introduces roughly **1e-14** of absolute error. Under an identity transform the
error is exactly zero — which means a test whose actor sits at identity is not exercising this at
all, however non-identity its intent.

**Measured, September 25, 2026:** an `ARTACBoard` placed at yaw 90 reads its rotation back as
**89.999999999999986**, about 1.4e-14 off.

**Why yaw 90 is unusually forgiving.** Its rotation matrix entries are 0 and ±1 to within 1e-16,
so almost nothing is lost. An arbitrary angle — yaw 37.4, or any pitch/roll combination — has no
exactly representable entries and produces materially larger error.
`RTACGridConversionTest.cpp` spawns its board at `Location (500,-300,120)`, `Yaw 90`. That
coverage is **real** — it proves the transform is applied, which was false before `ARTACBoard`
gained a `SceneRoot` — but it is **gentle**. It does not stress-test rounding, and a future
author should not read a green run as evidence that rotated-board hit-testing is robust.

**The actual risk: click-derived inputs are safe, computed-derived inputs are not.**
`RTACWorldPositionToGridPosition` uses `FloorToInt32`, which is maximally sensitive at an exact
tile boundary — 1e-14 of error is enough to floor `2.0` down to `1`.

- **A real click cannot practically hit this.** The world position comes from deprojecting a
  screen pixel through a perspective ray. The vulnerable band is roughly 1e-12 units wide on a
  200-unit tile; a click will not land in it.
- **Computed positions land in it by construction.** Anything derived from
  `RTACGridToLocalOffset` produces exact multiples of `TileSize` — an entity snapped to its own
  tile, a projectile evaluated at tile steps, a round trip taken at a tile *corner* rather than
  its centre. The conversion test's Case 8 avoids this only because it offsets by half a tile
  before inverting.

**Status: CONFIRMED — first observed September 26, 2026.** This supersedes the earlier status,
"latent, unconfirmed, low priority," which was accurate while the run of September 25, 2026 passed
43/43 with the mechanism demonstrably present but never flipped. The entry predicted this failure
and the prediction was borne out exactly, mechanism included. The click/computed distinction above
is unchanged and still correct; what is new is a **third** path into the unsafe case that neither
bullet covers. Whether `RTACWorldPositionToGridPosition` should tolerate boundary error remains an
open design question, not a settled one — no guard was added.

**The observation.** During Phase 2 Part A item 2's runtime probing, `RTAC.ScreenToGrid 475.5 155.0`
returned `tile (Row 2, Column 2)` where `Column 3` was predicted. The Row was correct; only the
Column flipped, and it flipped *down*, exactly as `FloorToInt32` does when 600.0 arrives as
599.99999…. Board at `Location (500,-300,120)`, `Yaw 90` — the same pose measured above, reading
back as 89.999999999999986 — `TileSize 200`, grid 3x6, PIE viewport 951x520.

**The third path: an exactly-centred screen pixel is a computed input.** It arrives through the
deprojection path, so the "a real click cannot practically hit this" reasoning above does not cover
it — but px 475.5 is the exact optical centre of a 951-wide viewport, so `x_ndc` is exactly 0, the
deprojected ray carries zero lateral offset, and the hit lands on the camera's look-at coordinate
exactly rather than in some tile's interior.

**And `ARTACCombatCamera` puts the look-at point on a tile boundary whenever a grid dimension is
even.** `FrameBoard()` looks at `CenterLocal = (Rows*TileSize/2, Columns*TileSize/2, 0)`. For an
even count that is an exact multiple of `TileSize` — a tile boundary; for an odd count it is a
half-multiple — a tile centre. The 3x6 default therefore lands the optical centre on the column
2/3 boundary (600 = 3x200) and in the middle of row 1 (300 = 1.5x200), which is precisely the
observed Row-correct/Column-wrong split. **This is reachable by construction on the default board,
at the screen centre, with no unusual input** — not an exotic case. It is also why the 18-tile
sweep is clean: tile centres sit at odd multiples of half a tile, so none of them lands on a
boundary in either axis.

---

## Camera and Projection (UE5.8)

### `UCameraComponent::FieldOfView` is horizontal *as authored*, not necessarily as applied

The header documents `FieldOfView` as "the horizontal field of view (in degrees) in perspective
mode" (`Runtime/Engine/Classes/Camera/CameraComponent.h:37`, field at `:44`). That is true of the
value you set. It is **not** necessarily the angle the projection matrix uses on the horizontal
axis, because an aspect-ratio axis constraint can reinterpret it.

**This project is affected by default.** `Engine/Config/BaseEngine.ini:2900` sets
`AspectRatioAxisConstraint=AspectRatio_MaintainYFOV`, and `ProjectAtlantis/Config/` does not
override it (grepped, zero hits). `MaintainYFOV` means the **vertical** FOV is what stays fixed.

The conversion, `Runtime/Engine/Private/Camera/CameraStackTypes.cpp:331-333`:

    const float HalfXFOV = FMath::DegreesToRadians(FMath::Max(0.001f, ViewInfo.FOV) / 2.f);
    const float HalfYFOV = FMath::Atan(FMath::Tan(HalfXFOV) / ViewInfo.AspectRatio);
    MatrixHalfFOV = HalfYFOV;

with the axis multipliers chosen at `:287-299` — under `MaintainYFOV`,
`XAxisMultiplier = SizeY/SizeX` and `YAxisMultiplier = 1.0`. Net effect:

    theta_v = atan( tan(FOV/2) / Camera.AspectRatio )   <- fixed, regardless of window shape
    theta_h = atan( tan(theta_v) * ViewportAspect )     <- varies with window shape

`UCameraComponent::AspectRatio` defaults to `1.777778` (16:9) and `FieldOfView` to `90.0f` —
`Runtime/Engine/Private/Camera/CameraComponent.cpp:81` and `:78`.

**Why it matters.** At a 16:9 viewport with the default `AspectRatio`, a 60-degree `FieldOfView`
gives theta_v = 18 deg and theta_h = 30 deg — exactly as if the value were used as a horizontal
FOV. The two readings agree *at the design aspect ratio only*, and diverge asymmetrically:

| Viewport | theta_h (FOV 60, AspectRatio 16:9) | Effect |
|---|---|---|
| 21:9 ultrawide | 37.2 deg | wider view; content shrinks, nothing crops |
| 16:9 design target | 30.0 deg | as authored |
| 4:3 | 23.4 deg | **narrower view; horizontal content crops** |

A camera that frames on a horizontal extent therefore cannot assume a fixed horizontal FOV. It
must name the narrowest aspect ratio it guarantees and derive its distance from that. This is why
`ARTACCombatCamera` carries a `MinAspectRatio` knob rather than a bare FOV: the assumption is made
explicit and tunable instead of silently baked into a distance constant.

**Verified September 25, 2026:** `CameraComponent.h:37,44`; `CameraComponent.cpp:78,81`;
`CameraStackTypes.cpp:287,299,331-333`; `BaseEngine.ini:2900`; plus a grep of
`ProjectAtlantis/Config/` for `AspectRatioAxisConstraint` returning zero hits.

### `FMath::RayPlaneIntersection` is an infinite-line intersection, unguarded on sign and parallel

`FMath::RayPlaneIntersection` (`Plane.h:643-652`) computes
`Distance = Dot(PlaneOrigin - RayOrigin, N) / Dot(RayDirection, N)` and returns
`RayOrigin + RayDirection * Distance`. **`Distance` may be negative** — the "ray" is an infinite
line, so a direction pointing *away* from the plane still yields a point, behind the origin. There
is also no parallel check; the header says outright *"Assumes that the line and plane do indeed
intersect; you must make sure they're not parallel before calling."* Exactly parallel divides by
zero, giving ±inf or NaN, which `FMath::FloorToInt32` is not defined for.

**`RTACScreenToGridPosition` performs neither check.** It is currently saved by its bounds check.

**Reachability is a function of camera pitch, and the clamp permits the bad range.** Vertical
half-FOV is fixed at `atan(tan(FOV/2) / AspectRatio)` = 17.98 degrees for FOV 60 at 16:9 (see the
entry above), so the top of frame sits at `Pitch + 17.98` degrees. At the default Pitch -40 that is
22 degrees below horizontal — every pixel looks downward and the case is unreachable. But
`ARTACCombatCamera::PitchDegrees` is clamped to (-89, -1), and for **|Pitch| < 17.98** the horizon
enters the frame and above-horizon pixels produce upward rays.

**Probed live, September 26, 2026, at Pitch -10.** Horizon computed at py 118.8;
`RTAC.ScreenToGrid 475.5 60.0` returned `NO HIT (returned false)`. The intersection parameter is
-2867.5 — about 2868 units behind the camera — putting the hit at board-local X = -3825, so
`floor(-19.13)` = -20 and `IsValidPosition` rejects it. **The return value is correct; the reason is
not.** The bounds check is doing work a sign guard should do. Return value observed; the
intermediate is derived, since the probe deliberately logs no intermediates.

**Status: latent, mechanism confirmed, no false positive reachable via `ARTACCombatCamera`** —
`FrameBoard()` always looks at the board, so the board is in front and a behind-camera intersection
cannot land inside the grid footprint. It would become a false positive only for a view target
whose backward ray crosses the board plane inside the grid — a camera facing away from the board,
possible once something other than `ARTACCombatCamera` is the view target. Not demonstrated. The
exactly-parallel NaN case is also unobserved and hard to hit deliberately.

---

## Editor and Asset Tooling (UE5.8)

### A targeted level save reports success without persisting actor deletions (One File Per Actor)

**Observed September 25, 2026.** After removing twelve actors from `Lvl_ThirdPerson` through the
editor, saving the level *by path* — `save_assets(["/Game/ThirdPerson/Lvl_ThirdPerson"])` via the
MCP `AssetTools` toolset — returned `true` and persisted nothing. The deleted actors' packages kept
their original timestamps and the `.umap` was untouched. `save_assets([])`, which saves all dirty
assets, removed them correctly.

**Why.** This project's maps use One File Per Actor: each actor lives in its own package under
`Content/__ExternalActors__/<Map>/`, not inside the `.umap`. Deleting an actor dirties *that
actor's* package and marks it pending-delete. The level package itself need not be dirty at all,
so a save scoped to the level path has nothing to do and truthfully reports success — while the
external packages sit unsaved on disk.

**The trap is that the return value is not a lie, and is not useful.** `true` means "the asset you
named was handled," not "your deletion is now on disk." Any cleanup verified by a save's return
value will report clean while the files remain.

**Guard.** After deleting actors, save all dirty assets rather than a named path, and verify
against the filesystem rather than the return value — `git status`, or timestamps under
`Content/__ExternalActors__/`. Note `git status` can mislead in the other direction too: once the
`.uasset` files go, the now-empty hash directories remain, and git does not track empty
directories, so a clean status is consistent with leftover empty folders. Those are harmless.

**Verified live:** twelve `remove_from_scene` calls, all `true`;
`save_assets(["/Game/ThirdPerson/Lvl_ThirdPerson"])` → `true`, files unchanged at 18:09:07;
`save_assets([])` → `true`, all six `.uasset` files gone, tracked `__ExternalActors__` count
unchanged at 479, `git status --untracked-files=all` empty.

---

## Verifying a Live Engine-Source Claim Requires More Than a Clean Log Line

A single "it worked" log line only becomes evidence when paired with checks that could have failed
and didn't. The procedure below was established September 26, 2026, while verifying a live claim
about `UWorldSubsystem` lifecycle and lookup behavior for the Match-State Ownership question
(`combat_decisions.md` → Open Questions → "Match-State Ownership"). It is recorded here as a
reusable pattern, not tied to the throwaway probe it was first run against; that question's own
findings belong in `combat_decisions.md` and are not restated here (Failure Mode 7).

**1. DLL timestamp, not just build success.** A successful build does not prove the running editor
is executing the change: UBT can skip relinking (`CLAUDE.md` → How to Build → build verification),
and an editor that was not restarted keeps the DLL it already loaded. Confirm the DLL's on-disk
timestamp postdates every changed source file, AND confirm the running editor's own module-load log
line postdates the DLL write.

- *How it was run:* `ls -l --time-style=full-iso` on
  `Plugins/RTAC/Binaries/Win64/UnrealEditor-RTAC.dll` and on each changed source file. The
  module-load line is `LogRTAC: RTAC module loaded.` (`RTACModule.cpp:9`) in
  `Saved/Logs/ProjectAtlantis.log`. **Log timestamps are UTC, file times are local** —
  `[2026.09.26-16.35.41]` is 12:35:41 EDT — so convert before comparing.
- *Judgment call — newer is not the same as containing the change.* A timestamp proves the file was
  rewritten, not what went into it. `grep -c -a "<string unique to the change>"` on the DLL checks
  content directly: nonzero after adding code (used this way September 26, 2026), zero after
  removing it. For a removal there is no source timestamp left to compare against, so the content
  check becomes the primary evidence.

**2. Single-editor-instance check.** Confirm exactly one editor process is running, and that no
secondary log file has been written during the current session, before trusting any MCP read —
MCP can silently bind to the wrong editor instance without erroring (`CLAUDE.md` → MCP, the
September 2, 2026 incident).

- *How it was run:* `tasklist //FI "IMAGENAME eq UnrealEditor.exe"` from Git Bash, and
  `ls -l --time-style=full-iso Saved/Logs/ProjectAtlantis_2.log`.
- *Judgment call — check the secondary log's timestamp, not its existence.* A `_2.log` outlives the
  session that wrote it: one last written September 2, 2026 — the date of the two-editor incident
  above — was still present on September 26. Its presence alone says nothing about a second editor
  running now; a modification time inside the current session does.

**3. Cross-check MCP reads against the disk log directly.** Read the claim through MCP, then
independently grep the same claim in the raw log file on disk. Agreement between the two rules out
an MCP-specific reporting problem.

- *How it was run:* `EditorToolset.LogsToolset` → `GetLogEntries(category="LogRTAC",
  pattern="<unique string>", maxEntries=50)`, then
  `grep -n "<unique string>" Saved/Logs/ProjectAtlantis.log`.
- *Judgment call — take a baseline first.* Count the same string in the disk log *before* the run.
  A baseline of zero is what makes the post-run line attributable to the run rather than to an
  earlier session's leftover output.

**4. Positive AND negative controls, together.** A negative control (the thing you expect to be
ABSENT actually resolves as absent) only means something if paired with a positive control proving
the lookup mechanism itself still works (something you expect to be PRESENT actually resolves). An
absent result with no positive control is ambiguous between "correctly absent" and "the tool
silently failed."

- *How it was run:* `editor_toolset.toolsets.object.ObjectTools` → `get_class` on an object path —
  `/Game/ThirdPerson/Lvl_ThirdPerson.Lvl_ThirdPerson:<Name>_0` for the editor world,
  `/Game/ThirdPerson/UEDPIE_0_Lvl_ThirdPerson.Lvl_ThirdPerson:<Name>_0` for the PIE world. An absent
  object returns the error `… is not valid Object for property 'instance'`. Positive control: an
  engine world subsystem known to be present in the same world, `…:WorldMetricsSubsystem_0`.
- *Side effect to expect:* each rejected lookup also writes
  `LogScript: Warning: <path> is not valid Object for property 'instance'` to the editor log. Those
  lines are the negative control's own footprint, not failures — match their timestamps to the
  lookups before reading anything into them.

**5. Distinguish "absent" from "never registered."** Where possible, confirm via a separate
registration/class-list lookup that the thing being checked for absence was actually loaded and
reflected by the engine, and its absence from a specific context is due to the mechanism under test
(e.g. `DoesSupportWorldType`), not a load/registration failure.

- *How it was run:* `ObjectTools` → `search_subclasses(base_class=/Script/Engine.WorldSubsystem,
  class_name="<Name>")`, which returns the class's `/Script/<Module>.<Class>` path when the class is
  registered.

**6. Watch for truncated or artificially limited command output.** A search or log read that
silently caps its own results (e.g. piping through `head`, a tool's default result limit) can read
as a complete, clean result when it is actually partial. During this same verification work, the
claim "no `final` world-subsystem precedent exists in 5.8" was reported and was wrong — the
underlying grep had been piped through `head -5` and returned exactly five hits, which was misread
as an exhaustive negative result rather than a truncated one. At least two ship with the engine,
both under `Engine/Plugins/WorldMetrics/Source/`: `UWorldMetricsSubsystem`
(`WorldMetricsCore/Public/WorldMetricsSubsystem.h:28-29`) and `UCsvMetricsSubsystem`
(`CsvMetrics/Public/CsvMetricsSubsystem.h:16-17`). Confirm a search or log read is unbounded, or
explicitly account for the possibility that a negative result is an artifact of a limit, before
treating an absence as confirmed.

- *The tell:* a result count exactly equal to the cap. `GetLogEntries` carries one too —
  `maxEntries`, default 1000 per `CLAUDE.md` → MCP.
- *Second occurrence, September 26, 2026 — a listing, not a search.* A time-sorted listing cut to
  its newest entries (`ls -l *.log | sort -k6,7 | tail -5`, run to check point 2) hid an older
  `ProjectAtlantis_2.log`, which was then reported as absent. A newest-first cutoff hides exactly
  the stale files point 2 has to account for.

---

*Created August 29, 2026, per Rule 13 (system date checked live before writing). Structure is
meant to extend indefinitely — each future verified API area gets its own `##` section following
this same pattern: what was checked, the exact citation, and any judgment calls made along the way
that a future reader would otherwise have to re-derive.*
