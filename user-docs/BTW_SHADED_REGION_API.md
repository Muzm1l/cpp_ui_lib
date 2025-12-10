# BTW Shaded Region API Documentation

## Overview

The BTW Shaded Region API allows you to display vertical shaded regions on BTW (Bit Time Waterfall) graphs. A shaded region is a **hatched vertical band** that spans from the top to the bottom of the graph (across all visible timestamps), with horizontal boundaries defined by range values.

Shaded regions are useful for:
- Highlighting specific range intervals (e.g., 30 to 40 nautical miles)
- Visualizing zones of interest
- Marking operational boundaries
- Emphasizing critical range bands

**Key Features:**
- **Hatch pattern**: 45-degree diagonal lines (/) for clear visibility
- **Automatic synchronization**: Regions are automatically synced across all BTW graphs in all containers
- **Efficient rendering**: Uses Qt's pattern brush system - no custom paint events needed

## Visual Representation

Shaded regions appear as vertical bands with diagonal hatch pattern on the BTW graph:

```
     Range (X-axis)
     0    30   40    100
     |    |----|     |
     |    |╱╱╱╱|     |  ← Vertical shaded region
     |    |╱╱╱╱|     |     with diagonal hatch pattern
     |    |╱╱╱╱|     |     (spans all timestamps)
     |    |╱╱╱╱|     |
     |    |╱╱╱╱|     |
     |    |----|     |
     └──────────────┘
     Time (Y-axis)
     (top to bottom)
```

**Key Visual Elements:**
- **Vertical Band**: Spans from top to bottom (all visible timestamps)
- **Horizontal Boundaries**: Defined by `startX` (left) and `endX` (right) range values
- **Hatch Pattern**: 45-degree diagonal lines (forward slash direction)
- **Border**: Medium gray border for visibility

## API Levels

The shaded region API is available at two levels:

### 1. GraphLayout Level (Recommended)

Use `GraphLayout` methods when you want to manage shaded regions across **all BTW graphs in all containers**. This is the recommended approach as it automatically synchronizes regions across all containers.

**Available Methods:**

```cpp
// Add a shaded region to all BTW graphs
QUuid addShadedRegionToAllBTW(qreal startX, qreal endX);

// Remove a shaded region from all BTW graphs by sync ID
bool removeShadedRegionFromAllBTW(const QUuid &syncId);

// Clear all shaded regions from all BTW graphs
void clearAllShadedRegions();

// Get all active shaded regions
std::vector<ShadedRegionSyncData> getAllShadedRegions() const;
```

### 2. BTWGraph Level

Use `BTWGraph` methods when you need to manage shaded regions for a specific BTW graph instance. This is typically used internally, but can be used directly if needed.

**Available Methods:**

```cpp
// Add a shaded region to this BTW graph
int addShadedRegion(qreal startX, qreal endX, const QDateTime &startY);

// Remove a shaded region by its identifier
void removeShadedRegion(int regionId);

// Clear all shaded regions
void clearShadedRegions();
```

## Data Structure

The `ShadedRegionSyncData` struct is used for synchronization:

```cpp
struct ShadedRegionSyncData
{
    int id;                 // Local region ID
    QUuid syncId;           // Global sync identifier across containers
    qreal startX;           // Starting X value (left range boundary)
    qreal endX;             // Ending X value (right range boundary)
    bool isDeleted;         // Flag to mark deleted regions
};
```

## Usage Examples

### Example 1: Adding a Shaded Region (GraphLayout - Recommended)

```cpp
#include "graphlayout.h"
#include <QUuid>

// Get your GraphLayout instance
GraphLayout *graphLayout = ...;

// Add a shaded region from range 30 to 40 to all BTW graphs
QUuid regionId = graphLayout->addShadedRegionToAllBTW(30.0, 40.0);
qDebug() << "Created shaded region with sync ID:" << regionId.toString();
```

### Example 2: Adding Multiple Shaded Regions

```cpp
#include "graphlayout.h"
#include <QUuid>
#include <vector>

GraphLayout *graphLayout = ...;

// Add multiple shaded regions for different range intervals
QUuid region1 = graphLayout->addShadedRegionToAllBTW(10.0, 20.0);
QUuid region2 = graphLayout->addShadedRegionToAllBTW(50.0, 60.0);
QUuid region3 = graphLayout->addShadedRegionToAllBTW(80.0, 90.0);

qDebug() << "Added 3 shaded regions with sync IDs:"
         << region1.toString() << region2.toString() << region3.toString();
```

### Example 3: Removing a Specific Region

```cpp
// Store the sync ID when creating
QUuid regionId = graphLayout->addShadedRegionToAllBTW(30.0, 40.0);

// Later, remove it from all BTW graphs
bool removed = graphLayout->removeShadedRegionFromAllBTW(regionId);
if (removed) {
    qDebug() << "Region removed successfully";
}
```

### Example 4: Clearing All Regions

```cpp
// Clear all shaded regions from all BTW graphs
graphLayout->clearAllShadedRegions();
```

### Example 5: Retrieving All Regions

```cpp
// Get all active shaded regions
std::vector<ShadedRegionSyncData> regions = graphLayout->getAllShadedRegions();

qDebug() << "Active shaded regions:" << regions.size();

// Iterate through regions
for (const auto &region : regions) {
    qDebug() << "Region syncId:" << region.syncId.toString()
             << "X range:" << region.startX << "to" << region.endX;
}
```

### Example 6: Using BTWGraph API Directly (Advanced)

```cpp
#include "btwgraph.h"
#include <QDateTime>

// Get your BTWGraph instance
BTWGraph *btwGraph = ...;

// Create a shaded region from range 30 to 40
QDateTime timestamp = QDateTime::currentDateTime();
int regionId = btwGraph->addShadedRegion(30.0, 40.0, timestamp);

// Remove it later
btwGraph->removeShadedRegion(regionId);

// Or clear all
btwGraph->clearShadedRegions();
```

### Example 7: Complete Integration Example

```cpp
#include "mainwindow.h"
#include "graphlayout.h"
#include <QPushButton>
#include <QUuid>
#include <vector>

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
        
        // Create button to list regions
        QPushButton *listButton = new QPushButton("List Regions", this);
        connect(listButton, &QPushButton::clicked, this, &MyWindow::onListShadedRegions);
    }

private slots:
    void onAddShadedRegion()
    {
        // Add shaded region from range 30 to 40
        QUuid regionId = graphLayout->addShadedRegionToAllBTW(30.0, 40.0);
        qDebug() << "Added shaded region with sync ID:" << regionId.toString();
        
        // Store the ID if you need to remove it later
        m_regionIds.push_back(regionId);
    }
    
    void onClearShadedRegions()
    {
        // Clear all shaded regions from all BTW graphs
        graphLayout->clearAllShadedRegions();
        m_regionIds.clear();
        qDebug() << "Cleared all shaded regions";
    }
    
    void onListShadedRegions()
    {
        // Get all active regions
        std::vector<ShadedRegionSyncData> regions = graphLayout->getAllShadedRegions();
        
        qDebug() << "Active shaded regions:" << regions.size();
        for (const auto &region : regions) {
            qDebug() << "  - Sync ID:" << region.syncId.toString()
                     << "Range:" << region.startX << "to" << region.endX;
        }
    }

private:
    GraphLayout *graphLayout;
    QTimer *timer;
    std::map<GraphType, std::vector<QPair<QString, QColor>>> seriesLabelsMap;
    std::vector<QUuid> m_regionIds;  // Store region IDs for later removal
};
```

## API Reference

### GraphLayout Methods

#### `QUuid addShadedRegionToAllBTW(qreal startX, qreal endX)`

Adds a cross-hatched shaded region to all BTW graphs across all containers.

**Parameters:**
- **`startX`**: Starting X value (left range boundary, e.g., 30.0)
  - Type: `qreal` (double)
  - Represents the left edge of the shaded region in range units
  - Must be less than `endX`
  
- **`endX`**: Ending X value (right range boundary, e.g., 40.0)
  - Type: `qreal` (double)
  - Represents the right edge of the shaded region in range units
  - Must be greater than `startX`

**Returns:**
- `QUuid`: Global sync identifier for the shaded region
  - Use this ID to remove the region later
  - This ID is synchronized across all containers

**Example:**
```cpp
QUuid regionId = graphLayout->addShadedRegionToAllBTW(30.0, 40.0);
```

#### `bool removeShadedRegionFromAllBTW(const QUuid &syncId)`

Removes a shaded region from all BTW graphs by its sync ID.

**Parameters:**
- **`syncId`**: The global sync ID returned by `addShadedRegionToAllBTW()`
  - Type: `QUuid`
  - Must be a valid sync ID that was previously returned

**Returns:**
- `bool`: `true` if the region was found and removed, `false` otherwise

**Example:**
```cpp
bool removed = graphLayout->removeShadedRegionFromAllBTW(regionId);
```

#### `void clearAllShadedRegions()`

Removes all shaded regions from all BTW graphs.

**Example:**
```cpp
graphLayout->clearAllShadedRegions();
```

#### `std::vector<ShadedRegionSyncData> getAllShadedRegions() const`

Gets all active (non-deleted) shaded regions.

**Returns:**
- `std::vector<ShadedRegionSyncData>`: Vector of all active shaded region data

**Example:**
```cpp
std::vector<ShadedRegionSyncData> regions = graphLayout->getAllShadedRegions();
```

### BTWGraph Methods

#### `int addShadedRegion(qreal startX, qreal endX, const QDateTime &startY)`

Adds a shaded region to this specific BTW graph instance.

**Parameters:**
- **`startX`**: Starting X value (left range boundary)
- **`endX`**: Ending X value (right range boundary)
- **`startY`**: Starting Y value (timestamp) - currently stored but not used for Y range calculation

**Returns:**
- `int`: Local region identifier (not synchronized across containers)

#### `void removeShadedRegion(int regionId)`

Removes a shaded region by its local identifier.

**Parameters:**
- **`regionId`**: The local region ID returned by `addShadedRegion()`

#### `void clearShadedRegions()`

Removes all shaded regions from this BTW graph.

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

## Hatch Pattern Implementation

The shaded regions use a **diagonal hatch pattern** (45-degree lines) for clear visibility:

- **Pattern Type**: Custom `QPixmap` with diagonal lines
- **Pattern Size**: 10x10 pixels (controls line spacing)
- **Line Style**: Single diagonal line direction
  - Forward diagonal: `/` (from bottom-left to top-right)
- **Line Color**: Dark gray (100, 100, 100, 180 alpha)
- **Border**: Medium gray border (150, 150, 150, 200 alpha)

**Why This Approach:**
- **Efficient**: Pattern is created once as a `QPixmap`, Qt handles tiling automatically
- **No Custom Paint**: Works directly with `QGraphicsPolygonItem` - no custom `paintEvent` needed
- **Performance**: Qt's rendering engine optimizes pattern rendering
- **Scalable**: Pattern automatically tiles to fill any region size
- **Clean Appearance**: Single-direction lines avoid visual noise from intersections

## Synchronization

When using `GraphLayout` methods, shaded regions are automatically synchronized:

1. **Automatic Sync**: When you add a region via `GraphLayout::addShadedRegionToAllBTW()`, it automatically appears in all BTW graphs across all containers
2. **Sync State**: All regions are tracked in `GraphContainerSyncState` for consistency
3. **Sync ID**: Each region has a global `QUuid` sync ID that is consistent across all containers
4. **Event Propagation**: Changes propagate through the signal/slot system:
   ```
   GraphLayout → GraphContainer → BTWGraph
   ```

## Important Notes

1. **Vertical Orientation**: Shaded regions are always drawn vertically, spanning all visible timestamps from top to bottom.

2. **X Range Validation**: Ensure `startX < endX`. Invalid ranges will be skipped during drawing.

3. **Automatic Redraw**: Adding, removing, or clearing regions automatically triggers a redraw of the graph.

4. **Region Persistence**: Shaded regions are stored and persist across graph redraws until explicitly removed.

5. **Visibility**: Regions are only visible when their X range overlaps with the visible range of the graph.

6. **Z-Order**: Shaded regions are drawn with `zValue = 500`, placing them:
   - Above the grid and data points
   - Below markers and interactive elements

7. **Synchronization**: When using `GraphLayout` API, regions are automatically synced across all BTW graphs. When using `BTWGraph` API directly, regions are local to that graph instance.

8. **Hatch Pattern**: The pattern is rendered efficiently using Qt's brush system - no performance impact from custom painting.

## Best Practices

1. **Use GraphLayout API**: Prefer using `GraphLayout` methods (`addShadedRegionToAllBTW`, `removeShadedRegionFromAllBTW`, `clearAllShadedRegions`) for automatic synchronization across all containers.

2. **Store Sync IDs**: Keep track of sync IDs if you need to remove specific regions later:
   ```cpp
   std::vector<QUuid> regionIds;
   regionIds.push_back(graphLayout->addShadedRegionToAllBTW(30.0, 40.0));
   ```

3. **Validate Range Values**: Ensure your range values are within the graph's data range for best visibility:
   ```cpp
   // Check if range is within visible bounds
   if (startX >= graphMinRange && endX <= graphMaxRange) {
       graphLayout->addShadedRegionToAllBTW(startX, endX);
   }
   ```

4. **Clear When Done**: Remove regions when they're no longer needed to avoid clutter:
   ```cpp
   // Remove specific region
   graphLayout->removeShadedRegionFromAllBTW(regionId);
   
   // Or clear all
   graphLayout->clearAllShadedRegions();
   ```

5. **Use Meaningful Ranges**: Choose range values that represent meaningful boundaries in your application context.

6. **Query Before Adding**: Check existing regions before adding new ones to avoid duplicates:
   ```cpp
   std::vector<ShadedRegionSyncData> existing = graphLayout->getAllShadedRegions();
   // Check if region already exists before adding
   ```

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

3. **Check Data Range**: Verify the region's X range overlaps with the graph's visible range.

4. **Enable Debug Output**: Check debug console for messages:
   ```cpp
   // Look for: "BTWGraph: Drew cross-hatch shaded region..."
   // Look for: "GraphLayout: Added shaded region to all BTW graphs..."
   ```

### Region Not Synchronized Across Containers

- **Use GraphLayout API**: Ensure you're using `GraphLayout::addShadedRegionToAllBTW()` instead of `BTWGraph::addShadedRegion()` directly
- **Check Container Setup**: Verify all containers are part of the same `GraphLayout` instance
- **Verify Sync Signals**: Check that sync signals are properly connected (this is automatic when using GraphLayout API)

### Region Appears in Wrong Location

- **Verify Coordinate System**: Remember that X values are range values, not screen pixels.
- **Check Data Mapping**: The region uses `mapDataToScreen()` to convert range values to screen coordinates.

### Performance Issues with Many Regions

- **Limit Number of Regions**: Consider limiting the number of simultaneous regions.
- **Clear Unused Regions**: Remove regions that are no longer needed.
- **Pattern Efficiency**: The cross-hatch pattern is efficiently rendered by Qt - performance should not be an issue.

## Related APIs

- **BTW Graph**: `BTWGraph` class for BTW-specific functionality
- **Graph Layout**: `GraphLayout` class for managing multiple graph containers
- **Graph Container**: `GraphContainer` for managing multiple graph types
- **Waterfall Graph**: `WaterfallGraph` base class for common graph operations
- **Sync State**: `GraphContainerSyncState` for synchronization state management

## See Also

- `GraphLayout` class documentation
- `BTWGraph` class documentation
- `GraphContainer` class documentation
- `WaterfallGraph` class documentation
- `sharedsyncstate.h` for sync data structures
