<!-- a975e0e8-4b0e-4d7c-a2a2-4370df5785ac 9c839faa-6421-4a27-b268-b7cd87ec4242 -->
# Plan: Dedicated Cursor Layer with Shared State Integration

## Problem

Currently, cursor drawing (crosshair and time axis cursor) is handled in the `overlayScene` and updates are triggered by mouse move events. This causes:

- Cursor updates tied to mouse event frequency (can be jittery)
- No unified cursor synchronization across containers
- Cursor mixed with other overlay elements (selection rectangles, etc.)
- No smooth, consistent update rate

## Solution

Create a dedicated cursor layer (`cursorScene` + `cursorView`) that:

- Reads cursor position from `GraphContainerSyncState` (shared state)
- Updates at a fixed rate (60fps = ~16ms interval) via QTimer
- Is completely independent from the existing overlay layer
- Sits above all other layers (highest z-order)
- Handles both crosshair (mouse position) and time axis cursor (shared time)

## Implementation Steps

### 1. Add Cursor Layer Member Variables to WaterfallGraph Header

**File**: `waterfallgraph.h`

- Add private member variables:
  - `QGraphicsScene *cursorScene` - Dedicated scene for cursor rendering
  - `QGraphicsView *cursorView` - View for cursor layer
  - `QTimer *cursorUpdateTimer` - Timer for fixed-rate cursor updates
  - `QGraphicsLineItem *cursorCrosshairHorizontal` - Horizontal crosshair line
  - `QGraphicsLineItem *cursorCrosshairVertical` - Vertical crosshair line
  - `QGraphicsLineItem *cursorTimeAxisLine` - Time axis cursor line
  - `GraphContainerSyncState *m_cursorSyncState` - Pointer to shared sync state
  - `QPointF m_lastMousePos` - Last known mouse position for crosshair
  - `bool m_cursorLayerEnabled` - Flag to enable/disable cursor layer

### 2. Add Cursor Layer Initialization in Constructor

**File**: `waterfallgraph.cpp` (constructor, after overlayView setup)

- Create `cursorScene` with transparent background
- Create `cursorView` with:
  - Transparent background
  - No scrollbars
  - Transparent to mouse events (`WA_TransparentForMouseEvents`)
  - Positioned above overlayView (call `raise()` after overlayView)
  - Same geometry as widget
- Create cursor graphics items:
  - `cursorCrosshairHorizontal` - Horizontal line, z-value 2000 (above overlay)
  - `cursorCrosshairVertical` - Vertical line, z-value 2000
  - `cursorTimeAxisLine` - Time axis line, z-value 1999
- Initialize all cursor items as invisible
- Create `cursorUpdateTimer` with 16ms interval (60fps)
- Connect timer to update slot

### 3. Add Cursor Layer Update Method

**File**: `waterfallgraph.cpp`

- Create `void updateCursorLayer()` slot method:
  - Read `cursorTime` from `m_cursorSyncState` if available
  - Update time axis cursor position using `mapTimeToY()`
  - Update crosshair position from `m_lastMousePos` (if mouse is over widget)
  - Show/hide cursor items based on validity and widget focus
  - Call `cursorView->update()` to trigger repaint

### 4. Store Shared State Pointer

**File**: `waterfallgraph.h` and `waterfallgraph.cpp`

- Add method: `void setCursorSyncState(GraphContainerSyncState *syncState)`
- Store pointer in `m_cursorSyncState` member
- Connect to timer updates when state is set

### 5. Update Mouse Move Event to Store Position

**File**: `waterfallgraph.cpp` (`mouseMoveEvent`)

- Store mouse position in `m_lastMousePos` (don't update cursor directly)
- Remove direct crosshair update calls from mouseMoveEvent
- Let timer-based update handle cursor rendering

### 6. Handle Resize Events for Cursor Layer

**File**: `waterfallgraph.cpp` (`resizeEvent`)

- Resize `cursorView` to match widget size
- Update cursor scene rect to match widget dimensions
- Ensure cursorView geometry matches widget

### 7. Handle Show Events for Cursor Layer

**File**: `waterfallgraph.cpp` (`showEvent`)

- Resize `cursorView` to match widget size
- Start `cursorUpdateTimer` if cursor layer is enabled
- Ensure proper z-ordering (cursorView on top)

### 8. Update GraphContainer to Pass Sync State

**File**: `graphcontainer.h` and `graphcontainer.cpp`

- Modify `setupWaterfallGraphProperties()` or `createWaterfallGraph()`:
  - Call `graph->setCursorSyncState(m_syncState)` for each waterfall graph
- Ensure sync state pointer is passed to all graph instances

### 9. Update Cursor Time Change Handling

**File**: `waterfallgraph.cpp`

- Modify `setTimeAxisCursor()` to update `m_cursorSyncState->cursorTime` if sync state exists
- Remove direct `timeAxisCursor` updates (now handled by timer)
- Keep method for backward compatibility but delegate to sync state

### 10. Clean Up Cursor Layer in Destructor

**File**: `waterfallgraph.cpp` (`~WaterfallGraph`)

- Stop and delete `cursorUpdateTimer`
- Delete cursor graphics items
- Delete `cursorScene` and `cursorView` (Qt parent-child will handle, but be explicit)

### 11. Add Cursor Layer Enable/Disable Methods

**File**: `waterfallgraph.h` and `waterfallgraph.cpp`

- Add public methods:
  - `void setCursorLayerEnabled(bool enabled)`
  - `bool isCursorLayerEnabled() const`
- When enabled: start timer, show cursor items
- When disabled: stop timer, hide cursor items

### 12. Update Crosshair Methods

**File**: `waterfallgraph.cpp`

- Modify `updateCrosshair()` to only update `m_lastMousePos` (no direct rendering)
- Modify `showCrosshair()` / `hideCrosshair()` to update cursor layer items
- Keep existing API for backward compatibility

### 13. Handle Mouse Enter/Leave for Cursor Layer (Qt 5.14 Compatible)

**File**: `waterfallgraph.h`

- Update `enterEvent` signature for Qt 5.14 compatibility:
  - Change from `void enterEvent(QEnterEvent *event) override;` 
  - To: `void enterEvent(QEvent *event) override;` (Qt 5.14 uses QEvent*, QEnterEvent* is Qt 5.15+)
  - Keep `leaveEvent(QEvent *event) override;` (same for all Qt versions)
  - Use conditional compilation for QEnterEvent header:
    ```cpp
    #if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
    #include <QEnterEvent>
    #endif
    ```


**File**: `waterfallgraph.cpp` (`enterEvent`, `leaveEvent`)

- Update `enterEvent` implementation for Qt 5.14 compatibility:
  - Change parameter from `QEnterEvent *event` to `QEvent *event`
  - Use conditional compilation to handle mouse position:
    ```cpp
    #if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
        QEnterEvent *enterEvent = static_cast<QEnterEvent*>(event);
        m_lastMousePos = enterEvent->pos();
    #else
        // Qt 5.14: use global cursor position and map to widget
        m_lastMousePos = mapFromGlobal(QCursor::pos());
    #endif
    ```

  - On enter: ensure cursor layer is visible and timer is running
  - Show crosshair items if cursor layer is enabled
  - Start cursor update timer if not already running

- Update `leaveEvent` implementation:
  - Hide crosshair when mouse leaves (but keep time axis cursor if valid)
  - Clear `m_lastMousePos` to invalid position
  - Notify cursor time cleared
  - Handle both cursor layer and legacy crosshair modes

- Key points for Qt 5.14 compatibility:
  - Use `QEvent*` as parameter type (works for both Qt 5.14 and 5.15+)
  - Use `#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)` for QEnterEvent-specific code
  - For Qt 5.14: use `mapFromGlobal(QCursor::pos())` to get mouse position
  - For Qt 5.15+: cast to `QEnterEvent*` and use `enterEvent->pos()`

### 14. Ensure Proper Z-Ordering

**File**: `waterfallgraph.cpp` (various)

- Ensure `cursorView` is always on top:
  - In `resizeEvent`: call `cursorView->raise()` after overlayView
  - In `showEvent`: call `cursorView->raise()`
  - In constructor: call `cursorView->raise()` after overlayView setup

### 15. Update GraphLayout Cursor Sync

**File**: `graphlayout.cpp` (`onContainerCursorTimeChanged`)

- Update `m_syncState.cursorTime` and `m_syncState.hasCursorTime` when cursor changes
- This ensures all containers read from the same shared state
- Remove direct `applySharedTimeAxisCursor` calls (now handled by timer reading sync state)

## Key Implementation Details

### Layer Stack (bottom to top)

1. `graphicsView` - Main graph rendering
2. `overlayView` - Interactive elements (selection, existing crosshair)
3. `cursorView` - **NEW** Dedicated cursor layer (highest z-order)

### Update Flow

1. Mouse moves → Store position in `m_lastMousePos`
2. Timer fires (60fps) → `updateCursorLayer()` slot
3. `updateCursorLayer()` reads from `m_cursorSyncState->cursorTime` and `m_lastMousePos`
4. Updates cursor graphics items positions
5. `cursorView->update()` triggers repaint

### Shared State Integration

- `GraphContainerSyncState.cursorTime` - Time for time axis cursor
- `GraphContainerSyncState.hasCursorTime` - Validity flag
- All containers read from same `GraphContainerSyncState` instance
- GraphLayout updates sync state when any container's cursor changes

### Performance Considerations

- 60fps = 16ms interval (smooth, invisible to eye)
- Cursor layer only updates graphics items (no scene clearing)
- Transparent to mouse events (no event processing overhead)
- Independent of main graph rendering

### Backward Compatibility

- Keep existing `setTimeAxisCursor()`, `updateCrosshair()` methods
- They now update state/position, timer handles rendering
- Existing API consumers continue to work

## Testing Checklist

- [ ] Cursor layer appears above all other layers
- [ ] Time axis cursor syncs across all containers
- [ ] Crosshair follows mouse smoothly at 60fps
- [ ] Cursor updates continue even when mouse is stationary
- [ ] Cursor layer is transparent to mouse events
- [ ] Resize events properly resize cursor layer
- [ ] Cursor layer starts/stops with widget visibility
- [ ] Multiple containers show synchronized time axis cursor
- [ ] Performance is smooth (no jitter, consistent 60fps)

### To-dos

- [ ] 