# GraphLayout API Documentation

This document provides comprehensive documentation for the `GraphLayout` class APIs, focusing on marker interaction, data management, and signal handling.

---

## Table of Contents

1. [Marker Click Events](#marker-click-events)
2. [Marker Management](#marker-management)
3. [Symbol Management](#symbol-management)
4. [Shaded Region API](#shaded-region-api)
5. [Data Source Management](#data-source-management)
6. [Container Management](#container-management)
7. [Manoeuvre Management](#manoeuvre-management)
8. [Layout Management](#layout-management)

---

## Marker Click Events

### `markerClickedWithData` Signal

**Signature:**
```cpp
void markerClickedWithData(const QDateTime &timestamp, qreal rangeValue, qreal bearingRate);
```

**Description:**
Emitted when a BTW marker is clicked. Provides comprehensive marker data for external integration.

**Parameters:**
| Parameter | Type | Description |
|-----------|------|-------------|
| `timestamp` | `QDateTime` | The timestamp when the marker is positioned in time (Y-axis) |
| `rangeValue` | `qreal` | The X-axis range value (horizontal position) |
| `bearingRate` | `qreal` | The bearing rate value shown in the box (rotation angle / 10) |

**Usage Example:**
```cpp
// Connect to the signal
connect(graphLayout, &GraphLayout::markerClickedWithData,
        this, [](const QDateTime &timestamp, qreal rangeValue, qreal bearingRate) {
    qDebug() << "BTW Marker Clicked:";
    qDebug() << "  Timestamp:" << timestamp.toString("yyyy-MM-dd hh:mm:ss.zzz");
    qDebug() << "  Range Value:" << rangeValue;
    qDebug() << "  Bearing Rate:" << bearingRate;
});
```

---

### `markerTimestampValueChanged` Signal

**Signature:**
```cpp
void markerTimestampValueChanged(const QDateTime &timestamp, qreal value);
```

**Description:**
Emitted when a marker timestamp and value change (new marker placed or marker clicked).

**Parameters:**
| Parameter | Type | Description |
|-----------|------|-------------|
| `timestamp` | `QDateTime` | The timestamp of the marker |
| `value` | `qreal` | The range value of the marker |

---

### `BTWManualMarkerPlaced` Signal

**Signature:**
```cpp
void BTWManualMarkerPlaced(const QDateTime &timestamp, const QPointF &position);
```

**Description:**
Emitted when a BTW manual marker is placed on the graph.

**Parameters:**
| Parameter | Type | Description |
|-----------|------|-------------|
| `timestamp` | `QDateTime` | The timestamp of the placed marker |
| `position` | `QPointF` | The scene position where the marker was placed |

---

### `BTWManualMarkerClicked` Signal

**Signature:**
```cpp
void BTWManualMarkerClicked(const QDateTime &timestamp, const QPointF &position);
```

**Description:**
Emitted when a BTW manual marker is clicked.

**Parameters:**
| Parameter | Type | Description |
|-----------|------|-------------|
| `timestamp` | `QDateTime` | The timestamp of the clicked marker |
| `position` | `QPointF` | The scene position where the marker was clicked |

---

### `RTWRMarkerTimestampCaptured` Signal

**Signature:**
```cpp
void RTWRMarkerTimestampCaptured(const QDateTime &timestamp, const QPointF &position);
```

**Description:**
Emitted when an RTW R marker is clicked.

---

### `RTWSymbolTimestampCaptured` Signal

**Signature:**
```cpp
void RTWSymbolTimestampCaptured(const QDateTime &timestamp, const QPointF &position, const QString &symbolName);
```

**Description:**
Emitted when an RTW symbol is clicked.

---

## Marker Management

### `addBTWMarker`

**Signature:**
```cpp
void addBTWMarker(const GraphType &graphType, const QDateTime &timestamp, qreal range, qreal delta);
```

**Description:**
Adds a BTW marker (circle with angled line) to the specified graph type's data source.

**Parameters:**
| Parameter | Type | Description |
|-----------|------|-------------|
| `graphType` | `GraphType` | The graph type to add the marker to (e.g., `GraphType::BTW`) |
| `timestamp` | `QDateTime` | The timestamp for the marker (Y-axis position) |
| `range` | `qreal` | The range value (X-axis position) |
| `delta` | `qreal` | The delta/bearing rate value (determines line angle) |

**Usage Example:**
```cpp
graphLayout->addBTWMarker(GraphType::BTW, 
                          QDateTime::currentDateTime(), 
                          15.5,   // range value
                          2.3);   // bearing rate (delta)
```

---

### `removeBTWMarker`

**Signature:**
```cpp
bool removeBTWMarker(const GraphType &graphType, const QDateTime &timestamp, qreal range, 
                     qreal toleranceMs = 1000, qreal rangeTolerance = 0.1);
```

**Description:**
Removes a BTW marker from the specified graph type's data source.

**Returns:** `true` if marker was found and removed, `false` otherwise.

---

### `clearBTWMarkers`

**Signature:**
```cpp
void clearBTWMarkers(const GraphType &graphType);
```

**Description:**
Clears all BTW markers from the specified graph type.

---

### `clearBTWManualMarkers`

**Signature:**
```cpp
void clearBTWManualMarkers();
```

**Description:**
Clears all BTW manual/interactive overlay markers from all BTW graphs.

---

### `deleteInteractiveMarkers`

**Signature:**
```cpp
void deleteInteractiveMarkers();
```

**Description:**
Deletes all interactive markers from all containers.

---

### `addRTWRMarker`

**Signature:**
```cpp
void addRTWRMarker(const GraphType &graphType, const QDateTime &timestamp, qreal range);
```

**Description:**
Adds an RTW R marker to the specified graph type.

---

### `removeRTWRMarker`

**Signature:**
```cpp
bool removeRTWRMarker(const GraphType &graphType, const QDateTime &timestamp, qreal range, 
                      qreal toleranceMs = 1000, qreal rangeTolerance = 0.1);
```

**Description:**
Removes an RTW R marker from the specified graph type.

---

### `clearRTWRMarkers`

**Signature:**
```cpp
void clearRTWRMarkers(const GraphType &graphType);
```

**Description:**
Clears all RTW R markers from the specified graph type.

---

## Symbol Management

### `addRTWSymbol`

**Signature:**
```cpp
void addRTWSymbol(const GraphType &graphType, const QString &symbolName, 
                  const QDateTime &timestamp, qreal range);
```

**Description:**
Adds an RTW symbol to the specified graph type.

**Parameters:**
| Parameter | Type | Description |
|-----------|------|-------------|
| `graphType` | `GraphType` | Target graph type |
| `symbolName` | `QString` | Name of the symbol (e.g., "MagentaCircle") |
| `timestamp` | `QDateTime` | Timestamp for symbol placement |
| `range` | `qreal` | Range value for symbol placement |

---

### `addBTWSymbol`

**Signature:**
```cpp
void addBTWSymbol(const GraphType &graphType, const QString &symbolName, 
                  const QDateTime &timestamp, qreal range);
```

**Description:**
Adds a BTW symbol (magenta circle) to the specified graph type.

---

### `removeRTWSymbol`

**Signature:**
```cpp
bool removeRTWSymbol(const GraphType &graphType, const QString &symbolName, 
                     const QDateTime &timestamp, qreal range, 
                     qreal toleranceMs = 1000, qreal rangeTolerance = 0.1);
```

**Description:**
Removes an RTW symbol from the specified graph type.

---

### `clearRTWSymbols` / `clearBTWSymbols`

**Signature:**
```cpp
void clearRTWSymbols(const GraphType &graphType);
void clearBTWSymbols(const GraphType &graphType);
```

**Description:**
Clears all RTW or BTW symbols from the specified graph type.

---

## Shaded Region API

### `addShadedRegionToAllBTW`

**Signature:**
```cpp
QUuid addShadedRegionToAllBTW(qreal startX, qreal endX);
```

**Description:**
Adds a shaded region to all BTW graphs. The region is drawn as a cross-hatched vertical band spanning from top to bottom.

**Parameters:**
| Parameter | Type | Description |
|-----------|------|-------------|
| `startX` | `qreal` | Starting X value (left range boundary) |
| `endX` | `qreal` | Ending X value (right range boundary) |

**Returns:** `QUuid` - The sync ID of the created region (used for removal).

**Usage Example:**
```cpp
QUuid regionId = graphLayout->addShadedRegionToAllBTW(10.0, 20.0);
// Later, to remove:
graphLayout->removeShadedRegionFromAllBTW(regionId);
```

---

### `removeShadedRegionFromAllBTW`

**Signature:**
```cpp
bool removeShadedRegionFromAllBTW(const QUuid &syncId);
```

**Description:**
Removes a shaded region from all BTW graphs by its sync ID.

**Returns:** `true` if region was found and removed.

---

### `clearAllShadedRegions`

**Signature:**
```cpp
void clearAllShadedRegions();
```

**Description:**
Clears all shaded regions from all BTW graphs.

---

### `getAllShadedRegions`

**Signature:**
```cpp
std::vector<ShadedRegionSyncData> getAllShadedRegions() const;
```

**Description:**
Gets all active shaded regions.

---

## Data Source Management

### `addDataOption`

**Signature:**
```cpp
// For specific container
void addDataOption(const QString &containerLabel, const GraphType &graphType, WaterfallData &dataSource);

// For all visible containers
void addDataOption(const GraphType &graphType, WaterfallData &dataSource);
```

**Description:**
Adds a data option (graph type with data source) to containers.

---

### `setDataToDataSource`

**Signature:**
```cpp
void setDataToDataSource(const GraphType &graphType, const QString &seriesLabel, 
                         const std::vector<qreal> &yData, const std::vector<QDateTime> &timestamps);
```

**Description:**
Sets data for a specific series in a graph type's data source.

---

### `addDataPointToDataSource`

**Signature:**
```cpp
void addDataPointToDataSource(const GraphType &graphType, const QString &seriesLabel, 
                              qreal yValue, const QDateTime &timestamp);
```

**Description:**
Adds a single data point to a series.

---

### `addDataPointsToDataSource`

**Signature:**
```cpp
void addDataPointsToDataSource(const GraphType &graphType, const QString &seriesLabel, 
                               const std::vector<qreal> &yValues, const std::vector<QDateTime> &timestamps);
```

**Description:**
Adds multiple data points to a series.

---

### `getDataSource`

**Signature:**
```cpp
WaterfallData* getDataSource(const GraphType &graphType);
```

**Description:**
Gets the data source for a specific graph type.

**Returns:** Pointer to `WaterfallData`, or `nullptr` if not found.

---

### `clearDataSource`

**Signature:**
```cpp
void clearDataSource(const GraphType &graphType, const QString &seriesLabel);
```

**Description:**
Clears data from a specific series in a graph type.

---

### `clearAllGraphs`

**Signature:**
```cpp
void clearAllGraphs();
```

**Description:**
Clears all data, markers, and symbols from all graphs.

---

## Container Management

### `getContainerLabels`

**Signature:**
```cpp
std::vector<QString> getContainerLabels() const;
```

**Description:**
Gets the labels of all containers in the layout.

---

### `setCurrentDataOption`

**Signature:**
```cpp
// For specific container
void setCurrentDataOption(const QString &containerLabel, const GraphType &graphType);

// For all containers
void setCurrentDataOption(const GraphType &graphType);
```

**Description:**
Sets the currently displayed graph type for container(s).

---

### `getCurrentDataOption`

**Signature:**
```cpp
GraphType getCurrentDataOption(const QString &containerLabel) const;
```

**Description:**
Gets the current graph type being displayed in a container.

---

## Manoeuvre Management

### `addManoeuvre`

**Signature:**
```cpp
void addManoeuvre(const Manoeuvre &manoeuvre);
```

**Description:**
Adds a manoeuvre overlay to all graphs.

---

### `startManoeuvreDrawing` / `endManoeuvreDrawing`

**Signature:**
```cpp
void startManoeuvreDrawing(const QDateTime &startTime, int bearing, int speed, int depth);
void endManoeuvreDrawing(const QDateTime &endTime);
```

**Description:**
Begins/ends a manoeuvre drawing session.

---

### `setManoeuvres`

**Signature:**
```cpp
void setManoeuvres(const std::vector<Manoeuvre> &manoeuvres);
```

**Description:**
Sets all manoeuvres at once.

---

### `clearManoeuvres`

**Signature:**
```cpp
void clearManoeuvres();
```

**Description:**
Clears all manoeuvres from all graphs.

---

## Layout Management

### `setLayoutType`

**Signature:**
```cpp
void setLayoutType(LayoutType layoutType);
```

**Description:**
Sets the layout arrangement for graph containers.

**Layout Types:**
| Type | Description |
|------|-------------|
| `GPW1W` | 1 window only |
| `GPW4W` | 4 windows in 2x2 grid |
| `GPW2WV` | 2 windows in vertical line |
| `GPW2WH` | 2 windows in horizontal line |
| `GPW4WH` | 4 windows in horizontal line |
| `NOGPW2WH` | 2 windows in horizontal line (full screen) |
| `HIDDEN` | Hidden layout |

---

### `redrawGraph` / `redrawAllGraphs`

**Signature:**
```cpp
void redrawGraph(const GraphType &graphType);
void redrawAllGraphs();
```

**Description:**
Forces a redraw of specific or all graphs.

---

### `setHardRangeLimits`

**Signature:**
```cpp
void setHardRangeLimits(const GraphType graphType, qreal yMin, qreal yMax);
```

**Description:**
Sets hard range limits for a graph type's Y-axis.

---

### `removeHardRangeLimits`

**Signature:**
```cpp
void removeHardRangeLimits(const GraphType graphType);
```

**Description:**
Removes hard range limits for a graph type.

---

## Complete Integration Example

```cpp
// Create GraphLayout
GraphLayout *layout = new GraphLayout(parentWidget, LayoutType::GPW2WH);

// Connect to marker click signal
connect(layout, &GraphLayout::markerClickedWithData,
        this, [](const QDateTime &timestamp, qreal rangeValue, qreal bearingRate) {
    // Handle marker click - get timestamp, range, and bearing rate
    qDebug() << "Marker clicked at:" << timestamp;
    qDebug() << "Range:" << rangeValue << "Bearing Rate:" << bearingRate;
    
    // Use data for external integration
    externalSystem->updateWithMarkerData(timestamp, rangeValue, bearingRate);
});

// Connect to marker placement signal
connect(layout, &GraphLayout::BTWManualMarkerPlaced,
        this, [](const QDateTime &timestamp, const QPointF &position) {
    qDebug() << "New marker placed at timestamp:" << timestamp;
});

// Add data to BTW graph
layout->addDataPointToDataSource(GraphType::BTW, "SeriesA", 15.5, QDateTime::currentDateTime());

// Add a BTW marker programmatically
layout->addBTWMarker(GraphType::BTW, QDateTime::currentDateTime(), 15.5, 2.0);

// Add shaded region
QUuid regionId = layout->addShadedRegionToAllBTW(10.0, 25.0);

// Later, clear everything
layout->clearAllGraphs();
```

---

## Signal Flow Diagram

```
User clicks BTW Marker
        │
        ▼
InteractiveGraphicsItem::regionClicked
        │
        ▼
BTWInteractiveOverlay::markerClicked
        │
        ▼
BTWGraph::onMarkerClicked
        │
        ├──► BTWGraph::markerClickedWithData(timestamp, range, bearingRate)
        │           │
        │           ▼
        │    GraphContainer::markerClickedWithData
        │           │
        │           ▼
        │    GraphLayout::markerClickedWithData  ◄── CONNECT HERE
        │
        └──► BTWGraph::markerTimestampValueChanged(timestamp, value)
                    │
                    ▼
             GraphLayout::markerTimestampValueChanged
```

---

## See Also

- [BTW_SHADED_REGION_API.md](BTW_SHADED_REGION_API.md) - Detailed shaded region documentation
- [MANOEUVRE_API_DOCUMENTATION.md](MANOEUVRE_API_DOCUMENTATION.md) - Manoeuvre system documentation
- [marker-timestamp-event.md](marker-timestamp-event.md) - Additional marker event documentation

