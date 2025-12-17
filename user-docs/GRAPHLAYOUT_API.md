# GraphLayout API Documentation

This document lists all APIs available from `GraphLayout` for:
- History Selection (TimeSelectionVisualizer)
- Markers and Symbols
- BTW Marker Timestamp Events

---

## History Selection APIs

### Time Selection Signals

**Note:** GraphLayout does not have direct public methods for adding or clearing time selections. These operations are handled through signals and slots. To programmatically manage time selections, you can:
1. Connect to the signals to be notified of user-created selections
2. Access individual `GraphContainer` instances (if needed) to programmatically add selections
3. Use the public slots to handle selection events

---

#### `void TimeSelectionCreated(const TimeSelectionSpan &selection)`
Signal emitted when a time selection is created by user interaction.

**Parameters:**
- `selection`: The created time selection span

**Usage:**
```cpp
connect(graphLayout, &GraphLayout::TimeSelectionCreated, 
        this, &MyClass::onTimeSelectionCreated);
```

---

#### `void TimeSelectionsCleared()`
Signal emitted when all time selections are cleared.

**Usage:**
```cpp
connect(graphLayout, &GraphLayout::TimeSelectionsCleared, 
        this, &MyClass::onTimeSelectionsCleared);
```

---

## Marker and Symbol APIs

### Adding Markers and Symbols

#### `void addRTWSymbol(const GraphType &graphType, const QString &symbolName, const QDateTime &timestamp, qreal range)`
Adds an RTW (Radar Time Waterfall) symbol to a specific graph type.

**Parameters:**
- `graphType`: The graph type (e.g., `GraphType::RTW`)
- `symbolName`: Name of the symbol to display
- `timestamp`: Timestamp when the symbol should appear
- `range`: Range value (Y-axis position) for the symbol

**Example:**
```cpp
graphLayout->addRTWSymbol(GraphType::RTW, "Triangle", QDateTime::currentDateTime(), 50.0);
```

---

#### `void addBTWSymbol(const GraphType &graphType, const QString &symbolName, const QDateTime &timestamp, qreal range)`
Adds a BTW (Bit Time Waterfall) symbol to a specific graph type.

**Parameters:**
- `graphType`: The graph type (e.g., `GraphType::BTW`)
- `symbolName`: Name of the symbol to display
- `timestamp`: Timestamp when the symbol should appear
- `range`: Range value (Y-axis position) for the symbol

**Example:**
```cpp
graphLayout->addBTWSymbol(GraphType::BTW, "MagentaCircle", QDateTime::currentDateTime(), 75.0);
```

---

#### `void addBTWMarker(const GraphType &graphType, const QDateTime &timestamp, qreal range, qreal delta)`
Adds a BTW marker to a specific graph type. Automatically adds a magenta circle symbol to all other graphs at the same timestamp.

**Parameters:**
- `graphType`: The graph type (e.g., `GraphType::BTW`)
- `timestamp`: Timestamp when the marker should appear
- `range`: Range value (Y-axis position) for the marker
- `delta`: Delta value for the marker

**Note:** This method automatically propagates a magenta circle symbol to all other graph types at the same timestamp.

**Example:**
```cpp
graphLayout->addBTWMarker(GraphType::BTW, QDateTime::currentDateTime(), 50.0, 5.0);
```

---

#### `void addRTWRMarker(const GraphType &graphType, const QDateTime &timestamp, qreal range)`
Adds an RTW R marker to a specific graph type.

**Parameters:**
- `graphType`: The graph type (e.g., `GraphType::RTW`)
- `timestamp`: Timestamp when the marker should appear
- `range`: Range value (Y-axis position) for the marker

**Example:**
```cpp
graphLayout->addRTWRMarker(GraphType::RTW, QDateTime::currentDateTime(), 60.0);
```

---

### Removing Markers and Symbols

#### `bool removeRTWSymbol(const GraphType &graphType, const QString &symbolName, const QDateTime &timestamp, qreal range, qreal toleranceMs = 1000, qreal rangeTolerance = 0.1)`
Removes an RTW symbol matching the specified criteria.

**Parameters:**
- `graphType`: The graph type
- `symbolName`: Name of the symbol to remove
- `timestamp`: Timestamp of the symbol (used for matching)
- `range`: Range value of the symbol (used for matching)
- `toleranceMs`: Time tolerance in milliseconds (default: 1000ms)
- `rangeTolerance`: Range tolerance value (default: 0.1)

**Returns:** `true` if the symbol was found and removed, `false` otherwise

**Example:**
```cpp
bool removed = graphLayout->removeRTWSymbol(GraphType::RTW, "Triangle", 
                                            timestamp, 50.0, 500, 0.05);
```

---

#### `bool removeBTWMarker(const GraphType &graphType, const QDateTime &timestamp, qreal range, qreal toleranceMs = 1000, qreal rangeTolerance = 0.1)`
Removes a BTW marker matching the specified criteria.

**Parameters:**
- `graphType`: The graph type
- `timestamp`: Timestamp of the marker (used for matching)
- `range`: Range value of the marker (used for matching)
- `toleranceMs`: Time tolerance in milliseconds (default: 1000ms)
- `rangeTolerance`: Range tolerance value (default: 0.1)

**Returns:** `true` if the marker was found and removed, `false` otherwise

**Example:**
```cpp
bool removed = graphLayout->removeBTWMarker(GraphType::BTW, timestamp, 50.0);
```

---

#### `bool removeRTWRMarker(const GraphType &graphType, const QDateTime &timestamp, qreal range, qreal toleranceMs = 1000, qreal rangeTolerance = 0.1)`
Removes an RTW R marker matching the specified criteria.

**Parameters:**
- `graphType`: The graph type
- `timestamp`: Timestamp of the marker (used for matching)
- `range`: Range value of the marker (used for matching)
- `toleranceMs`: Time tolerance in milliseconds (default: 1000ms)
- `rangeTolerance`: Range tolerance value (default: 0.1)

**Returns:** `true` if the marker was found and removed, `false` otherwise

**Example:**
```cpp
bool removed = graphLayout->removeRTWRMarker(GraphType::RTW, timestamp, 60.0);
```

---

### Clearing Markers and Symbols

#### `void clearRTWSymbols(const GraphType &graphType)`
Clears all RTW symbols from a specific graph type.

**Parameters:**
- `graphType`: The graph type to clear symbols from

**Example:**
```cpp
graphLayout->clearRTWSymbols(GraphType::RTW);
```

---

#### `void clearBTWSymbols(const GraphType &graphType)`
Clears all BTW symbols from a specific graph type.

**Parameters:**
- `graphType`: The graph type to clear symbols from

**Example:**
```cpp
graphLayout->clearBTWSymbols(GraphType::BTW);
```

---

#### `void clearBTWMarkers(const GraphType &graphType)`
Clears all BTW markers from a specific graph type.

**Parameters:**
- `graphType`: The graph type to clear markers from

**Example:**
```cpp
graphLayout->clearBTWMarkers(GraphType::BTW);
```

---

#### `void clearRTWRMarkers(const GraphType &graphType)`
Clears all RTW R markers from a specific graph type.

**Parameters:**
- `graphType`: The graph type to clear markers from

**Example:**
```cpp
graphLayout->clearRTWRMarkers(GraphType::RTW);
```

---

#### `void clearBTWManualMarkers()`
Clears all BTW manual markers (interactive overlay markers) from all BTW graphs.

**Note:** This clears only the interactive/manual markers placed by user interaction, not programmatically added markers.

**Example:**
```cpp
graphLayout->clearBTWManualMarkers();
```

---

#### `void deleteInteractiveMarkers()`
Deletes all interactive markers from all containers.

**Note:** This is a general method that clears interactive markers across all graph types.

**Example:**
```cpp
graphLayout->deleteInteractiveMarkers();
```

---

#### `void clearAllGraphs()`
Clears all data, markers, and symbols from all graphs.

**Note:** This is a comprehensive clear operation that removes everything from all graph types.

**Example:**
```cpp
graphLayout->clearAllGraphs();
```

---

### Redraw Methods

#### `void redrawGraph(const GraphType &graphType)`
Triggers a redraw of a specific graph type across all containers.

**Parameters:**
- `graphType`: The graph type to redraw

**Example:**
```cpp
graphLayout->redrawGraph(GraphType::BTW);
```

---

#### `void redrawAllGraphs()`
Triggers a redraw of all graphs in all containers.

**Example:**
```cpp
graphLayout->redrawAllGraphs();
```

---

## BTW Marker Timestamp Signals

### Marker Event Signals

#### `void RTWRMarkerTimestampCaptured(const QDateTime &timestamp, const QPointF &position)`
Signal emitted when an RTW R marker is clicked by the user.

**Parameters:**
- `timestamp`: The timestamp of the clicked R marker
- `position`: The scene position (QPointF) where the marker was clicked

**Usage:**
```cpp
connect(graphLayout, &GraphLayout::RTWRMarkerTimestampCaptured,
        this, [](const QDateTime &timestamp, const QPointF &position) {
    qDebug() << "RTW R marker clicked at:" << timestamp << "position:" << position;
});
```

---

#### `void BTWManualMarkerPlaced(const QDateTime &timestamp, const QPointF &position)`
Signal emitted when a BTW manual marker is placed by user interaction.

**Parameters:**
- `timestamp`: The timestamp of the placed marker
- `position`: The scene position (QPointF) where the marker was placed

**Usage:**
```cpp
connect(graphLayout, &GraphLayout::BTWManualMarkerPlaced,
        this, [](const QDateTime &timestamp, const QPointF &position) {
    qDebug() << "BTW manual marker placed at:" << timestamp << "position:" << position;
});
```

---

#### `void BTWManualMarkerClicked(const QDateTime &timestamp, const QPointF &position)`
Signal emitted when a BTW manual marker is clicked by the user.

**Parameters:**
- `timestamp`: The timestamp of the clicked marker
- `position`: The scene position (QPointF) where the marker was clicked

**Usage:**
```cpp
connect(graphLayout, &GraphLayout::BTWManualMarkerClicked,
        this, [](const QDateTime &timestamp, const QPointF &position) {
    qDebug() << "BTW manual marker clicked at:" << timestamp << "position:" << position;
});
```

---

#### `void markerTimestampValueChanged(const QDateTime &timestamp, qreal value)`
Signal emitted when a marker timestamp and value change (either from a new marker being placed or an existing marker being clicked).

**Parameters:**
- `timestamp`: The timestamp of the marker
- `value`: The value (range) of the marker

**Usage:**
```cpp
connect(graphLayout, &GraphLayout::markerTimestampValueChanged,
        this, [](const QDateTime &timestamp, qreal value) {
    qDebug() << "Marker timestamp/value changed:" << timestamp << "value:" << value;
});
```

---

## Public Slots

### `void onTimeSelectionCreated(const TimeSelectionSpan &selection)`
Public slot that can be connected to handle time selection creation events.

**Parameters:**
- `selection`: The created time selection span

---

### `void onTimeSelectionsCleared()`
Public slot that can be connected to handle time selection clearing events.

---

### `void onBTWManualMarkerPlaced(const QDateTime &timestamp, const QPointF &position)`
Public slot that can be connected to handle BTW manual marker placement events.

**Parameters:**
- `timestamp`: The timestamp of the placed marker
- `position`: The scene position where the marker was placed

---

## Notes

1. **Graph Types:** All marker and symbol methods require a `GraphType` parameter. Common graph types include:
   - `GraphType::BTW` - Bit Time Waterfall
   - `GraphType::RTW` - Radar Time Waterfall
   - `GraphType::BDW` - Bit Depth Waterfall
   - `GraphType::BRW` - Bit Range Waterfall
   - `GraphType::FDW` - Frequency Depth Waterfall
   - `GraphType::FTW` - Frequency Time Waterfall
   - `GraphType::LTW` - Linear Time Waterfall

2. **Time Selection Span:** The `TimeSelectionSpan` structure contains:
   - `startTime`: QDateTime for the start of the selection
   - `endTime`: QDateTime for the end of the selection

3. **Automatic Symbol Propagation:** When `addBTWMarker()` is called, it automatically adds a magenta circle symbol to all other graph types at the same timestamp.

4. **Tolerance Matching:** Remove methods use tolerance values to match markers/symbols. The default time tolerance is 1000ms (1 second) and range tolerance is 0.1.

5. **Signal Connections:** All signals can be connected using Qt's signal-slot mechanism. Use `QObject::connect()` to subscribe to these events.

---

## Example Usage

```cpp
// Create a GraphLayout
GraphLayout* graphLayout = new GraphLayout(parent, LayoutType::GPW4W, timer, seriesLabelsMap);

// Connect to marker timestamp signals
connect(graphLayout, &GraphLayout::BTWManualMarkerPlaced,
        this, [](const QDateTime &timestamp, const QPointF &position) {
    qDebug() << "BTW marker placed at:" << timestamp;
});

connect(graphLayout, &GraphLayout::markerTimestampValueChanged,
        this, [](const QDateTime &timestamp, qreal value) {
    qDebug() << "Marker changed - timestamp:" << timestamp << "value:" << value;
});

// Add a BTW marker (automatically adds symbol to all graphs)
graphLayout->addBTWMarker(GraphType::BTW, QDateTime::currentDateTime(), 50.0, 5.0);

// Add a time selection
TimeSelectionSpan selection(startTime, endTime);
graphLayout->addTimeSelection(selection);

// Clear all markers
graphLayout->clearBTWMarkers(GraphType::BTW);
graphLayout->clearBTWManualMarkers();
```

