# BTW Horizontal Line API Documentation

## Overview

The BTW Horizontal Line API allows you to draw horizontal lines on BTW (Bit Time Waterfall) graphs. These lines are distinct from manual markers and provide a way to mark specific timestamps with visual reference lines. The lines span the full width of the graph at a specific time position.

**Key Features:**
- Draw horizontal lines by clicking on BTW graphs (when mode is enabled)
- Delete horizontal lines by clicking on them (click-to-delete)
- Programmatically add, remove, and clear horizontal lines
- Lines are cached for performance optimization
- Separate from manual markers - lines don't interfere with marker functionality
- Full-width lines that span the entire graph width

---

## Table of Contents

1. [API Reference](#api-reference)
2. [Usage Examples](#usage-examples)
3. [Mode Toggle](#mode-toggle)
4. [Integration Guide](#integration-guide)
5. [Performance Notes](#performance-notes)

---

## API Reference

### Setting Horizontal Line Mode

#### `void setBTWHorizontalLineMode(const GraphType &graphType, bool enabled)`

Enables or disables horizontal line drawing mode for a specific BTW graph. When enabled, clicking on the graph will draw a horizontal line instead of creating a manual marker.

**Parameters:**
- `graphType`: The graph type (must be `GraphType::BTW`)
- `enabled`: `true` to enable horizontal line mode, `false` to disable (defaults to marker mode)

**Example:**
```cpp
// Enable horizontal line mode for BTW graphs
graphLayout->setBTWHorizontalLineMode(GraphType::BTW, true);

// Disable horizontal line mode (return to marker mode)
graphLayout->setBTWHorizontalLineMode(GraphType::BTW, false);
```

**Note:** When horizontal line mode is disabled, clicking on the graph will create manual markers as usual.

---

### Adding Horizontal Lines

#### `QUuid addBTWHorizontalLine(const GraphType &graphType, const QDateTime &timestamp, const QColor &color = Qt::yellow, qreal width = 2.0)`

Programmatically adds a horizontal line to a BTW graph at a specific timestamp.

**Parameters:**
- `graphType`: The graph type (must be `GraphType::BTW`)
- `timestamp`: The QDateTime timestamp where the line should be drawn (maps to Y position)
- `color`: Line color (default: `Qt::yellow`)
- `width`: Line width in pixels (default: `2.0`)

**Returns:** `QUuid` - Unique identifier for the line (can be used for removal)

**Example:**
```cpp
// Add a yellow line at the current time
QUuid lineId = graphLayout->addBTWHorizontalLine(
    GraphType::BTW, 
    QDateTime::currentDateTime()
);

// Add a red line with custom width
QUuid redLineId = graphLayout->addBTWHorizontalLine(
    GraphType::BTW, 
    QDateTime::currentDateTime().addSecs(-60),
    Qt::red,
    3.0
);
```

**Note:** The timestamp maps to the Y-axis position in the graph. The line spans the full width of the graph horizontally.

---

### Removing Horizontal Lines

#### `bool removeBTWHorizontalLine(const GraphType &graphType, const QUuid &lineId)`

Removes a specific horizontal line from a BTW graph by its unique identifier.

**Parameters:**
- `graphType`: The graph type (must be `GraphType::BTW`)
- `lineId`: The unique identifier returned when the line was added

**Returns:** `true` if the line was found and removed, `false` otherwise

**Example:**
```cpp
// Add a line and store its ID
QUuid lineId = graphLayout->addBTWHorizontalLine(
    GraphType::BTW, 
    QDateTime::currentDateTime()
);

// Later, remove the line
bool removed = graphLayout->removeBTWHorizontalLine(GraphType::BTW, lineId);
if (removed) {
    qDebug() << "Line removed successfully";
} else {
    qDebug() << "Line not found";
}
```

---

### Clearing Horizontal Lines

#### `void clearBTWHorizontalLines(const GraphType &graphType)`

Removes all horizontal lines from a BTW graph.

**Parameters:**
- `graphType`: The graph type (must be `GraphType::BTW`)

**Example:**
```cpp
// Clear all horizontal lines from BTW graphs
graphLayout->clearBTWHorizontalLines(GraphType::BTW);
```

---

## Usage Examples

### Basic Usage: Enable Mode and Draw Lines

```cpp
// Enable horizontal line mode
graphLayout->setBTWHorizontalLineMode(GraphType::BTW, true);

// Now when users click on BTW graphs, horizontal lines will be drawn
// instead of manual markers

// Disable mode to return to marker functionality
graphLayout->setBTWHorizontalLineMode(GraphType::BTW, false);
```

### Programmatic Line Management

```cpp
// Store line IDs for later management
QList<QUuid> lineIds;

// Add multiple lines at different timestamps
QDateTime baseTime = QDateTime::currentDateTime();
for (int i = 0; i < 5; ++i) {
    QUuid id = graphLayout->addBTWHorizontalLine(
        GraphType::BTW,
        baseTime.addSecs(-i * 60),  // Every minute
        Qt::yellow,
        2.0
    );
    lineIds.append(id);
}

// Remove a specific line
if (!lineIds.isEmpty()) {
    graphLayout->removeBTWHorizontalLine(GraphType::BTW, lineIds.first());
}

// Clear all remaining lines
graphLayout->clearBTWHorizontalLines(GraphType::BTW);
```

### Color-Coded Lines

```cpp
// Add lines with different colors for different purposes
QUuid warningLine = graphLayout->addBTWHorizontalLine(
    GraphType::BTW,
    warningTimestamp,
    Qt::yellow,  // Warning color
    2.0
);

QUuid errorLine = graphLayout->addBTWHorizontalLine(
    GraphType::BTW,
    errorTimestamp,
    Qt::red,     // Error color
    3.0          // Thicker line for emphasis
);

QUuid infoLine = graphLayout->addBTWHorizontalLine(
    GraphType::BTW,
    infoTimestamp,
    Qt::cyan,    // Info color
    1.5
);
```

### Event-Driven Line Management

```cpp
class MyApplication : public QMainWindow
{
    Q_OBJECT

private:
    GraphLayout* m_graphLayout;
    QMap<QUuid, QDateTime> m_lineTimestamps;  // Track lines and their timestamps

public:
    MyApplication(QWidget* parent = nullptr)
        : QMainWindow(parent)
    {
        // Setup graph layout...
        m_graphLayout = new GraphLayout(this, LayoutType::GPW4W, timer, seriesMap);
        
        // Enable horizontal line mode
        m_graphLayout->setBTWHorizontalLineMode(GraphType::BTW, true);
    }

private slots:
    void onEventOccurred(const QDateTime &eventTime)
    {
        // Add a line when an event occurs
        QUuid lineId = m_graphLayout->addBTWHorizontalLine(
            GraphType::BTW,
            eventTime,
            Qt::green,
            2.0
        );
        
        // Track the line
        m_lineTimestamps[lineId] = eventTime;
    }
    
    void onClearOldLines()
    {
        // Remove lines older than 1 hour
        QDateTime cutoff = QDateTime::currentDateTime().addSecs(-3600);
        QList<QUuid> toRemove;
        
        for (auto it = m_lineTimestamps.begin(); it != m_lineTimestamps.end(); ++it) {
            if (it.value() < cutoff) {
                toRemove.append(it.key());
            }
        }
        
        for (const QUuid &id : toRemove) {
            m_graphLayout->removeBTWHorizontalLine(GraphType::BTW, id);
            m_lineTimestamps.remove(id);
        }
    }
};
```

---

## Mode Toggle

### Switching Between Modes

The horizontal line mode and manual marker mode are mutually exclusive:

- **Horizontal Line Mode Enabled:** Clicking on BTW graphs draws horizontal lines or deletes existing lines
- **Horizontal Line Mode Disabled:** Clicking on BTW graphs creates manual markers (default behavior)

### Click-to-Delete Behavior

When horizontal line mode is enabled, clicking on the graph has two behaviors:

1. **Click on empty space:** Creates a new horizontal line at the clicked position
2. **Click on existing line:** Deletes the clicked line (within 5 pixel threshold)

This provides an intuitive way to manage lines without needing to track line IDs or use separate delete controls.

**Example:**
```cpp
// Enable line mode
graphLayout->setBTWHorizontalLineMode(GraphType::BTW, true);
// User clicks on empty space -> horizontal line is drawn
// User clicks on existing line -> line is deleted

// Disable line mode
graphLayout->setBTWHorizontalLineMode(GraphType::BTW, false);
// User clicks -> manual marker is created
```

### Mode State

There's no direct API to query the current mode state. You should track the mode state in your application if needed:

```cpp
class GraphManager {
private:
    bool m_horizontalLineModeEnabled = false;
    GraphLayout* m_graphLayout;

public:
    void setHorizontalLineMode(bool enabled) {
        m_horizontalLineModeEnabled = enabled;
        m_graphLayout->setBTWHorizontalLineMode(GraphType::BTW, enabled);
    }
    
    bool isHorizontalLineModeEnabled() const {
        return m_horizontalLineModeEnabled;
    }
};
```

---

## Integration Guide

### Basic Integration

```cpp
#include "graphlayout.h"
#include <QDateTime>
#include <QUuid>

// Create GraphLayout
GraphLayout* graphLayout = new GraphLayout(parent, LayoutType::GPW4W, timer, seriesMap);

// Enable horizontal line mode
graphLayout->setBTWHorizontalLineMode(GraphType::BTW, true);

// Users can now click on BTW graphs to draw lines
```

### Integration with UI Controls

```cpp
class MainWindow : public QMainWindow
{
    Q_OBJECT

private:
    GraphLayout* m_graphLayout;
    QPushButton* m_toggleLineModeButton;
    QPushButton* m_clearLinesButton;
    bool m_lineModeEnabled = false;

public:
    MainWindow(QWidget* parent = nullptr)
        : QMainWindow(parent)
    {
        // Setup graph layout...
        m_graphLayout = new GraphLayout(this, LayoutType::GPW4W, timer, seriesMap);
        
        // Create UI controls
        m_toggleLineModeButton = new QPushButton("Enable Line Mode", this);
        m_clearLinesButton = new QPushButton("Clear All Lines", this);
        
        // Connect buttons
        connect(m_toggleLineModeButton, &QPushButton::clicked, 
                this, &MainWindow::toggleLineMode);
        connect(m_clearLinesButton, &QPushButton::clicked,
                this, &MainWindow::clearAllLines);
    }

private slots:
    void toggleLineMode() {
        m_lineModeEnabled = !m_lineModeEnabled;
        m_graphLayout->setBTWHorizontalLineMode(GraphType::BTW, m_lineModeEnabled);
        
        if (m_lineModeEnabled) {
            m_toggleLineModeButton->setText("Disable Line Mode");
        } else {
            m_toggleLineModeButton->setText("Enable Line Mode");
        }
    }
    
    void clearAllLines() {
        m_graphLayout->clearBTWHorizontalLines(GraphType::BTW);
    }
};
```

---

## Performance Notes

### Caching

Horizontal lines are cached internally for performance. The `QGraphicsLineItem` objects are:
- Created once when first drawn
- Reused on subsequent redraws (only position is updated)
- Recreated only when necessary (after scene clear or when first added)

This caching mechanism ensures efficient rendering even with many lines.

### Click-to-Delete Performance

The click-to-delete feature is optimized for minimal performance impact:
- Uses cached `QGraphicsLineItem` objects for hit detection (no recalculation)
- Simple distance check (O(1) per line)
- Only checks visible lines (those with valid cached items)
- Total complexity is O(n) where n is the number of lines (typically very small)

The hit detection threshold is 5 pixels, providing a good balance between precision and ease of use.

### Best Practices

1. **Limit Line Count:** While there's no hard limit, consider removing old lines periodically to maintain performance
2. **Use Clear When Needed:** Use `clearBTWHorizontalLines()` when switching contexts or resetting views
3. **Store Line IDs:** Keep track of line IDs if you need to manage them individually
4. **Batch Operations:** When adding multiple lines, add them all before triggering a redraw

**Example:**
```cpp
// Good: Add all lines, then redraw happens automatically
for (const QDateTime &timestamp : timestamps) {
    graphLayout->addBTWHorizontalLine(GraphType::BTW, timestamp);
}

// Less efficient: Adding lines one at a time with manual redraws
for (const QDateTime &timestamp : timestamps) {
    graphLayout->addBTWHorizontalLine(GraphType::BTW, timestamp);
    graphLayout->redrawGraph(GraphType::BTW);  // Unnecessary
}
```

---

## Differences from Manual Markers

| Feature | Horizontal Lines | Manual Markers |
|---------|------------------|----------------|
| **Visual** | Full-width horizontal line | Circle marker at specific point |
| **Storage** | Separate storage system | Stored in BTWInteractiveOverlay |
| **Mode** | Requires mode toggle | Default behavior |
| **Use Case** | Time reference lines | Point markers with range/delta |
| **Interaction** | Click to draw line, click on line to delete | Click to place marker |
| **Removal** | Click on line (or via API with line ID) | Via interactive overlay |

**Note:** Horizontal lines and manual markers can coexist - they are stored and managed separately. However, when horizontal line mode is enabled, clicking will draw lines instead of markers.

---

## Troubleshooting

### Lines Not Appearing

**Problem:** Lines are added but not visible on the graph.

**Solutions:**
1. Verify the timestamp is within the visible time range
2. Check that the graph type is `GraphType::BTW`
3. Ensure the graph has been redrawn after adding lines
4. Verify the line color is visible against the graph background

### Lines Not Drawing on Click

**Problem:** Clicking on the graph doesn't draw lines.

**Solutions:**
1. Verify horizontal line mode is enabled: `setBTWHorizontalLineMode(GraphType::BTW, true)`
2. Check that you're clicking on a BTW graph (not RTW or other types)
3. Ensure the graph is not disabled or hidden
4. Verify mouse events are not being intercepted

### Performance Issues

**Problem:** UI becomes sluggish with many lines.

**Solutions:**
1. Remove old lines periodically using `removeBTWHorizontalLine()` or `clearBTWHorizontalLines()`
2. Limit the number of lines displayed at once
3. Use `clearBTWHorizontalLines()` when switching views or contexts

---

## Summary

The BTW Horizontal Line API provides a simple and efficient way to:
- Draw horizontal reference lines on BTW graphs
- Mark specific timestamps with visual indicators
- Manage lines programmatically with full control
- Maintain performance through intelligent caching

For questions or issues, refer to the source code in `graphlayout.h`, `graphlayout.cpp`, `btwgraph.h`, and `btwgraph.cpp`.

