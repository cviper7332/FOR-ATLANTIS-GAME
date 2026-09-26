#include "CoreMinimal.h"

#if !UE_BUILD_SHIPPING

#include "RTACModule.h" // LogRTAC -- Rule 9: dedicated category, never LogTemp.
#include "Presentation/RTACBoard.h"
#include "Presentation/RTACGridConversion.h"
#include "Simulation/RTACGrid.h"
#include "Simulation/RTACGridPosition.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"

/**
 * Runtime probes for RTACScreenToGridPosition -- Phase 2 Part A item 2's closing instrument.
 *
 * WHY THIS FILE EXISTS. RTACScreenToGridPosition's deprojection half had never executed, in any
 * build, ever. RTAC.Presentation.GridConversion.ScreenToGridPosition covers the geometry half
 * directly and the two camera-independent guard branches, and its own scope note says outright
 * that it stops short of the real DeprojectScreenPositionToWorld call -- that call needs a live
 * LocalPlayer with an attached viewport, which exists only inside a running PIE session. These
 * commands are the missing instrument: they drive the shipped function with a real
 * PlayerController inside PIE and report where a screen pixel actually lands.
 *
 * DIAGNOSTIC INSTRUMENTATION, NOT PRODUCTION API (Rule 9). On-demand only: nothing here runs per
 * frame and nothing logs until a command is typed, so there is no diagnostic spam to leave behind
 * in a production path. Header-less and under Private/ by design, so no production code can grow
 * a dependency on it, and compiled out of Shipping entirely by the guard above. This is a
 * deliberate removal candidate once Phase 2 Part B's real input layer exists.
 *
 * WHY A CONSOLE COMMAND AND NOT UFUNCTION(Exec). An Exec function on ARTACBoard or
 * ARTACCombatCamera would compile, read as correct, and silently never fire: exec dispatch is a
 * closed list. UPlayer::Exec (Player.cpp:112-156, UE 5.8) routes to PlayerInput, the
 * PlayerController, the Pawn, the HUD, the GameMode, the CheatManager, the GameState and the
 * PlayerCameraManager. A placed actor is not on it. Routing an Exec here would therefore have
 * required a PlayerController subclass -- project-side (Rule 11 violation) or an RTAC one nothing
 * yet needs. IConsoleManager registration needs neither.
 *
 * WHICH CONSOLE TO TYPE INTO, AND WHY THE EDITOR OUTPUT LOG WORKS. Either the PIE viewport
 * console or the editor's Output Log; the Output Log is the useful one, because that is where MCP
 * GetLogEntries reads from. Traced in 5.8 rather than assumed:
 * FConsoleCommandExecutor::ExecInternal sets World = GEditor->PlayWorld and enters that world
 * context (FConsoleCommandExecutor.cpp:95-99), then calls Player->Exec(PlayerWorld, ...) on
 * GEngine->GetDebugLocalPlayer() -- which during PIE is the PIE local player, because it returns
 * the first world context owning a GameInstance (UnrealEngine.cpp:14591-14606) and the editor
 * context owns none. ULocalPlayer::Exec falls through to ViewportClient->Exec
 * (LocalPlayer.cpp:1594) -> GEngine->Exec (GameViewportClient.cpp:3528) ->
 * ProcessUserConsoleInput(Cmd, Ar, InWorld) (UnrealEngine.cpp:5722) -> the delegates below, with
 * the PIE world carried through unchanged (ConsoleManager.cpp:2261-2264).
 *
 * WHY ECVF_Cheat, GIVEN THE SHIPPING GUARD ABOVE ALREADY REMOVES THIS FILE. The two are not
 * redundant: the guard covers Shipping, the flag covers Test. DISABLE_CHEAT_CVARS is
 * (UE_BUILD_SHIPPING || (UE_BUILD_TEST && !ALLOW_CHEAT_CVARS_IN_TEST)) (Build.h:440), and
 * IConsoleObject::IsEnabled() returns false for any ECVF_Cheat object when that is set
 * (IConsoleManager.h:466-475). FConsoleManager::ProcessUserConsoleInput then refuses the input at
 * ConsoleManager.cpp:3122 -- BEFORE it branches into AsCommand()/AsVariable(), so the flag gates
 * commands exactly as it gates variables. That is worth writing down because the flag's own doc
 * comment (IConsoleManager.h:72-76) says "Console VARIABLES marked with this flag", which reads as
 * though commands are exempt. They are not. Note what the flag does NOT do: in Development it does
 * not hide these from autocomplete, which filters on the same IsEnabled()
 * (FConsoleCommandExecutor.cpp:44) and is therefore true in Development.
 *
 * THIS FILE RE-DERIVES NO CONVERSION ARITHMETIC. It never recomputes a ray/plane intersection or
 * an inverse transform of its own. Doing so would test a copy of the boundary instead of the
 * shipped one -- both Failure Mode 7 and Rule 10's single-named-boundary requirement. The tile
 * marker and the sweep's forward leg go through RTACGridToLocalOffset, the real named function.
 * That is also why the marker highlights only the RESOLVED TILE and not the raw world-space hit
 * point: RTACScreenToGridPosition does not expose the hit, and duplicating the intersection in
 * order to draw it would defeat the entire purpose of the probe.
 *
 * THE THROWAWAY GRID IS NOT AN ANSWER TO MATCH-STATE OWNERSHIP. Hit-testing consults the grid
 * only through FRTACGrid::IsValidPosition, so Grid.Init(Board->Rows, Board->Columns) with every
 * tile left at its default is sufficient -- exactly the call shape RTACGridConversion.h already
 * describes ("Board holds no FRTACGrid of its own, so Grid is supplied separately by the
 * caller"). It carries no ownership, no tile state and no entities, and must not be read as a
 * step toward the Match-State Ownership Open Question (combat_decisions.md), which stays open.
 *
 * Rule 5: presentation calls into simulation and reads the result back, never the reverse.
 * Nothing under Simulation/ is touched, so Phase 2 Part B's falsifiable-test criterion -- an
 * empty git diff over Simulation/ -- is unaffected. Rule 8: each command registers its own
 * top-level console object, neither gates the other, and the shared context resolver below
 * refuses each command independently rather than short-circuiting one for the other's reason.
 */

namespace
{
	/** Everything a probe needs, resolved fresh per invocation -- no cached or hidden state. */
	struct FRTACProbeContext
	{
		APlayerController* PlayerController = nullptr;
		ARTACBoard* Board = nullptr;
		FRTACGrid Grid;
	};

	/**
	 * Resolves a probe's context, or logs exactly why it could not and returns false.
	 *
	 * Every refusal names the calling command, so two commands failing for different reasons in
	 * one log remain distinguishable.
	 */
	bool RTACResolveProbeContext(UWorld* World, const TCHAR* CommandName, FRTACProbeContext& OutContext)
	{
		if (World == nullptr || !World->IsGameWorld())
		{
			UE_LOG(LogRTAC, Warning,
				TEXT("%s: no game world (UWorld::IsGameWorld is false). This probe needs a running "
					 "PIE session -- the real DeprojectScreenPositionToWorld requires a live "
					 "LocalPlayer with an attached viewport, which the editor world has not got."),
				CommandName);
			return false;
		}

		OutContext.PlayerController = World->GetFirstPlayerController();
		if (OutContext.PlayerController == nullptr)
		{
			UE_LOG(LogRTAC, Warning, TEXT("%s: no PlayerController in the game world."), CommandName);
			return false;
		}

		int32 NumBoards = 0;
		for (TActorIterator<ARTACBoard> It(World); It; ++It)
		{
			++NumBoards;
			if (OutContext.Board == nullptr)
			{
				OutContext.Board = *It;
			}
		}

		if (OutContext.Board == nullptr)
		{
			UE_LOG(LogRTAC, Warning,
				TEXT("%s: no ARTACBoard placed in the level. Place one, and set it on the "
					 "ARTACCombatCamera instance -- Board is EditInstanceOnly and cannot be bound "
					 "on the class default."),
				CommandName);
			return false;
		}

		if (NumBoards > 1)
		{
			UE_LOG(LogRTAC, Warning,
				TEXT("%s: %d ARTACBoard actors in the level; probing '%s'. Every result below is "
					 "about that board only."),
				CommandName, NumBoards, *OutContext.Board->GetName());
		}

		if (!OutContext.Grid.Init(OutContext.Board->Rows, OutContext.Board->Columns))
		{
			// FRTACGrid::Init logs its own reason to LogRTAC on invalid dimensions.
			UE_LOG(LogRTAC, Warning, TEXT("%s: Grid.Init(%d, %d) failed."),
				CommandName, OutContext.Board->Rows, OutContext.Board->Columns);
			return false;
		}

		return true;
	}

	/**
	 * The bracketed suffix every probe line carries, so no logged result is ever ambiguous about
	 * which board, transform, tile size or grid dimensions produced it. Pixel coordinates are
	 * meaningless without the board pose and viewport they were measured against.
	 */
	FString RTACDescribeProbeContext(const FRTACProbeContext& Context)
	{
		const FTransform BoardTransform = Context.Board->GetActorTransform();
		const FVector BoardLocation = BoardTransform.GetLocation();
		const FRotator BoardRotation = BoardTransform.Rotator();

		return FString::Printf(
			TEXT("[board '%s' loc (%.1f,%.1f,%.1f) rot (P %.1f, Y %.1f, R %.1f), TileSize %.1f, grid %dx%d]"),
			*Context.Board->GetName(),
			BoardLocation.X, BoardLocation.Y, BoardLocation.Z,
			BoardRotation.Pitch, BoardRotation.Yaw, BoardRotation.Roll,
			Context.Board->TileSize,
			Context.Grid.GetRows(), Context.Grid.GetColumns());
	}

	/**
	 * A tile's center in world space, built from RTACGridToLocalOffset rather than from any
	 * arithmetic of this file's own. The half-tile shift is what turns that function's CORNER
	 * offset into a center, per RTACGridConversion.h's corner-vs-center note -- a tile's offset is
	 * its minimum corner, so the center is one half-tile further along both local axes.
	 */
	FVector RTACTileCenterWorld(const ARTACBoard& Board, const FRTACGridPosition& Position)
	{
		const float TileSize = Board.TileSize;
		const FVector CornerLocal = RTACGridToLocalOffset(Position, TileSize);
		const FVector CenterLocal = CornerLocal + FVector(TileSize * 0.5f, TileSize * 0.5f, 0.0f);

		return Board.GetActorTransform().TransformPosition(CenterLocal);
	}

	/**
	 * Highlights a resolved tile for a few seconds. This is the arithmetic-free half of the check:
	 * a human compares the highlight against the tile they aimed at on screen, which is evidence
	 * that shares no computation with the code under test.
	 *
	 * Extent is in world units and is NOT scaled by the board's transform, so a non-unit board
	 * scale would draw a marker of the wrong size. The conversion under test is unaffected --
	 * InverseTransformPosition handles scale correctly; only this marker would mislead.
	 */
	void RTACDrawTileMarker(UWorld* World, const ARTACBoard& Board, const FRTACGridPosition& Position)
	{
		const float TileSize = Board.TileSize;

		DrawDebugBox(
			World,
			RTACTileCenterWorld(Board, Position),
			FVector(TileSize * 0.5f, TileSize * 0.5f, 4.0f),
			Board.GetActorQuat(),
			FColor::Green,
			/*bPersistentLines=*/false,
			/*LifeTime=*/5.0f,
			/*DepthPriority=*/0,
			/*Thickness=*/3.0f);
	}

	/**
	 * RTAC.ScreenToGrid <ScreenX> <ScreenY> -- drive the shipped inverse conversion with one
	 * explicit viewport pixel.
	 *
	 * Explicit coordinates rather than the live mouse position, deliberately: a stated pixel makes
	 * a prediction reproducible and checkable after the fact, which is what makes a run evidence
	 * rather than a demonstration.
	 */
	void RTACScreenToGridCommand(const TArray<FString>& Args, UWorld* World)
	{
		static const TCHAR* const CommandName = TEXT("RTAC.ScreenToGrid");

		if (Args.Num() != 2 || !FCString::IsNumeric(*Args[0]) || !FCString::IsNumeric(*Args[1]))
		{
			UE_LOG(LogRTAC, Warning,
				TEXT("%s: usage is 'RTAC.ScreenToGrid <ScreenX> <ScreenY>', two numeric viewport "
					 "pixel coordinates."),
				CommandName);
			return;
		}

		FRTACProbeContext Context;
		if (!RTACResolveProbeContext(World, CommandName, Context))
		{
			return;
		}

		const FVector2D ScreenPosition(FCString::Atof(*Args[0]), FCString::Atof(*Args[1]));
		const FString ContextText = RTACDescribeProbeContext(Context);

		// Domains crossed, named per Rule 10: screen px -> world cm (deprojection, then ray/plane
		// intersection) -> board-local cm -> grid row/column. Every hop is inside this one call.
		FRTACGridPosition Resolved;
		const bool bHit = RTACScreenToGridPosition(
			Context.PlayerController,
			ScreenPosition,
			*Context.Board,
			Context.Grid,
			Context.Board->TileSize,
			Resolved);

		// A miss logs at Log, not Warning. Returning false for a pixel genuinely off the board is
		// this function's correct behaviour and half its contract -- flagging it as a warning would
		// make a successful negative-case probe read as a defect.
		if (!bHit)
		{
			UE_LOG(LogRTAC, Log, TEXT("%s: screen px (%.1f, %.1f) -> NO HIT (returned false) %s"),
				CommandName, ScreenPosition.X, ScreenPosition.Y, *ContextText);
			return;
		}

		UE_LOG(LogRTAC, Log, TEXT("%s: screen px (%.1f, %.1f) -> tile (Row %d, Column %d) %s"),
			CommandName, ScreenPosition.X, ScreenPosition.Y, Resolved.Row, Resolved.Column, *ContextText);

		RTACDrawTileMarker(World, *Context.Board, Resolved);
	}

	/**
	 * RTAC.ScreenToGridSweep -- round-trip every tile center through both conversion directions.
	 *
	 * WHAT THIS DOES AND DOES NOT PROVE, stated here so a green N/N is not over-read. The two
	 * engine calls are mutual inverses through the same view-projection matrix and the same
	 * constrained view rect (GameplayStatics.cpp:3319-3337 and :3235-3254), so an error inside
	 * them cancels: this cannot detect a mirrored or misoriented camera. What it does prove is
	 * that RTAC's own inverse path -- deprojection output shape, ray/plane intersection,
	 * board-local transform, floor, bounds check -- executes and composes correctly at runtime
	 * across the whole board rather than at one lucky pixel. Camera orientation is evidenced
	 * separately, by the forward-direction PIE measurement Decision #15's addendum records.
	 */
	void RTACScreenToGridSweepCommand(const TArray<FString>& /*Args*/, UWorld* World)
	{
		static const TCHAR* const CommandName = TEXT("RTAC.ScreenToGridSweep");

		FRTACProbeContext Context;
		if (!RTACResolveProbeContext(World, CommandName, Context))
		{
			return;
		}

		const FString ContextText = RTACDescribeProbeContext(Context);
		const float TileSize = Context.Board->TileSize;

		UE_LOG(LogRTAC, Log, TEXT("=== %s -- starting -- %s ==="), CommandName, *ContextText);

		// The WHOLE board, deliberately, not a sample. A single working pixel cannot rule out a
		// structurally-excluded failing axis: that is precisely how the 25/25 conversion test
		// stayed green while Decision #16's pairing error sat inside it (Failure Mode 5).
		int32 NumTiles = 0;
		int32 NumRoundTripped = 0;

		for (int32 Row = 0; Row < Context.Grid.GetRows(); ++Row)
		{
			for (int32 Column = 0; Column < Context.Grid.GetColumns(); ++Column)
			{
				++NumTiles;
				const FRTACGridPosition Tile(Row, Column);

				// Forward leg: grid -> board-local (the real named function) -> world -> screen.
				// bPlayerViewportRelative is left at its false default so this shares
				// DeprojectScreenPositionToWorld's own coordinate space exactly.
				FVector2D ScreenPosition;
				if (!Context.PlayerController->ProjectWorldLocationToScreen(
						RTACTileCenterWorld(*Context.Board, Tile), ScreenPosition))
				{
					UE_LOG(LogRTAC, Warning,
						TEXT("  [FAIL] tile (Row %d, Column %d): forward projection failed -- the "
							 "center is off screen or behind the camera, so the inverse leg was "
							 "never attempted. Frame the board first."),
						Row, Column);
					continue;
				}

				// Inverse leg: the function under test, on the pixel just measured.
				FRTACGridPosition Resolved;
				const bool bHit = RTACScreenToGridPosition(
					Context.PlayerController,
					ScreenPosition,
					*Context.Board,
					Context.Grid,
					TileSize,
					Resolved);

				if (bHit && Resolved == Tile)
				{
					++NumRoundTripped;
					UE_LOG(LogRTAC, Log,
						TEXT("  [PASS] tile (Row %d, Column %d) -> screen px (%.1f, %.1f) -> tile (Row %d, Column %d)"),
						Row, Column, ScreenPosition.X, ScreenPosition.Y, Resolved.Row, Resolved.Column);
				}
				else if (bHit)
				{
					UE_LOG(LogRTAC, Warning,
						TEXT("  [FAIL] tile (Row %d, Column %d) -> screen px (%.1f, %.1f) -> tile "
							 "(Row %d, Column %d) -- WRONG TILE"),
						Row, Column, ScreenPosition.X, ScreenPosition.Y, Resolved.Row, Resolved.Column);
				}
				else
				{
					UE_LOG(LogRTAC, Warning,
						TEXT("  [FAIL] tile (Row %d, Column %d) -> screen px (%.1f, %.1f) -> NO HIT "
							 "(returned false)"),
						Row, Column, ScreenPosition.X, ScreenPosition.Y);
				}
			}
		}

		// Same bookended shape as the automation tests' own blocks, on purpose: the identical
		// GetLogEntries(category="LogRTAC", pattern=".*") call reads both (CLAUDE.md -> MCP).
		UE_LOG(LogRTAC, Log, TEXT("%s complete: %d/%d tiles round-tripped"),
			CommandName, NumRoundTripped, NumTiles);
	}
} // namespace

// Two independent top-level registrations (Rule 8) -- neither command is reachable through the
// other's guard. ECVF_Cheat marks both as debug-only; see the header comment above for what that
// flag does and does not add on top of the !UE_BUILD_SHIPPING guard.
static FAutoConsoleCommandWithWorldAndArgs GRTACScreenToGridConsoleCommand(
	TEXT("RTAC.ScreenToGrid"),
	TEXT("Resolve a viewport pixel to a grid tile through RTACScreenToGridPosition, inside PIE. "
		 "Usage: RTAC.ScreenToGrid <ScreenX> <ScreenY>"),
	FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&RTACScreenToGridCommand),
	ECVF_Cheat);

static FAutoConsoleCommandWithWorldAndArgs GRTACScreenToGridSweepConsoleCommand(
	TEXT("RTAC.ScreenToGridSweep"),
	TEXT("Round-trip every tile center through RTACGridToLocalOffset and RTACScreenToGridPosition, "
		 "inside PIE, and report N/N. Takes no arguments."),
	FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&RTACScreenToGridSweepCommand),
	ECVF_Cheat);

#endif // !UE_BUILD_SHIPPING
