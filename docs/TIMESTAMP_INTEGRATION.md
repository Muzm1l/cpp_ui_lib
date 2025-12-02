# Timestamp Integration Guide

This document explains how to capture timestamps from the GraphLayout system when integrated into external projects. The system provides Qt signals for three types of timestamp events:

1. **History Selection Timestamp** - When a time range is selected on the timeline
2. **RTW R Marker Timestamp** - When an RTW R marker is clicked
3. **BTW Manual Marker Timestamp** - When a BTW manual marker is placed or clicked

---

## Overview

The `GraphLayout` class exposes Qt signals that can be connected to slots in your application. These signals are automatically forwarded from the underlying graph components (`RTWGraph`, `BTWGraph`) through `GraphContainer` to `GraphLayout`, providing a unified interface for timestamp capture.

---

## Available Signals

### 1. History Selection Timestamp

**Signal**: `TimeSelectionCreated(const TimeSelectionSpan &selection)`

**When Emitted**: When a user selects a time range on the timeline view (by dragging on the timeline slider or using the time selection visualizer).

**Parameters**:
- `selection`: A `TimeSelectionSpan` struct containing:
  - `startTime`: `QTime` - Start time of the selection
  - `endTime`: `QTime` - End time of the selection

**Example**:
```cpp
// Connect to the signal
connect(graphLayout, &GraphLayout::TimeSelectionCreated,
        this, [this](const TimeSelectionSpan &selection) {
    qDebug() << "Time selection created:";
    qDebug() << "  Start:" << selection.startTime.toString("hh:mm:ss");
    qDebug() << "  End:" << selection.endTime.toString("hh:mm:ss");
    
    // Convert to QDateTime if needed (using current date)
    QDateTime startDateTime = QDateTime::currentDateTime();
    startDateTime.setTime(selection.startTime);
    QDateTime endDateTime = QDateTime::currentDateTime();
    endDateTime.setTime(selection.endTime);
    
    // Process the selection
    processTimeSelection(startDateTime, endDateTime);
});
```

---

### 2. RTW R Marker Timestamp

**Signal**: `RTWRMarkerTimestampCaptured(const QDateTime &timestamp, const QPointF &position)`

**When Emitted**: When a user clicks on an RTW R marker (yellow "R" symbol) displayed on an RTW graph.

**Parameters**:
- `timestamp`: `QDateTime` - The timestamp corresponding to the R marker's position on the graph
- `position`: `QPointF` - The scene position (x, y coordinates) where the marker was clicked

**Example**:
```cpp
// Connect to the signal
connect(graphLayout, &GraphLayout::RTWRMarkerTimestampCaptured,
        this, [this](const QDateTime &timestamp, const QPointF &position) {
    qDebug() << "RTW R marker clicked:";
    qDebug() << "  Timestamp:" << timestamp.toString("yyyy-MM-dd hh:mm:ss.zzz");
    qDebug() << "  Position:" << position;
    
    // Process the R marker timestamp
    processRTWRMarker(timestamp, position);
});
```

---

### 3. BTW Manual Marker Timestamps

The system provides two signals for BTW manual markers:

#### 3a. Marker Placed

**Signal**: `BTWManualMarkerPlaced(const QDateTime &timestamp, const QPointF &position)`

**When Emitted**: When a user places a new manual marker on a BTW graph by clicking on empty space.

**Parameters**:
- `timestamp`: `QDateTime` - The timestamp corresponding to the marker's position on the graph
- `position`: `QPointF` - The scene position (x, y coordinates) where the marker was placed

#### 3b. Marker Clicked

**Signal**: `BTWManualMarkerClicked(const QDateTime &timestamp, const QPointF &position)`

**When Emitted**: When a user clicks on an existing BTW manual marker.

**Parameters**:
- `timestamp`: `QDateTime` - The timestamp corresponding to the marker's position on the graph
- `position`: `QPointF` - The scene position (x, y coordinates) where the marker was clicked

**Example**:
```cpp
// Connect to marker placed signal
connect(graphLayout, &GraphLayout::BTWManualMarkerPlaced,
        this, [this](const QDateTime &timestamp, const QPointF &position) {
    qDebug() << "BTW manual marker placed:";
    qDebug() << "  Timestamp:" << timestamp.toString("yyyy-MM-dd hh:mm:ss.zzz");
    qDebug() << "  Position:" << position;
    
    // Process the marker placement
    processBTWMarkerPlacement(timestamp, position);
});

// Connect to marker clicked signal
connect(graphLayout, &GraphLayout::BTWManualMarkerClicked,
        this, [this](const QDateTime &timestamp, const QPointF &position) {
    qDebug() << "BTW manual marker clicked:";
    qDebug() << "  Timestamp:" << timestamp.toString("yyyy-MM-dd hh:mm:ss.zzz");
    qDebug() << "  Position:" << position;
    
    // Process the marker click
    processBTWMarkerClick(timestamp, position);
});
```

---

## Complete Integration Example

Here's a complete example showing how to integrate all timestamp signals in your application:

```cpp
#include "graphlayout.h"
#include <QDateTime>
#include <QDebug>

class MyApplication : public QWidget
{
    Q_OBJECT

public:
    MyApplication(QWidget *parent = nullptr) : QWidget(parent)
    {
        // Create GraphLayout instance
        graphLayout = new GraphLayout(this, LayoutType::GPW4W);
        
        // Connect all timestamp signals
        setupTimestampConnections();
    }

private slots:
    void setupTimestampConnections()
    {
        // Connect history selection timestamp
        connect(graphLayout, &GraphLayout::TimeSelectionCreated,
                this, &MyApplication::onTimeSelectionCreated);
        
        // Connect RTW R marker timestamp
        connect(graphLayout, &GraphLayout::RTWRMarkerTimestampCaptured,
                this, &MyApplication::onRTWRMarkerTimestampCaptured);
        
        // Connect BTW manual marker timestamps
        connect(graphLayout, &GraphLayout::BTWManualMarkerPlaced,
                this, &MyApplication::onBTWManualMarkerPlaced);
        connect(graphLayout, &GraphLayout::BTWManualMarkerClicked,
                this, &MyApplication::onBTWManualMarkerClicked);
    }

    void onTimeSelectionCreated(const TimeSelectionSpan &selection)
    {
        // Convert QTime to QDateTime (using current date)
        QDateTime startDateTime = QDateTime::currentDateTime();
        startDateTime.setTime(selection.startTime);
        QDateTime endDateTime = QDateTime::currentDateTime();
        endDateTime.setTime(selection.endTime);
        
        qDebug() << "History Selection:";
        qDebug() << "  Start:" << startDateTime.toString("yyyy-MM-dd hh:mm:ss");
        qDebug() << "  End:" << endDateTime.toString("yyyy-MM-dd hh:mm:ss");
        
        // Store or process the selection
        storeTimeSelection(startDateTime, endDateTime);
    }

    void onRTWRMarkerTimestampCaptured(const QDateTime &timestamp, const QPointF &position)
    {
        qDebug() << "RTW R Marker:";
        qDebug() << "  Timestamp:" << timestamp.toString("yyyy-MM-dd hh:mm:ss.zzz");
        qDebug() << "  Position:" << position;
        
        // Store or process the R marker timestamp
        storeRTWRMarker(timestamp, position);
    }

    void onBTWManualMarkerPlaced(const QDateTime &timestamp, const QPointF &position)
    {
        qDebug() << "BTW Manual Marker Placed:";
        qDebug() << "  Timestamp:" << timestamp.toString("yyyy-MM-dd hh:mm:ss.zzz");
        qDebug() << "  Position:" << position;
        
        // Store or process the marker placement
        storeBTWMarker(timestamp, position, true); // true = placed
    }

    void onBTWManualMarkerClicked(const QDateTime &timestamp, const QPointF &position)
    {
        qDebug() << "BTW Manual Marker Clicked:";
        qDebug() << "  Timestamp:" << timestamp.toString("yyyy-MM-dd hh:mm:ss.zzz");
        qDebug() << "  Position:" << position;
        
        // Store or process the marker click
        storeBTWMarker(timestamp, position, false); // false = clicked
    }

private:
    GraphLayout *graphLayout;
    
    // Your storage/processing methods
    void storeTimeSelection(const QDateTime &start, const QDateTime &end) { /* ... */ }
    void storeRTWRMarker(const QDateTime &timestamp, const QPointF &position) { /* ... */ }
    void storeBTWMarker(const QDateTime &timestamp, const QPointF &position, bool isPlaced) { /* ... */ }
};
```

---

## Signal Connection Best Practices

### 1. Connect in Constructor or Setup Function

Connect signals early in your application lifecycle, typically in the constructor or a dedicated setup function:

```cpp
MyApplication::MyApplication(QWidget *parent) : QWidget(parent)
{
    graphLayout = new GraphLayout(this, LayoutType::GPW4W);
    setupConnections(); // Dedicated setup function
}

void MyApplication::setupConnections()
{
    // All signal connections here
}
```

### 2. Use Lambda Functions for Simple Handlers

For simple timestamp logging or processing, lambda functions are convenient:

```cpp
connect(graphLayout, &GraphLayout::RTWRMarkerTimestampCaptured,
        this, [this](const QDateTime &timestamp, const QPointF &position) {
    // Simple processing
    qDebug() << "R marker:" << timestamp;
});
```

### 3. Use Slot Functions for Complex Logic

For complex processing, use dedicated slot functions:

```cpp
connect(graphLayout, &GraphLayout::RTWRMarkerTimestampCaptured,
        this, &MyApplication::onRTWRMarkerTimestampCaptured);

// Later in the class
void MyApplication::onRTWRMarkerTimestampCaptured(const QDateTime &timestamp, const QPointF &position)
{
    // Complex processing logic
    validateTimestamp(timestamp);
    updateDatabase(timestamp, position);
    notifyOtherComponents(timestamp);
}
```

### 4. Handle Multiple GraphLayout Instances

If you have multiple `GraphLayout` instances, connect each one separately:

```cpp
for (GraphLayout *layout : graphLayouts) {
    connect(layout, &GraphLayout::RTWRMarkerTimestampCaptured,
            this, &MyApplication::onRTWRMarkerTimestampCaptured);
}
```

---

## Timestamp Format and Conversion

### QDateTime Formatting

Timestamps are provided as `QDateTime` objects. You can format them in various ways:

```cpp
QDateTime timestamp = ...; // From signal

// ISO format
QString isoString = timestamp.toString(Qt::ISODate); // "2024-01-15T14:30:45"

// Custom format
QString customString = timestamp.toString("yyyy-MM-dd hh:mm:ss.zzz"); // "2024-01-15 14:30:45.123"

// Unix timestamp (milliseconds)
qint64 unixMs = timestamp.toMSecsSinceEpoch();

// Unix timestamp (seconds)
qint64 unixSec = timestamp.toSecsSinceEpoch();
```

### TimeSelectionSpan to QDateTime Conversion

For history selections, you receive `QTime` objects. Convert them to `QDateTime`:

```cpp
void onTimeSelectionCreated(const TimeSelectionSpan &selection)
{
    // Option 1: Use current date
    QDateTime startDateTime = QDateTime::currentDateTime();
    startDateTime.setTime(selection.startTime);
    
    QDateTime endDateTime = QDateTime::currentDateTime();
    endDateTime.setTime(selection.endTime);
    
    // Option 2: Use a specific date
    QDate targetDate(2024, 1, 15);
    QDateTime startDateTime(targetDate, selection.startTime);
    QDateTime endDateTime(targetDate, selection.endTime);
}
```

---

## Troubleshooting

### Signals Not Emitted

If signals are not being emitted:

1. **Check Graph Type**: Ensure the graph type (RTW, BTW) is actually displayed in the layout
2. **Check Graph Visibility**: Ensure the graph container is visible
3. **Check Signal Connections**: Verify connections are made after `GraphLayout` is fully initialized
4. **Check Console Logs**: Look for debug messages indicating marker clicks/placements

### Invalid Timestamps

If timestamps are invalid (`timestamp.isValid() == false`):

1. **Graph Time Range**: Ensure the graph has a valid time range set
2. **Timeline Customization**: The timeline view may need to be customized (slider moved) to initialize time ranges
3. **Data Availability**: Ensure data is loaded into the graph

### Multiple Signals for Same Event

If you receive multiple signals for the same event:

1. **Check Connection Count**: Ensure you're not connecting the same signal multiple times
2. **Use Qt::UniqueConnection**: Add `Qt::UniqueConnection` to prevent duplicate connections:
   ```cpp
   connect(graphLayout, &GraphLayout::RTWRMarkerTimestampCaptured,
           this, &MyApplication::onRTWRMarkerTimestampCaptured,
           Qt::UniqueConnection);
   ```

---

## Signal Flow Architecture

Understanding the signal flow helps with debugging:

```
RTWGraph::onMouseClick()
    ↓ emits rMarkerTimestampCaptured
GraphContainer::onRTWRMarkerTimestampCaptured()
    ↓ emits RTWRMarkerTimestampCaptured
GraphLayout::RTWRMarkerTimestampCaptured()
    ↓ emits to external application
YourApplication::onRTWRMarkerTimestampCaptured()
```

The same pattern applies to BTW markers and time selections.

---

## Related Documentation

- **Internal Implementation**: See `docs/TIMESTAMP_CAPTURE.md` for internal implementation details
- **GraphLayout API**: See `graphlayout.h` for the complete API
- **GraphContainer API**: See `graphcontainer.h` for container-level API
- **RTW Graph**: See `rtwgraph.h` for RTW-specific functionality
- **BTW Graph**: See `btwgraph.h` for BTW-specific functionality

---

## Summary

- **History Selection**: Connect to `GraphLayout::TimeSelectionCreated` signal
- **RTW R Marker**: Connect to `GraphLayout::RTWRMarkerTimestampCaptured` signal
- **BTW Manual Marker (Placed)**: Connect to `GraphLayout::BTWManualMarkerPlaced` signal
- **BTW Manual Marker (Clicked)**: Connect to `GraphLayout::BTWManualMarkerClicked` signal

All signals are automatically forwarded from the graph components through `GraphContainer` to `GraphLayout`, providing a clean integration interface for external projects.

