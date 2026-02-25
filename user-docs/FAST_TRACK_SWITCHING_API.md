# Fast Track Switching API

**Last Updated:** 2024

## Overview

The Fast Track Switching API enables immediate visual feedback when switching between tracks in multi-track systems (e.g., 64-track systems). This is critical for maintaining responsive UI performance even after hours of runtime, when historical data accumulation could otherwise cause noticeable delays.

## Problem Statement

In a 64-track system running for several hours:
- **Before:** Switching tracks triggers a full historical rebuild, causing UI freeze
- **After:** Switching tracks renders only the visible window immediately, with full historical data available on-demand

### Performance Characteristics

| Aspect | Before | After |
|--------|--------|-------|
| Track switch cost | O(total history) | O(visible pixels) |
| UI responsiveness | Blocked | Immediate |
| Perceived latency | High | Minimal |
| Memory usage | Accumulates in caches | Caches cleared on track change |

## API Reference

### `void WaterfallGraph::markTrackChanged()`

Marks that a track change has occurred and enables fast track switching mode.

#### Behavior

1. **Clears all caches** to prevent memory accumulation:
   - Visible data cache (`m_cachedVisibleData`)
   - PaintEvent rendering caches (`m_dataLinePaths`, `m_batchedLinePaths`, `m_scatterPoints`, etc.)
   - Graphics item caches

2. **Enables fast track switch mode** (`m_fastTrackSwitchMode = true`)

3. **Triggers immediate render** focused on visible window only

4. **Automatically resets** after rendering completes - normal operation resumes

#### When to Call

Call this method when switching between tracks in your multi-track system:

```cpp
// Example: Switching from Track 1 to Track 2
void switchToTrack(int trackNumber) {
    // Update data source to new track
    WaterfallData* newTrackData = getTrackData(trackNumber);
    waterfallGraph->setDataSource(*newTrackData);
    
    // Mark track change for fast switching
    waterfallGraph->markTrackChanged();
    
    // Graph will now render immediately with visible-window-first strategy
}
```

#### Implementation Details

The method performs the following operations:

```cpp
void WaterfallGraph::markTrackChanged()
{
    // 1. Clear all caches
    invalidateAllVisibleDataCache();
    m_dataLinePaths.clear();
    m_batchedLinePaths.clear();
    m_scatterPoints.clear();
    m_dataLineColors.clear();
    m_scatterColors.clear();
    
    // 2. Cleanup graphics items
    cleanupAllScatterplotItems();
    
    // 3. Enable fast track switch mode
    m_fastTrackSwitchMode = true;
    
    // 4. Trigger render
    markAllSeriesDirty();
    setRenderState(RenderState::FULL_REDRAW);
    draw();
}
```

The `m_fastTrackSwitchMode` flag is automatically reset to `false` at the end of the `FULL_REDRAW` rendering cycle, ensuring normal operation resumes for subsequent updates.

## Architecture

### Visible-Window First Rendering Strategy

The implementation follows a "Visible-Window First Rendering" strategy:

1. **Immediate Paint:** Only builds geometry for data within the current visible time window (`timeMin` to `timeMax`)
2. **Deferred History:** Full historical data remains available but is not rebuilt immediately
3. **On-Demand:** Historical data is built incrementally as the user pans/zooms

### Cache Management

The visible data cache (`m_cachedVisibleData`) filters data by the current time window:

```cpp
// Cache only contains data within visible window
qint64 timeMinEpoch = timeMin.toMSecsSinceEpoch();
qint64 timeMaxEpoch = timeMax.toMSecsSinceEpoch();

// Binary search for visible range
auto startIt = std::lower_bound(timestampsEpoch.begin(), timestampsEpoch.end(), timeMinEpoch);
auto endIt = std::upper_bound(timestampsEpoch.begin(), timestampsEpoch.end(), timeMaxEpoch);

// Cache size is bounded by visible window, not total history
```

This ensures:
- **Bounded memory:** Cache size scales with screen resolution, not runtime duration
- **Predictable performance:** Build cost is independent of how long the system has been running
- **Immediate feedback:** User sees graph instantly, even with hours of historical data

## Integration Examples

### Example 1: Basic Track Switching

```cpp
class TrackManager {
    WaterfallGraph* graph;
    std::vector<WaterfallData*> tracks;
    int currentTrack;
    
public:
    void switchTrack(int trackIndex) {
        if (trackIndex < 0 || trackIndex >= tracks.size())
            return;
            
        currentTrack = trackIndex;
        
        // Update data source
        graph->setDataSource(*tracks[trackIndex]);
        
        // Enable fast track switching
        graph->markTrackChanged();
    }
};
```

### Example 2: Track Switching with UI Feedback

```cpp
void MainWindow::onTrackSelected(int trackNumber) {
    // Show loading indicator (optional - render is fast)
    statusBar()->showMessage(QString("Switching to track %1...").arg(trackNumber));
    
    // Switch track with fast rendering
    WaterfallData* trackData = m_trackManager->getTrackData(trackNumber);
    m_waterfallGraph->setDataSource(*trackData);
    m_waterfallGraph->markTrackChanged();
    
    // Update UI
    statusBar()->showMessage(QString("Track %1 active").arg(trackNumber), 2000);
    updateTrackLabel(trackNumber);
}
```

### Example 3: Batch Track Operations

```cpp
void GraphContainer::switchAllGraphsToTrack(int trackNumber) {
    // Switch all graphs in the container to the same track
    for (auto* graph : m_waterfallGraphs) {
        WaterfallData* trackData = getTrackDataForGraph(graph, trackNumber);
        graph->setDataSource(*trackData);
        graph->markTrackChanged();
    }
}
```

## Performance Considerations

### Memory Management

The `markTrackChanged()` method aggressively clears caches to prevent memory accumulation:

- **Before track change:** Caches may contain data from all 64 tracks accumulated over hours
- **After track change:** Caches are cleared, memory is freed
- **During render:** Only visible window data is cached

### Rendering Performance

- **First paint:** O(visible pixels) - typically < 100ms even with hours of history
- **Subsequent updates:** Normal incremental updates (unchanged)
- **Background refinement:** Can be added later for higher detail levels

### Best Practices

1. **Always call `markTrackChanged()` after `setDataSource()`** when switching tracks
2. **Don't call it for incremental data updates** - use normal incremental rendering
3. **Call it once per track switch** - the flag auto-resets after rendering
4. **Combine with `setDataSource()`** - they work together for optimal performance

## Troubleshooting

### Issue: Track switch still feels slow

**Possible causes:**
- Not calling `markTrackChanged()` after `setDataSource()`
- Very large visible time window (reduce time interval)
- Too many series visible simultaneously

**Solution:**
```cpp
// Ensure markTrackChanged() is called
graph->setDataSource(*newData);
graph->markTrackChanged();  // Don't forget this!
```

### Issue: Graph shows stale data after track switch

**Possible causes:**
- Caches not being cleared properly
- Old graphics items not removed

**Solution:**
The `markTrackChanged()` method should handle this automatically. If issues persist, check that:
- `invalidateAllVisibleDataCache()` is being called
- Graphics items are being cleaned up in `FULL_REDRAW` case

### Issue: Normal updates not working after track switch

**Possible causes:**
- Fast track switch mode flag not resetting

**Solution:**
The flag should auto-reset. Verify the `FULL_REDRAW` case ends with:
```cpp
m_fastTrackSwitchMode = false;
m_dirtySeries.clear();
m_renderState = RenderState::CLEAN;
```

## Related APIs

- `WaterfallGraph::setDataSource()` - Sets the data source for the graph
- `WaterfallGraph::forceFullRedraw()` - Forces a full redraw (doesn't clear caches)
- `WaterfallGraph::setTimeInterval()` - Sets the visible time window duration
- `WaterfallGraph::invalidateAllVisibleDataCache()` - Manually clear caches (usually not needed)

## Future Enhancements

Potential future improvements:
- Background refinement after initial paint
- Progressive detail loading
- Predictive pre-rendering for adjacent tracks
- Configurable detail levels during fast switch

---

## Appendix: Implementation Notes

### Internal State Machine

The fast track switch mode integrates with the existing render state machine:

```
CLEAN → markTrackChanged() → FULL_REDRAW (with m_fastTrackSwitchMode=true) → CLEAN
```

The flag is only active during the `FULL_REDRAW` cycle and is reset before returning to `CLEAN` state.

### Cache Invalidation Strategy

The method invalidates caches in this order:
1. Visible data cache (per-series filtered data)
2. PaintEvent rendering caches (batched paths, scatter points)
3. Graphics item caches (scatterplot items)

This ensures no stale data remains from the previous track.

Initialization
When you pass a timer with a 10-second interval to the GraphLayout constructor:
1. GraphLayout timer connection
The timer is stored in m_timer, but it is not automatically connected to GraphLayout::onTimerTick().
Connection only happens if no timer is provided (then it creates a 1-second timer and connects it).
So GraphLayout::onTimerTick() will not be called unless you manually connect it.
2. Timer passed to child components
The timer is passed to all GraphContainer instances (line 361-364 in graphlayout.cpp).
Each GraphContainer also stores the timer but doesn't connect it automatically (unless null).
The timer is also passed to:
TimeSelectionVisualizer (line 87 in graphcontainer.cpp)
TimelineView (line 99 in graphcontainer.cpp)
3. Impact on functionality
GraphLayout::onTimerTick() won't run automatically, so:
setCurrentTime() won't be called every 10 seconds
The sync state's currentNavTime won't update automatically
Child components (TimelineView, TimeSelectionVisualizer) may still use the timer if they connect it themselves.
4. What you need to do
If you want the timer to work with GraphLayout, you need to manually connect it after initialization:
QTimer *timer = new QTimer();timer->setInterval(10000); // 10 secondsGraphLayout *layout = new GraphLayout(parent, layoutType, timer, seriesLabelsMap);// Manually connect the timerconnect(timer, &QTimer::timeout, layout, &GraphLayout::onTimerTick);timer->start();
This is a design limitation: when a timer is provided, it's assumed you'll manage the connection yourself, whereas when no timer is provided, it creates and connects a default 1-second timer automatically.

--------------

QTimer *timer = new QTimer();
timer->setInterval(10000); // 10 seconds
GraphLayout *layout = new GraphLayout(parent, layoutType, timer, seriesLabelsMap);

// Manually connect the timer
connect(timer, &QTimer::timeout, layout, &GraphLayout::onTimerTick);
timer->start();





