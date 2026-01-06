# System Architecture Documentation (Syed)

## 1. System Architecture Overview

### 1.1 High-Level Architecture

The system is a Qt-based C++ application for real-time visualization of multiple waterfall graph types with synchronized navigation and interactive features. The architecture follows a layered design pattern with clear separation of concerns.

**Architecture Layers:**

1. **Application Layer** (`MainWindow`)
   - Top-level window management
   - Coordinates multiple `GraphLayout` instances
   - Manages timers for time updates (1-second intervals)
   - Optional simulation data generation via `Simulator`

2. **Layout Management Layer** (`GraphLayout`)
   - Central orchestration hub for multiple graph containers
   - Manages layouts: 1x1, 2x2, 2x1 (vertical/horizontal), 4x1
   - Centralized data source management (one `WaterfallData` per graph type)
   - Synchronization hub using hub-and-spoke pattern
   - Propagates changes across all containers via Qt signals/slots

3. **Container Layer** (`GraphContainer`)
   - Individual graph container widget
   - Wraps a `WaterfallGraph` with UI controls
   - Manages `TimelineView` for time-based navigation
   - Handles `ZoomPanel` for Y-axis range customization
   - Supports multiple data options (graph types) via combo box
   - Shares `GraphContainerSyncState` for cross-container synchronization

4. **Visualization Layer** (`WaterfallGraph` and specialized graphs)
   - Base `WaterfallGraph` class provides common functionality
   - Specialized graph types: `BDWGraph`, `BRWGraph`, `BTWGraph`, `FDWGraph`, `FTWGraph`, `LTWGraph`, `RTWGraph`
   - Each graph type extends base with type-specific features
   - Uses Qt's `QGraphicsScene`/`QGraphicsView` for 2D rendering

5. **Data Layer** (`WaterfallData`)
   - Stores time-series data as vectors of (value, timestamp) pairs
   - Supports multiple series per data source
   - Provides efficient query methods (binary search for time filtering)
   - Handles data point addition and bulk updates
   - Stores symbols (RTW symbols, BTW symbols/magenta circles) and markers

### 1.2 Major Features

**Multi-Graph Visualization:**
- Seven graph types: BDW, BRW, BTW, FDW, FTW, LTW, RTW
- Each graph can display multiple data series with different colors
- Real-time data updates (typically 1-second intervals)

**Synchronization:**
- Time interval synchronization across all containers
- Time scope (visible window) synchronization
- Cursor time synchronization
- BTW marker synchronization (interactive markers)
- Shaded region synchronization (BTW graphs)
- Follow mode synchronization (auto-scroll vs frozen)

**Interactive Features:**
- BTW graphs: Interactive overlay with draggable/rotatable markers
- RTW graphs: Clickable R markers and symbols
- Time selection spans for marking time ranges
- Zoom panel for Y-axis range customization
- Timeline view for time navigation and animation

**Symbol System:**
- RTW symbols: Custom symbols (TM, DP, LY, CircleI, Triangle, etc.)
- BTW symbols: Magenta circles synchronized across all graphs
- Symbols stored in `WaterfallData` and persist with data

**Performance Optimizations:**
- Incremental rendering system (state machine-based)
- Visible data caching to avoid full data scans
- Binary search for time range filtering
- Graphics item reuse and position updates instead of recreation

---

## 2. Drawing Architecture

### 2.1 Incremental Rendering System

The system has transitioned from full redraws to an **incremental rendering architecture** based on a state machine. This dramatically improves performance, especially during real-time updates.

**Render State Machine:**

The rendering system uses four states managed by `WaterfallGraph::RenderState`:

1. **`CLEAN`**: No pending updates, graph is up-to-date
2. **`RANGE_UPDATE_ONLY`**: Only data ranges (Y min/max) need updating, no redraw needed
3. **`INCREMENTAL_UPDATE`**: Only dirty series need redrawing, existing items can be repositioned
4. **`FULL_REDRAW`**: Complete clear and rebuild of all graphics items

**State Transitions:**

```
CLEAN → (data added) → INCREMENTAL_UPDATE → (draw) → CLEAN
CLEAN → (time range changed) → INCREMENTAL_UPDATE → (draw) → CLEAN
CLEAN → (interval changed) → FULL_REDRAW → (draw) → CLEAN
INCREMENTAL_UPDATE → (interval changed) → FULL_REDRAW (FULL_REDRAW takes precedence)
```

**Key Principle:** `FULL_REDRAW` always takes precedence and cannot be downgraded. Once set, it remains until the draw completes.

### 2.2 Drawing Flow

**Entry Point: `WaterfallGraph::draw()`**

```cpp
void WaterfallGraph::draw()
{
    // If state is CLEAN, set to FULL_REDRAW
    // Otherwise, respect current state (INCREMENTAL_UPDATE, etc.)
    if (m_renderState == RenderState::CLEAN) {
        setRenderState(RenderState::FULL_REDRAW);
    }
    
    // Delegate to incremental draw which handles all states
    drawIncremental();
}
```

**State Handler: `WaterfallGraph::drawIncremental()`**

The method uses a switch statement to handle each state:

- **`CLEAN`**: Early return, nothing to do
- **`RANGE_UPDATE_ONLY`**: Updates data ranges only, no graphics changes
- **`INCREMENTAL_UPDATE`**: 
  - Updates data ranges if needed
  - Redraws only dirty series (marked via `markSeriesDirty()`)
  - Uses `updateScatterplotItemsIncremental()` to:
    - Update positions of existing items
    - Add new items for new data points
    - Remove items for data points that moved out of range
- **`FULL_REDRAW`**:
  - Clears all graphics items (preserves overlay/cursor layers)
  - Rebuilds drawing area and grid
  - Updates data ranges
  - Redraws all series from scratch

### 2.3 Dirty Series Tracking

When data is added via `addDataPoint()` or `addDataPoints()`:
1. Data is added to `WaterfallData`
2. Series is marked dirty: `markSeriesDirty(seriesLabel)`
3. Range update is marked: `markRangeUpdateNeeded()`
4. Render state transitions to `INCREMENTAL_UPDATE`
5. `drawIncremental()` redraws only the dirty series

**Benefits:**
- Only changed series are redrawn
- Existing graphics items are repositioned, not recreated
- Items outside new time range are removed efficiently
- Dramatically reduces CPU usage during real-time updates

### 2.4 Visible Data Caching

To avoid scanning entire datasets on every draw:

**Cache Structure:**
- `m_cachedVisibleData[seriesLabel]`: Filtered data points within current time range
- `m_cachedTimeRange[seriesLabel]`: Time range used for the cache
- Cache is invalidated when time range or data changes

**Cache Update Strategies:**
- **Incremental Update**: When time range unchanged, only new data points are added to cache
- **Full Update**: When time range changes, cache is completely rebuilt using binary search

**Binary Search Optimization:**
- `findFirstVisibleIndex()`: O(log n) to find first point >= timeMin
- `findLastVisibleIndex()`: O(log n) to find last point <= timeMax
- Avoids O(n) linear scans of entire datasets

### 2.5 Graphics Item Management

**Scatterplot Items:**
- Stored in `m_seriesScatterplotItems[seriesLabel]` as vector of `QGraphicsPixmapItem*`
- During incremental updates:
  - Existing items are repositioned via `setPos()`
  - New items are created only for new data points
  - Old items are removed when data moves out of time range

**Symbol Items:**
- RTW symbols: `QGraphicsPixmapItem` with z-value 1000
- BTW symbols (magenta circles): `QGraphicsPixmapItem` with z-value 1003
- Symbols are removed and recreated when time range changes (not tracked like scatterplot items)

**Specialized Graph Overrides:**
- `BTWGraph::draw()`: Handles shaded regions, horizontal lines, interactive markers
- `RTWGraph::draw()`: Handles ADOPTED line, R markers, RTW symbols
- Other graphs: Extend base functionality with type-specific rendering

### 2.6 Time Range Updates

When time range changes (timer tick, animation, zoom):
1. `setTimeRange()` sets state to `INCREMENTAL_UPDATE`
2. Visible data cache is invalidated
3. `drawIncremental()` is called
4. Existing scatterplot items are repositioned
5. Symbols and markers are redrawn at new positions
6. Items outside new range are removed

**Optimization:** Only items that actually moved are updated, not all items.

---

## 3. Symbol and Sync State Architecture

### 3.1 Symbol System

**Symbol Types:**

1. **RTW Symbols** (`RTWSymbolData`):
   - Custom symbols: TM, DP, LY, CircleI, Triangle, etc.
   - Stored in `WaterfallData::rtwSymbols`
   - Added via `RTWGraph::addRTWSymbol()` or `WaterfallData::addRTWSymbol()`
   - Drawn via `RTWGraph::drawRTWSymbols()`
   - Each symbol has: symbolName, timestamp, range value

2. **BTW Symbols** (`BTWSymbolData`):
   - Magenta circles synchronized across all graphs
   - Stored in `WaterfallData::btwSymbols`
   - Added automatically when BTW markers are placed
   - Drawn via `WaterfallGraph::drawBTWSymbols()` (base class) or `BTWGraph::drawBTWSymbols()` (override)
   - Each symbol has: symbolName ("MagentaCircle"), timestamp, range value

**Symbol Storage:**
- Symbols are stored in `WaterfallData`, not in graphics items
- This ensures symbols persist with data and survive graph switches
- Symbols are filtered by time range when drawing (only visible symbols are rendered)

**Symbol Drawing:**
- Symbols are drawn as `QGraphicsPixmapItem` objects
- RTW symbols: z-value 1000, stored with timestamp/symbolName in item data
- BTW symbols: z-value 1003
- Symbols are removed and recreated when time range changes (to update positions)
- Old symbol items are cleaned up before drawing new ones to prevent duplicates

### 3.2 Synchronization Architecture

**Hub-and-Spoke Pattern:**

`GraphLayout` acts as the central hub, with all `GraphContainer` instances as spokes:

```
GraphLayout (Hub)
├── GraphContainer 1 (Spoke)
├── GraphContainer 2 (Spoke)
├── GraphContainer 3 (Spoke)
└── GraphContainer 4 (Spoke)
```

**Sync State Structure** (`GraphContainerSyncState`):

The sync state is a shared object passed to all containers, containing:

1. **Time Interval Sync:**
   - `currentInterval`: Current time interval (15min, 30min, 1hr, etc.)
   - `hasInterval`: Flag indicating interval is set

2. **Time Scope Sync:**
   - `currentTimeScope`: Visible time window (TimeSelectionSpan)
   - `hasTimeScope`: Flag indicating scope is set

3. **Cursor Time Sync:**
   - `cursorTime`: Current cursor timestamp
   - `hasCursorTime`: Flag indicating cursor time is set

4. **BTW Marker Sync:**
   - `btwMarkers`: Vector of `BTWSyncMarkerData`
   - `hasBTWMarkers`: Flag indicating markers exist
   - Each marker has: `QUuid id`, `timestamp`, `rangeValue`, `bearingRate`

5. **Shaded Region Sync:**
   - `shadedRegions`: Vector of `ShadedRegionSyncData`
   - `hasShadedRegions`: Flag indicating regions exist
   - Each region has: `QUuid syncId`, `startX`, `endX`, `id` (local)

6. **Follow Mode Sync:**
   - `isGraphContainerInFollowMode`: Auto-scroll vs frozen mode

### 3.3 Synchronization Flow

**BTW Marker Synchronization:**

1. User places marker in BTW graph → `BTWGraph::onMarkerAdded()`
2. Marker data converted to `BTWSyncMarkerData` with `QUuid`
3. Signal emitted: `BTWGraph::markerAdded(BTWSyncMarkerData)`
4. `GraphContainer` receives signal → `onBTWMarkerSyncDataChanged()`
5. `GraphContainer` emits to `GraphLayout` → `onBTWMarkerSyncDataChanged()`
6. `GraphLayout` updates sync state and propagates to all other containers
7. Other containers create/update markers in their BTW graphs
8. `GraphLayout` also adds magenta circle symbols to all graphs at marker timestamp

**Shaded Region Synchronization:**

1. User creates shaded region → `BTWGraph::addShadedRegion()`
2. Region assigned `QUuid syncId`
3. Signal emitted: `BTWGraph::shadedRegionAdded(ShadedRegionSyncData)`
4. `GraphContainer` receives → `onShadedRegionSyncAdded()`
5. `GraphContainer` emits to `GraphLayout` → `onShadedRegionSyncAdded()`
6. `GraphLayout` updates sync state and propagates to all other containers
7. Other containers create regions in their BTW graphs using same `syncId`

**Time Interval Synchronization:**

1. User changes interval in one container → `GraphContainer::onTimeIntervalChanged()`
2. Signal emitted: `GraphContainer::IntervalChanged(TimeInterval)`
3. `GraphLayout` receives → `onContainerIntervalChanged()`
4. `GraphLayout` updates sync state and calls `setTimeInterval()` on all containers
5. All containers update their graphs' time intervals
6. All graphs redraw with new interval

**Preventing Loops:**
- Source container is skipped during propagation
- `setTimeInterval()` doesn't emit signals (silent update)
- `m_updatingTimeInterval` flag prevents recursive updates

### 3.4 Symbol Synchronization

**BTW Symbol (Magenta Circle) Propagation:**

When a BTW marker is placed:
1. `GraphLayout::onBTWMarkerPlaced()` is called
2. For each graph type (excluding BTW):
   - Finds data point at marker timestamp
   - Adds magenta circle symbol to that graph's `WaterfallData`
3. All graphs redraw, showing magenta circles at marker timestamp

**Symbol Drawing During Updates:**
- Symbols are redrawn during `INCREMENTAL_UPDATE` and `RANGE_UPDATE_ONLY` states
- This ensures symbols update positions when time range changes
- Old symbol items are removed before drawing new ones to prevent duplicates

### 3.5 Sync State Management

**GraphContainerSyncState Methods:**

- `addOrUpdateBTWMarker()`: Add or update marker in sync state
- `removeBTWMarker()`: Mark marker as deleted (soft delete)
- `getBTWMarker()`: Retrieve marker by ID
- `getActiveBTWMarkers()`: Get all non-deleted markers
- `clearBTWMarkers()`: Clear all markers

Similar methods exist for shaded regions.

**Sync State Access:**
- `GraphLayout` owns the sync state (`m_syncState`)
- Containers receive pointer to sync state in constructor
- Sync state is shared across all containers in a layout
- External components can access via `GraphLayout::getSyncState()`

---

## Summary

The system architecture emphasizes:
- **Incremental rendering** for performance (only redraw what changed)
- **Centralized synchronization** via hub-and-spoke pattern
- **Persistent symbols** stored with data, not in graphics items
- **State machine-based drawing** to minimize unnecessary redraws
- **Efficient data filtering** using binary search and caching

This architecture enables real-time visualization of multiple graph types with synchronized navigation while maintaining high performance even with frequent data updates.








