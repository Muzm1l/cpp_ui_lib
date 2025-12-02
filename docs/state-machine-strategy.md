# State Machine Strategy for WaterfallGraph Rendering

## Current State Analysis

### Current Flags
1. `m_needsFullRedraw` (bool) - Indicates full scene clear needed
2. `m_dataDirty` (bool) - **UNUSED** - Set but never checked, only cleared
3. `m_rangeDirty` (bool) - Indicates ranges need recalculation
4. `m_dirtySeries` (std::set<QString>) - Tracks which series need redrawing

### Current Issues
- `m_dataDirty` is redundant (set but never used)
- Multiple boolean flags create complex conditional logic
- No clear state hierarchy or transitions
- Logic scattered across multiple if-statements in `drawIncremental()`

## Proposed State Machine Design

### State Enum

```cpp
enum class RenderState {
    CLEAN,                    // No updates needed
    RANGE_UPDATE_ONLY,         // Only ranges need recalculation (no redraw)
    INCREMENTAL_UPDATE,        // Specific series need redrawing (ranges may need update)
    FULL_REDRAW               // Everything needs redrawing (scene clear + all series)
};
```

### State Transitions

```
CLEAN
  ├─> RANGE_UPDATE_ONLY        (when: range changes but no visual update needed)
  ├─> INCREMENTAL_UPDATE        (when: data points added to specific series)
  └─> FULL_REDRAW              (when: time range, grid, or data source changes)

RANGE_UPDATE_ONLY
  ├─> CLEAN                    (after: range update complete)
  ├─> INCREMENTAL_UPDATE       (when: series also become dirty)
  └─> FULL_REDRAW              (when: structural changes occur)

INCREMENTAL_UPDATE
  ├─> CLEAN                    (after: dirty series redrawn)
  ├─> RANGE_UPDATE_ONLY        (when: only ranges need update after series redraw)
  └─> FULL_REDRAW              (when: structural changes occur)

FULL_REDRAW
  └─> CLEAN                    (after: complete redraw finished)
```

### State Semantics

**CLEAN**
- No work needed
- All flags cleared
- Graphics items up to date

**RANGE_UPDATE_ONLY**
- Ranges need recalculation
- No series need redrawing
- No scene clearing needed
- Transition: Update ranges → CLEAN

**INCREMENTAL_UPDATE**
- One or more specific series need redrawing
- Ranges may or may not need updating (tracked separately)
- No scene clearing needed
- Transition: Update ranges (if needed) → Redraw dirty series → CLEAN

**FULL_REDRAW**
- Complete scene clear required
- All series need redrawing
- Grid and drawing area need updating
- Ranges need recalculation
- Transition: Clear scene → Update drawing area → Draw grid → Update ranges → Redraw all series → CLEAN

## Implementation Strategy

### 1. Replace Flags with State Enum

**File**: `waterfallgraph.h`

```cpp
enum class RenderState {
    CLEAN,
    RANGE_UPDATE_ONLY,
    INCREMENTAL_UPDATE,
    FULL_REDRAW
};

// Replace:
// bool m_needsFullRedraw;
// bool m_dataDirty;
// bool m_rangeDirty;
// std::set<QString> m_dirtySeries;

// With:
RenderState m_renderState;
std::set<QString> m_dirtySeries;  // Only used when state is INCREMENTAL_UPDATE
bool m_rangeUpdateNeeded;         // Separate flag for range updates (can be combined with state)
```

### 2. State Transition Methods

**File**: `waterfallgraph.h`

```cpp
private:
    void setRenderState(RenderState newState);
    void markSeriesDirty(const QString &seriesLabel);
    void markAllSeriesDirty();
    void markRangeUpdateNeeded();
    void transitionToAppropriateState();
```

### 3. State Transition Logic

**Rules**:
- `FULL_REDRAW` is the highest priority (supersedes all other states)
- `INCREMENTAL_UPDATE` requires at least one dirty series
- `RANGE_UPDATE_ONLY` when only ranges need update and no series are dirty
- `CLEAN` only after all work is complete

**Transition Helper**:
```cpp
void WaterfallGraph::transitionToAppropriateState()
{
    // FULL_REDRAW takes precedence
    if (m_renderState == RenderState::FULL_REDRAW)
        return;
    
    // If series are dirty, need incremental update
    if (!m_dirtySeries.empty())
    {
        m_renderState = RenderState::INCREMENTAL_UPDATE;
        return;
    }
    
    // If only ranges need update
    if (m_rangeUpdateNeeded || !dataRangesValid)
    {
        m_renderState = RenderState::RANGE_UPDATE_ONLY;
        return;
    }
    
    // Otherwise clean
    m_renderState = RenderState::CLEAN;
}
```

### 4. Simplified drawIncremental() Logic

**File**: `waterfallgraph.cpp`

```cpp
void WaterfallGraph::drawIncremental()
{
    if (!graphicsScene)
        return;

    switch (m_renderState)
    {
        case RenderState::CLEAN:
            return; // Nothing to do

        case RenderState::RANGE_UPDATE_ONLY:
            updateDataRanges();
            m_rangeUpdateNeeded = false;
            m_renderState = RenderState::CLEAN;
            break;

        case RenderState::INCREMENTAL_UPDATE:
            // Update ranges if needed
            if (m_rangeUpdateNeeded || !dataRangesValid)
            {
                updateDataRanges();
                m_rangeUpdateNeeded = false;
            }
            
            // Redraw only dirty series
            if (dataSource && !dataSource->isEmpty() && dataRangesValid)
            {
                for (const QString &seriesLabel : m_dirtySeries)
                {
                    if (isSeriesVisible(seriesLabel))
                    {
                        drawDataSeries(seriesLabel);
                    }
                }
            }
            
            m_dirtySeries.clear();
            m_renderState = RenderState::CLEAN;
            break;

        case RenderState::FULL_REDRAW:
            // Clear scene and graphics item maps
            graphicsScene->clear();
            m_seriesPathItems.clear();
            for (auto &pair : m_seriesPointItems)
            {
                pair.second.clear();
            }
            m_seriesPointItems.clear();

            // Update drawing area and grid
            setupDrawingArea();
            if (gridEnabled)
            {
                drawGrid();
            }

            // Update ranges
            if (dataSource && !dataSource->isEmpty())
            {
                updateDataRanges();
            }
            m_rangeUpdateNeeded = false;

            // Redraw all series
            if (dataSource && !dataSource->isEmpty() && dataRangesValid)
            {
                std::vector<QString> allSeries = dataSource->getDataSeriesLabels();
                for (const QString &seriesLabel : allSeries)
                {
                    if (isSeriesVisible(seriesLabel))
                    {
                        drawDataSeries(seriesLabel);
                    }
                }
            }
            
            m_dirtySeries.clear();
            m_renderState = RenderState::CLEAN;
            break;
    }
}
```

### 5. Helper Methods for State Management

**File**: `waterfallgraph.cpp`

```cpp
void WaterfallGraph::setRenderState(RenderState newState)
{
    // FULL_REDRAW can only be set explicitly, never downgraded
    if (m_renderState == RenderState::FULL_REDRAW && newState != RenderState::FULL_REDRAW)
    {
        return; // Don't downgrade from FULL_REDRAW
    }
    
    // FULL_REDRAW always supersedes
    if (newState == RenderState::FULL_REDRAW)
    {
        m_renderState = RenderState::FULL_REDRAW;
        markAllSeriesDirty();
        return;
    }
    
    m_renderState = newState;
}

void WaterfallGraph::markSeriesDirty(const QString &seriesLabel)
{
    m_dirtySeries.insert(seriesLabel);
    transitionToAppropriateState();
}

void WaterfallGraph::markAllSeriesDirty()
{
    if (dataSource && !dataSource->isEmpty())
    {
        std::vector<QString> allSeries = dataSource->getDataSeriesLabels();
        for (const QString &seriesLabel : allSeries)
        {
            m_dirtySeries.insert(seriesLabel);
        }
    }
    setRenderState(RenderState::FULL_REDRAW);
}

void WaterfallGraph::markRangeUpdateNeeded()
{
    m_rangeUpdateNeeded = true;
    transitionToAppropriateState();
}
```

### 6. Update All State Mutation Points

Replace all flag assignments with state transitions:

**Before**:
```cpp
m_needsFullRedraw = true;
m_dirtySeries.insert(seriesLabel);
m_rangeDirty = true;
```

**After**:
```cpp
markSeriesDirty(seriesLabel);
markRangeUpdateNeeded();
// Or for full redraw:
setRenderState(RenderState::FULL_REDRAW);
```

## Benefits

1. **Clear State Hierarchy**: Explicit states with well-defined meanings
2. **Simplified Logic**: Single switch statement instead of nested if-statements
3. **Easier Debugging**: State can be logged/inspected at any point
4. **Type Safety**: Enum prevents invalid state combinations
5. **Maintainability**: State transitions are explicit and traceable
6. **Removes Redundancy**: Eliminates unused `m_dataDirty` flag
7. **Better Performance**: State machine can short-circuit early (CLEAN state)

## Migration Path

1. Add enum and new member variables alongside existing flags
2. Implement state transition methods
3. Update `drawIncremental()` to use switch statement
4. Replace all flag assignments with state transition calls
5. Remove old boolean flags
6. Test thoroughly

## Edge Cases Handled

- **State Priority**: FULL_REDRAW always takes precedence
- **Empty Dirty Series**: Automatically transitions to RANGE_UPDATE_ONLY or CLEAN
- **Range Update Without Series**: Handled by RANGE_UPDATE_ONLY state
- **Multiple Rapid Updates**: State machine consolidates them appropriately

