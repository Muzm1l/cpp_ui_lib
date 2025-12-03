# Overlay System Architecture Analysis

## Overview

The overlay system in the BTW graph provides a **two-layer rendering architecture** that separates:
1. **Main Graphics Scene** (`graphicsScene`) - Static data visualization (scatterplots, blue markers, symbols)
2. **Overlay Scene** (`overlayScene`) - Interactive elements (green markers, crosshair, selection rectangles)

This separation allows interactive elements to be manipulated without redrawing the entire graph.

---

## Architecture Components

### 1. **WaterfallGraph Base Class**

#### Dual Scene Setup
```cpp
// Main scene for data visualization
QGraphicsScene *graphicsScene;      // Cleared and redrawn on each draw()
QGraphicsView *graphicsView;        // Main view showing data

// Overlay scene for interactive elements
QGraphicsScene *overlayScene;       // Persistent, not cleared
QGraphicsView *overlayView;         // Transparent overlay on top
```

#### Key Properties:
- **Overlay View is Transparent**: `setAttribute(Qt::WA_TransparentForMouseEvents, true)`
  - Mouse events pass through to `WaterfallGraph` widget
  - `WaterfallGraph` handles mouse events and coordinates with overlay
  
- **Overlay View Positioning**: 
  - Positioned absolutely on top of main view: `overlayView->setGeometry(QRect(0, 0, width, height))`
  - Raised above main view: `overlayView->raise()`
  - Same coordinate system as main scene

- **Overlay Scene Contents**:
  - Selection rectangles
  - Crosshair lines
  - Time axis cursor
  - **Interactive markers** (managed by `BTWInteractiveOverlay`)

---

### 2. **BTWInteractiveOverlay Class**

#### Purpose
Manages interactive markers (green markers) on the BTW graph's overlay scene.

#### Key Responsibilities:
1. **Marker Lifecycle Management**
   - Create markers (`addDataPointMarker`, `addReferenceLineMarker`, etc.)
   - Remove markers (`removeMarker`, `clearAllMarkers`)
   - Track markers in `QList<InteractiveGraphicsItem*> m_markers`

2. **Signal Routing**
   - Connects `InteractiveGraphicsItem` signals to overlay signals
   - Routes events: `itemMoved` → `markerMoved`, `itemRotated` → `markerRotated`
   - Emits signals to `BTWGraph` for external integration

3. **Bearing Rate Display**
   - Creates text labels showing rotation angle (R/L prefix)
   - Updates labels when marker moves/rotates
   - Stores label items in `m_bearingRateItems` map

#### Marker Creation Flow:
```
User clicks on BTW graph
  ↓
BTWGraph::onMouseClick()
  ↓
BTWInteractiveOverlay::addDataPointMarker()
  ↓
Creates InteractiveGraphicsItem
  ↓
Sets custom draw function (green circle + line)
  ↓
Stores timestamp in marker data (key 0)
  ↓
Stores delta in marker data (key 1, initially 0.0)
  ↓
Adds to overlayScene
  ↓
Connects signals (itemMoved, itemRotated, regionClicked)
  ↓
Creates bearing rate box
  ↓
Emits markerAdded signal
  ↓
BTWGraph::onMarkerAdded() receives signal
  ↓
Emits manualMarkerPlaced signal
  ↓
GraphLayout::onBTWManualMarkerPlaced()
  ↓
addBTWSymbolToAllGraphs() → Magenta circles in other graphs
```

---

### 3. **InteractiveGraphicsItem Class**

#### Purpose
A `QGraphicsItem` that supports dragging and rotation with custom drawing.

#### Key Features:

1. **Custom Drawing**
   - Uses `std::function<void(QPainter*, const QRectF&)> m_customDrawFunction`
   - Allows each marker type to have unique appearance
   - Green markers use lambda to draw circle + line

2. **Interaction Regions**
   - **Drag Region**: Central area for moving the marker
   - **Rotate Regions**: Endpoints of the line for rotation
   - Regions can be shown/hidden via `setShowDragRegion()`, `setShowRotateRegion()`

3. **Mouse Event Handling**
   - `mousePressEvent`: Detects which region was clicked
   - `mouseMoveEvent`: Handles dragging or rotation based on region
   - `mouseReleaseEvent`: Finalizes movement/rotation, emits signals
   - `hoverEnterEvent`/`hoverMoveEvent`: Updates cursor based on region

4. **Data Storage**
   - Uses `QGraphicsItem::setData(key, value)` to store:
     - Key 0: `QDateTime` timestamp
     - Key 1: `qreal` delta (rotation angle / 10.0)

5. **Signals**
   - `itemMoved(QPointF newPosition)`: Emitted when marker is dragged
   - `itemRotated(qreal angle)`: Emitted when marker is rotated
   - `regionClicked(InteractionRegion, QPointF)`: Emitted when region is clicked

---

## Data Flow: Green Marker → Magenta Circles

### When Green Marker is Placed:
```
1. User clicks → BTWGraph::onMouseClick()
2. Creates green marker in overlayScene
3. markerAdded signal → BTWGraph::onMarkerAdded()
4. Extracts timestamp from marker data
5. Emits manualMarkerPlaced(timestamp, position)
6. GraphContainer::onBTWManualMarkerPlaced()
7. GraphLayout::onBTWManualMarkerPlaced()
8. Calculates range from X position
9. Calls addBTWSymbolToAllGraphs(timestamp, 0.0)
10. For each graph type (FDW, BDW, BRW, LTW, RTW, FTW):
    - Finds data point at timestamp
    - Adds magenta circle (BTW symbol) to data source
    - Redraws graph
```

### When Green Marker is Moved:
```
1. User drags marker → InteractiveGraphicsItem::mouseMoveEvent()
2. Updates marker position
3. Emits itemMoved signal
4. BTWInteractiveOverlay::onMarkerMoved()
5. Updates bearing rate box position
6. Emits markerMoved signal
7. BTWGraph::onMarkerMoved()
8. Extracts new timestamp from new position
9. Emits manualMarkerPlaced(newTimestamp, newPosition)
10. Same flow as placement → magenta circles update
```

### When Green Marker is Rotated:
```
1. User drags rotation handle → InteractiveGraphicsItem::mouseMoveEvent()
2. Updates marker rotation
3. Emits itemRotated signal
4. BTWInteractiveOverlay::onMarkerRotated()
5. Updates bearing rate box (shows new angle)
6. Stores delta in marker data: marker->setData(1, rotation / 10.0)
7. Emits markerRotated signal
8. BTWGraph::onMarkerRotated()
9. Extracts timestamp from marker
10. Emits manualMarkerPlaced(timestamp, position)
11. Same flow → magenta circles update
```

---

## Key Design Decisions

### 1. **Why Two Separate Scenes?**

**Problem**: If markers were in `graphicsScene`, every marker move would require:
- Clearing entire scene
- Redrawing all data points
- Redrawing all markers
- Very slow and flickering

**Solution**: Overlay scene is persistent
- Only interactive elements in overlay
- Main scene redraws only when data changes
- Smooth, responsive interaction

### 2. **Why Transparent Overlay View?**

**Problem**: Need mouse events to reach `WaterfallGraph` for:
- Graph navigation (pan, zoom)
- Selection rectangles
- Crosshair updates

**Solution**: `overlayView->setAttribute(Qt::WA_TransparentForMouseEvents, true)`
- Mouse events pass through to widget
- Widget coordinates with overlay for marker interaction
- Best of both worlds: interaction + navigation

### 3. **Why Custom Draw Functions?**

**Problem**: Different marker types need different appearances
- Data point: Green circle + line
- Reference line: Line with label
- Annotation: Text box

**Solution**: `setCustomDrawFunction()` lambda
- Each marker type defines its own drawing
- Flexible and extensible
- No need for marker subclass hierarchy

### 4. **Why Store Data in QGraphicsItem?**

**Problem**: Need to associate metadata with markers:
- Timestamp (for magenta circle placement)
- Delta (rotation angle)
- Series label

**Solution**: `QGraphicsItem::setData(key, value)`
- Standard Qt mechanism
- Type-safe with `QVariant`
- Easy to retrieve later

---

## Marker Types Comparison

| Type | Scene | Appearance | Purpose | Persistence |
|------|-------|------------|---------|-------------|
| **Green Marker** | `overlayScene` | Green circle + line | Interactive placement | Until removed |
| **Blue Marker** | `graphicsScene` | Blue circle + line + label | From data source | Until data cleared |
| **Magenta Circle** | `graphicsScene` | Magenta circle | BTW symbol in other graphs | Until data cleared |

---

## Coordinate System

Both scenes use the **same coordinate system**:
- X-axis: Range (data value)
- Y-axis: Time (timestamp)
- Mapping: `mapDataToScreen(range, timestamp)` converts data → screen coordinates
- Overlay markers use scene coordinates directly

---

## Performance Considerations

### Advantages:
1. **No Redraw on Interaction**: Moving green marker doesn't redraw data
2. **Fast Updates**: Only overlay scene updates during drag/rotate
3. **Smooth Animation**: 60 FPS possible for marker movement

### Potential Issues:
1. **Coordinate Synchronization**: Must ensure overlay and main scene stay aligned
2. **Memory**: Overlay items persist (but typically few items)
3. **Z-Order**: Must manage z-values correctly (overlay > main)

---

## Future Enhancements

1. **Marker Persistence**: Save green markers to data source on finalization
2. **Multi-Marker Selection**: Select and manipulate multiple markers
3. **Marker Snapping**: Snap to data points or grid
4. **Undo/Redo**: Track marker operations for undo

---

## Code Locations

- **Overlay Setup**: `waterfallgraph.cpp:95-147`
- **BTW Overlay Manager**: `btwinteractiveoverlay.h/cpp`
- **Interactive Item**: `interactivegraphicsitem.h/cpp`
- **BTW Integration**: `btwgraph.cpp:334-352` (setup), `btwgraph.cpp:368-400` (onMarkerAdded)
- **Signal Routing**: `graphcontainer.cpp:866-870`, `graphlayout.cpp:1488-1555`


