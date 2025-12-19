## Timeline / Waterfall Hang & Crash Analysis

> **Status**: Root cause identified and quantified via Valgrind callgrind + KCachegrind analysis.  
> **Primary Issue**: Per-tick `TimeScopeChanged` emissions causing ~1M QGraphicsPixmapItem create/destroy cycles.  
> **Secondary Issue**: ~1M `QDateTime::msecsTo()` calls with expensive timezone operations.  
> **Observed**: 10-second crosshair delay after 30 minutes due to event queue saturation.  
> **Fix Required**: Remove `emitTimeScopeChanged()` from `TimelineVisualizerWidget::setCurrentTime()`.

---

### Context

This document summarizes the investigation into recent hangs and crashes in the UI sandbox application related to the timeline view, graph layout, and waterfall / BTW graphs. Symptoms observed:

- Application UI becomes unresponsive and the OS offers a “force quit”.
- `top` shows the `ui-sandbox` process at ~100% CPU on a single core for extended periods.
- Under `gdb`, the main thread is repeatedly found inside Qt graphics and time/date functions.

### Key Stack Traces Collected

#### 1. Crash in `WaterfallGraph::mapDataToScreen` / `QDateTime`

Example backtrace:

- `QDateTime::msecsTo(QDateTime const&)`
- `WaterfallGraph::mapDataToScreen(double, QDateTime const&) const`
- `WaterfallGraph::drawScatterplot(QString const&, QColor const&, double, QColor const&)`
- `BTWGraph::draw()`
- `WaterfallGraph::setTimeRange(QDateTime const&, QDateTime const&)`
- `GraphContainer::setTimeScope(TimeSelectionSpan const&)`
- `GraphLayout::onContainerTimeScopeChanged(TimeSelectionSpan const&)`
- `GraphContainer::TimeScopeChanged(TimeSelectionSpan const&)`
- `TimelineView::TimeScopeChanged(TimeSelectionSpan const&)`
- `TimelineView::onVisibleTimeWindowChanged(TimeSelectionSpan const&)`
- `TimelineVisualizerWidget::visibleTimeWindowChanged(TimeSelectionSpan const&)`
- `TimelineVisualizerWidget::emitTimeScopeChanged()`
- `TimelineVisualizerWidget::setCurrentTime(QTime const&)`
- `TimelineView::onTimerTick()`
- `QTimer::timeout(QTimer::QPrivateSignal)`

Interpretation:

- A 1 Hz `QTimer` in `TimelineView` calls `setCurrentTime`, which emits a new *visible time window* (`TimeSelectionSpan`).
- This propagates through `TimelineView` → `GraphContainer` → `GraphLayout` and eventually causes `WaterfallGraph::setTimeRange` to be called.
- `setTimeRange` forces a **full redraw**; `BTWGraph::draw` then calls `drawScatterplot`, which calls `mapDataToScreen` for each visible data point.
- The immediate crash location is inside `QDateTime::msecsTo`, invoked by `mapDataToScreen`. That implies either:
  - `this` (the `WaterfallGraph` / `QDateTime` object) is corrupted (use-after-free / memory corruption), or
  - The time values being passed are invalid/uninitialized.

This part remains a secondary concern; the primary issue found is the hang / 100% CPU from repeated full redraws.

#### 2. Hang / 100% CPU inside `QGraphicsScene::clear`

When interrupting a hung run with `gdb`, the main-thread backtrace is consistently:

- `QGraphicsItem::~QGraphicsItem()`
- `QGraphicsPixmapItem::~QGraphicsPixmapItem()`
- `QGraphicsScene::clear()`
- `BTWGraph::draw()`
- `WaterfallGraph::setTimeRange(QDateTime const&, QDateTime const&)`
- `GraphContainer::setTimeScope(TimeSelectionSpan const&)`
- `GraphLayout::onContainerTimeScopeChanged(TimeSelectionSpan const&)`
- `GraphContainer::TimeScopeChanged(TimeSelectionSpan const&)`
- `TimelineView::TimeScopeChanged(TimeSelectionSpan const&)`
- `TimelineView::onVisibleTimeWindowChanged(TimeSelectionSpan const&)`
- `TimelineVisualizerWidget::visibleTimeWindowChanged(TimeSelectionSpan const&)`
- `TimelineVisualizerWidget::emitTimeScopeChanged()`
- `TimelineVisualizerWidget::setCurrentTime(QTime const&)`
- `TimelineView::onTimerTick()`
- `QTimer::timeout(QTimer::QPrivateSignal)`

This is essentially the same chain as above, with the hot point clearly being:

- `BTWGraph::draw()` → `graphicsScene->clear()` → destruction of many `QGraphicsPixmapItem` instances on every timer tick.

### Root Cause Summary

1. **Timeline timer emits TimeScopeChanged every second in FOLLOW mode**  
   - `TimelineView::onTimerTick()` calls `TimelineVisualizerWidget::setCurrentTime()`.
   - `setCurrentTime()` updates the slider’s time window and calls `emitTimeScopeChanged()`, which emits `visibleTimeWindowChanged(TimeSelectionSpan)`.
   - `TimelineView::onVisibleTimeWindowChanged()` emits `TimeScopeChanged(TimeSelectionSpan)` to `GraphContainer`.
   - `GraphContainer::onTimeScopeChanged()` calls `WaterfallGraph::setTimeRange()`.
   - `setTimeRange()` forces a full redraw (`RenderState::FULL_REDRAW`, calls `draw()` immediately).

2. **BTWGraph redraw is extremely heavy**  
   - `BTWGraph::draw()` calls `graphicsScene->clear()` and recreates:
     - All base waterfall content (grid, background).
     - Scatter plots for all relevant series using cached pixmaps.
     - All BTW symbols, markers, shaded regions, etc.
   - On non-trivial datasets, clearing the scene and re-allocating thousands of `QGraphicsPixmapItem`s is expensive.

3. **Result: 1 Hz full redraw *per container* in sync**  
   - When multiple `GraphContainer`s are synchronized by `GraphLayout`, each timer tick in one timeline view can propagate `TimeScopeChanged` to other containers.
   - Each container’s BTWGraph does a full `QGraphicsScene::clear` + redraw.
   - This drives the main thread CPU to ~100% and makes the UI unresponsive, producing “force quit” prompts.

4. **Crash possibility**  
   - Because this pattern is heavily reallocating and deleting graphics items on the GUI thread, any logic errors (double-deletion, use-after-free of items or graphs, or invalid `QDateTime` state) can surface as crashes inside:
     - `QGraphicsScene::clear()`
     - `QGraphicsPixmapItem` destructor
     - `QDateTime::toMSecsSinceEpoch` / `msecsTo`
   - The hang and high CPU are clearly explained by the redraw loop; the crash appears to be a secondary symptom of the same over-aggressive redraw pattern and/or a subtle memory misuse not yet fully isolated.

---

### Valgrind Callgrind Profiling Results

A Valgrind callgrind profile (`callgrind.out.92227`) was captured to quantify where CPU time is spent. Summary: **34,410,182,485 instructions total**.

#### Top CPU Consumers

| Operation | Call Count | Instructions | % of Total |
|-----------|------------|--------------|------------|
| `QGraphicsPixmapItem::~QGraphicsPixmapItem()` | **1,000,001** | 13,665,154,233 | **39.7%** |
| `QGraphicsPixmapItem::~QGraphicsPixmapItem()'2` (secondary dtor) | 1,000,001 | 13,529,714,004 | **39.3%** |
| `GraphContainer::TimeScopeChanged` signal chain | 1,236 | 21,625,853,615 | 62.8% |
| `WaterfallGraph::setTimeRange()` | 1,374 | 3,961,380,725 | 11.5% |
| `BTWGraph::draw()` | 828 | 3,510,202,082 | 10.2% |
| `QGraphicsPolygonItem::~QGraphicsPolygonItem()` | 17,886 | 252,760,001 | 0.7% |
| `QGraphicsPixmapItem` constructor | 1,000,001 | ~12,000,012 | <0.1% |

#### Key Finding: ~79% of CPU in Pixmap Item Lifecycle

Creating and destroying `QGraphicsPixmapItem` objects consumed **~27 billion instructions** out of 34.4 billion total.

The flow observed:
1. `QTimer::timeout` (1 Hz) triggers `TimelineView::onTimerTick()`
2. → `TimelineVisualizerWidget::setCurrentTime()` → `emitTimeScopeChanged()`
3. → `GraphContainer::TimeScopeChanged` propagates to all containers
4. → `WaterfallGraph::setTimeRange()` forces `draw()`
5. → **`BTWGraph::draw()` calls `graphicsScene->clear()`** — destroys all items
6. → **Recreates ~1,200 `QGraphicsPixmapItem` objects** for scatterplot points

With **828 `BTWGraph::draw()` calls** × **~1,200 pixmap items per call** ≈ **1,000,000 pixmap create/destroy cycles**.

#### Signal Cascade

- `WaterfallGraph::setTimeRange()` was called **1,374 times**.
- Each call immediately calls `draw()`.
- `GraphContainer::TimeScopeChanged` signal emitted **1,236 times**, with a total cost of **21.6 billion instructions** (includes everything downstream).

#### Scatterplot Rendering

- `WaterfallGraph::drawScatterplot()` creates one `QGraphicsPixmapItem` per data point.
- On each `draw()`, the entire scene is cleared and all items recreated from scratch.
- This is the primary driver of the observed 100% CPU and UI unresponsiveness.

#### Secondary Optimization Opportunity

Instead of:
```cpp
graphicsScene->clear();  // Very expensive - destroys ALL items
// ... recreate all items from scratch
```

Consider:
- Reusing existing `QGraphicsItem` objects (update positions instead of recreate).
- Batching points into a custom `QGraphicsItem` that draws multiple points in one `paint()` call.
- Implementing incremental rendering (only add/remove items that changed).

---

### KCachegrind Visual Analysis

Three call graph exports were captured from KCachegrind to visualize the hot paths:

#### 1. BTWGraph Draw Chain (`btwgraph_cachegring_1`)

| Function | Instructions | Call Count |
|----------|--------------|------------|
| `BTWGraph::draw()` | **29.6 billion** | 9,035 |
| `WaterfallGraph::setTimeRange()` | 26.1 billion | 8,148 → draw |
| `WaterfallGraph::drawScatterplot()` | 14.0 billion | 26,859 |
| `QGraphicsScene::clear()` | 13.9 billion | 16,534 |
| `QGraphicsPixmapItem::~QGraphicsPixmapItem()` | 13.6 billion | **997,360** |
| `WaterfallGraph::mapDataToScreen()` | 9.1 billion | **999,992** |
| `QDateTime::msecsTo()` | 8.9 billion | 999,374 |

**Key insight**: `mapDataToScreen()` is called **~1 million times**, and each call invokes `QDateTime::msecsTo()` which triggers expensive timezone operations (`mktime`, `localtime_r`, `tzset`).

#### 2. TimeScopeChanged Signal Propagation (`timeslopchanged_kcg_1`)

| Function | Instructions | Call Count |
|----------|--------------|------------|
| `GraphLayout::onContainerTimeScopeChanged()` | 19.3 billion | **1,968** |
| `GraphContainer::setTimeScope()` | 19.2 billion | **5,904** |
| `WaterfallGraph::setTimeRange()` | 19.0 billion | 5,904 |
| `BTWGraph::draw()` | 18.8 billion | 5,850 |

**Key insight**: Each `TimeScopeChanged` signal (1,968 emissions) propagates to ~3 containers (5,904 / 1,968 ≈ 3), each triggering a full redraw.

#### 3. Crosshair Update Delay (`updatecrosshair_ckg_1`)

| Function | Instructions |
|----------|--------------|
| `ZoomPanel::updateCrosshairLabel()` | 1.3 million |
| `QGraphicsTextItem::setPlainText()` | 737K |
| Font/text layout operations | ~500K |

**This explains the observed "10-second crosshair delay after 30 minutes":**

The crosshair update itself is **cheap** (~1.3M instructions), but it competes with the massive redraw operations (billions of instructions). After extended runtime:

1. Timer ticks queue up thousands of `TimeScopeChanged` signals
2. Each signal triggers ~19 billion instructions of redraw work
3. The main thread is saturated processing redraws
4. Lightweight operations like crosshair updates get starved
5. **Result**: 10+ second UI lag for simple updates

#### Complete Call Flow Visualization

```
Timer tick (1 Hz)
    ↓
emitTimeScopeChanged()
    ↓
GraphLayout::onContainerTimeScopeChanged() × 1,968 times
    ↓
GraphContainer::setTimeScope() × 5,904 times (3 containers each)
    ↓
WaterfallGraph::setTimeRange() → BTWGraph::draw()
    ↓
QGraphicsScene::clear() destroys ~1M QGraphicsPixmapItem
    ↓
drawScatterplot() recreates them + mapDataToScreen() × 1M
    ↓
QDateTime::msecsTo() × 1M (expensive timezone ops)
    ↓
Main thread saturated → crosshair updates delayed 10+ seconds
```

#### Additional Finding: QDateTime Timezone Overhead

The callgrind data reveals significant time spent in timezone-related functions:

| Function | Instructions | Call Count |
|----------|--------------|------------|
| `qMkTime(tm*)` | 7.6 billion | 1,999,089 |
| `mktime` | 7.5 billion | 1,999,089 |
| `localtime_r` | 5.5 billion | 1,999,320 |
| `__tz_convert` | 5.5 billion | 1,999,320 |
| `__tzfile_compute` | 4.7 billion | 1,999,320 |
| `__tzset_parse_tz` | 3.7 billion | 1,999,320 |

**Optimization opportunity**: If timestamps are always in the same timezone (e.g., UTC), consider:
- Using `QDateTime::toMSecsSinceEpoch()` once and caching it
- Pre-computing time offsets for the visible range
- Using integer millisecond arithmetic instead of repeated `QDateTime::msecsTo()` calls

---

### Design Intention vs. Current Behavior

Intended behavior:

- The timeline view (slider) should:
  - Animate smoothly in FOLLOW mode (showing the last N minutes/hours).
  - Allow the user to drag the slider to choose a different visible time window (FROZEN mode).
- Graph containers should:
  - Update their time ranges when the user changes the visible window.
  - Optionally auto-advance when new data arrives, but not necessarily on every second if nothing changed.

Current behavior:

- `TimelineVisualizerWidget::setCurrentTime()` emits `visibleTimeWindowChanged` (via `emitTimeScopeChanged()`) **on every timer tick in FOLLOW mode**, even when:
  - No new data has arrived.
  - The user has not interacted with the slider.
- Every such emission translates into a `GraphContainer::TimeScopeChanged` → `setTimeRange` → `BTWGraph::draw()` → `graphicsScene->clear()` chain.
- This is **unnecessary work** and causes the observed hangs.

### Proposed Fix (High-Level)

The core fix is to **stop broadcasting full time-scope changes from the timeline view on every timer tick**, and instead:

- Keep the timeline view’s slider animation internal to `TimelineVisualizerWidget` when in FOLLOW mode.
- Only emit `visibleTimeWindowChanged` when:
  - The user explicitly changes the slider (dragging or releasing it).
  - The time interval changes (e.g. 15 min → 30 min).
  - Some higher-level logic decides a window update is needed (e.g. when new data arrives and the graph range should be shifted).

This preserves:

- Smooth animation in the timeline itself.
- User-driven time scope synchronization across containers.

While avoiding:

- Unbounded 1 Hz full BTWGraph redraws when the data and window haven’t really changed.

### Concrete Code Change (TimelineVisualizerWidget)

In `TimelineVisualizerWidget::setCurrentTime(const QTime &currentTime)`:

- **Before (simplified):**

```cpp
void TimelineVisualizerWidget::setCurrentTime(const QTime &currentTime)
{
    m_lastCurrentTime = m_currentTime;
    m_currentTime = currentTime;

    if (m_timelineViewMode == TimelineViewMode::FOLLOW_MODE) {
        updatePixelSpeed();
        m_backgroundNeedsRedraw = true;
    }

    if (!m_sliderState.isDragging()) {
        if (m_timelineViewMode == TimelineViewMode::FOLLOW_MODE) {
            // Adjust slider window [now - interval, now]
            // ...
            m_sliderState.setTimeWindow(newWindow, rect().height(), m_timeLineLength);
            m_sliderState.setYPosition(0, rect().height(), m_timeLineLength);
            m_sliderVisibleWindow = m_sliderState.getTimeWindow();

            if (m_manoeuvreOverlay) {
                m_manoeuvreOverlay->setTimeRange(newWindow.startTime, newWindow.endTime);
            }

            emitTimeScopeChanged();  // <-- CAUSES 1 Hz full redraws
        }

        updateVisualization();
    }
}
```

- **After (proposed): remove per-tick `emitTimeScopeChanged()` and keep slider animation local:**

```cpp
void TimelineVisualizerWidget::setCurrentTime(const QTime &currentTime)
{
    m_lastCurrentTime = m_currentTime;
    m_currentTime = currentTime;

    if (m_timelineViewMode == TimelineViewMode::FOLLOW_MODE) {
        updatePixelSpeed();
        m_backgroundNeedsRedraw = true;
    }

    if (!m_sliderState.isDragging()) {
        if (m_timelineViewMode == TimelineViewMode::FOLLOW_MODE) {
            QDateTime now = QDateTime::currentDateTime();
            int intervalSeconds = m_timeLineLength.hour() * 3600
                                + m_timeLineLength.minute() * 60
                                + m_timeLineLength.second();
            QDateTime startTime = now.addSecs(-intervalSeconds);
            TimeSelectionSpan newWindow(startTime, now);

            m_sliderState.setTimeWindow(newWindow, rect().height(), m_timeLineLength);
            m_sliderState.setYPosition(0, rect().height(), m_timeLineLength);
            m_sliderVisibleWindow = m_sliderState.getTimeWindow();

            if (m_manoeuvreOverlay && newWindow.startTime.isValid() && newWindow.endTime.isValid()) {
                m_manoeuvreOverlay->setTimeRange(newWindow.startTime, newWindow.endTime);
            }

            // IMPORTANT: do NOT call emitTimeScopeChanged() on every timer tick.
            // GraphContainer / GraphLayout already move time ranges when data changes.
            // emitTimeScopeChanged();
        }

        updateVisualization();  // repaint timeline widget itself
    }
}
```

Notes:

- Existing code paths that call `emitTimeScopeChanged()` due to **user interaction** (e.g. slider drag in `mouseMoveEvent` / `mouseReleaseEvent`) should be kept so that user-driven time scope changes still propagate to graphs.
- `GraphContainer::onTimerTick()` and `GraphContainer::onDataChanged()` already contain logic to:
  - Check if new data has arrived near “now”.
  - Shift `WaterfallGraph` time ranges accordingly and sync the timeline, then redraw.
  This is the correct place to drive full graph redraws based on data, not the per-second timer in the timeline view alone.

### Optional Hardening (Safety Checks)

To guard against invalid time ranges and help debug any remaining crashes:

1. **Defensive checks in `WaterfallGraph::setTimeRange`:**

```cpp
void WaterfallGraph::setTimeRange(const QDateTime &timeMin, const QDateTime &timeMax)
{
    if (!timeMin.isValid() || !timeMax.isValid() || timeMin >= timeMax) {
        qWarning() << "WaterfallGraph::setTimeRange: invalid range"
                   << "timeMin:" << timeMin.toString("yyyy-MM-dd hh:mm:ss.zzz")
                   << "timeMax:" << timeMax.toString("yyyy-MM-dd hh:mm:ss.zzz");
        return;
    }

    customTimeMin = timeMin;
    customTimeMax = timeMax;
    customTimeRangeEnabled = true;

    this->timeMin = timeMin;
    this->timeMax = timeMax;

    setRenderState(RenderState::FULL_REDRAW);
    draw();
}
```

2. **Defensive checks in `WaterfallGraph::mapDataToScreen`:**

```cpp
QPointF WaterfallGraph::mapDataToScreen(qreal yValue, const QDateTime &timestamp) const
{
    if (!dataRangesValid || drawingArea.isEmpty()) {
        return QPointF(0, 0);
    }

    if (!timestamp.isValid() || !timeMax.isValid()) {
        qWarning() << "mapDataToScreen: invalid timestamp/timeMax"
                   << "timestamp valid:" << timestamp.isValid()
                   << "timeMax valid:" << timeMax.isValid()
                   << " timestamp:" << timestamp.toString("yyyy-MM-dd hh:mm:ss.zzz")
                   << " timeMax:" << timeMax.toString("yyyy-MM-dd hh:mm:ss.zzz");
        return QPointF(0, 0);
    }

    qreal x = drawingArea.left()
            + ((yValue - yMin) / (yMax - yMin)) * drawingArea.width();

    qint64 timeOffset = timestamp.msecsTo(timeMax);
    qreal y = drawingArea.top()
            + (timeOffset / (qreal)getTimeIntervalMs()) * drawingArea.height();

    return QPointF(x, y);
}
```

These changes:

- Prevent obviously invalid time ranges from driving redraws.
- Provide detailed logging if invalid data is ever seen, making further diagnosis easier.

### Expected Outcome After Fix

After removing the per-tick `emitTimeScopeChanged()` call from `TimelineVisualizerWidget::setCurrentTime` and rebuilding:

- **CPU usage**: `ui-sandbox` should no longer sit at ~100% CPU when idle in FOLLOW mode.
- **Responsiveness**: The UI should stop showing “force quit” prompts due to a spinning main thread.
- **Behavior**:
  - Timeline still scrolls in FOLLOW mode.
  - Graphs update:
    - When new data arrives (via `GraphContainer::onDataChanged` / `onTimerTick`).
    - When the user changes the visible time window (dragging the slider, which still emits scope changes).

If, after this optimization, crashes still occur inside `QGraphicsScene::clear` or `QDateTime::msecsTo`, they should be much rarer and easier to reproduce. At that point, running with AddressSanitizer/Valgrind and analyzing any remaining `qWarning` logs from the hardening checks above will be the recommended next step.

---

### Files and Data Sources

- **GDB backtraces**: Collected via `gdb -p <pid>` during hang and crash events.
- **Valgrind callgrind profile**: `callgrind.out.92227` (551,569 lines, 34.4 billion instructions recorded).
- **KCachegrind call graph exports** (in `screenshots/` folder):
  - `btwgraph_cachegring_1` — BTWGraph draw chain visualization
  - `timeslopchanged_kcg_1` — TimeScopeChanged signal propagation
  - `updatecrosshair_ckg_1` — Crosshair label update chain (explains 10-second delay)
- **Process monitoring**: `top` showing `ui-sandbox` at ~100% CPU on one core.

### Summary of Required Changes

| Priority | Change | Location | Impact |
|----------|--------|----------|--------|
| **P0** | Remove `emitTimeScopeChanged()` from per-tick path | `TimelineVisualizerWidget::setCurrentTime()` | Eliminates 1 Hz full redraws |
| **P1** | Add validity checks for time ranges | `WaterfallGraph::setTimeRange()`, `mapDataToScreen()` | Prevents crashes from invalid data |
| **P2** | Consider incremental rendering / item reuse | `BTWGraph::draw()`, `drawScatterplot()` | Major performance improvement |
| **P3** | Optimize `QDateTime::msecsTo()` calls | `WaterfallGraph::mapDataToScreen()` | Reduces timezone overhead (~7.6B instructions) |

### Observed Symptoms vs. Root Causes

| Symptom | Root Cause | Fix |
|---------|-----------|-----|
| 100% CPU, UI hangs | 1 Hz full BTWGraph redraws via `TimeScopeChanged` | P0: Remove per-tick signal emission |
| Force quit prompts | Main thread saturated, event loop blocked | P0 + P2: Reduce redraw frequency and cost |
| 10-second crosshair delay after 30 min | Lightweight updates starved by heavy redraws | P0: Eliminates event queue buildup |
| Crashes in `QDateTime::msecsTo` | Possible invalid timestamps during rapid reallocation | P1: Add validity checks |

---

*Analysis date: December 2024*

