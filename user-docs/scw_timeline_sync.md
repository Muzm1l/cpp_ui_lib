# SCW TimelineView and Waterfall TimelineView Signal Synchronization

## Overview

The SCW (SCWWindow) timeline view is synchronized with the waterfall graph timeline views in GraphLayout. This ensures that when you interact with either timeline (SCW or waterfall), both stay in sync.

## Signal Flow Architecture

### 1. SCW TimelineView → Waterfall Graphs (SCW → GraphLayout)

When the SCW timeline view changes (user drags slider, changes interval, etc.):

```
SCW TimelineView
    ↓ (emits TimeScopeChanged)
GraphLayout::syncExternalTimelineView()
    ↓ (connects to)
GraphContainer::onTimeScopeChanged()
    ↓ (calls)
WaterfallGraph::setTimeRange()
    ↓ (updates)
WaterfallGraph time range
```

**Code Location:** `graphlayout.cpp:1368-1369`
```cpp
connect(externalTimelineView, &TimelineView::TimeScopeChanged,
        container, &GraphContainer::onTimeScopeChanged, Qt::UniqueConnection);
```

### 2. Waterfall TimelineView → SCW TimelineView (GraphLayout → SCW)

When a waterfall graph timeline view changes:

```
GraphLayout TimelineView
    ↓ (emits TimeScopeChanged)
GraphLayout::syncExternalTimelineView()
    ↓ (connects to)
SCW TimelineView::setVisibleTimeWindow()
    ↓ (updates)
SCW TimelineView slider position
```

**Code Location:** `graphlayout.cpp:1350-1353`
```cpp
connect(externalTimelineView, &TimelineView::TimeScopeChanged,
        graphTimelineView, &TimelineView::setVisibleTimeWindow, Qt::UniqueConnection);
connect(graphTimelineView, &TimelineView::TimeScopeChanged,
        externalTimelineView, &TimelineView::setVisibleTimeWindow, Qt::UniqueConnection);
```

### 3. Bidirectional Synchronization

The synchronization is **bidirectional** for:
- **Time Scope Changes** (slider position/time window)
- **Time Interval Changes** (15 min, 30 min, 1 hour, etc.)
- **Absolute/Relative Time Mode** (Abs/Rel button)

## Detailed Signal Connections

### Time Scope Synchronization

**SCW → Waterfall:**
- Signal: `TimelineView::TimeScopeChanged` (from SCW)
- Slot: `GraphContainer::onTimeScopeChanged`
- Effect: Updates waterfall graph time range via `WaterfallGraph::setTimeRange()`

**Waterfall → SCW:**
- Signal: `TimelineView::TimeScopeChanged` (from GraphLayout timeline)
- Slot: `TimelineView::setVisibleTimeWindow` (on SCW timeline)
- Effect: Updates SCW timeline slider position

### Time Interval Synchronization

**SCW → Waterfall:**
- Signal: `TimelineView::TimeIntervalChanged` (from SCW)
- Slot: `GraphContainer::onTimeIntervalChanged`
- Effect: Updates waterfall graph time interval

**Waterfall → SCW:**
- Signal: `TimelineView::TimeIntervalChanged` (from GraphLayout timeline)
- Slot: `TimelineView::setTimeLineLength` (on SCW timeline)
- Effect: Updates SCW timeline interval

### Absolute/Relative Time Mode Synchronization

**SCW → Waterfall:**
- Signal: `TimelineView::AbsoluteTimeModeChanged` (from SCW)
- Slot: `TimelineView::setIsAbsoluteTime` (on GraphLayout timeline)
- Effect: Updates waterfall timeline Abs/Rel mode

**Waterfall → SCW:**
- Signal: `TimelineView::AbsoluteTimeModeChanged` (from GraphLayout timeline)
- Slot: `TimelineView::setIsAbsoluteTime` (on SCW timeline)
- Effect: Updates SCW timeline Abs/Rel mode

## Setup Process

The synchronization is established in `MainWindow::setupSCWWindow()`:

```cpp
// Sync SCW timeline view with GraphLayout timeline views
if (graphgrid && scwWindow->getTimelineView())
{
    graphgrid->syncExternalTimelineView(scwWindow->getTimelineView());
}
```

**Code Location:** `mainwindow.cpp:817-821`

## Implementation Details

### GraphLayout::syncExternalTimelineView()

This method sets up all the bidirectional connections:

1. **Connects SCW timeline to all GraphLayout timeline views:**
   - Time scope changes (bidirectional)
   - Time interval changes (bidirectional)
   - Absolute/relative mode changes (bidirectional)

2. **Connects SCW timeline to all visible GraphContainers:**
   - Time scope changes → `GraphContainer::onTimeScopeChanged`
   - Time interval changes → `GraphContainer::onTimeIntervalChanged`

**Code Location:** `graphlayout.cpp:1325-1378`

### GraphContainer::onTimeScopeChanged()

When called, this method:
1. Updates the waterfall graph's time range: `WaterfallGraph::setTimeRange()`
2. Triggers a redraw: `WaterfallGraph::draw()`
3. Updates sync state: `m_syncState->currentTimeScope`
4. Emits signal for GraphLayout hub: `TimeScopeChanged(selection)`

**Code Location:** `graphcontainer.cpp:1241-1275`

### Preventing Feedback Loops

The synchronization uses `setVisibleTimeWindow()` (silent) and `setTimeWindowSilent()` to prevent feedback loops:

- `setVisibleTimeWindow()` - Updates without emitting signals
- `setTimeWindowSilent()` - Updates without emitting signals

**Code Location:** `timelineview.cpp:1954-1961`, `timelineview.cpp:1304-1324`

## Sync State Pattern

The synchronization also uses a shared `GraphContainerSyncState` object:

- **SCWWindow** receives sync state in constructor
- **GraphLayout** maintains sync state
- **TimelineView** reads from sync state in `onTimerTick()`

This ensures all timeline views stay synchronized even when signals are temporarily disconnected.

**Code Location:** `scwwindow.cpp:1215-1271` (SCWWindow::onTimerTick)

## Visual Behavior

When synchronized:

1. **Dragging SCW timeline slider:**
   - SCW timeline updates immediately
   - Waterfall graphs update their time ranges
   - GraphLayout timeline views update their sliders

2. **Dragging waterfall timeline slider:**
   - Waterfall timeline updates immediately
   - SCW timeline slider updates to match
   - All waterfall graphs in GraphLayout update

3. **Changing time interval:**
   - Clicking interval button on either timeline updates both
   - All waterfall graphs adjust their intervals

4. **Toggling Abs/Rel mode:**
   - Clicking Abs/Rel button on either timeline updates both
   - All timeline views switch between absolute and relative time display

## Testing the Synchronization

To verify synchronization is working:

1. **Enable SCW Window:**
   - The SCW tab should appear in the main window
   - SCW timeline view should be visible on the left side

2. **Test Time Scope Sync:**
   - Drag the SCW timeline slider
   - Observe waterfall graphs update their time ranges
   - Drag a waterfall timeline slider
   - Observe SCW timeline slider updates

3. **Test Interval Sync:**
   - Click interval button on SCW timeline
   - Observe waterfall timeline intervals update
   - Click interval button on waterfall timeline
   - Observe SCW timeline interval updates

4. **Test Mode Sync:**
   - Click Abs/Rel button on SCW timeline
   - Observe waterfall timeline mode changes
   - Click Abs/Rel button on waterfall timeline
   - Observe SCW timeline mode changes

## Troubleshooting

If synchronization is not working:

1. **Check setupSCWWindow() is called:**
   - Verify `setupSCWWindow()` is uncommented in `mainwindow.cpp`
   - Check that SCW tab appears in the UI

2. **Check syncExternalTimelineView() is called:**
   - Verify `scwWindow->getTimelineView()` returns non-null
   - Check debug output for "GraphLayout: Syncing external timeline view"

3. **Check signal connections:**
   - Verify `Qt::UniqueConnection` is used to prevent duplicate connections
   - Check that signals are being emitted (use debug output)

4. **Check sync state:**
   - Verify `GraphContainerSyncState` is shared between SCWWindow and GraphLayout
   - Check that sync state is being updated in `onTimerTick()`

## Related Files

- `mainwindow.cpp` - SCW setup and initialization
- `graphlayout.cpp` - Signal synchronization logic
- `graphcontainer.cpp` - Waterfall graph update handlers
- `timelineview.cpp` - Timeline view signal emissions
- `scwwindow.cpp` - SCW timeline view management
- `sharedsyncstate.h` - Shared state structure

