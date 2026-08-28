# AGENTS.md — FOR ATLANTIS Safety Ruleset

This document is the **canonical Safety Ruleset** for FOR ATLANTIS. Rules are non-negotiable
invariants — on agent conduct now, and on architecture as soon as combat code exists.
Violations are regressions, not style issues.

For project layout, build process, MCP setup, and UE5.8 API gotchas, see [`../CLAUDE.md`](../CLAUDE.md).
For combat design rulings, see [`combat_decisions.md`](combat_decisions.md).

---

## Provenance and Status

This ruleset is **adapted from the PRS Safety Ruleset**
(`D:\Dev\PerceptionRenderingSystem\docs\AGENTS.md`, last updated August 16, 2026). PRS is a
physics/rendering project; FOR ATLANTIS is a game. Rules that were PRS-specific (Eigen lifetime,
no-Unreal-headers-in-PRSCore, UE5Bridge) are dropped. Rules about agent conduct, state ownership,
pipeline discipline, and epistemics transfer directly, and are the majority of the value.

**Every `Caught:` entry below is a real incident on PRS, not on this project.** They are kept
because they are the evidence for why each rule exists, and because the same agents, the same
engine, and the same machine are involved here. **No incidents have been logged against FOR
ATLANTIS yet.** When one occurs, log it here with its date, in the same format — do not delete
the PRS case that predicted it.

**Adoption status:** Rules 1–4 (Conduct) are in force **now**. Rules 5–11 (Architecture) bind
combat code as it is written; there is none yet, so they currently read as design constraints
rather than audit targets.

Mapping back to PRS rule numbers is given per rule, so a cross-project conversation can cite
either.

---

# Part I — Conduct Rules (in force now)

### Rule 1 — Research/Apply Separation (Mandatory)
*PRS Rule 9*

Any message that asks for research, source verification, or a design recommendation must be
answered with **text only** — no code, no diff, and nothing presented as a one-click applicable
change.

- A diff is drafted and shown only in a SEPARATE, later message, after the research/design
  answer has been given and explicitly approved on its own.
- If a response to a research-only request includes a diff anyway, that is a signal to stop and
  re-ask before proceeding — not something to treat as pre-approved because it happened to appear.
- This applies symmetrically: prompts asking for research must say so explicitly, and must not
  bundle an implementation step into the same message.

**Caught (PRS, T11, July 1, 2026):** a research/design prompt for AA API verification was
answered with an already-applied code change instead of a text answer, twice in a row, including
once after an explicit hold instruction. Caught and manually reverted.

**Enforcement tightening (PRS, August 12, 2026 — carried over, in force here).** The same failure
recurred: a research/summary response was followed immediately by an unapproved file write,
directly after an explicit "show the diff before applying" instruction. Self-reporting after the
fact does not satisfy this rule. For CC/Opus specifically:

- CC/Opus must not call any file-write or file-edit tool in a research, verification, or
  design-recommendation task under any circumstances, regardless of confidence or how minor the
  change appears.
- Before CC/Opus performs **any** file-write or file-edit — including applying a diff already
  shown and discussed in a prior message — it must stop and request verbal confirmation from
  Omar **in that specific instance**. A standing or general approval earlier in the conversation
  does not count.
- This is a temporary tightening, not a permanent demotion. Revisit once CC/Opus demonstrates
  reliable compliance across several consecutive tasks.

**Why it matters more here than on PRS:** this project has no version control (Rule 3). An
unapproved write on PRS was recoverable from git. Here it is not.

---

### Rule 2 — Live-Document Verification Before Any Status Claim (Mandatory)
*PRS Rule 10*

No claim about project state — what is "done," "outstanding," "blocked," "already decided," or
"still needs to happen" — may be made by Claude (claude.ai), CC, or CC/Opus without **first
reading the specific governing document live, in that turn.** This applies even when the claim
feels like it is restating something established earlier in the same conversation, from memory,
or from a prior AI's summary.

**Governing documents for this project:**

| Claim about… | Read first |
|---|---|
| Any combat design ruling, what's decided vs. deferred | `docs/combat_decisions.md` |
| Build process, MCP, project layout, engine version | `CLAUDE.md` |
| These rules, or whether something is a violation | `docs/AGENTS.md` (this file) |
| Whether code exists / what a class does | The source file itself — not the module map in `CLAUDE.md`, which is a summary |
| Any UE5.8 API behavior | `C:\Program Files\Epic Games\UE_5.8\Engine\Source\` |

**Standing requirement:**
- Before stating what a decision's status currently is: read the file that governs it, live.
- Before proposing or initiating any task premised on "X hasn't happened yet" or "Y is still
  needed": verify against the live governing document, not a summary, a memory, or an earlier
  AI's claim in the same conversation.
- A prior AI's status claim in this same conversation is not a substitute for a live read, even
  seconds later, even when it looks authoritative.
- If a live read is genuinely impractical in the moment, state the claim as **unverified**
  explicitly. Do not present it as fact.
- **This applies to Claude, CC, and CC/Opus equally.** It is not satisfied by one of the three
  verifying and the others trusting that verification secondhand within the same task.

**Caught (PRS, August 16, 2026), twice in one session:** a "still NOT STARTED" status claim
relayed and acted on without a live read, when the file had said otherwise for over two weeks;
and a measurement task initiated before reading the protocol that already invalidated its premise.

**Why this is distinct from a knowledge gap:** in both cases the correct information was sitting
on disk, unambiguous, and simply was not read before being reasoned about or acted on.

---

### Rule 3 — No Undo: Destructive Actions Require Explicit Confirmation (Mandatory)
*New — specific to this project*

**FOR ATLANTIS is not under version control as of August 26, 2026.** Nothing is recoverable
after an overwrite. Until git exists, every destructive action is permanent.

- Before any file deletion, mass rename, bulk edit, or full-file overwrite: state exactly what
  will change and get explicit confirmation. Reading first is mandatory — never overwrite a file
  whose current contents you have not read in this session.
- Prefer additive changes. Prefer a targeted edit over rewriting a file.
- Never bulk-operate on `Content/__ExternalActors__/` or `__ExternalObjects__/`. They are
  machine-generated one-file-per-actor packages, edited only through the editor, and the engine
  drops or rewrites them under operations that look harmless from the filesystem.
- MCP edits to Blueprints are destructive in the same way and are covered by this rule.
- **Raise setting up git whenever the topic is open.** This rule is a workaround for a missing
  safety net, not a substitute for one. It should be retired, not lived with.

**Addendum, August 26, 2026:** as of today, the project has git and a remote at
`https://github.com/cviper7332/FOR-ATLANTIS-GAME.git` (branch `main`, initial commit `207d361`).
See `CLAUDE.md` → Version Control for details. "No undo" above no longer means "no safety net" —
a committed and pushed state is recoverable. It does **not** retire this rule: uncommitted local
changes, and any operation run before a change is committed, are still unrecoverable exactly as
described above, and care before destructive actions remains good practice regardless. Treat this
as the safety net finally existing, not as permission to be less careful with it.

---

### Rule 4 — Append, Don't Rewrite: The Decision Log (Mandatory)
*From the format convention already established in `combat_decisions.md`*

`docs/combat_decisions.md` is the authoritative record of what has been decided.

- Numbered decisions, each with `Date`, `Phase`, `Author`, `Status` in that order under the heading.
- `Status` is set at creation, never backfilled.
- **Once an entry exists, it is not silently edited.** Corrections are appended as dated addenda
  that state what the entry previously said and why it was wrong. The PRS decision log does this
  consistently and it is why its corrections are auditable.
- Controlled `Status` vocabulary: `CLOSED — fixed in <commit>`, `CLOSED — enacted in <commit>`,
  `PARTIAL — <done>; OUTSTANDING: <remains>`, `OPEN`, `N/A — design rationale, no action implied`,
  `CLOSED — superseded by Decision #N` (citing the superseding number is mandatory).
- A conclusion reached in conversation is **not decided** until it is in the file. When a chat
  settles a design question that isn't logged, propose an entry.

---

### Rule 12 — Diffs Must Be Shown as Their Own Block, Not Buried in Tool Output (Mandatory)
*New — established August 26, 2026, after a diff shown only inside a Bash tool-output block was
treated as "shown" when it had not actually been reviewed as a distinct thing to approve.*
*Numbered 12, not 5, on purpose — placed in Part I because it is a conduct rule, not an
architecture rule, but appended after Rule 11 rather than renumbering Part II's Rules 5–11.
Rules 1–4 and 12 are Conduct; Rules 5–11 are Architecture. The gap in sequence is intentional,
not an error.*

Any diff presented for review — before a commit, before an overwrite, before any destructive
action — must be printed as its own explicit block in the reply itself (a fenced code block, or
the diff clearly reproduced outside of a raw tool-call/tool-output wrapper), not left sitting
only inside a Bash tool's IN/OUT transcript.

- A diff inside a Bash output block reads as *evidence a command ran*, not as *content being
  submitted for approval* — the two are easy to conflate, and conflating them is exactly how an
  unreviewed diff gets treated as reviewed.
- This applies regardless of tier (CC or CC/Opus) and regardless of how mechanical the
  underlying change seems.
- Showing the diff via `git diff` inside a Bash call is still fine as the mechanism to *generate*
  the diff — the requirement is that the diff's actual content is then also reproduced as its own
  visible block in the reply, not left implicit in the tool transcript alone.

---

# Part II — Architecture Rules (bind combat code as it is written)

### Rule 5 — Simulation / Presentation Separation (Mandatory)
*PRS Rules 1, 4 and 6, adapted*

The combat **simulation** — grid state, tile properties, elevation, HP, damage resolution, turn
and timing rules — must be expressible and testable without a running engine.

- Simulation types hold POD, standard containers, and other simulation types. No `AActor*`,
  no `UObject*`, no `FVector` inside the rules layer. Grid coordinates are grid coordinates,
  not world transforms.
- **Presentation owns** actors, components, meshes, materials, cameras, VFX, UMG widgets, and
  UObject lifetime. **Simulation owns** grid state, tile modifiers, elevation values, entity
  stats, and resolution rules.
- Presentation reads simulation state and renders it. Simulation never reads presentation state,
  never queries a camera, and never asks where an actor currently is on screen.
- Decision #1 makes this concrete: the camera is isometric 2.5D, but *"grid logic remains
  flat/2D underneath."* If a camera change would require touching grid code, this rule is
  already broken.
- Decision #3 makes it concrete again: elevation is *"a tile property independent of those
  BN3-style modifiers"* — a value in the simulation, not a Z offset discovered from a mesh.
- If simulation types must appear in a `UCLASS` header, use pimpl. Forward declarations go at
  file scope, not nested in the class body (see `CLAUDE.md` → UE5.8 API Gotchas).

**Why:** a grid combat system that can only be exercised by launching PIE and playing it cannot
be regression-tested, and every balance change becomes a manual playtest. This rule is what makes
Failure Modes 3–5 checkable at all.

**Also:** Decision #2 puts PHIS on the far side of a hard boundary — no dependency in either
direction. Combat must not read faction belief state; PHIS must not consume combat outcomes.
Introducing one requires a new decision entry, not a convenient include.

**Addendum, August 26, 2026:** the phrase "expressible and testable without a running engine"
above conflates two separate requirements, and only the first applies to this project.
**Expressible without engine types stands unchanged** — no `AActor*`, no `UObject*`, no
`FVector` in the rules layer, POD and standard containers only, exactly as the bullets above
describe. **Testable without a running engine does not.** That half was inherited from PRS,
where a standalone non-UE5 build was a genuine project goal — PRSCore is engine-agnostic by
design and CI-gated on compiling with no Unreal headers.
RTAC (realtime arena action strategic combat system) has no such goal: its portability target is
"across UE5 projects," not away from UE5 (see `RTAC.uplugin`). Going
forward, simulation code may be compiled and tested through UE5's own tooling — UE Automation
Tests, running inside the editor — and a separate engine-agnostic build system is neither
required nor to be built. The **Why** paragraph above is unaffected: its actual concern is that
combat not be exercisable *only* by launching PIE and playing it by hand, and an
automation-test suite satisfies that fully. Caught during `PHASES.md` drafting.

**Addendum #2, August 26, 2026:** the bullet above ("No `AActor*`, no `UObject*`, no `FVector`
inside the rules layer") bundles three separate claims under one prohibition, and only two of
them still hold on their own merits once the prior addendum's engine-independence goal is set
aside. This addendum narrows the bullet only — Decision #1/#3's role in this rule, and the
**Why** paragraph above, are both unaffected.

- **Simulation/presentation separation itself is unchanged** — still fully justified independent
  of PRS, and independent of everything below.
- **No `UObject*`/`AActor*` ownership in simulation state stands, re-justified.** The original
  rule implied engine-independence and version-drift resistance as its reasons; neither survives
  scrutiny (see `combat_decisions.md` Decision #6). The actual reasons: (1) test cost — a test
  over plain structs is a function call, a test over `AActor`s needs a `UWorld`; (2) Rule 6
  determinism — `UObject` lifecycle, GC timing, and construction/registration order make
  bit-identical replay from a seed genuinely hard, where POD makes it close to free; (3) future
  serialization — save state, replays, and seed+input bug reproduction are cheap over POD and
  expensive to retrofit onto an actor graph. None of these three require the engine to be
  absent, only that simulation *state* not be owned by actor-lifecycle objects.
- **The blanket `FVector` prohibition is dropped.** `FVector` is a POD math type in the engine's
  `Core` module — banning it bought nothing once engine-independence was off the table.
  **UE Core value types — `FIntPoint`, `TArray`, `TMap`, `FString` — are explicitly permitted,
  and preferred over hand-rolled or `std::` equivalents,** since this is a UE5-only plugin
  (Rule 11) and Core types interoperate with the rest of the codebase without translation at the
  presentation boundary.
- **`FVector` specifically should still not be used for grid coordinates — but that is Rule 10's
  concern, not this rule's.** A grid coordinate is a discrete rows×columns index (Decision #5),
  not a float 3-vector; using `FVector` for one would be a domain-discipline violation (grid
  coordinates vs. world units), regardless of which module the type lives in. Cited here
  explicitly so dropping the blanket engine-type ban doesn't read as license to blur that
  boundary — `FIntPoint` remains correct for a grid coordinate, `FVector` remains wrong for one,
  for an entirely different reason than "it's an engine type."

---

### Rule 6 — Explicit State Ownership and Determinism (Mandatory)
*PRS Rule 3*

No hidden state in the simulation layer.

- All per-match temporal state lives in an explicit state struct, passed in and out each tick by
  the caller. No hidden globals, no mutable statics, no function-local statics.
- **RNG state lives in that struct and is seeded explicitly.** A real-time grid game with chip
  draws, enemy behavior, and (per Decision #3) possible tile instability will need randomness;
  if it comes from a global generator, no match is reproducible and no bug report is actionable.
- Mode and phase switches must explicitly reset or transition state. A `reset()` is a complete
  standalone reinitialization — it does not depend on some earlier init call having run.
- Same seed plus same inputs must produce the same match. Treat any divergence as a bug in this
  rule, not a curiosity.

---

### Rule 7 — Fixed Tick Order (Mandatory)
*PRS Rule 6*

Every combat frame follows one declared order, and no stage reads the output of a later stage.

```
Input  →  Simulation tick  →  Resolution  →  Presentation/Render
```

Inputs and outputs at each stage are declared explicitly; immutability between stages is
preserved. When the order is settled for real, write it here and in a decision entry — a
real-time grid game's feel lives in this order, and reordering it silently changes the game.

**Engine-level counterpart:** the PRS project lost time to a capture that ticked before the
camera was final for the frame, causing movement judder, fixed by moving it to
`TG_PostUpdateWork`. If any Atlantis component's correctness depends on running after another,
set its tick group and tick dependency explicitly rather than relying on default ordering.

---

### Rule 8 — Independent Feature Gating (Mandatory)
*PRS Rule 8*

Each system's enable/disable state and update logic must be gated **exclusively by its own
state**.

- No system's update may be nested inside, or short-circuited by, another system's guard
  condition. An early return keyed on one feature must never gate an unrelated one.
- Each system gets its own top-level gate. Adding a new system must never require folding its
  logic into an existing system's `if` block as a nested branch.
- This applies to any shared entry point — a combat manager tick, a game mode update, a
  player controller input router.

**Caught (PRS, T10c draft):** thermal capture was placed after an NVG-specific early return, so
enabling thermal alone with NVG off would silently capture nothing. Caught in review before
implementation.

**Why this one is easy to miss:** violating it doesn't crash and doesn't produce visibly wrong
output when both features happen to be exercised together. It surfaces only when one is used
independently of the other — exactly the untested-condition gap Failure Mode 5 is about.

**Concrete risk here:** elevation (Decision #3) is explicitly built *after* the core BN3 loop is
playable. If the core loop's tile update is written so elevation can only be added as a nested
branch inside it, that is this rule being violated in advance.

---

### Rule 9 — Debug and Observability (Mandatory)
*PRS Rule 7*

- Major simulation entry points log failures with a context string, under a dedicated log
  category (e.g. `LogAtlantisCombat`) — not `LogTemp`.
- Debug builds may log state transitions and tick-index mismatches.
- **Diagnostic instrumentation added for tuning — per-frame log spam — must be removed before
  the work is called done.** A per-second diagnostic left in the production path is a defect on
  PRS's list; it will be one here.

---

### Rule 10 — Units and Domain Discipline (Mandatory)
*PRS Failure Mode 2, generalized*

The same number means different things in different domains. Name the domain of every stage's
input and output explicitly.

For this project the domains that will collide are:

- **Grid coordinates vs. world units.** A tile index is not centimetres. Conversions happen at
  exactly one boundary, in one named function, in the presentation layer.
- **Frames vs. seconds.** BN3-derived timings are often authored in frames at a fixed rate; UE
  ticks in variable delta seconds. Pick one authoritative representation for design data and
  convert once, with the factor written down. A "20" that is sometimes frames and sometimes
  ticks is the same class of bug as PRS's NETD-in-the-wrong-domain.
- **Elevation levels vs. height.** Per Decision #3 elevation is a tile property. It is a discrete
  level in the simulation; any world-space height is a presentation-side lookup from that level.
- **Screen space vs. grid space.** Isometric 2.5D (Decision #1) means the two are related by a
  projection, not by an axis swap. Hit-testing belongs in grid space.

---

### Rule 11 — Combat Code Lives Inside the RTAC Plugin (Mandatory)
*New — established alongside RTAC's creation, August 26, 2026*

All combat-specific code — grid state, tile logic, movement, damage resolution, combat UI,
combat-specific actors/components — lives inside `ProjectAtlantis/Plugins/RTAC/`, not in the
main project's `Source/ProjectAtlantis/` module.

- This is what makes RTAC portable to a future UE5 project, per its stated purpose
  (`RTAC.uplugin` → "portable across UE5 projects"). Code that leaks into the main module breaks
  that portability silently — it won't fail loudly, it'll just mean RTAC doesn't actually work
  when copied elsewhere.
- The only things that belong in the main project relating to combat: whatever thin integration
  is required to *invoke* RTAC (e.g., a gamemode/state flag that triggers combat start, per the
  exploration-vs-combat mode question still open in `combat_decisions.md`), not the combat logic
  itself.
- `Variant_Combat/` (Epic's own template code) is exempt — it's reference material only, per
  `CLAUDE.md`, not something this rule retroactively applies to.
- Before adding any new combat-related source file, confirm it's going into `Plugins/RTAC/`,
  not `Source/ProjectAtlantis/`.

---

# Recurring Failure Modes — checklist before writing systems or tests

These are real bugs that reached "looks correct" before being caught on PRS. Each cost a
multi-session investigation. Run this list before writing any combat stage, any balance constant,
or any test oracle. Phrased as checks, not stories.

## Design / tuning

**1. A constant the output is invariant to is inert, a silent bug, or regulated away — never
"working" by default.**
If changing a tuning value doesn't change the output, find out *which* of the three before
shipping it as if it matters. A factor appearing in both a computation and its inverse cancels
exactly (inert / bug). A factor feeding a self-correcting system can be correctly invariant at
steady state while still being live during a transient.

- *Bit PRS:* a lens gather term cancelled in normalization, so f-number and transmission had zero
  effect for an entire phase. Separately, a gain constant cancelled exactly at controller steady
  state — correct behavior, indistinguishable from a bug under a steady-state-only test.
- *Guard:* for every new balance scalar — damage multiplier, elevation modifier, tile effect
  strength, cooldown scale — write a test that varies it and asserts the output moves in the
  intended direction **and magnitude**, and state which condition the test runs under. If it
  can't move the output under any condition, delete it or document it as deliberately inert.
- *Live risk here:* Decision #3 requires elevation to be *situationally* strong, never a
  strictly-better axis. That is precisely a claim about a parameter's effect being
  condition-dependent — so it cannot be validated by a single steady-state test, and "it seemed
  to work in a playtest" is not evidence.

**2. Balance numbers belong in one domain, stated explicitly.** See Rule 10. A damage number
applied pre- vs. post-mitigation, or a modifier applied additively vs. multiplicatively, are
different systems wearing the same constant.

## Test / validation

**3. An oracle must measure the same quantity, statistic, and domain the system actually
produces — through the same transformations.**
Comparing output to "expected" is worthless if "expected" was computed differently.

- *Bit PRS:* a controller regulated the 95th percentile while the oracle used the mean, producing
  a phantom error that was chased for days. Another test compared a ratio against a fourth-root.
- *Guard:* before asserting `output == expected`, state in one sentence what quantity and units
  the system emits, and confirm `expected` is computed in those same terms.

**4. Verify the whole chain with a worked numerical example, not each formula in isolation.**
Most of these were *composition* errors — correct pieces, wrong assembly — which per-function
review cannot catch.

- *Guard:* trace one concrete scenario end to end by hand — this attacker, this chip, this tile,
  this elevation, against this defender — agree the resulting number, then require the
  implementation to reproduce it. Add a round-trip identity test wherever one exists.

**5. Test on a representative configuration; a degenerate setup can mask real behavior.**

- *Bit PRS:* a controller test ran on a single pixel, where the regulated statistic was trivially
  equal to the mean by construction. It reported 0.00% error and hid the real behavior for two
  phases.
- *Guard:* if the deployed path runs on a grid, test on a grid — not one tile. Ask what the
  degenerate case collapses, and whether that collapsed thing is the thing under test.
- *Live risk here:* a combat test on a flat all-same-elevation grid collapses exactly the axis
  Decision #3 introduces. A test with one enemy collapses everything about targeting and
  telegraphing. An open question in `combat_decisions.md` already asks whether a 3x6 grid gives
  elevation enough room to read — that question cannot be answered on a degenerate board.

## Process / epistemics

**6. When a question is a fact that lives in source, read source before deliberating.**
Multi-model discussion is for judgment, not lookups.

- *Bit PRS:* a contradiction between two constants drove a four-model debate over a value that
  was one `grep` away in a header. The docs were stale; the code was right.
- *Guard:* sort each open question into "fact in source" vs. "judgment call." Facts → read the
  file. Only judgment calls get the panel.
- **Mechanism claims are facts in source, not judgment calls.** Not only constants: any claim
  about *how something behaves* that lives in checkable source — engine APIs, MCP tool semantics,
  Blueprint wiring, build-system behavior, or what a doc comment asserts the code does. "The
  engine drops external actor packages on duplicate" is a lookup, not an opinion, and it stays a
  lookup however confidently it is stated. These misfile as judgment calls precisely because they
  *feel* like reasoning rather than retrieval.
- *Guard:* before a mechanism claim is used as a **premise** — for an instruction, a plan, or
  another decision — name the check that would falsify it, and run it. Where a check is genuinely
  unavailable before answering, the claim is **labelled a hypothesis and never used as a
  premise**. An unrequested restriction is flagged as a proposal when introduced, not asserted as
  a constraint.
- *Bit PRS three times in one session:* two doc comments describing behavior that did not exist;
  a Blueprint key documented as hold-to-enable when the graph wired a press-to-toggle FlipFlop;
  and two successive wrong claims about whether asset duplication preserves One-File-Per-Actor
  packages. Every refutation was one tool call away, and in two of three the claim had already
  become a premise for instructions.

**7. One quantity, one authoritative location. Duplication drifts.**

- *Bit PRS:* two sources of delta-time; one noise constant written in code and again in three
  docs that fell out of sync.
- *Guard:* every shared value — grid dimensions, tick rate, damage constants, tile modifier
  strengths — has exactly one source of truth; everything else references it. When one changes,
  reconcile all copies. **This file, `CLAUDE.md`, and `combat_decisions.md` are subject to the
  same rule:** a number written in two of them will drift.

**8. A confident, fluent answer can be wrong on mechanism — including from a model. Cross-check
against computation or experiment, not against plausibility.**

- *Bit PRS:* a confident explanation for a measured offset was falsified only because the
  diagnostic experiment was actually run; the real cause was something else entirely.
- *Guard:* when a mechanism is asserted, design the cheapest experiment that distinguishes it
  from the alternative, and run it. **Agreement across several models is not verification** — it
  can be one unchecked assumption wearing several signatures.
- *Bit PRS again, at the instrument level:* a regression proof ran on a stale binary. The
  "control" run returned output byte-identical to the regressed run because the build system
  silently skipped recompiling the one changed file. It was caught only because
  identical-to-the-digit agreement across a real source change is implausible.
- *Guard:* for any before/after proof, confirm the changed file actually recompiled and that
  control and experiment outputs genuinely differ before trusting either. See `CLAUDE.md` →
  build verification: check the `UnrealEditor-ProjectAtlantis.dll` timestamp. **An experiment
  that cannot fail is not evidence.**

---

## Audit Procedure

Before closing any milestone, verify each rule:

| Rule | Check |
|---|---|
| 1 — Research/Apply Separation | Research/design questions answered text-only; every write had its own explicit go-ahead |
| 2 — Live-Document Verification | Every project-state claim was preceded by a live read of the governing document, in that turn |
| 3 — No Undo | No file deleted, overwritten, or bulk-edited without prior confirmation; no hand-edits under `__ExternalActors__` / `__ExternalObjects__` |
| 4 — Append, Don't Rewrite | No existing decision entry silently edited; corrections present as dated addenda |
| 5 — Simulation/Presentation | No `UObject*`/`AActor*`/`FVector` in the rules layer; no simulation→presentation reads; no PHIS dependency either direction |
| 6 — Explicit State | No statics or globals in the simulation; RNG seeded from explicit state; same seed + same inputs → same match |
| 7 — Fixed Tick Order | Input → Simulation → Resolution → Presentation; no stage inversion; tick groups set explicitly where order matters |
| 8 — Independent Gating | No system's update path nested inside another system's guard condition |
| 9 — Observability | Dedicated log category, not `LogTemp`; no per-frame diagnostic spam left in the production path |
| 10 — Units and Domains | Grid vs. world, frames vs. seconds, elevation level vs. height, screen vs. grid — each converted at exactly one named boundary |
| 11 — Combat Code Lives Inside RTAC | No combat logic (grid, tiles, movement, damage resolution, combat UI/actors) added under `Source/ProjectAtlantis/`; new combat source goes in `Plugins/RTAC/` |
| 12 — Diffs Shown as Their Own Block | Every diff presented for review before a commit/overwrite/destructive action was printed as its own visible block in the reply, not left only inside Bash tool output |

---

*Created August 26, 2026 — adapted from `D:\Dev\PerceptionRenderingSystem\docs\AGENTS.md`
(PRS Safety Ruleset, last updated August 16, 2026). PRS Rules 2 (Eigen lifetime) and 5 (no Unreal
headers in PRSCore) dropped as project-specific; PRS Rules 1/4/6 merged into Rule 5
(Simulation/Presentation Separation); Rule 3 (No Undo) added for this project's lack of version
control; Rule 4 (Append, Don't Rewrite) promoted from the format convention in
`combat_decisions.md`; Failure Mode 2 promoted to Rule 10 (Units and Domain Discipline). All
`Caught:` cases are PRS incidents — none have been logged against FOR ATLANTIS yet.*
