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

---

*Created August 29, 2026, per Rule 13 (system date checked live before writing). Structure is
meant to extend indefinitely — each future verified API area gets its own `##` section following
this same pattern: what was checked, the exact citation, and any judgment calls made along the way
that a future reader would otherwise have to re-derive.*
