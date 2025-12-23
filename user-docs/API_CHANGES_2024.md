# API Changes and Integration Guide

This document describes recent API changes and how to integrate them into your codebase.

**Last Updated:** 2024

---

## Table of Contents

1. [Magenta Circles for BTW Markers](#magenta-circles-for-btw-markers)
2. [BTW Horizontal Line Delete Mode](#btw-horizontal-line-delete-mode)
3. [Clear Graphs Fix](#clear-graphs-fix)
4. [Integration Examples](#integration-examples)

---

## Magenta Circles for BTW Markers

### Overview

Magenta circles are now automatically added to all graphs (including SCW graphs) whenever a BTW marker (automatic or manual) is placed. This provides visual synchronization across all graph types.

### New APIs

#### `void GraphLayout::addBTWSymbolsForExistingBTWMarkers()`

Batch method to add magenta circles for all existing BTW markers in the data source. This is more efficient than processing markers individually.

**When to use:**
- When loading data that already contains BTW markers
- When you need to ensure all existing markers have corresponding magenta circles
- After restoring data from a file

**Example:**
```cpp
// After loading data with existing BTW markers
graphLayout->addBTWSymbolsForExistingBTWMarkers();
```

**Performance:** This method processes all markers in a single batch, performing only one redraw at the end instead of redrawing per marker.

---

#### `void SCWWindow::addBTWSymbolToAllGraphs(const QDateTime &timestamp)`

Adds a magenta circle to all SCW graphs at a specific timestamp.

**Parameters:**
- `timestamp`: The timestamp where the magenta circle should appear

**Example:**
```cpp
// Add magenta circle to all SCW graphs
scwWindow->addBTWSymbolToAllGraphs(QDateTime::currentDateTime());
```

**Note:** This method automatically finds the appropriate range value for each SCW graph based on data points at the timestamp, or uses the range center as a fallback.

---

#### Signal: `void GraphLayout::BTWSymbolAddedToAllGraphs(const QDateTime &timestamp)`

Signal emitted whenever a magenta circle is added to all graphs (both GraphLayout graphs and SCW graphs).

**Parameters:**
- `timestamp`: The timestamp where the magenta circle was added

**Usage:**
```cpp
connect(graphLayout, &GraphLayout::BTWSymbolAddedToAllGraphs,
        [](const QDateTime &timestamp) {
    qDebug() << "Magenta circle added at:" << timestamp;
});
```

---

### Automatic Integration

Magenta circles are automatically added when:

1. **Manual BTW markers are placed** (green markers):
   - User clicks on BTW graph → `BTWGraph::onMarkerAdded()`
   - Emits `manualMarkerPlaced` signal
   - `GraphLayout::onBTWManualMarkerPlaced()` handles it
   - Calls `addBTWSymbolToAllGraphs()` → adds magenta circles to all GraphLayout graphs
   - Emits `BTWSymbolAddedToAllGraphs` signal
   - `SCWWindow::addBTWSymbolToAllGraphs()` is called → adds magenta circles to SCW graphs

2. **Automatic BTW markers are added** (blue markers):
   - `GraphLayout::addBTWMarker()` is called
   - Calls `addBTWSymbolToAllGraphs()` → adds magenta circles to all GraphLayout graphs
   - Emits `BTWSymbolAddedToAllGraphs` signal
   - `SCWWindow::addBTWSymbolToAllGraphs()` is called → adds magenta circles to SCW graphs

### Integration in MainWindow

The signal connection is automatically set up in `MainWindow` constructor:

```cpp
// Connect BTW symbol signal to SCWWindow (if it exists)
connect(graphgrid, &GraphLayout::BTWSymbolAddedToAllGraphs,
        [this](const QDateTime &timestamp) {
    if (scwWindow && timestamp.isValid()) {
        scwWindow->addBTWSymbolToAllGraphs(timestamp);
    }
});
```

And also in `setupSCWWindow()`:

```cpp
// Connect BTW symbol signal to SCWWindow for magenta circles
if (graphgrid && scwWindow)
{
    connect(graphgrid, &GraphLayout::BTWSymbolAddedToAllGraphs,
            scwWindow, &SCWWindow::addBTWSymbolToAllGraphs, Qt::UniqueConnection);
}
```

### Manual Integration

If you're not using `MainWindow`, you need to connect the signal manually:

```cpp
// In your setup code
if (graphLayout && scwWindow)
{
    connect(graphLayout, &GraphLayout::BTWSymbolAddedToAllGraphs,
            scwWindow, &SCWWindow::addBTWSymbolToAllGraphs);
}
```

### Handling Existing Data

If you load data that already contains BTW markers but no magenta circles, call the batch method:

```cpp
// After loading data
graphLayout->addBTWSymbolsForExistingBTWMarkers();
```

This will efficiently add magenta circles for all existing markers.

---

## BTW Horizontal Line Delete Mode

### Overview

The BTW horizontal line mode has been enhanced to support three distinct modes:
- **Normal**: Default marker mode (clicking creates markers)
- **DrawLine**: Draw mode (clicking adds lines; clicking on existing line deletes it)
- **DeleteLine**: Delete-only mode (clicking only deletes lines; doesn't add new ones)

### API Changes

#### New Enum: `BTWGraph::HorizontalLineMode`

```cpp
enum class HorizontalLineMode {
    Normal,      // Normal marker mode (default)
    DrawLine,    // Draw lines mode: clicking adds lines, clicking on existing line deletes it
    DeleteLine   // Delete mode: clicking only deletes lines, doesn't add new ones
};
```

#### Updated Method: `void GraphLayout::setBTWHorizontalLineMode(const GraphType &graphType, BTWGraph::HorizontalLineMode mode)`

Sets the horizontal line mode using the new enum.

**Parameters:**
- `graphType`: The graph type (must be `GraphType::BTW`)
- `mode`: The mode to set (`Normal`, `DrawLine`, or `DeleteLine`)

**Example:**
```cpp
// Enable draw line mode
graphLayout->setBTWHorizontalLineMode(GraphType::BTW, BTWGraph::HorizontalLineMode::DrawLine);

// Enable delete-only mode
graphLayout->setBTWHorizontalLineMode(GraphType::BTW, BTWGraph::HorizontalLineMode::DeleteLine);

// Return to normal marker mode
graphLayout->setBTWHorizontalLineMode(GraphType::BTW, BTWGraph::HorizontalLineMode::Normal);
```

#### Legacy Method: `void GraphLayout::setBTWHorizontalLineMode(const GraphType &graphType, bool enabled)`

The boolean version is still available for backward compatibility:
- `true` → `DrawLine` mode
- `false` → `Normal` mode

**Example:**
```cpp
// Legacy boolean interface (still works)
graphLayout->setBTWHorizontalLineMode(GraphType::BTW, true);  // DrawLine mode
graphLayout->setBTWHorizontalLineMode(GraphType::BTW, false); // Normal mode
```

### Behavior by Mode

#### Normal Mode
- Clicking on graph creates manual markers (green markers)
- No horizontal line functionality

#### DrawLine Mode
- Clicking on empty space adds a new horizontal line
- Clicking on an existing horizontal line deletes it
- Provides both add and delete functionality in one mode

#### DeleteLine Mode
- Clicking on an existing horizontal line deletes it
- Clicking on empty space does nothing (no new lines added)
- Useful when you only want to remove lines without accidentally adding new ones

### Timestamp Access

Horizontal lines now provide timestamp information through multiple methods:

#### Signal: `void BTWGraph::horizontalLinePlaced(const QUuid &lineId, const QDateTime &timestamp)`

Signal emitted when a horizontal line is placed, providing both the line ID and timestamp.

**Parameters:**
- `lineId`: Unique identifier for the line
- `timestamp`: The timestamp where the line was placed

**Example:**
```cpp
// Connect to signal to get timestamp when line is added
connect(btwGraph, &BTWGraph::horizontalLinePlaced,
        [](const QUuid &lineId, const QDateTime &timestamp) {
    qDebug() << "Line added with ID:" << lineId << "at time:" << timestamp;
});
```

#### Method: `QDateTime BTWGraph::getHorizontalLineTimestamp(const QUuid &lineId) const`

Retrieves the timestamp of a horizontal line by its ID.

**Parameters:**
- `lineId`: The unique identifier of the line

**Returns:**
- `QDateTime`: The timestamp of the line, or invalid `QDateTime` if not found

**Example:**
```cpp
// Get timestamp from a line ID
QUuid lineId = btwGraph->addHorizontalLine(QDateTime::currentDateTime());
QDateTime timestamp = btwGraph->getHorizontalLineTimestamp(lineId);
if (timestamp.isValid()) {
    qDebug() << "Line timestamp:" << timestamp;
}
```

#### Method: `QDateTime GraphLayout::getBTWHorizontalLineTimestamp(const GraphType &graphType, const QUuid &lineId) const`

Wrapper method to get timestamp from any BTW graph in the layout.

**Parameters:**
- `graphType`: The graph type (must be `GraphType::BTW`)
- `lineId`: The unique identifier of the line

**Returns:**
- `QDateTime`: The timestamp of the line, or invalid `QDateTime` if not found

**Example:**
```cpp
// Get timestamp using GraphLayout API
QUuid lineId = graphLayout->addBTWHorizontalLine(GraphType::BTW, QDateTime::currentDateTime());
QDateTime timestamp = graphLayout->getBTWHorizontalLineTimestamp(GraphType::BTW, lineId);
if (timestamp.isValid()) {
    qDebug() << "Line timestamp:" << timestamp;
}
```

### Integration

No changes required for existing code using the boolean interface. The new enum-based interface is optional but recommended for clarity.

**Migration Example:**
```cpp
// Old code (still works)
graphLayout->setBTWHorizontalLineMode(GraphType::BTW, true);

// New code (recommended)
graphLayout->setBTWHorizontalLineMode(GraphType::BTW, BTWGraph::HorizontalLineMode::DrawLine);
```

---

## Clear Graphs Fix

### Overview

Fixed an issue where `clearAllGraphs()` was clearing symbols but not graph plots. The fix ensures that scatterplot items are properly cleaned up when data sources are empty.

### What Changed

All graph types now explicitly cleanup scatterplot items when the data source is empty:

- **BTWGraph**: Added cleanup in `draw()` method
- **FDWGraph**: Added cleanup in `draw()` method
- **FTWGraph**: Added cleanup in `draw()` method
- **BDWGraph**: Added cleanup in `draw()` method
- **BRWGraph**: Added cleanup in `draw()` method
- **LTWGraph**: Added cleanup in `draw()` method
- **RTWGraph**: Added cleanup in `draw()` method

### API Behavior

#### `void GraphLayout::clearAllGraphs()`

This method now properly clears:
- All data points (scatterplot items)
- All markers
- All symbols

**Example:**
```cpp
// Clear all graphs - now properly removes plots AND symbols
graphLayout->clearAllGraphs();
```

### No Integration Required

This is a bug fix with no API changes. Existing code will automatically benefit from the fix.

---

## Integration Examples

### Complete Example: Setting Up Magenta Circles for SCW

```cpp
#include "graphlayout.h"
#include "scwwindow.h"

class MyApplication : public QWidget
{
    Q_OBJECT

public:
    MyApplication(QWidget *parent = nullptr) : QWidget(parent)
    {
        // Create GraphLayout
        graphLayout = new GraphLayout(this, LayoutType::GPW4W, timer, seriesLabelsMap);
        
        // Create SCWWindow
        scwWindow = new SCWWindow(this, timer, graphLayout->getSyncState());
        
        // Connect magenta circle signal
        connect(graphLayout, &GraphLayout::BTWSymbolAddedToAllGraphs,
                scwWindow, &SCWWindow::addBTWSymbolToAllGraphs);
        
        // If loading existing data with BTW markers, add magenta circles
        // graphLayout->addBTWSymbolsForExistingBTWMarkers();
    }

private:
    GraphLayout *graphLayout;
    SCWWindow *scwWindow;
    QTimer *timer;
};
```

### Example: Using Horizontal Line Delete Mode

```cpp
// Enable delete-only mode for BTW graphs
graphLayout->setBTWHorizontalLineMode(GraphType::BTW, BTWGraph::HorizontalLineMode::DeleteLine);

// User can now only delete lines, not add new ones
// Switch back to draw mode when needed
graphLayout->setBTWHorizontalLineMode(GraphType::BTW, BTWGraph::HorizontalLineMode::DrawLine);
```

### Example: Accessing Horizontal Line Timestamps

```cpp
// Method 1: Using the signal (recommended for real-time updates)
connect(btwGraph, &BTWGraph::horizontalLinePlaced,
        [](const QUuid &lineId, const QDateTime &timestamp) {
    qDebug() << "New line at:" << timestamp.toString();
    // Store lineId and timestamp for later use
});

// Method 2: Using the getter method (for existing lines)
QUuid lineId = graphLayout->addBTWHorizontalLine(GraphType::BTW, QDateTime::currentDateTime());
QDateTime timestamp = graphLayout->getBTWHorizontalLineTimestamp(GraphType::BTW, lineId);
if (timestamp.isValid()) {
    // Use the timestamp
    processLineTimestamp(timestamp);
}
```

### Example: Handling Data Loading with Existing Markers

```cpp
void MyClass::loadDataFromFile(const QString &filename)
{
    // Load data (may contain BTW markers)
    loadData(filename);
    
    // Ensure all existing BTW markers have magenta circles
    graphLayout->addBTWSymbolsForExistingBTWMarkers();
}
```

---

## Summary of Changes

### New Methods
- `GraphLayout::addBTWSymbolsForExistingBTWMarkers()` - Batch add magenta circles for existing markers
- `SCWWindow::addBTWSymbolToAllGraphs(const QDateTime &timestamp)` - Add magenta circles to SCW graphs
- `BTWGraph::getHorizontalLineTimestamp(const QUuid &lineId)` - Get timestamp of a horizontal line by ID
- `GraphLayout::getBTWHorizontalLineTimestamp(const GraphType &graphType, const QUuid &lineId)` - Get timestamp from any BTW graph

### New Signals
- `GraphLayout::BTWSymbolAddedToAllGraphs(const QDateTime &timestamp)` - Emitted when magenta circles are added
- `BTWGraph::horizontalLinePlaced(const QUuid &lineId, const QDateTime &timestamp)` - Emitted when a horizontal line is placed (includes timestamp)

### Enhanced Methods
- `GraphLayout::setBTWHorizontalLineMode()` - Now supports enum-based mode selection

### Bug Fixes
- `GraphLayout::clearAllGraphs()` - Now properly clears scatterplot items

### Backward Compatibility
- All existing boolean-based APIs still work
- No breaking changes to existing code

---

## Migration Checklist

- [ ] If using SCW graphs, connect `BTWSymbolAddedToAllGraphs` signal to `SCWWindow::addBTWSymbolToAllGraphs`
- [ ] If loading data with existing BTW markers, call `addBTWSymbolsForExistingBTWMarkers()` after loading
- [ ] (Optional) Migrate from boolean to enum-based horizontal line mode API
- [ ] Test that `clearAllGraphs()` properly clears both plots and symbols

---

## Related Documentation

- [GraphLayout API Documentation](GRAPHLAYOUT_API.md)
- [BTW Horizontal Line API Documentation](BTW_HORIZONTAL_LINE_API.md)
- [SCW Window Documentation](scwwindow.md)
- [Overlay System Analysis](OVERLAY_SYSTEM_ANALYSIS.md)

