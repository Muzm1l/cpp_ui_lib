# Manual: SCW Window — recent updates and integration notes

This document describes the recent changes to the **SCW window** (`SCWWindow`) and the
graphs it hosts, the new public APIs they expose, and — importantly — **why the magenta
"synced" circles may not appear on the SCW graphs when the library is integrated into a
host system**, plus notes on how to wire the sync correctly.

**See also:** [`SYMBOL_API.md`](./SYMBOL_API.md) for the BTW/RTW symbol APIs, and
[`GRAPH_LAYOUT_INTEGRATION_API.md`](./GRAPH_LAYOUT_INTEGRATION_API.md) for `GraphLayout`
integration.

---

## 1. What SCW is

`SCWWindow` is a standalone `QWidget` (it is **not** a `GraphContainer` and **not** owned
by `GraphLayout`). It contains:

- one left-hand `TimelineView` strip, and
- eight window cells, each a `QFrame` containing a series-label `QPushButton` and a
  plain `WaterfallGraph` (`m_waterfallGraphs[0..7]`).

In the sandbox it is created and wired by `MainWindow::setupSCWWindow()`. In a host
system, the integrator constructs and wires it themselves (see §5).

---

## 2. Summary of recent SCW updates

| # | Change | Where |
|---|--------|-------|
| 1 | **White border** on all SCW graphs | `WaterfallGraph::setBorderColor()` |
| 2 | **Yellow crosshair** — horizontal line marks a time (shown on the SCW timeline strip), vertical line shows **no value** | `WaterfallGraph::setCrosshairColor()` + `setCrosshairEnabled()` + `setCursorTimeChangedCallback()` |
| 3 | **Hover selection** — graphs select on mouse-over instead of click | `SCWWindow::eventFilter()` |
| 4 | **Hover query API** — which SCW window the mouse is over | `SCWWindow::getHoveredWindowIndex()` / `getHoveredSeriesName()` / `windowHovered()` |
| 5 | **Abs/Rel button removed** from the SCW timeline | `TimelineView` constructor flag `showTimeModeButton` |

---

## 3. New / changed public APIs

### 3.1 `WaterfallGraph` (shared, used by all graph types)

```cpp
// Frame border colour (default QColor(150,150,150) grey). SCW sets white.
void setBorderColor(const QColor &color);

// Crosshair line colour (default cyan). SCW sets yellow.
// Updates both the legacy overlay crosshair and the cursor-layer crosshair.
void setCrosshairColor(const QColor &color);
```

These are per-instance, so changing them on SCW graphs does **not** affect the main-grid
graphs. The 1px frame border is drawn in `WaterfallGraph::paintEvent()`.

The crosshair itself is enabled per graph via the existing
`setCrosshairEnabled(true)` (SCW uses the legacy overlay mode:
`setCursorLayerEnabled(false)`). The **time** under the horizontal line is pushed to the
timeline strip through the existing callback:

```cpp
graph->setCursorTimeChangedCallback([timelineView](const QDateTime &time, qreal) {
    if (time.isValid()) timelineView->updateCrosshairTimestampFromTime(time);
    else                timelineView->clearCrosshairTimestamp();
});
```

The vertical line intentionally has **no value label** (SCW has no zoom/value panel and no
X-position callback is wired).

### 3.2 `SCWWindow` — hover API

```cpp
// Which SCW window (0..7) the mouse is currently over, or -1 if none.
int getHoveredWindowIndex() const;

// Resolved series name for the hovered window ("" when none is hovered).
QString getHoveredSeriesName() const;

signals:
    // Existing: emitted when a window becomes selected (hover now drives selection).
    void seriesSelected(const QString &seriesName);

    // New: emitted whenever the hovered window changes. windowIndex is 0..7,
    // or -1 when the mouse leaves all graphs (seriesName is "" in that case).
    void windowHovered(int windowIndex, const QString &seriesName);
```

`windowHovered(-1, "")` fires on mouse-leave; the selection frame (yellow) persists after
leaving, but the hovered index returns to `-1`.

### 3.3 `TimelineView` — hide Abs/Rel button

```cpp
explicit TimelineView(QWidget* parent = nullptr, QTimer* timer = nullptr,
                      GraphContainerSyncState *syncState = nullptr,
                      bool sliderVisible = true, bool chevronVisible = true,
                      int timeModeButtonHeight = TIMELINE_VIEW_BUTTON_SIZE / 2,
                      int intervalButtonHeight = TIMELINE_VIEW_BUTTON_SIZE / 2,
                      bool showTimeModeButton = true);   // <-- new, pass false for SCW
```

When `showTimeModeButton == false` the Abs/Rel button is not created/added/connected. All
other references to it are null-guarded, so incoming Abs/Rel sync from the main system
still updates the SCW timeline's relative labels without the button.

---

## 4. Magenta "synced" circles — how the sync is meant to work

Placing a **BTW marker** should drop a **hollow** magenta circle (`"MagentaCircle"`) on the
other graphs **and** on the SCW graphs. The circle is drawn hollow everywhere; the
`isSynced` flag no longer changes its appearance. The reference path (as wired in the
sandbox) is:

```
User places a BTW manual marker           (or host calls GraphLayout::addBTWMarker)
  │
  ▼
GraphContainer::onBTWManualMarkerPlaced ── emits BTWManualMarkerPlaced
  │
  ▼
GraphLayout::onBTWManualMarkerPlaced  /  GraphLayout::addBTWMarker()
  │
  ▼
GraphLayout::addBTWSymbolToAllGraphs(timestamp, range)
  │   • adds "MagentaCircle" (isSynced=true) to every NON-BTW graph type in the
  │     layout that has a data point at `timestamp`
  │   • sets  anySymbolAdded = true  only if at least one was added
  │
  ├─ if (anySymbolAdded)  emit GraphLayout::BTWSymbolAddedToAllGraphs(timestamp)
  │
  ▼
[host connection]  scwWindow->addBTWSymbolToAllGraphs(timestamp)
      • for each of the 8 SCW graphs, finds the nearest data point (±1s) and adds
        a "MagentaCircle"; falls back to the Y-range centre if none is found; redraws
```

Key file references: `GraphLayout::addBTWSymbolToAllGraphs` (emits the signal),
`SCWWindow::addBTWSymbolToAllGraphs` (consumes it), and the sandbox connection in
`MainWindow::setupSCWWindow()`.

---

## 5. Why it does not sync when integrated — and notes to fix

The SCW magenta sync depends on **two fragile links** that the sandbox happens to satisfy
but a host system often does not. Check these in order.

### 5.1 The signal→slot connection must be made by the host

`SCWWindow` is a **sibling** of `GraphLayout`, not owned by it, so the library performs
**no automatic connection**. The host must wire it exactly once, after both objects exist:

```cpp
connect(graphLayout, &GraphLayout::BTWSymbolAddedToAllGraphs,
        scwWindow,   &SCWWindow::addBTWSymbolToAllGraphs,
        Qt::UniqueConnection);
```

Notes:
- Use **one** connection with `Qt::UniqueConnection`. The sandbox currently makes this
  connection in **two** places (a lambda in `MainWindow` setup and a direct connect in
  `setupSCWWindow()`); it is harmless only because SCW de-duplicates, but a host should
  make it once.
- Also share the sync state and timeline as the sandbox does:
  `new SCWWindow(parent, timer, graphLayout->getSyncState())` and
  `graphLayout->syncExternalTimelineView(scwWindow->getTimelineView())`.

### 5.2 The emit is gated behind `anySymbolAdded` — the real trap

`GraphLayout::BTWSymbolAddedToAllGraphs` is emitted **only if a NON-BTW graph in the
layout received a magenta circle** (`anySymbolAdded == true`). This means SCW sync is a
**side effect of updating the layout's other graphs**, not a direct consequence of placing
a BTW marker. It silently does nothing when, in the integrated system:

- the layout is **BTW-only** (or the currently-visible layout has no other graph type), or
- the other graph types have **no data point within ±1s** of the marker timestamp, or
- those graphs **already** have a magenta circle at that timestamp (dedup).

In the sandbox there are always several populated non-BTW graphs, so the signal fires; in a
host with a narrower layout it may never fire.

**Recommended fixes (pick one):**

1. **Decouple SCW notification from the other graphs (preferred).** Emit a notification
   whenever a BTW marker is placed, regardless of `anySymbolAdded`. For example, always
   emit `BTWSymbolAddedToAllGraphs(timestamp)` (or a new dedicated `BTWMarkerPlaced(ts,
   range)` signal) from the BTW-marker entry points — `GraphLayout::onBTWManualMarkerPlaced`
   and `GraphLayout::addBTWMarker` — and connect SCW to that. This makes SCW independent of
   which other graph types exist or have data.

2. **Drive SCW from `BTWManualMarkerPlaced` instead.** `GraphLayout::BTWManualMarkerPlaced`
   is emitted unconditionally for interactive placement. Connect it to a small slot that
   calls `scwWindow->addBTWSymbolToAllGraphs(timestamp)`. Caveat: this covers only
   interactive placement, **not** programmatic `addBTWMarker()`; combine with fix (1) if you
   also add markers via the API.

### 5.3 All programmatic additions must go through the fan-out API

Only `GraphLayout::addBTWMarker()` and interactive manual-marker placement fan out to the
other graphs and emit the signal. These do **not**:

- `GraphLayout::addBTWSymbol(GraphType::BTW, ...)` — adds to the BTW engine only; no fan-out,
  no signal.
- writing symbols straight into a `WaterfallData` / `GraphEngine`.

If the host adds magenta circles by any of these direct paths, either switch to
`addBTWMarker()`, or additionally call
`graphLayout->addBTWSymbolToAllGraphs(timestamp, range)` (which will emit if it adds to a
non-BTW graph — subject to §5.2), or notify SCW directly.

### 5.4 Timestamp alignment and SCW data availability

`SCWWindow::addBTWSymbolToAllGraphs` matches the incoming timestamp against each SCW
series' own data with a **±1s tolerance**, and only falls back to the Y-range centre if the
series' combined Y-range is valid (non-zero). It silently skips a graph when:

- the SCW series has **no data yet** at sync time, or
- SCW timestamps differ from the marker timestamp by **more than 1s**.

Notes:
- Ensure SCW graphs are **populated** (e.g. by the simulator/host feed) before/at the time
  markers are placed, or rely on the range-centre fallback (SCW sets a custom Y-range of
  `-20..20`, so the fallback is valid).
- Use a **single time source** across `GraphLayout` and `SCWWindow` (shared
  `GraphContainerSyncState` / `TimeScopeBus` / system start time) so timestamps line up
  within tolerance.
- If markers can be placed on times SCW has not sampled, consider widening the SCW tolerance
  or always storing at the range centre.

### 5.5 Quick integration checklist

- [ ] `SCWWindow` constructed with the layout's shared sync state.
- [ ] `graphLayout->syncExternalTimelineView(scwWindow->getTimelineView())` called.
- [ ] `GraphLayout::BTWSymbolAddedToAllGraphs → SCWWindow::addBTWSymbolToAllGraphs`
      connected once (`Qt::UniqueConnection`).
- [ ] Magenta circles added via `addBTWMarker()` / interactive placement (not a direct
      engine/data write).
- [ ] SCW notification does **not** depend on other layout graphs (apply §5.2 fix if the
      host layout is BTW-only or sparsely populated).
- [ ] SCW series populated and on the same time base (±1s) as the marker timestamps.
