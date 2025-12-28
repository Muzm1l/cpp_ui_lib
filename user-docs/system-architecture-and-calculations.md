# System Architecture and Calculation Flow

## 1. High-Level Architecture

### 1.1 Model-View Separation

The system follows a **Model-View-Controller** pattern with clear separation of concerns:

```
┌─────────────────────────────────────────────────────────┐
│                    GraphLayout                          │
│  (Hub/Controller - manages multiple containers)        │
└──────────────────┬──────────────────────────────────────┘
                   │
        ┌──────────┴──────────┐
        │                     │
┌───────▼────────┐    ┌───────▼────────┐
│ GraphEngine   │    │ GraphContainer  │
│  (Model)      │    │   (View)        │
│               │    │                 │
│ - WaterfallData│   │ - WaterfallGraph│
│ - Emits signals│   │ - TimelineView  │
│ - No UI       │   │ - ZoomPanel     │
└───────┬────────┘    └─────────────────┘
        │                     │
        └──────────┬──────────┘
                   │
            ┌──────▼──────┐
            │ WaterfallGraph│
            │  (Rendering)   │
            └───────────────┘
```

### 1.2 Core Components

#### A. GraphEngine (Model Layer)
- **Purpose**: Non-UI persistent state manager
- **Owns**: `WaterfallData` (data storage)
- **Emits Signals**:
  - `dataAppended(seriesLabel)` - New data points added
  - `dataRangeChanged()` - Y-range or time-range changed
  - `symbolsChanged()` - BTW/RTW symbols updated
  - `markersChanged()` - Interactive markers changed
- **No Graphics**: Pure data management, no `QGraphics*` or `QWidget` classes
- **Location**: `graphengine.h` / `graphengine.cpp`

#### B. WaterfallGraph (View Layer)
- **Purpose**: Rendering and display engine
- **Layered Rendering Architecture**:
  1. **Waterfall Buffer** (`QPixmap`) - Scroll buffer for waterfall effect (optimization)
  2. **Direct QPainter** - Scatter points, data lines (batched rendering for performance)
  3. **Overlay Scene** (`QGraphicsScene`) - Interactive elements (symbols, markers, lines)
  4. **Cursor Scene** (`QGraphicsScene`) - Crosshair, time cursor
- **Location**: `waterfallgraph.h` / `waterfallgraph.cpp`

#### C. GraphContainer (View Container)
- **Purpose**: Manages graph lifecycle and UI components
- **Components**:
  - `WaterfallGraph` instances (one per graph type: BDW, BRW, BTW, FDW, etc.)
  - `TimelineView` - Time axis visualization
  - `ZoomPanel` - Range controls
  - `QComboBox` - Graph type selector
- **Location**: `graphcontainer.h` / `graphcontainer.cpp`

#### D. GraphLayout (Hub/Controller)
- **Purpose**: Central orchestration for multiple containers
- **Responsibilities**:
  - Manages layout configurations (1x1, 2x2, 2x1, 4x1)
  - Centralized data source management (one `GraphEngine` per graph type)
  - Synchronization hub using hub-and-spoke pattern
  - Propagates changes across all containers
- **Location**: `graphlayout.h` / `graphlayout.cpp`

## 2. Calculation Flow

### 2.1 Coordinate Mapping (`mapDataToScreen`)

The core calculation converts data coordinates → screen pixels. This is the most performance-critical operation in the system.

#### X-Axis Mapping (Y-Value → Screen X)
```cpp
normalizedX = (yValue - yMin) / (yMax - yMin)
x = drawingArea.left() + normalizedX * drawingArea.width()
```

#### Y-Axis Mapping (Timestamp → Screen Y)
```cpp
// Epoch-based calculation (avoids timezone operations)
timeOffset = timeMaxEpoch - timestampEpoch  // Positive = timestamp is in the past
normalizedY = timeOffset / timeIntervalMs
y = drawingArea.top() + normalizedY * drawingArea.height()
```

#### Performance Optimizations

1. **Cached Reciprocals**: Pre-compute `1.0 / (yMax - yMin)` and `1.0 / timeIntervalMs` to avoid divisions
   - Stored in `m_cachedYRangeReciprocal` and `m_cachedTimeIntervalMsReciprocal`
   - Updated in `updateCoordinateMappingCaches()`

2. **Epoch Milliseconds**: Use `toMSecsSinceEpoch()` instead of `msecsTo()`
   - Avoids expensive timezone operations (reading `/etc/localtime`)
   - Overload: `mapDataToScreen(qreal yValue, qint64 timestampEpochMs)`

3. **Fast Path**: Early returns for invalid/infinite values
   ```cpp
   if (!qIsFinite(yValue) || m_cachedYRange <= 0.0) {
       return QPointF(0, 0);
   }
   ```

4. **Validation**: Check `dataRangesValid` and `drawingArea.isEmpty()` before calculations

**Location**: `waterfallgraph.cpp:2610-2713`

### 2.2 Data Range Calculation (`updateDataRanges`)

Calculates and updates the visible data ranges for both Y-axis (value) and time axis.

#### Process Flow

1. **Get Combined Y-Range from Data**:
   ```cpp
   auto yRange = dataSource->getCombinedYRange();
   qreal dataYMin = yRange.first;
   qreal dataYMax = yRange.second;
   ```

2. **Apply Range Limiting** (if enabled):
   ```cpp
   if (rangeLimitingEnabled) {
       yMin = qMax(customYMin, dataYMin);  // Take larger of custom min and data min
       yMax = qMin(customYMax, dataYMax);  // Take smaller of custom max and data max
       
       // Ensure min < max
       if (yMin >= yMax) {
           // Fallback to data range if custom range invalid
           yMin = dataYMin;
           yMax = dataYMax;
       }
   } else {
       // Use data range directly
       yMin = dataYMin;
       yMax = dataYMax;
   }
   ```

3. **Set Time Range**:
   ```cpp
   if (customTimeRangeEnabled) {
       timeMin = customTimeMin;
       timeMax = customTimeMax;
   } else {
       setTimeRangeFromData();  // Calculate from data timestamps
   }
   ```

4. **Update Coordinate Mapping Caches**:
   ```cpp
   updateCoordinateMappingCaches();
   // This updates:
   // - m_cachedTimeIntervalMs and reciprocal
   // - m_cachedYRange and reciprocal
   // - m_cachedTimeMaxEpoch
   ```

**Location**: `waterfallgraph.cpp:2438-2517`

### 2.3 Render State Machine

The system uses a state machine to optimize rendering by only redrawing what's necessary.

#### States

- **`CLEAN`**: No updates needed, graph is up-to-date
- **`INCREMENTAL_UPDATE`**: Only dirty series need redrawing
- **`RANGE_UPDATE_ONLY`**: Only ranges changed, reposition existing items
- **`FULL_REDRAW`**: Complete clear and rebuild of all graphics items

#### State Transitions

```
CLEAN → (data added) → INCREMENTAL_UPDATE → (draw) → CLEAN
CLEAN → (time range changed) → INCREMENTAL_UPDATE → (draw) → CLEAN
CLEAN → (interval changed) → FULL_REDRAW → (draw) → CLEAN
INCREMENTAL_UPDATE → (interval changed) → FULL_REDRAW (FULL_REDRAW takes precedence)
```

#### Dirty Series Tracking

- Only series that changed are marked dirty via `markSeriesDirty(seriesLabel)`
- `m_dirtySeries` set tracks which series need redrawing
- Prevents unnecessary redraws of unchanged data

**Key Principle**: `FULL_REDRAW` always takes precedence and cannot be downgraded.

**Location**: `waterfallgraph.cpp:1139-1205`

### 2.4 Inverse Mapping (`mapScreenXToRange`, `mapScreenToTime`)

#### Screen X → Data Range
```cpp
normalizedX = (xPos - drawingArea.left()) / drawingArea.width()
normalizedX = qMax(0.0, qMin(1.0, normalizedX))  // Clamp to [0,1]
range = yMin + normalizedX * (yMax - yMin)
```

#### Screen Y → Time
```cpp
normalizedY = (yPos - drawingArea.top()) / drawingArea.height()
normalizedY = qMax(0.0, qMin(1.0, normalizedY))  // Clamp to [0,1]
timeOffsetMs = normalizedY * m_cachedTimeIntervalMs
selectionTime = timeMax.addMSecs(-timeOffsetMs)
```

**Location**: `waterfallgraph.cpp:2721-2735`, `3225-3261`

## 3. Performance Optimizations

### 3.1 Visibility Caching

Filters data points to visible time range before rendering:

- **`updateVisibleDataCacheFull()`**: Full cache rebuild when time range changes
- **`updateVisibleDataCacheIncremental()`**: Only processes new data points
- Uses epoch milliseconds to avoid QDateTime conversions in loops
- Stores filtered data as `std::vector<std::pair<qreal, qint64>>` (value, epoch ms)

**Location**: `waterfallgraph.cpp:1350-1500`

### 3.2 Batch Rendering

#### Scatter Points
- Stored in `QMap<QString, QVector<QPointF>> m_scatterPoints`
- Colors stored in `QMap<QString, QColor> m_scatterColors`
- Rendered in single `paintEvent()` call using `QPainter::drawPoints()`

#### Data Lines
- Stored as `QMap<QString, QPainterPath> m_dataLinePaths`
- Colors stored in `QMap<QString, QColor> m_dataLineColors`
- Rendered directly in `paintEvent()` using `QPainter::drawPath()`

#### Waterfall Buffer
- Scroll buffer optimization: only draws new row, scrolls old data
- `QPixmap m_waterfallBuffer` stores the waterfall image
- `scrollWaterfallBuffer(int pixels)` moves existing data up
- `updateWaterfallBufferRow()` draws only the new bottom row

**Location**: `waterfallgraph.cpp:paintEvent()`, `waterfallgraph.h:150-160`

### 3.3 Level of Detail (LOD)

For large datasets, skips points to improve performance:

```cpp
size_t step = calculateLODStep(dataSize);
// For intervals >= 1 hour: step = max(1, dataSize / 1000)
// Otherwise: step = 1 (no skipping)
```

**Location**: `waterfallgraph.cpp:2569-2601`

### 3.4 Rendering Optimizations

1. **Antialiasing Disabled**: `graphicsView->setRenderHint(QPainter::Antialiasing)` commented out
   - Significant CPU savings for bulk data rendering

2. **Visibility Checks**: Early returns if widget not visible
   ```cpp
   if (!isVisible()) return;
   ```

3. **QGraphicsScene Elimination**: Direct `QPainter` rendering for bulk data
   - Only interactive elements use `QGraphicsScene` (overlayScene)

## 4. Data Flow

### 4.1 Data Addition Flow

```
External Data Source
    ↓
GraphLayout::addDataPointToDataSource()
    ↓
GraphEngine::addDataPoint()
    ↓
WaterfallData::addDataPointToSeries()
    ↓
GraphEngine emits dataAppended() signal
    ↓
WaterfallGraph::markSeriesDirty()
    ↓
WaterfallGraph::drawIncremental()
    ↓
Update visible data cache
    ↓
Render to screen (paintEvent / QGraphicsScene)
```

### 4.2 Signal/Slot Connections

**GraphEngine → WaterfallGraph**:
```cpp
connect(m_engine, &GraphEngine::dataAppended, this, [this](const QString &seriesLabel) {
    if (!isVisible()) return;  // Optimization
    markSeriesDirty(seriesLabel);
    markRangeUpdateNeeded();
    drawIncremental();
});

connect(m_engine, &GraphEngine::dataRangeChanged, this, [this]() {
    if (!isVisible()) return;  // Optimization
    markRangeUpdateNeeded();
    dataRangesValid = false;
    drawIncremental();
});
```

**Location**: `waterfallgraph.cpp:380-413`

## 5. Specialized Graph Types

All graph types inherit from `WaterfallGraph` and add type-specific features:

### BDW/BRW/FDW Graphs
- **Zero Axis Line**: Vertical dashed line at `m_zeroAxisValue`
- **Update Conditions**: 
  - Full redraw (`FULL_REDRAW`)
  - Range updates (`RANGE_UPDATE_ONLY`)
  - Incremental updates (`INCREMENTAL_UPDATE`) - **Critical for time-dependent positioning**
  - Line doesn't exist yet (`!m_zeroAxisLineItem`)
- **Location**: `bdwgraph.cpp`, `brwgraph.cpp`, `fdwgraph.cpp`

### BTW Graph
- **Shaded Regions**: Rectangular regions with hatch pattern
- **Horizontal Lines**: Time-based reference lines
- **Interactive Markers**: Draggable markers with constraints
- **BTW Symbols**: Magenta circles positioned by timestamp
- **Location**: `btwgraph.cpp`, `btwinteractiveoverlay.cpp`

### RTW Graph
- **Symbol Rendering**: Special positioning for RTW symbols
- **Location**: `rtwgraph.cpp`, `rtwsymboldrawing.cpp`

## 6. Synchronization

### 6.1 GraphContainerSyncState

Synchronizes state across multiple containers:
- **Markers**: BTW interactive markers
- **Shaded Regions**: BTW shaded regions
- **Time Scope**: Shared time range selection
- **Cursor Time**: Shared time cursor position

### 6.2 Shared Time Axis

Cursor position synchronized across all graphs:
- `applySharedTimeAxisCursor(const QDateTime &time)`
- Updates cursor in all visible graphs simultaneously

### 6.3 BTW Interactive Overlay

Manages interactive markers with position constraints:
- Stores original data values (timestamp, range value)
- Updates screen positions on time range changes
- Uses `mapDataToScreen()` with epoch milliseconds for performance

**Location**: `btwinteractiveoverlay.cpp:870-921`

## 7. Key Data Structures

### 7.1 WaterfallData
- **Storage**: `std::map<QString, std::vector<qreal>>` for Y-values
- **Timestamps**: `std::map<QString, std::vector<QDateTime>>` and epoch milliseconds
- **Symbols**: BTW symbols, RTW symbols stored separately
- **Location**: `waterfalldata.h` / `waterfalldata.cpp`

### 7.2 Cached Values (WaterfallGraph)

**Coordinate Mapping Caches**:
- `m_cachedTimeIntervalMs` - Time interval in milliseconds
- `m_cachedTimeIntervalMsReciprocal` - `1.0 / timeIntervalMs`
- `m_cachedYRange` - `yMax - yMin`
- `m_cachedYRangeReciprocal` - `1.0 / (yMax - yMin)`
- `m_cachedTimeMaxEpoch` - `timeMax.toMSecsSinceEpoch()`

**Visibility Caches**:
- `m_cachedVisibleData` - Filtered data points per series
- `m_lastProcessedIndex` - Last processed index per series for incremental updates
- `m_cachedTimeRange` - Cached time range per series

**Location**: `waterfallgraph.h:190-237`

## 8. Critical Performance Considerations

### 8.1 Timezone Operations

**Problem**: `QDateTime::msecsTo()` triggers expensive timezone reads (`/etc/localtime`)

**Solution**: Use epoch milliseconds throughout:
- Convert to epoch once: `timestamp.toMSecsSinceEpoch()`
- Use epoch-based calculations: `timeOffset = timeMaxEpoch - timestampEpoch`
- Overload `mapDataToScreen()` to accept `qint64 timestampEpochMs`

### 8.2 Division Operations

**Problem**: Division is slower than multiplication

**Solution**: Pre-compute reciprocals:
- `m_cachedYRangeReciprocal = 1.0 / (yMax - yMin)`
- `m_cachedTimeIntervalMsReciprocal = 1.0 / timeIntervalMs`
- Use multiplication: `normalizedX = (yValue - yMin) * m_cachedYRangeReciprocal`

### 8.3 QGraphicsScene Overhead

**Problem**: `QGraphicsScene` has significant overhead for bulk data rendering

**Solution**: 
- Direct `QPainter` rendering for scatter points and data lines
- Only use `QGraphicsScene` (overlayScene) for interactive elements
- Batch rendering in single `paintEvent()` call

### 8.4 Visibility Filtering

**Problem**: Rendering all data points even when outside visible time range

**Solution**: 
- Filter data to visible time range before rendering
- Incremental cache updates for new data only
- Use binary search for efficient time range filtering

## 9. Coordinate System

### 9.1 Screen Coordinates

- **Origin**: Top-left corner (0, 0)
- **X-axis**: Left to right (value increases)
- **Y-axis**: Top to bottom (time increases downward - top = current time, bottom = past)

### 9.2 Data Coordinates

- **X-axis**: Y-value (range value) from `yMin` to `yMax`
- **Y-axis**: Time from `timeMin` (past) to `timeMax` (current)

### 9.3 Drawing Area

- `drawingArea` (`QRectF`) defines the visible plotting area
- Excludes margins, labels, and UI controls
- Updated in `setupDrawingArea()` based on widget size and margins

**Location**: `waterfallgraph.cpp:setupDrawingArea()`

## 10. Summary

The system architecture separates data management (GraphEngine) from rendering (WaterfallGraph), uses cached calculations for coordinate mapping, batches rendering operations, and implements incremental updates to minimize CPU usage. The calculation flow prioritizes performance through:

1. **Epoch-based time calculations** (avoids timezone operations)
2. **Cached reciprocals** (avoids divisions)
3. **Visibility filtering** (only renders visible data)
4. **Batch rendering** (single paintEvent call)
5. **Incremental updates** (only redraws changed series)
6. **Direct QPainter rendering** (avoids QGraphicsScene overhead for bulk data)

This architecture enables real-time performance for large datasets with multiple graph types and synchronized navigation.

