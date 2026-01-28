# TimelineView Slider Architecture and Design

## Overview

The TimelineView slider is a vertical time range selector that allows users to navigate through a 12-hour time window. The slider represents a time interval (e.g., 15 minutes) and can be dragged to select different time ranges. The architecture is designed for performance, with a clear separation between geometry calculations, state management, and rendering.

## Architecture Components

### 1. SliderGeometry (Static Helper Class)

**Purpose**: Centralized calculations for slider dimensions and position conversions.

**Location**: `timelineview.h` (lines 44-77), `timelineview.cpp` (lines 12-78)

**Key Methods**:
- `calculateSliderRect()` - Calculates the QRect for rendering the slider
- `calculateSliderHeight()` - Computes slider height based on time interval
- `calculateSliderYFromTime()` - Converts time window to Y position
- `calculateTimeWindowFromY()` - Converts Y position to time window
- `getSliderBounds()` - Returns min/max Y positions for clamping

**Design Principles**:
- All methods are static (no instance state)
- Pure functions: inputs → outputs with no side effects
- Handles all coordinate system conversions (time ↔ pixels)

**Key Constants**:
- `TWELVE_HOURS_IN_MINUTES = 720` - Total time range
- `MINIMUM_SLIDER_HEIGHT = 20` - Minimum pixel height for visibility

**Example Calculation**:
```cpp
// Slider height is proportional to time interval
int intervalMinutes = timeInterval.hour() * 60 + timeInterval.minute();
double rectangleHeightRatio = intervalMinutes / 720.0;
int rectangleHeight = rectangleHeightRatio * widgetHeight;
return qMax(rectangleHeight, MINIMUM_SLIDER_HEIGHT);
```

### 2. SliderState (State Manager Class)

**Purpose**: Encapsulates slider position, time window, and drag state.

**Location**: `timelineview.h` (lines 79-110), `timelineview.cpp` (lines 80-256)

**State Variables**:
- `m_yPosition` - Current pixel Y position (source of truth)
- `m_timeWindow` - TimeSelectionSpan representing selected time range
- `m_isDragging` - Boolean flag for drag state
- `m_dragStartMousePos` - Mouse position when drag started
- `m_dragStartSliderY` - Slider Y position when drag started

**Key Methods**:

#### Position Management
- `setYPosition()` - Sets Y position and optionally syncs time window
- `getYPosition()` - Returns current Y position

#### Time Window Management
- `setTimeWindow()` - Sets time window and syncs position
- `getTimeWindow()` - Returns current time window

#### Drag State Management
- `startDrag()` - Initializes drag state
- `updateDrag()` - Updates position during drag
- `endDrag()` - Finalizes drag and syncs time window
- `isDragging()` - Returns drag state

#### Synchronization
- `syncTimeWindowFromPosition()` - Calculates time window from Y position
- `syncPositionFromTimeWindow()` - Calculates Y position from time window
- `clampToBounds()` - Ensures position stays within valid range

**Design Principles**:
- **Single Source of Truth**: `m_yPosition` is the primary state
- **Bidirectional Sync**: Position ↔ Time window conversions
- **Drag Handling**: Tracks drag delta for smooth interaction
- **Boundary Enforcement**: Always clamps to valid ranges

**Time Range Logic**:
- **Normal Mode**: 12 hours ago to now (720 minutes)
- **Drag Mode**: Application start time to now (when dragging with valid start time)
- **Inverted Y-axis**: Y=0 represents "now" (top), Y=height represents past (bottom)

### 3. TimelineVisualizerWidget (Main Widget)

**Purpose**: The QWidget that renders the timeline and handles user interaction.

**Location**: `timelineview.h` (lines 112-291), `timelineview.cpp` (lines 258-2206)

**Key Members**:
- `m_sliderState` - SliderState instance managing slider state
- `m_cachedBackground` - QPixmap cache for static background elements
- `m_backgroundNeedsRedraw` - Flag to invalidate cache
- `m_sliderVisible` - Visibility flag for slider

## Rendering Architecture

### Paint Flow

The rendering system uses a **two-layer approach** for optimal performance:

```
┌─────────────────────────────────────┐
│     paintEvent() Entry Point        │
└──────────────┬──────────────────────┘
               │
               ▼
┌─────────────────────────────────────┐
│  1. Segment Management              │
│     - Create/remove segments        │
│     - Update segment positions      │
└──────────────┬──────────────────────┘
               │
               ▼
┌─────────────────────────────────────┐
│  2. Background Cache Check          │
│     - Check if cache is valid       │
│     - Regenerate if needed          │
└──────────────┬──────────────────────┘
               │
               ▼
┌─────────────────────────────────────┐
│  3. renderBackgroundToCache()       │
│     - Draw static elements only     │
│     - Timeline segments (ticks)     │
│     - Timestamp labels              │
│     - Border                        │
│     - NO SLIDER (critical!)         │
└──────────────┬──────────────────────┘
               │
               ▼
┌─────────────────────────────────────┐
│  4. Blit Cached Background          │
│     painter.drawPixmap(0,0, cache)  │
└──────────────┬──────────────────────┘
               │
               ▼
┌─────────────────────────────────────┐
│  5. Draw Dynamic Elements           │
│     - Slider (directly, not cached) │
│     - Crosshair timestamp label     │
└─────────────────────────────────────┘
```

### paintEvent() Implementation

**Location**: `timelineview.cpp` (lines 779-878)

**Key Steps**:

1. **Segment Management** (lines 784-850)
   - Removes segments that have scrolled out of view
   - Creates new segments as needed
   - Updates segment positions based on smooth offset

2. **Background Cache Management** (lines 852-856)
   ```cpp
   if (m_backgroundNeedsRedraw || m_cachedBackground.size() != rect().size())
   {
       renderBackgroundToCache();
   }
   ```

3. **Blit Cached Background** (line 859)
   ```cpp
   painter.drawPixmap(0, 0, m_cachedBackground);
   ```
   - Fast operation: just copies pre-rendered pixmap
   - Contains all static elements (segments, labels, border)

4. **Draw Slider** (lines 863-870)
   ```cpp
   if (m_sliderVisible)
   {
       QRect sliderRect = SliderGeometry::calculateSliderRect(
           rect().height(), rect().width(), m_timeLineLength,
           m_sliderState.getYPosition());
       QColor sliderColor(255, 255, 255, 128); // 50% opacity white
       painter.fillRect(sliderRect, sliderColor);
   }
   ```
   - **Critical**: Slider is drawn directly, NOT from cache
   - This enables immediate updates during drag without cache regeneration
   - Uses semi-transparent white (50% opacity)

5. **Draw Crosshair Label** (lines 872-877)
   - Lightweight overlay for mouse hover timestamp
   - Only drawn when visible and valid

### renderBackgroundToCache() Implementation

**Location**: `timelineview.cpp` (lines 715-777)

**Purpose**: Pre-renders all static elements to a QPixmap for fast blitting.

**What Gets Cached**:
- ✅ Timeline segments (ticks)
- ✅ Timestamp labels (regular intervals)
- ✅ Navtime labels (if available)
- ✅ Border
- ❌ **Slider** (explicitly NOT cached - see line 764 comment)

**Why Slider is NOT Cached**:
- Slider position changes frequently during drag
- Cache regeneration is expensive (requires redrawing all segments)
- Drawing slider directly is fast (single fillRect call)
- Enables smooth 60fps dragging without performance issues

**Cache Invalidation Triggers**:
- Widget resize (`resizeEvent`)
- Segment changes (add/remove)
- Time interval changes
- Background content changes
- Explicit flag: `m_backgroundNeedsRedraw = true`

## Mouse Interaction Flow

### Drag Sequence

```
┌─────────────────────────────────────┐
│   mousePressEvent()                 │
│   - Check if click is on slider     │
│   - Call m_sliderState.startDrag()  │
│   - Set cursor to ClosedHandCursor  │
└──────────────┬──────────────────────┘
               │
               ▼
┌─────────────────────────────────────┐
│   mouseMoveEvent() (during drag)    │
│   - Call m_sliderState.updateDrag() │
│   - Update time window               │
│   - Call repaint() for immediate    │
│     visual feedback                  │
│   - Emit timeScopeChanged signal    │
└──────────────┬──────────────────────┘
               │
               ▼
┌─────────────────────────────────────┐
│   mouseReleaseEvent()               │
│   - Call m_sliderState.endDrag()    │
│   - Check if at top (snap to follow)│
│   - Switch to FROZEN_MODE or        │
│     FOLLOW_MODE                     │
│   - Invalidate background cache     │
│   - Emit timelineViewModeChanged    │
└─────────────────────────────────────┘
```

### mousePressEvent() Details

**Location**: `timelineview.cpp` (lines 1155-1176)

**Logic**:
1. Check if left button pressed and not already dragging
2. Calculate current slider rectangle using `SliderGeometry::calculateSliderRect()`
3. Check if mouse position is within slider rectangle
4. If yes:
   - Call `m_sliderState.startDrag(pos)`
   - Set cursor to `Qt::ClosedHandCursor`
   - Accept event

### mouseMoveEvent() Details

**Location**: `timelineview.cpp` (lines 1178-1230)

**Two Modes**:

#### During Drag (lines 1180-1216)
```cpp
if (m_sliderState.isDragging())
{
    // Update slider position
    m_sliderState.updateDrag(event->pos(), rect().height(), 
                             m_timeLineLength, m_applicationStartTime);
    
    // Update overlay if present
    if (m_manoeuvreOverlay) { ... }
    
    // Trigger immediate repaint (smooth dragging)
    repaint();
    
    // Emit signal for external components
    emitTimeScopeChanged();
}
```

**Key Points**:
- Uses `repaint()` (not `update()`) for immediate rendering
- Does NOT invalidate background cache (performance optimization)
- Updates related overlays (manoeuvre, crosshair)
- Emits signals for synchronization

#### Hover Mode (lines 1218-1228)
```cpp
else
{
    // Update cursor based on hover
    QRect sliderRect = SliderGeometry::calculateSliderRect(...);
    setCursor(sliderRect.contains(pos) ? Qt::OpenHandCursor : Qt::ArrowCursor);
}
```

### mouseReleaseEvent() Details

**Location**: `timelineview.cpp` (lines 1232-1305)

**Key Logic**:

1. **End Drag** (line 1237)
   ```cpp
   m_sliderState.endDrag(rect().height(), m_timeLineLength, m_applicationStartTime);
   ```

2. **Snap Detection** (lines 1239-1260)
   - If slider Y position ≤ 5 pixels from top
   - Snap to top (Y=0)
   - Switch to `FOLLOW_MODE`
   - Set time window to latest data

3. **Frozen Mode** (lines 1261-1277)
   - If slider not at top
   - Switch to `FROZEN_MODE`
   - Preserve time window from drag

4. **Cache Invalidation** (line 1283)
   ```cpp
   m_backgroundNeedsRedraw = true;
   ```
   - Next paintEvent will regenerate background cache
   - Ensures static elements reflect new time window

## Performance Optimizations

### 1. Background Caching

**Problem**: Redrawing all timeline segments on every paint is expensive.

**Solution**: Cache static elements in `m_cachedBackground` QPixmap.

**Benefits**:
- Background blit is O(1) operation
- Only regenerates when content actually changes
- Reduces CPU usage during animation

**Trade-offs**:
- Memory: One pixmap copy of widget size
- Complexity: Must track cache validity

### 2. Slider Not Cached

**Problem**: Slider position changes every frame during drag.

**Solution**: Draw slider directly in `paintEvent()`, not in cache.

**Benefits**:
- No cache regeneration during drag
- Smooth 60fps dragging
- Single `fillRect()` call is very fast

**Implementation**:
```cpp
// In renderBackgroundToCache() - line 764
// CRITICAL FIX: Slider is NOT drawn in background cache

// In paintEvent() - lines 863-870
// Draw slider directly (not from cache)
QRect sliderRect = SliderGeometry::calculateSliderRect(...);
painter.fillRect(sliderRect, sliderColor);
```

### 3. Immediate Repaint During Drag

**Problem**: Using `update()` queues a paint event, causing lag.

**Solution**: Use `repaint()` during drag for immediate rendering.

**Code**:
```cpp
// In mouseMoveEvent() during drag - line 1210
repaint(); // Immediate, not queued
```

**Trade-off**: May cause higher CPU usage, but ensures smooth interaction.

### 4. Segment Lazy Creation

**Problem**: Creating all timeline segments upfront is wasteful.

**Solution**: Create segments on-demand as they scroll into view.

**Implementation**:
- Remove segments that scroll out of view (lines 788-806)
- Create new segments as needed (lines 808-849)
- Maintain buffer of 2 segments beyond visible range

## Coordinate System

### Y-Axis Mapping

The slider uses an **inverted Y-axis**:

```
Y = 0 (top)     → Represents "now" (current time)
Y = height      → Represents "12 hours ago" (past)
```

**Conversion Formula**:
```cpp
// Position ratio: 0.0 = 12 hours ago, 1.0 = now
double positionRatio = 1.0 - (static_cast<double>(m_yPosition) / static_cast<double>(widgetHeight));

// Minutes from start
int minutesFromStart = static_cast<int>(positionRatio * totalMinutes);

// Window end time (top edge of slider)
QDateTime windowEnd = rangeStart.addSecs(minutesFromStart * 60);
```

### Time Window Calculation

The slider represents a time window with:
- **Top edge (Y position)**: Window end time
- **Bottom edge (Y + height)**: Window start time
- **Height**: Proportional to time interval

**Example**:
- Time interval: 15 minutes
- Widget height: 1000 pixels
- 12 hours = 720 minutes
- Slider height = (15 / 720) * 1000 = ~20.8 pixels

## Integration Points

### Signals

**visibleTimeWindowChanged** (line 288)
- Emitted when slider position changes
- Carries `TimeSelectionSpan` with new time window
- Used by external components (e.g., waterfall graph) to sync

**timelineViewModeChanged** (line 289)
- Emitted when mode switches between FOLLOW_MODE and FROZEN_MODE
- Used by parent TimelineView for UI updates

### External Synchronization

**setTimeWindowSilent()** (line 172)
- Sets time window without emitting signals
- Used for external synchronization (prevents feedback loops)

**setVisibleTimeWindow()** (line 157)
- Public method to set time window from external source
- Updates slider state and position
- Emits signals (unless called via silent variant)

## Key Design Decisions

### 1. Why SliderState Instead of Direct Members?

**Benefits**:
- Encapsulation: All slider logic in one place
- Testability: Can test state management independently
- Maintainability: Clear separation of concerns
- Reusability: Could be used in other widgets

### 2. Why SliderGeometry as Static Class?

**Benefits**:
- No instance overhead
- Pure functions (easier to reason about)
- Centralized calculations (single source of truth)
- Easy to test independently

### 3. Why Two-Layer Rendering?

**Benefits**:
- Performance: Cache static, draw dynamic
- Flexibility: Easy to add/remove dynamic elements
- Clarity: Clear separation of concerns

### 4. Why Inverted Y-Axis?

**Rationale**:
- Intuitive: Top = now, bottom = past
- Matches timeline visualization convention
- Aligns with user mental model

## Code Locations Reference

| Component | Header | Implementation |
|-----------|--------|----------------|
| SliderGeometry | `timelineview.h:44-77` | `timelineview.cpp:12-78` |
| SliderState | `timelineview.h:79-110` | `timelineview.cpp:80-256` |
| TimelineVisualizerWidget | `timelineview.h:112-291` | `timelineview.cpp:258-2206` |
| paintEvent() | - | `timelineview.cpp:779-878` |
| renderBackgroundToCache() | - | `timelineview.cpp:715-777` |
| mousePressEvent() | - | `timelineview.cpp:1155-1176` |
| mouseMoveEvent() | - | `timelineview.cpp:1178-1230` |
| mouseReleaseEvent() | - | `timelineview.cpp:1232-1305` |

## Summary

The TimelineView slider architecture follows a clean separation of concerns:

1. **SliderGeometry**: Pure calculation functions (time ↔ pixels)
2. **SliderState**: State management and synchronization
3. **TimelineVisualizerWidget**: Rendering and user interaction

The rendering system uses a two-layer approach:
- **Background cache**: Static elements (fast blit)
- **Direct drawing**: Dynamic elements (slider, crosshair)

This design enables:
- ✅ Smooth 60fps dragging
- ✅ Efficient rendering (cached background)
- ✅ Clear code organization
- ✅ Easy maintenance and testing
- ✅ Flexible time range selection


