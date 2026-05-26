# Performance Architecture Decisions and Methodology

**Purpose:** Convert the architecture guidance into a concrete, prioritized execution plan for reducing jank, CPU cost, and memory pressure in `cpp_ui_lib`.

**Scope:** `GraphLayout`, `GraphContainer`, `WaterfallGraph`, `TimelineView`, `GraphEngine`, `WaterfallData`, and `CircularBuffer`.

---

## 1) Reduce Full Redraws (Biggest Win)

The system is designed to be incremental-first, so unnecessary full redraws are the highest-value target.

### What to audit

- Full redraw transitions triggered by:
  - Scope/time-window changes
  - Follow/frozen mode transitions
  - Graph type switches
  - Layout mode changes (panel count/geometry)
- Any path that promotes state to full redraw "just to be safe" without proving correctness risk.

### Decision rules

- Treat full redraw as an exception path, not a default.
- Do not promote incremental updates to full redraw unless one of these is true:
  - Scene structure changed (item type/count topology changed)
  - Axis transform changed in a way incremental repositioning cannot represent
  - Cache coherence cannot be proven with current metadata
- Every full redraw trigger must be explicitly documented with:
  - Trigger reason
  - Correctness rationale
  - Profiling evidence

### Implementation approach

- Add per-trigger counters and timing:
  - `fullRedraw.count.byReason`
  - `incrementalUpdate.count`
  - `fullRedraw.ms.total`
- Log render-state transitions in debug/perf mode only.
- Refactor invalidation API to target smaller regions:
  - Per-series dirty
  - Per-overlay dirty (symbols/markers/shading)
  - Per-axis/range dirty

### Success criteria

- At steady streaming load, incremental updates dominate (`>90%` update cycles).
- Full redraw count per minute drops materially in follow/scope interaction scenarios.
- Frame-time spikes at interaction boundaries are reduced.

---

## 2) Cache Validation Frequency

Repeated cache validation on every paint or signal creates avoidable CPU overhead.

### Problem pattern

- `ensure*Valid()` style checks called at high frequency from multiple call sites.
- Validation repeats within the same frame/update cycle with unchanged inputs.

### Decision rules

- Validate once per frame/update tick, not once per consumer call.
- Use dirty flags keyed by change domain (data, scope, geometry, style).
- Coalesce multiple invalidation events before recomputing.

### Implementation approach

- Introduce frame-scoped validation gate:
  - `cacheValidationEpoch`
  - `lastValidatedEpoch`
- Convert eager revalidation to deferred revalidation:
  - Set dirty flags on events
  - Rebuild cache at draw boundary only
- For rapid event bursts, collapse to one revalidation + one repaint.

### Success criteria

- Validation calls per second decrease significantly under same workload.
- CPU samples in cache-validation functions drop in callgrind/perf reports.

---

## 3) Signal/Slot Overhead in Hot Paths

Signals are correct for architecture boundaries, but expensive in inner loops.

### Where signals remain preferred

- Cross-layer fan-out notifications (engine -> multiple listeners)
- Module boundaries where loose coupling is intentional

### Where direct calls are preferred

- Tight append loops
- Per-point/per-series paint loop hooks
- High-frequency, single-consumer paths

### Decision rules

- Keep signals at boundaries; use direct calls inside hot loops.
- If replacing signal with direct call:
  - Document the coupling and reason
  - Keep the API narrow
  - Preserve testability (mockable seam or adapter)

### Implementation approach

- Identify top emitters in profiler traces.
- Replace high-frequency inner-loop emits with:
  - Direct function calls, or
  - Batched "update ready" signal once per cycle.

### Success criteria

- Signal dispatch cost drops from hottest stacks.
- No regression in synchronization behavior across panels.

---

## 4) Circular Buffer and Copy-Heavy Paths

Unnecessary copying increases CPU, memory bandwidth, and latency.

### What to check

- Range-query APIs returning owning containers when a non-owning view is sufficient.
- Append paths that copy data across temporary vectors before final storage.
- Repeated full-buffer materialization for partial visible-range operations.

### Decision rules

- Prefer zero-copy reads for render/query paths:
  - iterators
  - `const` references
  - spans/views where safe
- Copy only at ownership boundaries.
- Preserve data-lifetime safety explicitly in API contracts.

### Implementation approach

- Add view-based APIs in data layer for visible-range iteration.
- Keep copy-based APIs only for callers that need ownership.
- Remove redundant transform stages in append/query pipeline.

### Success criteria

- Reduced allocations and memcpy-heavy samples.
- Reduced instruction count in data extraction helpers.

---

## 5) N x Panels x Graph Types Memory Pressure

Duplicated per-view cache state scales poorly and amplifies invalidation work.

### Problem pattern

- Each panel holds large caches per graph type.
- Sync events trigger N-way invalidation and rebuild.

### Decision rules

- Separate shared immutable projections from panel-local mutable presentation.
- Lazy-initialize expensive per-panel caches.
- Evict or compact inactive graph-type caches.

### Implementation approach

- Introduce shared read-only data projection keyed by:
  - graph type
  - scope range
  - version/epoch
- Keep panel-local cache for geometry/style deltas only.
- Apply capacity limits and LRU policy for inactive panel/type combinations.

### Success criteria

- Lower resident memory for multi-panel layouts.
- Reduced sync-event invalidation work across panels.

---

## 6) Timeline/Scope Change Propagation

Scope updates are user-visible and jank-sensitive, especially in drag/follow transitions.

### Problem pattern

- Every timeline delta synchronously broadcasts to all panels.
- Rapid user interaction causes repeated immediate redraws.

### Decision rules

- Coalesce scope updates into repaint cadence.
- Prefer asynchronous propagation for bursty interactions.
- Maintain immediate response for end-of-gesture commit points.

### Implementation approach

- Add scope-update coalescer:
  - Store latest pending scope
  - Schedule one update on next event-loop/frame boundary
- During active drag:
  - Collapse intermediate updates
  - Render latest scope only
- On drag release:
  - Force commit update to guarantee final accuracy

### Success criteria

- Reduced input-to-frame latency variance during timeline drag.
- Fewer redundant repaints and less navigation jank.

---

## Prioritized Execution Order

1. **Full redraw trigger audit + instrumentation**
2. **Cache validation deferral/coalescing**
3. **Scope propagation coalescing**
4. **Copy-heavy path elimination in data layer**
5. **Selective signal-to-direct-call changes in hot loops**
6. **Shared/lazy cache architecture for memory scaling**

This order gives fast wins first (CPU + jank), then deeper structural improvements.

---

## Measurement Plan (Before/After Required)

- **CPU:** callgrind/perf hotspots for draw path, cache validation, signal dispatch
- **Memory:** heaptrack/valgrind for retained cache growth across multi-panel scenarios
- **Interaction:** frame-time and hitch counts during:
  - Follow <-> frozen toggles
  - Timeline drag and release
  - Graph type switching
- **Correctness:** cross-panel sync parity checks (scope, cursor, markers/symbols)

No optimization should be accepted without before/after evidence for both performance and behavioral correctness.

---

## 7) WaterfallGraph Render/Update Code Reference

Below are the two most important code paths to inspect first when implementing the decisions above.

### A) Core render path (`draw` + `drawIncremental`)

```cpp
void WaterfallGraph::draw()
{
    if (!graphicsScene)
        return;

    // Only set FULL_REDRAW if we're in CLEAN state
    if (m_renderState == RenderState::CLEAN)
    {
        setRenderState(RenderState::FULL_REDRAW);
    }

    drawIncremental();
}

void WaterfallGraph::drawIncremental()
{
    if (!isVisible() || !graphicsScene)
        return;

    switch (m_renderState)
    {
        case RenderState::CLEAN:
            return;

        case RenderState::RANGE_UPDATE_ONLY:
            if (dataSource && !dataSource->isEmpty()) {
                updateDataRanges();
            }
            m_rangeUpdateNeeded = false;
            m_renderState = RenderState::CLEAN;
            break;

        case RenderState::INCREMENTAL_UPDATE:
            if (m_rangeUpdateNeeded || !dataRangesValid) {
                if (dataSource && !dataSource->isEmpty()) {
                    updateDataRanges();
                }
                m_rangeUpdateNeeded = false;
            }

            if (dataSource && !dataSource->isEmpty() && dataRangesValid) {
                for (const QString &seriesLabel : m_dirtySeries) {
                    if (isSeriesVisible(seriesLabel)) {
                        if (m_useLineDrawing) drawDataLine(seriesLabel, false);
                        else drawDataSeries(seriesLabel);
                    }
                }
            }

            drawBTWSymbols();
            m_dirtySeries.clear();
            m_renderState = RenderState::CLEAN;
            break;

        case RenderState::FULL_REDRAW:
            invalidateAllVisibleDataCache();
            cleanupAllScatterplotItems();
            // ... clear path/point/render caches and rebuild all series ...
            drawBTWSymbols();
            m_fastTrackSwitchMode = false;
            m_dirtySeries.clear();
            m_renderState = RenderState::CLEAN;
            break;
    }
}
```

Why this matters:

- This is where incremental work can accidentally become full redraw work.
- Section 1 and Section 2 changes should be validated against this exact switch logic first.

### B) Update triggers and state transitions (`setRenderState`, time/data updates)

```cpp
void WaterfallGraph::setRenderState(RenderState newState)
{
    if (newState == RenderState::CLEAN) {
        m_renderState = RenderState::CLEAN;
        return;
    }

    if (newState == RenderState::FULL_REDRAW) {
        m_renderState = RenderState::FULL_REDRAW;
        markAllSeriesDirty();
        return;
    }

    // Prevent downgrade from FULL_REDRAW
    if (m_renderState == RenderState::FULL_REDRAW) {
        return;
    }

    m_renderState = newState;
}

void WaterfallGraph::triggerIncrementalRedraw(const QString &seriesLabel)
{
    markSeriesDirty(seriesLabel);
    m_rangeUpdateNeeded = false;
    setRenderState(RenderState::INCREMENTAL_UPDATE);
    draw();
}

void WaterfallGraph::setTimeRange(const QDateTime &timeMin, const QDateTime &timeMax)
{
    if (timeMin >= timeMax) return;
    bool rangeChanged = (this->timeMin != timeMin || this->timeMax != timeMax);
    if (!rangeChanged) return;

    invalidateAllVisibleDataCache();
    m_mapScreenToTimeCacheValid = false;
    this->timeMin = timeMin;
    this->timeMax = timeMax;

    // Time-range change currently forces full redraw
    setRenderState(RenderState::FULL_REDRAW);
}

void WaterfallGraph::setTimeInterval(TimeInterval interval)
{
    timeInterval = interval;
    m_cachesValid = false;
    m_mapDataToScreenCache.clear();
    invalidateAllVisibleDataCache();
    setRenderState(RenderState::FULL_REDRAW);
    draw();
}
```

Why this matters:

- This is the primary trigger surface for Sections 1, 2, and 6.
- Any optimization should begin by reducing unnecessary calls that end in `FULL_REDRAW` from these update methods.

### Quick trigger map (current behavior)

- `triggerIncrementalRedraw()` -> `INCREMENTAL_UPDATE` -> `draw()`
- `setTimeRange()` -> `FULL_REDRAW` (no immediate `draw()` in this method)
- `setTimeInterval()` -> `FULL_REDRAW` + immediate `draw()`
- `setDataSource()` / `attachEngine()` -> `FULL_REDRAW` + immediate `draw()`
- Engine `dataAppended` signal path -> `markSeriesDirty` + `markRangeUpdateNeeded` + `drawIncremental()`
