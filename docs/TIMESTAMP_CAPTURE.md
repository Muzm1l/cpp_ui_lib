# Timestamp Capture Guide

This document explains how to capture timestamps when clicking on RTW R markers and BTW manual markers in the graph system.

## Overview

The system provides timestamp capture functionality for two types of markers:

1. **RTW R Markers**: Automatic yellow "R" markers displayed on RTW graphs
2. **BTW Manual Markers**: Interactive markers that users can place by clicking on BTW graphs

Both marker types use the same underlying mechanism: converting the marker's Y position (screen coordinates) to a timestamp using the graph's time mapping function.

---

## RTW R Marker Timestamp Capture

### How It Works

RTW R markers are automatic markers drawn on the RTW graph. When a user clicks on an R marker:

1. The click is detected in `RTWGraph::onMouseClick()`
2. The system checks if the clicked item is a `QGraphicsTextItem` with text "R"
3. The Y position of the click is extracted
4. The Y position is converted to a timestamp using `mapScreenToTime(yPos)`
5. The timestamp is logged to the console

### Code Location

**File**: `rtwgraph.cpp`  
**Function**: `RTWGraph::onMouseClick(const QPointF &scenePos)`  
**Lines**: 115-151

### Implementation Details

```cpp
void RTWGraph::onMouseClick(const QPointF &scenePos)
{
    // Check if we clicked on an R marker (QGraphicsTextItem with text "R") in graphicsScene
    if (graphicsScene) {
        QGraphicsItem *itemAtPos = graphicsScene->itemAt(scenePos, QTransform());
        if (itemAtPos) {
            QGraphicsTextItem *textItem = qgraphicsitem_cast<QGraphicsTextItem*>(itemAtPos);
            if (textItem && textItem->toPlainText() == "R") {
                // This is an R marker - calculate timestamp from Y position
                qreal yPos = scenePos.y();
                QDateTime timestamp = mapScreenToTime(yPos);
                
                if (timestamp.isValid()) {
                    // Timestamp captured successfully
                    qDebug() << "RTW R MARKER SELECTED - TIMESTAMP RETURNED";
                    qDebug() << "RTWGraph: TIMESTAMP:" << timestamp.toString("yyyy-MM-dd hh:mm:ss.zzz");
                }
                return; // Don't call parent - we've handled the R marker click
            }
        }
    }
    
    // Call parent implementation for other clicks
    WaterfallGraph::onMouseClick(scenePos);
}
```

### Console Output

When an RTW R marker is clicked, you'll see:

```
RTWGraph mouse clicked at scene position: QPointF(x, y)
RTWGraph: itemAtPos: <pointer> at scene position: QPointF(x, y)
RTWGraph: textItem: <pointer>
RTWGraph: Text item text: "R"
========================================
RTW R MARKER SELECTED - TIMESTAMP RETURNED
========================================
RTWGraph: R marker clicked at scene position: QPointF(x, y)
RTWGraph: Marker Y position: <y_value>
RTWGraph: TIMESTAMP: 2025-11-27 11:55:26.000
========================================
```

---

## BTW Manual Marker Timestamp Capture

### How It Works

BTW manual markers are interactive markers that users can place by clicking on empty space in a BTW graph. When a user clicks on an existing manual marker:

1. The click is detected by the `BTWInteractiveOverlay` system
2. The `markerClicked` signal is emitted
3. `BTWGraph::onMarkerClicked()` slot receives the signal
4. The marker's scene position is extracted
5. The Y position is converted to a timestamp using `mapScreenToTime(yPos)`
6. The timestamp is logged to the console

### Code Location

**File**: `btwgraph.cpp`  
**Function**: `BTWGraph::onMarkerClicked(InteractiveGraphicsItem *marker, const QPointF &position)`  
**Lines**: 474-500

### Implementation Details

```cpp
void BTWGraph::onMarkerClicked(InteractiveGraphicsItem *marker, const QPointF &position)
{
    if (!marker) {
        qDebug() << "BTWGraph: Marker clicked - NULL marker";
        return;
    }
    
    // Get the marker's scene position (Y coordinate)
    QPointF scenePos = marker->scenePos();
    qreal yPos = scenePos.y();
    
    // Convert Y position to timestamp using mapScreenToTime
    QDateTime timestamp = mapScreenToTime(yPos);
    
    if (timestamp.isValid()) {
        // Timestamp captured successfully
        qDebug() << "BTW MARKER SELECTED - TIMESTAMP RETURNED";
        qDebug() << "BTWGraph: TIMESTAMP:" << timestamp.toString("yyyy-MM-dd hh:mm:ss.zzz");
    } else {
        qDebug() << "BTWGraph: Marker clicked - Could not determine timestamp (invalid)";
    }
}
```

### Signal Connection

The `onMarkerClicked` slot is connected to the `BTWInteractiveOverlay::markerClicked` signal in `BTWGraph::setupInteractiveOverlay()`:

```cpp
connect(m_interactiveOverlay, &BTWInteractiveOverlay::markerClicked,
        this, &BTWGraph::onMarkerClicked);
```

### Console Output

When a BTW manual marker is clicked, you'll see:

```
========================================
BTW MARKER SELECTED - TIMESTAMP RETURNED
========================================
BTWGraph: Marker clicked at position: QPointF(x, y)
BTWGraph: Marker scene position: QPointF(x, y)
BTWGraph: Marker Y position: <y_value>
BTWGraph: TIMESTAMP: 2025-11-27 11:55:26.000
========================================
```

---

## Key Functions

### `mapScreenToTime(qreal yPosition)`

This function (inherited from `WaterfallGraph`) converts a Y screen coordinate to a `QDateTime` timestamp. It uses the graph's current time range (`timeMin` and `timeMax`) and drawing area to perform the conversion.

**Location**: `waterfallgraph.cpp`  
**Usage**: `QDateTime timestamp = mapScreenToTime(yPos);`

---

## How to Access Captured Timestamps

Currently, timestamps are only logged to the console. To capture and use them in your application, you have several options:

### Option 1: Add a Signal (Recommended)

Emit a signal when a timestamp is captured, allowing other parts of your application to respond:

**In `rtwgraph.h`:**
```cpp
signals:
    void rMarkerTimestampCaptured(const QDateTime &timestamp, const QPointF &position);
```

**In `rtwgraph.cpp`:**
```cpp
if (timestamp.isValid()) {
    emit rMarkerTimestampCaptured(timestamp, scenePos);
    // ... existing logging code ...
}
```

**In `btwgraph.h`:**
```cpp
signals:
    void manualMarkerTimestampCaptured(const QDateTime &timestamp, const QPointF &position);
```

**In `btwgraph.cpp`:**
```cpp
if (timestamp.isValid()) {
    emit manualMarkerTimestampCaptured(timestamp, scenePos);
    // ... existing logging code ...
}
```

### Option 2: Store in a Member Variable

Store the last captured timestamp in a member variable:

**In `rtwgraph.h`:**
```cpp
private:
    QDateTime lastCapturedRTWTimestamp;
```

**In `rtwgraph.cpp`:**
```cpp
if (timestamp.isValid()) {
    lastCapturedRTWTimestamp = timestamp;
    // ... existing logging code ...
}
```

### Option 3: Call a Callback Function

Use a callback function similar to the cursor time callback:

**In `rtwgraph.h`:**
```cpp
private:
    std::function<void(const QDateTime &, const QPointF &)> rMarkerTimestampCallback;
public:
    void setRMarkerTimestampCallback(std::function<void(const QDateTime &, const QPointF &)> callback);
```

**In `rtwgraph.cpp`:**
```cpp
if (timestamp.isValid()) {
    if (rMarkerTimestampCallback) {
        rMarkerTimestampCallback(timestamp, scenePos);
    }
    // ... existing logging code ...
}
```

---

## Timestamp Format

Timestamps are returned as `QDateTime` objects and can be formatted in various ways:

- **Default format**: `timestamp.toString()` → `"Thu Nov 27 11:55:26 2025"`
- **ISO format**: `timestamp.toString(Qt::ISODate)` → `"2025-11-27T11:55:26"`
- **Custom format**: `timestamp.toString("yyyy-MM-dd hh:mm:ss.zzz")` → `"2025-11-27 11:55:26.000"`
- **Unix timestamp**: `timestamp.toMSecsSinceEpoch()` → milliseconds since epoch

---

## Troubleshooting

### Timestamp is Invalid

If `timestamp.isValid()` returns `false`, possible causes:

1. **Graph time range not set**: The graph's `timeMin` and `timeMax` may not be initialized
2. **Y position out of range**: The clicked position may be outside the visible time range
3. **Drawing area not set**: The graph's `drawingArea` may not be properly initialized

**Solution**: Ensure the graph has valid time ranges and drawing area before clicking markers.

### Marker Click Not Detected

If clicking a marker doesn't trigger timestamp capture:

1. **RTW R Marker**: Check that the clicked item is a `QGraphicsTextItem` with text "R"
2. **BTW Manual Marker**: Check that the marker is an `InteractiveGraphicsItem` in the overlay scene
3. **Event propagation**: Ensure mouse events are not being intercepted by other widgets

**Solution**: Check console logs for debug messages indicating what item was clicked.

---

## Example: Connecting to Signals

If you implement signals (Option 1), you can connect them in your application code:

```cpp
// In your main window or container class
RTWGraph *rtwGraph = ...;
BTWGraph *btwGraph = ...;

// Connect RTW R marker timestamp signal
connect(rtwGraph, &RTWGraph::rMarkerTimestampCaptured,
        this, [this](const QDateTime &timestamp, const QPointF &position) {
    qDebug() << "RTW R marker timestamp received:" << timestamp;
    // Do something with the timestamp
    processRTWTimestamp(timestamp, position);
});

// Connect BTW manual marker timestamp signal
connect(btwGraph, &BTWGraph::manualMarkerTimestampCaptured,
        this, [this](const QDateTime &timestamp, const QPointF &position) {
    qDebug() << "BTW manual marker timestamp received:" << timestamp;
    // Do something with the timestamp
    processBTWTimestamp(timestamp, position);
});
```

---

## Summary

- **RTW R Markers**: Timestamps are captured in `RTWGraph::onMouseClick()` when clicking yellow "R" markers
- **BTW Manual Markers**: Timestamps are captured in `BTWGraph::onMarkerClicked()` when clicking interactive markers
- **Conversion Method**: Both use `mapScreenToTime(yPos)` to convert Y screen coordinates to timestamps
- **Current Behavior**: Timestamps are logged to console with clear markers
- **Extension Options**: Add signals, store in variables, or use callbacks to access timestamps programmatically

---

## Related Files

- `rtwgraph.cpp` / `rtwgraph.h` - RTW graph implementation
- `btwgraph.cpp` / `btwgraph.h` - BTW graph implementation
- `waterfallgraph.cpp` / `waterfallgraph.h` - Base graph class with `mapScreenToTime()`
- `btwinteractiveoverlay.cpp` / `btwinteractiveoverlay.h` - BTW interactive marker system

