# SCW (Situational Control Window) - Working and API Documentation

## Table of Contents

1. [Overview](#overview)
2. [Architecture](#architecture)
3. [Data Series Types](#data-series-types)
4. [API Reference](#api-reference)
5. [Timeline Synchronization](#timeline-synchronization)
6. [BTW Symbols (Magenta Circles)](#btw-symbols-magenta-circles)
7. [Rendering Configuration](#rendering-configuration)
8. [Integration Guide](#integration-guide)
9. [Usage Examples](#usage-examples)
10. [Troubleshooting](#troubleshooting)

---

## Overview

The **SCW (Situational Control Window)** is a specialized Qt widget (`SCWWindow`) that provides a multi-graph visualization system for displaying time-series data across 8 synchronized waterfall graphs. It features:

- **8 Display Windows**: 1 timeline view + 7 waterfall graphs
- **16 Data Series**: Organized into 4 categories (RULER, B, A, E, ADOPTED)
- **Bidirectional Timeline Synchronization**: SCW timeline syncs with GraphLayout timeline views
- **Line Drawing**: All graphs use line rendering (not scatterplot points)
- **BTW Symbol Support**: Magenta circles can be displayed at specific timestamps
- **Window Selection**: Visual feedback with yellow borders
- **Series Cycling**: Windows 5-7 can cycle through multiple series

### Key Features

- **Real-time Data Updates**: Supports both `setDataPoints()` (replace) and `addDataPoints()` (append)
- **Incremental Rendering**: Efficient updates when new data is added
- **Synchronized Navigation**: Time interval, time scope, and absolute/relative mode sync across all timelines
- **Event Filtering**: Mouse events for window selection
- **Timer Integration**: Supports timer-based updates via `QTimer`

---

## Architecture

### Layout Structure

```
SCWWindow
├── QHBoxLayout (main horizontal layout)
│   ├── TimelineView (fixed width, expanding height)
│   │   └── Time navigation controls (interval buttons, slider, Abs/Rel toggle)
│   └── QFrame[0-7] (8 container frames, each with expanding size policy)
│       └── QVBoxLayout
│           ├── QPushButton (label button, fixed height 30px)
│           │   └── Shows current series name
│           └── WaterfallGraph (expanding, fills remaining space)
│               └── Line drawing mode enabled
```

### Window Configuration

| Window Index | Series Type | Series Options | Behavior |
|--------------|-------------|----------------|-----------|
| 0 | ADOPTED | ADOPTED | Fixed (always displays ADOPTED) |
| 1 | RULER_1 | RULER_1 | Fixed (always displays RULER_1) |
| 2 | RULER_2 | RULER_2 | Fixed (always displays RULER_2) |
| 3 | RULER_3 | RULER_3 | Fixed (always displays RULER_3) |
| 4 | RULER_4 | RULER_4 | Fixed (always displays RULER_4) |
| 5 | SCW_SERIES_B | BRAT, BOT, BFT, BOPT, BOTC | Cycling (button cycles through series) |
| 6 | SCW_SERIES_A | ATMA, ATMAF | Cycling (button cycles through series) |
| 7 | SCW_SERIES_E | EXTERNAL1-5 | Cycling (button cycles through series) |

### Data Source Management

Each series type has its own `QMap` storing `WaterfallData*` instances:

- `m_dataSourcesAdopted`: Maps `SCW_SERIES_ADOPTED` → `WaterfallData*` (1 series)
- `m_dataSourcesR`: Maps `SCW_SERIES_R` → `WaterfallData*` (4 series)
- `m_dataSourcesB`: Maps `SCW_SERIES_B` → `WaterfallData*` (5 series)
- `m_dataSourcesA`: Maps `SCW_SERIES_A` → `WaterfallData*` (2 series)
- `m_dataSourcesE`: Maps `SCW_SERIES_E` → `WaterfallData*` (5 series)

**Total: 17 data sources** (1 + 4 + 5 + 2 + 5)

### Component Relationships

```
SCWWindow
├── TimelineView (m_timelineView)
│   └── Synchronized with GraphLayout timeline views
├── WaterfallGraph[0-7] (m_waterfallGraphs)
│   ├── Line drawing enabled (m_useLineDrawing = true)
│   ├── Event filter installed for mouse selection
│   └── Connected to WaterfallData sources
└── GraphContainerSyncState (m_syncState)
    └── Shared state for timeline synchronization
```

---

## Data Series Types

### SCW_SERIES_ADOPTED

Single series for the ADOPTED data.

```cpp
enum class SCW_SERIES_ADOPTED
{
    ADOPTED,  // Window 0
};
```

**Helper Functions:**
- `QString scwSeriesAdoptedToString(SCW_SERIES_ADOPTED series)`
- `SCW_SERIES_ADOPTED stringToScwSeriesAdopted(const QString &str)`

### SCW_SERIES_R (RULER Series)

Fixed series displayed in windows 1-4.

```cpp
enum class SCW_SERIES_R
{
    RULER_1,  // Window 1
    RULER_2,  // Window 2
    RULER_3,  // Window 3
    RULER_4,  // Window 4
};
```

**Helper Functions:**
- `QString scwSeriesRToString(SCW_SERIES_R series)`
- `SCW_SERIES_R stringToScwSeriesR(const QString &str)`

### SCW_SERIES_B

Cycling series for window 5.

```cpp
enum class SCW_SERIES_B
{
    BRAT,   // Bit Rate Average Time
    BOT,    // Bit Over Time
    BFT,    // Bit Frequency Time
    BOPT,   // Bit Optimal Time
    BOTC,   // Bit Over Time Count
};
```

**Helper Functions:**
- `QString scwSeriesBToString(SCW_SERIES_B series)`
- `SCW_SERIES_B stringToScwSeriesB(const QString &str)`

### SCW_SERIES_A

Cycling series for window 6.

```cpp
enum class SCW_SERIES_A
{
    ATMA,   // ATM Average
    ATMAF,  // ATM Average Frequency
};
```

**Helper Functions:**
- `QString scwSeriesAToString(SCW_SERIES_A series)`
- `SCW_SERIES_A stringToScwSeriesA(const QString &str)`

### SCW_SERIES_E (External Series)

Cycling series for window 7.

```cpp
enum class SCW_SERIES_E
{
    EXTERNAL1,
    EXTERNAL2,
    EXTERNAL3,
    EXTERNAL4,
    EXTERNAL5,
};
```

**Helper Functions:**
- `QString scwSeriesEToString(SCW_SERIES_E series)`
- `SCW_SERIES_E stringToScwSeriesE(const QString &str)`

---

## API Reference

### Constructor

```cpp
explicit SCWWindow(QWidget *parent = nullptr, 
                   QTimer *timer = nullptr, 
                   GraphContainerSyncState *syncState = nullptr);
```

**Parameters:**
- `parent`: Parent widget (typically a tab widget or main window)
- `timer`: `QTimer` instance for TimelineView synchronization (optional)
- `syncState`: Pointer to shared `GraphContainerSyncState` for synchronization with GraphLayout (optional)

**Description:**
Creates a new `SCWWindow` instance with:
- 8 waterfall graphs (all configured for line drawing)
- 1 timeline view (synchronized with GraphLayout if `syncState` provided)
- 17 data sources (one per series)
- Event filters installed for mouse selection

**Example:**
```cpp
QTimer* timer = new QTimer(this);
timer->setInterval(1000); // 1 second
GraphContainerSyncState* syncState = graphLayout->getSyncState();
SCWWindow* scwWindow = new SCWWindow(parentWidget, timer, syncState);
```

### Data Management APIs

#### setDataPoints (ADOPTED Series)

```cpp
void setDataPoints(SCW_SERIES_ADOPTED series, 
                   const std::vector<qreal> &yData, 
                   const std::vector<QDateTime> &timestamps);
```

**Parameters:**
- `series`: The ADOPTED series enum (ADOPTED)
- `yData`: Vector of data values (qreal)
- `timestamps`: Vector of QDateTime timestamps (must match yData size)

**Description:**
Replaces all existing data for the ADOPTED series. The graph in window 0 is updated and redrawn.

**Example:**
```cpp
std::vector<qreal> values = {10.5, 20.3, 30.7, 40.2};
std::vector<QDateTime> times = {
    QDateTime::currentDateTime().addSecs(-30),
    QDateTime::currentDateTime().addSecs(-20),
    QDateTime::currentDateTime().addSecs(-10),
    QDateTime::currentDateTime()
};
scwWindow->setDataPoints(SCW_SERIES_ADOPTED::ADOPTED, values, times);
```

#### addDataPoints (ADOPTED Series)

```cpp
void addDataPoints(SCW_SERIES_ADOPTED series, 
                   const std::vector<qreal> &yData, 
                   const std::vector<QDateTime> &timestamps);
```

**Description:**
Appends new data points to the ADOPTED series. The graph performs an incremental redraw.

#### setDataPoints (RULER Series)

```cpp
void setDataPoints(SCW_SERIES_R series, 
                   const std::vector<qreal> &yData, 
                   const std::vector<QDateTime> &timestamps);
```

**Parameters:**
- `series`: The RULER series enum (RULER_1, RULER_2, RULER_3, or RULER_4)
- `yData`: Vector of data values
- `timestamps`: Vector of QDateTime timestamps

**Description:**
Replaces all existing data for the specified RULER series. The corresponding WaterfallGraph is updated and redrawn.

#### addDataPoints (RULER Series)

```cpp
void addDataPoints(SCW_SERIES_R series, 
                   const std::vector<qreal> &yData, 
                   const std::vector<QDateTime> &timestamps);
```

**Description:**
Appends new data points to the specified RULER series. The graph performs an incremental redraw.

#### setDataPoints (SCW_SERIES_B)

```cpp
void setDataPoints(SCW_SERIES_B series, 
                   const std::vector<qreal> &yData, 
                   const std::vector<QDateTime> &timestamps);
```

**Parameters:**
- `series`: The B series enum (BRAT, BOT, BFT, BOPT, or BOTC)
- `yData`: Vector of data values
- `timestamps`: Vector of QDateTime timestamps

**Description:**
Replaces all existing data for the specified B series. If the series is currently displayed in window 5, the graph is updated immediately. Otherwise, the data source is updated but the graph won't refresh until that series is selected.

#### addDataPoints (SCW_SERIES_B)

```cpp
void addDataPoints(SCW_SERIES_B series, 
                   const std::vector<qreal> &yData, 
                   const std::vector<QDateTime> &timestamps);
```

**Description:**
Appends new data points to the specified B series. If the series is currently displayed in window 5, the graph performs an incremental redraw.

#### setDataPoints (SCW_SERIES_A)

```cpp
void setDataPoints(SCW_SERIES_A series, 
                   const std::vector<qreal> &yData, 
                   const std::vector<QDateTime> &timestamps);
```

**Parameters:**
- `series`: The A series enum (ATMA or ATMAF)
- `yData`: Vector of data values
- `timestamps`: Vector of QDateTime timestamps

**Description:**
Replaces all existing data for the specified A series. If the series is currently displayed in window 6, the graph is updated immediately.

#### addDataPoints (SCW_SERIES_A)

```cpp
void addDataPoints(SCW_SERIES_A series, 
                   const std::vector<qreal> &yData, 
                   const std::vector<QDateTime> &timestamps);
```

**Description:**
Appends new data points to the specified A series. If the series is currently displayed in window 6, the graph performs an incremental redraw.

#### setDataPoints (SCW_SERIES_E)

```cpp
void setDataPoints(SCW_SERIES_E series, 
                   const std::vector<qreal> &yData, 
                   const std::vector<QDateTime> &timestamps);
```

**Parameters:**
- `series`: The E series enum (EXTERNAL1 through EXTERNAL5)
- `yData`: Vector of data values
- `timestamps`: Vector of QDateTime timestamps

**Description:**
Replaces all existing data for the specified E series. If the series is currently displayed in window 7, the graph is updated immediately.

#### addDataPoints (SCW_SERIES_E)

```cpp
void addDataPoints(SCW_SERIES_E series, 
                   const std::vector<qreal> &yData, 
                   const std::vector<QDateTime> &timestamps);
```

**Description:**
Appends new data points to the specified E series. If the series is currently displayed in window 7, the graph performs an incremental redraw.

### Utility APIs

#### `TimelineView* getTimelineView() const`

**Returns:** Pointer to the internal `TimelineView` instance, or `nullptr` if not initialized

**Description:**
Provides access to the `TimelineView` for synchronization purposes. Use this when you need to sync the SCW timeline with external timeline views (e.g., from `GraphLayout`).

**Example:**
```cpp
TimelineView* scwTimelineView = scwWindow->getTimelineView();
if (scwTimelineView && graphLayout) {
    graphLayout->syncExternalTimelineView(scwTimelineView);
}
```

#### `void clearAllGraphs()`

**Description:**
Clears all data from all graphs in the `SCWWindow`. This method clears all data points from all 17 data sources and triggers a redraw of all graphs.

**Example:**
```cpp
scwWindow->clearAllGraphs();
```

**Note:** This only clears the data points. It does not clear markers, symbols, or other annotations.

#### `void addBTWSymbolToAllGraphs(const QDateTime &timestamp)`

**Parameters:**
- `timestamp`: The QDateTime timestamp where the magenta circle should be displayed

**Description:**
Adds a BTW symbol (magenta circle) to all 8 SCW graphs at the specified timestamp. This is typically called when a BTW marker is placed in the main GraphLayout.

**Example:**
```cpp
QDateTime markerTime = QDateTime::currentDateTime();
scwWindow->addBTWSymbolToAllGraphs(markerTime);
```

### Signals

#### `void seriesSelected(const QString &seriesName)`

**Parameters:**
- `seriesName`: QString containing the series name (e.g., "ADOPTED", "RULER_1", "BRAT", "ATMA", "EXTERNAL1")

**Description:**
Emitted when a window is selected (either by clicking the graph or clicking the button for windows 1-4). The signal carries the name of the currently displayed series.

**Example Connection:**
```cpp
connect(scwWindow, &SCWWindow::seriesSelected, 
        [](const QString &seriesName) {
            qDebug() << "Selected series:" << seriesName;
            // Handle selection change
        });
```

**Series Name Values:**
- Window 0: "ADOPTED"
- Windows 1-4: "RULER_1", "RULER_2", "RULER_3", "RULER_4"
- Window 5: "BRAT", "BOT", "BFT", "BOPT", or "BOTC" (depending on current cycle)
- Window 6: "ATMA" or "ATMAF" (depending on current cycle)
- Window 7: "EXTERNAL1", "EXTERNAL2", "EXTERNAL3", "EXTERNAL4", or "EXTERNAL5" (depending on current cycle)

---

## Timeline Synchronization

### Overview

SCW timeline view is **bidirectionally synchronized** with GraphLayout timeline views. This ensures that:

- Time interval changes (15 min, 30 min, 1 hour, etc.) sync across all timelines
- Time scope changes (slider position/time window) sync across all timelines
- Absolute/Relative time mode changes sync across all timelines

### Signal Flow

#### SCW Timeline → Waterfall Graphs (SCW → GraphLayout)

When the SCW timeline view changes:

```
SCW TimelineView
    ↓ (emits TimeScopeChanged)
GraphLayout::syncExternalTimelineView()
    ↓ (connects to)
GraphContainer::onTimeScopeChanged()
    ↓ (calls)
WaterfallGraph::setTimeRange()
    ↓ (updates)
WaterfallGraph time range
```

#### Waterfall Timeline → SCW Timeline (GraphLayout → SCW)

When a waterfall graph timeline view changes:

```
GraphLayout TimelineView
    ↓ (emits TimeScopeChanged)
GraphLayout::syncExternalTimelineView()
    ↓ (connects to)
SCW TimelineView::setVisibleTimeWindow()
    ↓ (updates)
SCW TimelineView slider position
```

### Synchronized Properties

1. **Time Interval**: 15 min, 30 min, 1 hour, 2 hours, 4 hours, 8 hours
2. **Time Scope**: Visible time window (slider position)
3. **Absolute/Relative Mode**: Time display format toggle

### Setup Code

```cpp
// In MainWindow::setupSCWWindow()
GraphContainerSyncState* syncState = graphLayout->getSyncState();
SCWWindow* scwWindow = new SCWWindow(parentWidget, timer, syncState);

// Sync the timeline views
if (graphLayout && scwWindow->getTimelineView()) {
    graphLayout->syncExternalTimelineView(scwWindow->getTimelineView());
}
```

---

## BTW Symbols (Magenta Circles)

### Overview

SCW supports displaying BTW symbols (magenta circles) at specific timestamps. These symbols are synchronized with BTW markers placed in the main GraphLayout.

### Adding BTW Symbols

#### Programmatic Addition

```cpp
void SCWWindow::addBTWSymbolToAllGraphs(const QDateTime &timestamp);
```

This method adds a magenta circle to all 8 SCW graphs at the specified timestamp.

#### Automatic Synchronization

When a BTW marker is placed in GraphLayout, the `BTWSymbolAddedToAllGraphs` signal is emitted. Connect this to SCW:

```cpp
// In MainWindow::setupSCWWindow()
connect(graphLayout, &GraphLayout::BTWSymbolAddedToAllGraphs,
        scwWindow, &SCWWindow::addBTWSymbolToAllGraphs);
```

### Symbol Rendering

- **Color**: Magenta (Qt::magenta)
- **Size**: Fixed size circle
- **Position**: Y-axis position calculated from timestamp
- **Z-Value**: 1003 (above data lines, below overlays)
- **Cleanup**: Old symbols are automatically removed before drawing new ones to prevent duplicates

### Implementation Details

The symbols are rendered using `QGraphicsPixmapItem` in the `overlayScene` of each `WaterfallGraph`. The `drawBTWSymbols()` method:

1. Removes all existing BTW symbol items (z-value 1003)
2. Queries the data source for BTW symbols at visible timestamps
3. Maps timestamps to screen Y coordinates
4. Creates and positions pixmap items for each symbol

---

## Rendering Configuration

### Line Drawing Mode

All SCW graphs use **line drawing** instead of scatterplot points. This is configured automatically:

```cpp
// In SCWWindow::setupWaterfallGraphs()
m_waterfallGraphs[i]->setUseLineDrawing(true);
```

### Rendering Pipeline

1. **Data Series**: Rendered as connected lines using `QPainterPath`
2. **BTW Symbols**: Rendered as magenta circles using `QGraphicsPixmapItem`
3. **Grid**: Rendered if enabled
4. **Crosshair**: Rendered on mouse hover

### Performance Optimizations

- **Incremental Updates**: Only dirty series are redrawn when new data is added
- **Visible Data Caching**: Only data within the visible time range is processed
- **LOD (Level of Detail)**: Large datasets use batched paths for efficient rendering
- **Coordinate Mapping Cache**: Screen coordinate calculations are cached

---

## Integration Guide

### Step 1: Create SCWWindow

```cpp
#include "scwwindow.h"

// Create timer
QTimer* timeUpdateTimer = new QTimer(this);
timeUpdateTimer->setInterval(1000); // 1 second
timeUpdateTimer->start();

// Get sync state from GraphLayout
GraphContainerSyncState* syncState = graphLayout->getSyncState();

// Create SCWWindow
SCWWindow* scwWindow = new SCWWindow(parentWidget, timeUpdateTimer, syncState);
scwWindow->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
```

### Step 2: Add to Layout

```cpp
QVBoxLayout* layout = new QVBoxLayout(parentWidget);
layout->setContentsMargins(0, 0, 0, 0);
layout->addWidget(scwWindow);
```

### Step 3: Synchronize Timeline Views

```cpp
if (graphLayout && scwWindow->getTimelineView()) {
    graphLayout->syncExternalTimelineView(scwWindow->getTimelineView());
}
```

### Step 4: Connect BTW Symbol Signals (Optional)

```cpp
connect(graphLayout, &GraphLayout::BTWSymbolAddedToAllGraphs,
        scwWindow, &SCWWindow::addBTWSymbolToAllGraphs);
```

### Step 5: Connect Selection Signal (Optional)

```cpp
connect(scwWindow, &SCWWindow::seriesSelected, 
        this, &MyClass::onSeriesSelected);

void MyClass::onSeriesSelected(const QString &seriesName)
{
    qDebug() << "User selected series:" << seriesName;
    // Update UI, log selection, etc.
}
```

---

## Usage Examples

### Example 1: Real-time Data Streaming

```cpp
// In a timer callback or data stream handler
void onNewDataReceived(SCW_SERIES_R series, qreal value) {
    std::vector<qreal> singleValue = {value};
    std::vector<QDateTime> singleTime = {QDateTime::currentDateTime()};
    scwWindow->addDataPoints(series, singleValue, singleTime);
}
```

### Example 2: Batch Historical Data Loading

```cpp
void loadHistoricalData() {
    auto historicalData = fetchFromDatabase();
    
    // Load ADOPTED data
    scwWindow->setDataPoints(SCW_SERIES_ADOPTED::ADOPTED, 
                             historicalData.adoptedValues, 
                             historicalData.adoptedTimestamps);
    
    // Load RULER data
    for (int i = 0; i < 4; ++i) {
        SCW_SERIES_R series = static_cast<SCW_SERIES_R>(i);
        scwWindow->setDataPoints(series, 
                                 historicalData.rulerValues[i], 
                                 historicalData.rulerTimestamps[i]);
    }
}
```

### Example 3: Cycling Series Data Updates

```cpp
// Add data to all B series (window 5 cycles through these)
void updateAllBSeries(const std::vector<qreal> &values, 
                      const std::vector<QDateTime> &times) {
    scwWindow->addDataPoints(SCW_SERIES_B::BRAT, values, times);
    scwWindow->addDataPoints(SCW_SERIES_B::BOT, values, times);
    scwWindow->addDataPoints(SCW_SERIES_B::BFT, values, times);
    scwWindow->addDataPoints(SCW_SERIES_B::BOPT, values, times);
    scwWindow->addDataPoints(SCW_SERIES_B::BOTC, values, times);
}
```

### Example 4: Complete Integration

```cpp
class MyApplication : public QMainWindow
{
    Q_OBJECT

private:
    QTimer* m_timer;
    GraphLayout* m_graphLayout;
    SCWWindow* m_scwWindow;

public:
    MyApplication(QWidget* parent = nullptr)
        : QMainWindow(parent)
    {
        // Setup timer
        m_timer = new QTimer(this);
        m_timer->setInterval(1000);
        m_timer->start();
        
        // Create GraphLayout
        m_graphLayout = new GraphLayout(this, LayoutType::GPW4W, m_timer);
        
        // Create SCWWindow with sync state
        GraphContainerSyncState* syncState = m_graphLayout->getSyncState();
        m_scwWindow = new SCWWindow(this, m_timer, syncState);
        
        // Synchronize timeline views
        if (m_graphLayout && m_scwWindow->getTimelineView()) {
            m_graphLayout->syncExternalTimelineView(m_scwWindow->getTimelineView());
        }
        
        // Connect BTW symbol signals
        connect(m_graphLayout, &GraphLayout::BTWSymbolAddedToAllGraphs,
                m_scwWindow, &SCWWindow::addBTWSymbolToAllGraphs);
        
        // Connect selection signal
        connect(m_scwWindow, &SCWWindow::seriesSelected,
                this, &MyApplication::onSeriesSelected);
        
        // Setup UI layout
        QTabWidget* tabWidget = new QTabWidget(this);
        tabWidget->addTab(m_graphLayout, "Graphs");
        
        QWidget* scwTab = new QWidget();
        QVBoxLayout* scwLayout = new QVBoxLayout(scwTab);
        scwLayout->setContentsMargins(0, 0, 0, 0);
        scwLayout->addWidget(m_scwWindow);
        tabWidget->addTab(scwTab, "SCW");
        
        setCentralWidget(tabWidget);
    }

private slots:
    void onSeriesSelected(const QString& seriesName)
    {
        statusBar()->showMessage("Selected: " + seriesName);
    }
};
```

---

## Troubleshooting

### Issue: Graphs Not Updating

**Symptoms:** Data is added but graphs don't refresh.

**Solutions:**
1. Ensure timestamps are valid `QDateTime` objects
2. Check that data and timestamp vectors have matching sizes
3. For cycling windows (5-7), verify the series is currently displayed
4. Check debug output for error messages
5. Verify that `setUseLineDrawing(true)` was called during setup

### Issue: Timeline Not Synchronizing

**Symptoms:** SCW timeline and GraphLayout timelines are out of sync.

**Solutions:**
1. Verify `syncState` is passed to SCWWindow constructor
2. Check that `syncExternalTimelineView()` is called after SCWWindow creation
3. Ensure the same `GraphContainerSyncState` instance is used
4. Check debug output for connection errors

### Issue: BTW Symbols Not Appearing

**Symptoms:** Magenta circles don't show up in SCW graphs.

**Solutions:**
1. Verify `addBTWSymbolToAllGraphs()` is being called
2. Check that timestamps are within the visible time range
3. Ensure the signal connection is established:
   ```cpp
   connect(graphLayout, &GraphLayout::BTWSymbolAddedToAllGraphs,
           scwWindow, &SCWWindow::addBTWSymbolToAllGraphs);
   ```
4. Check that BTW symbols exist in the data source

### Issue: Duplicate Magenta Circles

**Symptoms:** Multiple magenta circles appear at the same timestamp.

**Solutions:**
1. This should be fixed in the current implementation
2. The `drawBTWSymbols()` method now always removes old symbols before drawing new ones
3. If duplicates persist, check that `drawBTWSymbols()` is not being called multiple times without cleanup

### Issue: Graphs Showing Points Instead of Lines

**Symptoms:** Data is displayed as scatterplot points instead of connected lines.

**Solutions:**
1. Verify `setUseLineDrawing(true)` is called for all graphs
2. Check that the flag is set before the first `draw()` call
3. Ensure `WaterfallGraph::drawDataLine()` is being called (not `drawDataSeries()`)

### Issue: Selection Not Working

**Symptoms:** Clicking graphs doesn't select windows.

**Solutions:**
1. Verify event filters are installed (done automatically in constructor)
2. Check that graphs are not disabled or hidden
3. Ensure mouse events are not being intercepted by parent widgets
4. For windows 1-4, button clicks should also work

### Issue: Performance Problems

**Symptoms:** UI becomes sluggish with many data points.

**Solutions:**
1. Use `addDataPoints()` for incremental updates (more efficient than `setDataPoints()`)
2. Limit the number of data points per series
3. Consider reducing update frequency if using a simulator
4. Check that visible data caching is working (only visible data is processed)

---

## Summary

The SCW (Situational Control Window) provides a comprehensive solution for displaying multiple synchronized waterfall graphs with:

- **8 Display Windows**: 1 timeline + 7 waterfall graphs
- **17 Data Series**: Organized into 5 categories
- **Bidirectional Timeline Synchronization**: With GraphLayout timeline views
- **Line Drawing**: All graphs use connected line rendering
- **BTW Symbol Support**: Magenta circles at specific timestamps
- **Window Selection**: Visual feedback with yellow borders
- **Series Cycling**: Windows 5-7 can cycle through multiple series

For questions or issues, refer to the source code in `scwwindow.h` and `scwwindow.cpp`, or check the integration example in `mainwindow.cpp`.

