# BTW Shaded Region API Documentation

## Overview

The BTW Shaded Region API allows you to display vertical shaded regions on BTW (Bit Time Waterfall) graphs. A shaded region is a semi-transparent vertical band that spans from the top to the bottom of the graph (across all visible timestamps), with horizontal boundaries defined by range values.

Shaded regions are useful for:
- Highlighting specific range intervals (e.g., 30 to 40 nautical miles)
- Visualizing zones of interest
- Marking operational boundaries
- Emphasizing critical range bands

## Visual Representation

Shaded regions appear as vertical bands on the BTW graph:

```
     Range (X-axis)
     0    30   40    100
     |    |----|     |
     |    |████|     |  ← Vertical shaded region
     |    |████|     |     (spans all timestamps)
     |    |████|     |
     |    |████|     |
     |    |████|     |
     |    |----|     |
     └──────────────┘
     Time (Y-axis)
     (top to bottom)
```

**Key Visual Elements:**
- **Vertical Band**: Spans from top to bottom (all visible timestamps)
- **Horizontal Boundaries**: Defined by `startX` (left) and `endX` (right) range values
- **Semi-transparent Fill**: Gray fill with 80% opacity
- **Border**: Light gray border for visibility

## API Methods

The shaded region API is available on the `BTWGraph` class:

### 1. Add Shaded Region

```cpp
int addShadedRegion(qreal startX, qreal endX, const QDateTime &startY);
```

**Parameters:**
- **`startX`**: Starting X value (left range boundary, e.g., 30.0)
  - Type: `qreal` (double)
  - Represents the left edge of the shaded region in range units
  - Must be less than `endX`
  
- **`endX`**: Ending X value (right range boundary, e.g., 40.0)
  - Type: `qreal` (double)
  - Represents the right edge of the shaded region in range units
  - Must be greater than `startX`
  
- **`startY`**: Starting Y value (timestamp)
  - Type: `QDateTime`
  - Currently stored but not used for Y range calculation
  - The region always spans the full height (all visible timestamps)

**Returns:**
- `int`: Unique identifier for the shaded region
  - Use this ID to remove the region later
  - IDs are auto-incremented starting from 1

**Example:**
```cpp
BTWGraph *btwGraph = ...;
QDateTime currentTime = QDateTime::currentDateTime();

// Create a vertical shaded region from range 30 to 40
int regionId = btwGraph->addShadedRegion(30.0, 40.0, currentTime);
qDebug() << "Created shaded region with ID:" << regionId;
```

### 2. Remove Shaded Region

```cpp
void removeShadedRegion(int regionId);
```

**Parameters:**
- **`regionId`**: The unique identifier returned by `addShadedRegion()`
  - Type: `int`
  - Must be a valid region ID that was previously returned by `addShadedRegion()`

**Example:**
```cpp
// Remove the region we created earlier
btwGraph->removeShadedRegion(regionId);
```

### 3. Clear All Shaded Regions

```cpp
void clearShadedRegions();
```

Removes all shaded regions from the graph.

**Example:**
```cpp
// Clear all shaded regions
btwGraph->clearShadedRegions();
```

## Usage Examples

### Example 1: Adding a Single Shaded Region

```cpp
#include "btwgraph.h"
#include <QDateTime>

// Get your BTWGraph instance
BTWGraph *btwGraph = ...;

// Create a shaded region from range 30 to 40
QDateTime timestamp = QDateTime::currentDateTime();
int regionId = btwGraph->addShadedRegion(30.0, 40.0, timestamp);

qDebug() << "Added shaded region" << regionId << "from range 30 to 40";
```

### Example 2: Adding Multiple Shaded Regions

```cpp
#include "btwgraph.h"
#include <QDateTime>

BTWGraph *btwGraph = ...;
QDateTime currentTime = QDateTime::currentDateTime();

// Add multiple shaded regions for different range intervals
int region1 = btwGraph->addShadedRegion(10.0, 20.0, currentTime);
int region2 = btwGraph->addShadedRegion(50.0, 60.0, currentTime);
int region3 = btwGraph->addShadedRegion(80.0, 90.0, currentTime);

qDebug() << "Added 3 shaded regions with IDs:" << region1 << region2 << region3;
```

### Example 3: Removing a Specific Region

```cpp
// Store the region ID when creating
int regionId = btwGraph->addShadedRegion(30.0, 40.0, QDateTime::currentDateTime());

// Later, remove it
btwGraph->removeShadedRegion(regionId);
```

### Example 4: Clearing All Regions

```cpp
// Add several regions
btwGraph->addShadedRegion(10.0, 20.0, QDateTime::currentDateTime());
btwGraph->addShadedRegion(30.0, 40.0, QDateTime::currentDateTime());
btwGraph->addShadedRegion(50.0, 60.0, QDateTime::currentDateTime());

// Clear all at once
btwGraph->clearShadedRegions();
```

### Example 5: Accessing BTW Graph from GraphContainer

```cpp
#include "graphcontainer.h"
#include "btwgraph.h"
#include "graphtype.h"

// Get GraphContainer instance
GraphContainer *container = ...;

// Get the BTW graph from the container
WaterfallGraph *btwGraphBase = container->getWaterfallGraph(GraphType::BTW);
if (btwGraphBase) {
    BTWGraph *btwGraph = qobject_cast<BTWGraph*>(btwGraphBase);
    if (btwGraph) {
        // Now you can use the shaded region API
        int regionId = btwGraph->addShadedRegion(30.0, 40.0, QDateTime::currentDateTime());
    }
}
```

### Example 6: Accessing BTW Graph from GraphLayout

```cpp
#include "graphlayout.h"
#include "graphcontainer.h"
#include "btwgraph.h"
#include "graphtype.h"

// Get GraphLayout instance
GraphLayout *graphLayout = ...;

// Find the first GraphContainer
QList<GraphContainer*> containers = graphLayout->findChildren<GraphContainer*>();
if (!containers.isEmpty()) {
    GraphContainer *container = containers.first();
    
    // Get the BTW graph
    WaterfallGraph *btwGraphBase = container->getWaterfallGraph(GraphType::BTW);
    if (btwGraphBase) {
        BTWGraph *btwGraph = qobject_cast<BTWGraph*>(btwGraphBase);
        if (btwGraph) {
            // Add shaded region
            int regionId = btwGraph->addShadedRegion(30.0, 40.0, QDateTime::currentDateTime());
        }
    }
}
```

### Example 7: Complete Integration Example

```cpp
#include "mainwindow.h"
#include "graphlayout.h"
#include "graphcontainer.h"
#include "btwgraph.h"
#include "graphtype.h"
#include <QPushButton>
#include <QDateTime>
#include <QTimer>

class MyWindow : public QMainWindow
{
    Q_OBJECT

public:
    MyWindow(QWidget *parent = nullptr) : QMainWindow(parent)
    {
        // Create GraphLayout
        graphLayout = new GraphLayout(this, LayoutType::GPW4W, timer, seriesLabelsMap);
        
        // Create button to add shaded region
        QPushButton *addButton = new QPushButton("Add Shaded Region", this);
        connect(addButton, &QPushButton::clicked, this, &MyWindow::onAddShadedRegion);
        
        // Create button to clear regions
        QPushButton *clearButton = new QPushButton("Clear Regions", this);
        connect(clearButton, &QPushButton::clicked, this, &MyWindow::onClearShadedRegions);
    }

private slots:
    void onAddShadedRegion()
    {
        // Find BTW graph
        QList<GraphContainer*> containers = graphLayout->findChildren<GraphContainer*>();
        if (!containers.isEmpty()) {
            GraphContainer *container = containers.first();
            WaterfallGraph *btwGraphBase = container->getWaterfallGraph(GraphType::BTW);
            if (btwGraphBase) {
                BTWGraph *btwGraph = qobject_cast<BTWGraph*>(btwGraphBase);
                if (btwGraph) {
                    // Add shaded region from range 30 to 40
                    int regionId = btwGraph->addShadedRegion(30.0, 40.0, QDateTime::currentDateTime());
                    qDebug() << "Added shaded region with ID:" << regionId;
                }
            }
        }
    }
    
    void onClearShadedRegions()
    {
        // Find BTW graph
        QList<GraphContainer*> containers = graphLayout->findChildren<GraphContainer*>();
        if (!containers.isEmpty()) {
            GraphContainer *container = containers.first();
            WaterfallGraph *btwGraphBase = container->getWaterfallGraph(GraphType::BTW);
            if (btwGraphBase) {
                BTWGraph *btwGraph = qobject_cast<BTWGraph*>(btwGraphBase);
                if (btwGraph) {
                    btwGraph->clearShadedRegions();
                }
            }
        }
    }

private:
    GraphLayout *graphLayout;
    QTimer *timer;
    std::map<GraphType, std::vector<QPair<QString, QColor>>> seriesLabelsMap;
};
```

## Coordinate System

### X-Axis (Range Values)
- **Direction**: Horizontal (left to right)
- **Units**: Range/distance units (e.g., nautical miles, kilometers)
- **Usage**: Defines the horizontal boundaries of the shaded region
- **Example**: `startX = 30.0, endX = 40.0` creates a region from range 30 to 40

### Y-Axis (Time)
- **Direction**: Vertical (top to bottom)
- **Units**: `QDateTime` (timestamps)
- **Usage**: The shaded region always spans the full visible height
  - From `timeMin` (top) to `timeMax` (bottom)
  - The `startY` parameter is stored but not used for Y range calculation

## Important Notes

1. **Vertical Orientation**: Shaded regions are always drawn vertically, spanning all visible timestamps from top to bottom.

2. **X Range Validation**: Ensure `startX < endX`. Invalid ranges will be skipped during drawing.

3. **Automatic Redraw**: Adding, removing, or clearing regions automatically triggers a redraw of the graph.

4. **Region Persistence**: Shaded regions are stored in a `QMap` and persist across graph redraws until explicitly removed.

5. **Visibility**: Regions are only visible when their X range overlaps with the visible range of the graph.

6. **Z-Order**: Shaded regions are drawn with `zValue = 500`, placing them:
   - Above the grid and data points
   - Below markers and interactive elements

7. **Performance**: Regions are recreated from stored data on each `draw()` call, ensuring they remain accurate when the graph view changes.

## Best Practices

1. **Store Region IDs**: Keep track of region IDs if you need to remove specific regions later:
   ```cpp
   std::vector<int> regionIds;
   regionIds.push_back(btwGraph->addShadedRegion(30.0, 40.0, QDateTime::currentDateTime()));
   ```

2. **Validate Range Values**: Ensure your range values are within the graph's data range for best visibility:
   ```cpp
   // Check if range is within visible bounds
   if (startX >= graphMinRange && endX <= graphMaxRange) {
       btwGraph->addShadedRegion(startX, endX, QDateTime::currentDateTime());
   }
   ```

3. **Clear When Done**: Remove regions when they're no longer needed to avoid clutter:
   ```cpp
   // Remove specific region
   btwGraph->removeShadedRegion(regionId);
   
   // Or clear all
   btwGraph->clearShadedRegions();
   ```

4. **Use Meaningful Ranges**: Choose range values that represent meaningful boundaries in your application context.

5. **Coordinate with Zoom Panel**: While shaded regions don't directly use zoom panel sticker values, they work well together to highlight specific range intervals.

## Troubleshooting

### Shaded Region Not Appearing

1. **Check Range Values**: Ensure `startX < endX`:
   ```cpp
   if (startX >= endX) {
       qDebug() << "Invalid range: startX must be less than endX";
       return;
   }
   ```

2. **Verify Graph is Visible**: Ensure the BTW graph is currently displayed in the container:
   ```cpp
   GraphContainer *container = ...;
   if (container->getCurrentDataOption() == GraphType::BTW) {
       // BTW graph is visible
   }
   ```

3. **Check Data Range**: Verify the region's X range overlaps with the graph's visible range:
   ```cpp
   // The region will only be visible if it overlaps with the graph's data range
   ```

4. **Enable Debug Output**: Check debug console for messages:
   ```cpp
   // Look for: "BTWGraph: Drew vertical shaded region..."
   ```

### Region Appears in Wrong Location

- **Verify Coordinate System**: Remember that X values are range values, not screen pixels.
- **Check Data Mapping**: The region uses `mapDataToScreen()` to convert range values to screen coordinates.

### Performance Issues with Many Regions

- **Limit Number of Regions**: Consider limiting the number of simultaneous regions.
- **Clear Unused Regions**: Remove regions that are no longer needed.

## Related APIs

- **BTW Graph**: `BTWGraph` class for BTW-specific functionality
- **Waterfall Graph**: `WaterfallGraph` base class for common graph operations
- **Graph Container**: `GraphContainer` for managing multiple graph types
- **Graph Layout**: `GraphLayout` for managing multiple graph containers

## See Also

- `BTWGraph` class documentation
- `GraphContainer` class documentation
- `GraphLayout` class documentation
- `WaterfallGraph` class documentation

