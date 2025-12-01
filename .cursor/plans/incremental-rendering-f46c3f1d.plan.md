<!-- f46c3f1d-2df0-4eed-a915-cbf93088cc40 ff20638e-f23f-42b0-8d30-22b53649698a -->
# Plan: State Machine Refactoring for WaterfallGraph Rendering

## Problem

Current implementation uses multiple boolean flags (`m_needsFullRedraw`, `m_dataDirty`, `m_rangeDirty`) and a set (`m_dirtySeries`) to track rendering state. This creates complex conditional logic, includes unused flags (`m_dataDirty`), and makes the rendering flow difficult to understand and maintain.

## Solution

Replace the flag-based system with a single `RenderState` enum that clearly defines the rendering state and simplifies the drawing logic through a state machine pattern.

## Implementation Steps

### 1. Add RenderState Enum and Update Header

**File**: `waterfallgraph.h`

- Add `enum class RenderState` with values: `CLEAN`, `RANGE_UPDATE_ONLY`, `INCREMENTAL_UPDATE`, `FULL_REDRAW`
- Replace existing flags:
- Remove: `bool m_needsFullRedraw`
- Remove: `bool m_dataDirty` (unused)
- Remove: `bool m_rangeDirty`
- Keep: `std::set<QString> m_dirtySeries` (still needed for INCREMENTAL_UPDATE state)
- Add: `RenderState m_renderState`
- Add: `bool m_rangeUpdateNeeded` (separate flag for range updates)

### 2. Add State Management Method Declarations

**File**: `waterfallgraph.h`

- Add private methods:
- `void setRenderState(RenderState newState)`
- `void markSeriesDirty(const QString &seriesLabel)`
- `void markAllSeriesDirty()`
- `void markRangeUpdateNeeded()`
- `void transitionToAppropriateState()`

### 3. Initialize State in Constructor

**File**: `waterfallgraph.cpp` (constructor)

- Replace flag initializations:
- Change `m_needsFullRedraw(true)` to `m_renderState(RenderState::FULL_REDRAW)`
- Remove `m_dataDirty(false)` and `m_rangeDirty(false)`
- Add `m_rangeUpdateNeeded(false)`

### 4. Implement State Transition Helper Methods

**File**: `waterfallgraph.cpp`

- Implement `transitionToAppropriateState()`:
- Check if state is FULL_REDRAW (don't downgrade)
- If series are dirty → INCREMENTAL_UPDATE
- If only ranges need update → RANGE_UPDATE_ONLY
- Otherwise → CLEAN

- Implement `setRenderState()`:
- Prevent downgrading from FULL_REDRAW
- FULL_REDRAW always supersedes other states
- Automatically mark all series dirty when setting FULL_REDRAW

- Implement `markSeriesDirty()`:
- Add series to dirty set
- Call `transitionToAppropriateState()`

- Implement `markAllSeriesDirty()`:
- Populate dirty set with all series
- Set state to FULL_REDRAW

- Implement `markRangeUpdateNeeded()`:
- Set `m_rangeUpdateNeeded = true`
- Call `transitionToAppropriateState()`

### 5. Refactor drawIncremental() to Use State Machine

**File**: `waterfallgraph.cpp` (lines 582-653)

- Replace all conditional logic with a single `switch (m_renderState)` statement
- **CLEAN case**: Early return (nothing to do)
- **RANGE_UPDATE_ONLY case**: 
- Call `updateDataRanges()`
- Clear `m_rangeUpdateNeeded`
- Set state to CLEAN
- **INCREMENTAL_UPDATE case**:
- Update ranges if `m_rangeUpdateNeeded` or `!dataRangesValid`
- Redraw only dirty series
- Clear dirty series set
- Set state to CLEAN
- **FULL_REDRAW case**:
- Clear scene and graphics item maps
- Update drawing area and draw grid
- Update ranges
- Redraw all visible series
- Clear dirty series set
- Set state to CLEAN

### 6. Update addDataPoint() Method

**File**: `waterfallgraph.cpp` (lines 281-298)

- Replace flag assignments with:
- `markSeriesDirty(seriesLabel)`
- `markRangeUpdateNeeded()`
- Keep call to `drawIncremental()`

### 7. Update addDataPoints() Method

**File**: `waterfallgraph.cpp` (lines 306-323)

- Replace flag assignments with:
- `markSeriesDirty(seriesLabel)`
- `markRangeUpdateNeeded()`
- Keep call to `drawIncremental()`

### 8. Update draw() Method

**File**: `waterfallgraph.cpp` (lines 556-576)

- Replace flag assignments with:
- `setRenderState(RenderState::FULL_REDRAW)` (this automatically marks all series dirty)
- Remove manual series marking (handled by `setRenderState()`)
- Keep call to `drawIncremental()`

### 9. Update setDataSource() Method

**File**: `waterfallgraph.cpp` (lines 188-202)

- Replace flag assignments with:
- `setRenderState(RenderState::FULL_REDRAW)`
- Remove manual series marking

### 10. Update Grid-Related Methods

**File**: `waterfallgraph.cpp`

- `setGridEnabled()` (line ~490): Replace `m_needsFullRedraw = true` with `setRenderState(RenderState::FULL_REDRAW)`
- `setGridDivisions()` (line ~518): Replace `m_needsFullRedraw = true` with `setRenderState(RenderState::FULL_REDRAW)`

### 11. Update Time Range Methods

**File**: `waterfallgraph.cpp`

- `setTimeRange()` (line ~2009): Replace flag assignments with `setRenderState(RenderState::FULL_REDRAW)`
- `setTimeMax()` (line ~2037): Replace flag assignments with `setRenderState(RenderState::FULL_REDRAW)`
- `setTimeMin()` (line ~2085): Replace flag assignments with `setRenderState(RenderState::FULL_REDRAW)`

### 12. Update setCustomYRange() Method

**File**: `waterfallgraph.cpp` (lines 1367-1430)

- Replace significant change detection logic:
- If significant change: `setRenderState(RenderState::FULL_REDRAW)`
- Otherwise: `markRangeUpdateNeeded()` (if ranges changed but not significantly)

### 13. Remove All References to Old Flags

**File**: `waterfallgraph.cpp`

- Search for any remaining references to:
- `m_needsFullRedraw`
- `m_dataDirty`
- `m_rangeDirty`
- Replace with appropriate state transition calls

### 14. Update setData() Methods (Optional Enhancement)

**File**: `waterfallgraph.cpp`

- `setData(seriesLabel, ...)` (line ~221): Consider using `setRenderState(RenderState::FULL_REDRAW)` since setting all data typically requires full redraw
- `setData(WaterfallData)` (line ~246): Same consideration

### 15. Testing and Verification

- Verify CLEAN state works (no unnecessary redraws)
- Verify RANGE_UPDATE_ONLY state (ranges update without redraw)
- Verify INCREMENTAL_UPDATE state (only dirty series redraw)
- Verify FULL_REDRAW state (complete scene redraw)
- Test state transitions (especially FULL_REDRAW priority)
- Performance test: ensure no regression in rendering performance
- Visual test: verify all rendering scenarios still work correctly

## Key Implementation Details

### State Priority Rules

1. FULL_REDRAW always takes precedence (cannot be downgraded)
2. INCREMENTAL_UPDATE requires at least one dirty series
3. RANGE_UPDATE_ONLY when only ranges need update
4. CLEAN only after all work is complete

### State Transitions

- All state mutations go through helper methods
- `transitionToAppropriateState()` automatically determines correct state
- State is always set to CLEAN after `drawIncremental()` completes

### Backward Compatibility

- Public API remains unchanged
- Behavior should be identical to current implementation
- Only internal implementation changes