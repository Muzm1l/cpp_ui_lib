# System Memory and Architecture Review Report

**Project:** `cpp_ui_lib`  
**Date:** 2026-03-26  
**Scope:** Architecture and memory consumption review for the real-time Qt/C++ visualization pipeline.

---

## Executive Summary

The system architecture is generally sound at the data model layer (shared `GraphEngine`/`WaterfallData` per `GraphType`), but memory pressure is dominated by view-layer multiplicative state:

- Multiple `GraphContainer` instances each pre-create multiple `WaterfallGraph` widgets.
- Each `WaterfallGraph` maintains its own caches, scene items, and pixmap buffers.
- `WaterfallData` can retain large histories, and currently stores parallel series structures.

Primary optimization direction:

1. Reduce the number of live heavyweight graph widgets and caches.
2. Enforce explicit retention/capacity policy for series history.
3. Tighten cache lifecycle and object reuse in rendering/timeline paths.

This is mostly a retained-memory architecture issue, not a single leak signature.

---

## Current Architecture State

### High-Level Module Map

- `main.cpp`, `mainwindow.cpp`: app shell, timer orchestration, top-level wiring.
- `graphlayout.cpp`: layout orchestrator, owns engines and container fleet, sync propagation.
- `graphengine.cpp`: per-graph-type state and signal source for data append/range changes.
- `waterfalldata.cpp`: core data model with circular-buffer-backed series and metadata.
- `graphcontainer.cpp`: UI container, timeline/zoom coupling, graph selection.
- `waterfallgraph.cpp`: render implementation, cache management, draw/incremental paths.
- `timelineview.cpp`: timeline visuals, segment/label logic, interaction and slider behavior.

### Data/Update Flow (Simplified)

1. Data source (simulator or external) pushes points into `GraphLayout`/`GraphEngine`.
2. `GraphEngine` updates `WaterfallData` and emits change signals.
3. `GraphContainer` and/or attached `WaterfallGraph` react and trigger redraw/incremental updates.
4. `WaterfallGraph` rebuilds/uses visible caches and paints buffered content.
5. Timeline and sync state trigger additional scope/range updates.

### Architectural Strengths

- Separation of concerns between app shell, orchestration, data model, and views.
- Centralized data ownership by graph type prevents duplication at model level.
- Existing incremental and cache-related logic already present in renderer.

### Architectural Risks

- Redundant update fan-out paths can increase redraw frequency and temporary allocations.
- Multiplicative view instantiation amplifies retained memory.
- Coexistence of newer engine-driven and legacy container pathways increases complexity.

---

## Memory Consumption Findings

### 1) Highest Impact: View Multiplication

`GraphContainer` creates graph widgets for all graph types, and layout creates multiple containers. This can keep many heavyweight `WaterfallGraph` instances resident.

Likely memory contributors per graph instance:

- Pixel buffers (waterfall/backing pixmaps)
- Per-series cache maps (`visible`, paths, scatter items)
- Scene graph and graphics items

**Impact:** High baseline RSS and retained memory even when many graphs are hidden.

### 2) High Impact: Data Retention and Parallel Series Storage

`WaterfallData` keeps parallel series collections (Y values, timestamp forms, symbols/markers). If capacity is not constrained, long sessions accumulate memory.

**Impact:** Memory growth over runtime and larger working set for cache operations.

### 3) High/Medium Impact: Duplicate Visible/Render Caches

Renderer maintains multiple representations of similar visible content for draw speed. This is useful for performance, but can over-retain memory if not bounded and pruned per series.

### 4) Medium Impact: Pixmap and Scene Item Lifecycles

Large pixmaps and graphics items can hold significant memory. If caches are not capped and objects are frequently recreated rather than reused, allocator pressure rises.

### 5) Medium/Lower Impact: Per-Frame Allocation Churn

Temporary vectors/containers in paint/timeline paths add churn; while not always leak-like, they raise peak usage and fragmentation.

---

## Prioritized Optimization Plan (Memory-First)

### Priority 1: Reduce live graph object count

- Move from eager creation of all graph types to lazy create-on-select.
- Optionally keep a small LRU cache of recently used graph widgets.
- Consider one graph widget per container with datasource/type switching.

**Expected effect:** Largest immediate RSS reduction.

### Priority 2: Enforce strict data retention policy

- Set explicit capacities for all series/circular buffers.
- Define retention by operational need (for example: N points or M hours).
- Bound symbol/marker histories similarly.

**Expected effect:** Prevent unbounded growth; predictable memory ceiling.

### Priority 3: Bound and prune renderer caches

- Cap per-series visible cache sizes to on-screen or near-screen requirements.
- Erase cache entries for inactive/removed series.
- Add controlled shrink behavior after large clears or track changes.

**Expected effect:** Lower retained memory after mode/track churn.

### Priority 4: Optimize pixmap and graphics object reuse

- Reuse scene objects where possible instead of delete/recreate loops.
- Add bounded policy (or LRU) for pixmap caches keyed by style/size.
- Defer buffer allocation until widget is visible and has stable geometry.

**Expected effect:** Reduced peak/retained graphics memory.

### Priority 5: Reduce allocation churn in hot paint/timeline paths

- Reuse temporary point/label containers.
- Reserve/resize predictable vectors instead of repeated growth.
- Pool timeline segment objects.

**Expected effect:** Lower allocator churn and fragmentation.

---

## System State Summary

- Model layer is relatively efficient (shared data ownership by graph type).
- Main memory risk is architectural fan-out in the view/render layer.
- Existing code already contains optimization hooks (incremental paths, cache invalidation helpers), but memory policy is not yet tightly bounded.
- Current direction should focus on **instance count + retention policy + cache lifecycle** before micro-optimizing small allocations.

---

## Recommended Measurement Plan

For each optimization step, capture before/after with the same scenario:

1. Process RSS over fixed runtime windows (for example: 5/15/30/60 min).
2. Peak heap and allocation hotspots (Massif/Heaptrack).
3. Count of live `WaterfallGraph` and graphics items per container.
4. Data series sizes and cache map cardinality.
5. Frame/update latency to ensure memory gains do not regress UX.

---

## Suggested Implementation Sequence

1. Lazy graph creation and/or single active graph instance per container.
2. Explicit circular-buffer capacities and retention limits.
3. Cache bounding and per-series pruning.
4. Pixmap/object reuse and capped style caches.
5. Hot-path temporary allocation cleanup.

This sequence minimizes risk and delivers visible memory wins early.

