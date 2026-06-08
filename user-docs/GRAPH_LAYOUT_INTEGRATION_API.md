# GraphLayout integration API

This document describes three **`GraphLayout`** APIs for external / main-system integration:

1. **Highlight a time region** in the history selection bar  
2. **Query which graphs are on screen**  
3. **Clear or redraw a single data line (series)** in a graph  

All examples assume you hold a `GraphLayout *graphgrid` created by your host application.

---

## 1. Highlight history selection region

### Purpose

Draw a highlighted band between two timestamps in each graph container’s **history selection** visualizer (the narrow vertical bar beside the timeline, to the left of the waterfall graph).

This is the same visual used when an operator drags a time range on that bar.

### API

```cpp
bool highlightHistorySelectionRegion(const QDateTime &startTime,
                                     const QDateTime &endTime);
```

**Header:** `graphlayout.h`

### Parameters

| Parameter   | Description |
|------------|-------------|
| `startTime` | One boundary of the region. May be earlier or later than `endTime`; order is normalized internally. |
| `endTime`   | The other boundary. |

Both must be **valid** `QDateTime` values.

### Return value

| Value | Meaning |
|-------|---------|
| `true`  | At least one container accepted and drew the region. |
| `false` | Both timestamps invalid, or **every** container rejected the selection (e.g. already at the 5-selection cap, or clamped to an invalid range). |

### Behavior (unchanged existing rules)

The call delegates to the existing `addTimeSelection()` path on each `GraphContainer`. **No limits or merge rules were changed:**

- Timestamps are normalized so `startTime ≤ endTime`.
- The span is **clamped** to each container’s valid data-time range (when configured).
- **Overlapping** selections on the same container are **merged** into one wider band.
- Each container keeps at most **`MAX_TIME_SELECTIONS` (5)**. If a container already has 5 bands, the new region is **silently skipped** for that container only.
- On success, `TimeSelectionCreated(span)` is emitted and the span is appended to `GraphContainerSyncState::timeSelections` (same as a user-created selection).

### Example

```cpp
QDateTime t0 = QDateTime::fromString("2026-06-08T10:15:00", Qt::ISODate);
QDateTime t1 = QDateTime::fromString("2026-06-08T10:45:00", Qt::ISODate);

if (graphgrid->highlightHistorySelectionRegion(t0, t1)) {
    // Highlight visible on at least one container
} else {
    // Invalid times, or all containers at 5-selection limit
}
```

### Related signals

```cpp
connect(graphgrid, &GraphLayout::TimeSelectionCreated,
        receiver, &Receiver::onTimeSelectionCreated);
```

---

## 2. Get graphs currently shown on screen

### Purpose

Return the **graph-type names** (e.g. `"BTW"`, `"BDW"`) for each container that is part of the **active layout**, in container order. Containers hidden by the current layout mode (1-window, 2-window, etc.) are excluded.

### API

```cpp
std::vector<QString> getVisibleGraphNames() const;
```

**Header:** `graphlayout.h`

### Return value

A `std::vector<QString>` of names from `graphTypeToString()` for each non-hidden container’s **current** graph (`getCurrentDataOption()`).

Example for default 2×2 layout:

```
BTW, BDW, RTW, FTW
```

Example after switching to 1-window layout:

```
BTW
```

### Notes

- Uses `!container->isHidden()`, **not** `isVisible()`. Hidden-by-layout containers are excluded, but the query still works when the Original View tab is not the active tab (unlike a raw `isVisible()` check).
- When the layout or a container’s graph type changes, `VisibleGraphsChanged()` is emitted. Connect to refresh UI:

```cpp
connect(graphgrid, &GraphLayout::VisibleGraphsChanged,
        receiver, &Receiver::refreshGraphList);
```

### Example

```cpp
std::vector<QString> names = graphgrid->getVisibleGraphNames();
for (size_t i = 0; i < names.size(); ++i) {
    qDebug() << (i + 1) << names[i];
}
```

---

## 3. Clear or redraw a particular data line (series)

### Purpose

Update or remove **one series** (one “data line”) inside a graph type (e.g. `"BTW-1"` on the BTW graph) without affecting other series on the same graph.

Data lives in `GraphEngine` / `WaterfallData`; containers redraw when notified.

### GraphLayout APIs (recommended for integration)

| Method | Use when |
|--------|----------|
| `setDataToDataSource(graphType, seriesLabel, yData, timestamps)` | Replace series data and redraw (clear + redraw in one step if you pass new points, or pass empty vectors to clear). |
| `clearDataSource(graphType, seriesLabel)` | Remove all points for one series, then notify containers to redraw. |
| `setDataToDataSourceInteractive(...)` | High-frequency / drag updates; incremental per-series redraw only. |
| `redrawGraph(graphType)` | Force redraw of one graph type after a data change if pixels look stale. |

**Header:** `graphlayout.h`  
**Types:** `GraphType` (`BTW`, `BDW`, `BRW`, `FDW`, `FTW`, `LTW`, `RTW`), `QString` series label (e.g. `"BTW-1"`, `"ADOPTED"`).

### Clear one series

```cpp
graphgrid->clearDataSource(GraphType::BTW, "BTW-1");
```

Implementation: `GraphEngine::clearDataSeries` → `notifyGraphDataChanged` → container `onDataChanged` → `WaterfallGraph::draw()`.

If old pixels remain, force a full redraw:

```cpp
graphgrid->clearDataSource(GraphType::BTW, "BTW-1");
graphgrid->redrawGraph(GraphType::BTW);
```

### Replace / redraw one series

```cpp
std::vector<float> yData = { 12.0f, 15.0f, 18.0f };
std::vector<QDateTime> timestamps = { t0, t1, t2 };

graphgrid->setDataToDataSource(GraphType::BTW, "BTW-1", yData, timestamps);
```

Passing **empty** `yData` and `timestamps` clears the series and triggers a **full** redraw path (stale scene items are cleared).

### Interactive (live drag) update

```cpp
graphgrid->setDataToDataSourceInteractive(GraphType::BTW, "BTW-1", yData, timestamps);
```

Uses `WaterfallGraph::triggerIncrementalRedraw(seriesLabel)` — only that series is marked dirty; cheaper for drag loops.

### Lower-level access (data only — no automatic redraw)

```cpp
WaterfallData *data = graphgrid->getDataSource(GraphType::BTW);
if (data) {
    data->clearDataSeries("BTW-1");
}
graphgrid->redrawGraph(GraphType::BTW);  // required after direct data edits
```

Prefer `clearDataSource` / `setDataToDataSource` so redraw and sync stay consistent.

### Direct `WaterfallGraph` access (if you have a widget pointer)

```cpp
graph->setData("BTW-1", yData, timestamps);
graph->setDataInteractive("BTW-1", yData, timestamps);
graph->triggerIncrementalRedraw("BTW-1");

// Clear all series on that widget (not one series):
graph->clearData();

// Clear one series on the widget:
graph->getDataSource()->clearDataSeries("BTW-1");
graph->forceFullRedraw("clear_series");
```

There is no `WaterfallGraph::clearDataSeries(label)`; use the data source or `GraphLayout::clearDataSource`.

---

## Quick reference

| Task | Call |
|------|------|
| Highlight time range in history bar | `highlightHistorySelectionRegion(t0, t1)` |
| List on-screen graph names | `getVisibleGraphNames()` |
| Clear one data line | `clearDataSource(type, label)` |
| Redraw one data line with new points | `setDataToDataSource(type, label, y, times)` |
| Live drag update for one line | `setDataToDataSourceInteractive(type, label, y, times)` |
| Force redraw after clear | `redrawGraph(type)` |

---

## Files

| File | Relevance |
|------|-----------|
| `graphlayout.h` / `graphlayout.cpp` | Public APIs |
| `graphcontainer.h` / `graphcontainer.cpp` | History selection, redraw |
| `timeselectionvisualizer.h` / `.cpp` | History bar drawing, 5-selection cap |
| `graphengine.h` / `.cpp` | Per-series data storage |
| `waterfallgraph.h` / `.cpp` | Per-series draw / incremental redraw |
| `timelineutils.h` | `TimeSelectionSpan`, `GraphType` via `graphtype.h` |
