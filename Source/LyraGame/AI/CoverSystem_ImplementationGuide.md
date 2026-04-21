# Cover System Implementation Guide

This guide outlines the basic steps to utilize and implement the custom Cover System within this Unreal Engine 5.7 (Lyra-based) project.

## Overview
The Cover System is built to work closely with Unreal's **Environment Query System (EQS)** and **Behavior Trees (BT)**. It relies on custom EQS Tests, a Cover Subsystem, and specific trace channels to evaluate safe, tactical positions for AI agents.

### Key C++ Components (located in `Source/LyraGame/AI/`)
*   **`ULyraCoverSubsystem`**: The central system managing and keeping track of cover nodes/data globally.
*   **`UEnvQueryTest_CoverVisibility`**: An EQS test that checks if a point blocks Line of Sight to a target. It leverages a specific collision trace channel (defaulted to `Lyra_ObjectChannel_Geometry` / TraceChannel 6 or 7).
*   **`UEnvQueryTest_Peekability`**: Validates whether an AI can shoot from the cover point (e.g., peeking to the left, right, or over).
*   **`UEnvQueryTest_FlankTarget`**: Evaluates whether a cover position allows the AI to flank its target.

---

## Step 1: Collision & Trace Channel Setup

The cover system relies on correct physics trace channels to establish what is 'cover' and what is not.
1. Open **Project Settings** -> **Engine - Collision**.
2. Verify or create the Trace Channels used by the system:
   *   `Lyra_ObjectChannel_Geometry` (Often mapped to `ECC_GameTraceChannel6`)
   *   `Lyra_TraceChannel_Cover` (Often mapped to `ECC_GameTraceChannel7`)
3. Ensure that your level's static meshes (e.g., walls, crates) are set to **Block** these trace channels.

## Step 2: Creating an EQS Query for Cover

To make AI find cover, you must build an EQS asset (`EQS_FindCover`).

1. **Create an EQS Asset:** Right-click in Content Browser -> Artificial Intelligence -> Environment Query.
2. **Generate Points:** Run a generator (like `Points: Cone` or `Points: Grid`) around the AI (`Querier`) or the Target.
3. **Add Cover Visibility Test:**
   *   Add the **Cover Visibility Test (Blocks LOS)** onto your generated points.
   *   Set the **Context** to your target (e.g., the Player).
   *   *Note:* The test is designed to ensure a **Hit** occurs on the cover trace channel, meaning the line of sight is blocked.
4. **Add Peekability Test (Optional):**
   *   Add the **Peekability** test to ensure the AI isn't completely walled in and can step out to shoot.

## Step 3: Behavior Tree Integration

Once your EQS asset is ready, call it inside your AI's Behavior Tree.

1. Open your AI's Behavior Tree (e.g., `BT_ShooterAI`).
2. Add a **Run EQS Query** node and select your `EQS_FindCover` asset.
3. Bind the result to a Blackboard Key (e.g., `CoverLocation` as a Vector or Object dependent on your points generation).
4. Follow up the EQS task with a **Move To** task pointing to the `CoverLocation` Blackboard key.

## Step 4: Placing Cover in the Map

While dynamic cover works via geometry traces, robust subsystems usually benefit from pre-placed cover markers or generated navmesh bounds. Ensure your levels have standard blocking volumes or tagged meshes that align with the `Lyra_TraceChannel_Cover` settings so the queries properly categorize those areas.

## Best Practices & Performance
*   **Don't over-generate:** EQS traces can be expensive. Throttle your Generator's point density.
*   **Async execution:** The custom trace-based EQS tests are designed to run asynchronously. Monitor performance in the visualizer.
*   **Use the Subsystem:** For complex levels, rely on the `ULyraCoverSubsystem` to cache cover locations rather than strictly raw-tracing the environment every tick.

