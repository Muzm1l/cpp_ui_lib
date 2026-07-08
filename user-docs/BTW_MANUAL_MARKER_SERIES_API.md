# Manual: BTW manual marker series binding

This document describes the **BTW manual marker series** API — how to bind
click-placed manual markers on BTW graphs to a specific data series, so the marker
lands on that series' trace at the clicked time instead of the raw clicked X position.

**See also:** [`SYMBOL_API.md`](./SYMBOL_API.md) (BTW symbols vs markers),
[`GRAPH_LAYOUT_INTEGRATION_API.md`](./GRAPH_LAYOUT_INTEGRATION_API.md),
[`HISTORY_SELECTION_API.md`](./HISTORY_SELECTION_API.md) (similar "pick a reference
series" pattern for history selections).

---

## 1. Concepts

| Term | Meaning |
|------|---------|
| **Manual marker** | The interactive pink circle on a BTW graph (`InteractiveGraphicsItem` on the overlay). Placed by clicking empty graph space or via `addBTWManualMarker()`. |
| **Series binding** | A configured series label (e.g. `"ADOPTED"`, `"BTW-1"`) that click-placed markers snap to. |
| **Interpolated range** | The Y/range value of the bound series at the clicked timestamp, computed by linear interpolation between adjacent samples. |
| **Raw X placement** | Default behaviour when no series is bound: the marker is placed at the clicked X position (mapped to range). |

### What changed

Previously, click-placed markers used a **"snap to nearest visible series"** heuristic.
That has been **replaced** by an explicit series binding: the integrator chooses which
series markers follow via `setBTWManualMarkerSeries()`.

### What series binding affects

| Placement method | Uses series binding? |
|------------------|----------------------|
| User click on BTW graph | **Yes** — when a non-empty series is configured |
| `GraphLayout::addBTWManualMarker()` | **No** — uses the `rangeValue` argument directly |
| `BTWGraph::addBTWManualMarker()` | **No** — uses the `rangeValue` argument directly |

---

## 2. Architecture

```
Integrator calls GraphLayout::setBTWManualMarkerSeries("ADOPTED")
  → fans out to every BTWGraph in every container (even hidden ones)
  → each BTWGraph stores m_manualMarkerSeries

User clicks empty BTW graph area
  → BTWGraph::onMouseClick()
  → timestamp = mapScreenToTime(click Y)
  → if series bound:
        range = dataSource->interpolateSeriesRangeAtTime(series, timestamp)
        screenPos = mapDataToScreen(range, timestamp)
     else:
        range = mapScreenXToRange(click X)
        screenPos = click position
  → BTWInteractiveOverlay::addDataPointMarker(...)
  → signals propagate: manualMarkerPlaced → GraphContainer → GraphLayout
```

**Source of truth:** `BTWGraph::m_manualMarkerSeries` (per graph instance).
**Integrator entry point:** `GraphLayout::setBTWManualMarkerSeries()` — sets the binding
on **all** BTW graphs at once.

---

## 3. `GraphLayout` public API (main entry point)

**Header:** `graphlayout.h`

```cpp
/**
 * Bind click-placed manual markers on all BTW graphs to a specific series.
 *
 * When a user clicks to place a manual marker, the marker is positioned on the
 * given series' interpolated range at the clicked time (the click's X position is
 * ignored). Applies to every BTW graph across all containers. Pass an empty
 * seriesLabel to clear the binding (markers then use the raw clicked X position).
 */
void setBTWManualMarkerSeries(const QString &seriesLabel);
```

### Related marker APIs (same family)

```cpp
// Programmatic placement — does NOT use series binding; range is explicit.
bool addBTWManualMarker(const QDateTime &timestamp, float rangeValue, float bearingRate = 0.0f);

// Remove all interactive overlay markers from every BTW graph.
void clearBTWManualMarkers();
```

### Example: bind markers to the adopted (measured) series

```cpp
// After BTW data sources are populated:
layout->setBTWManualMarkerSeries(QStringLiteral("ADOPTED"));

// User clicks anywhere on a BTW graph → marker lands on the ADOPTED trace
// at the clicked time, regardless of where they clicked horizontally.
```

### Example: revert to raw X placement

```cpp
layout->setBTWManualMarkerSeries(QString());   // or QStringLiteral("")
```

### Example: switch series at runtime

```cpp
// Follow computed track 1 instead:
layout->setBTWManualMarkerSeries(QStringLiteral("BTW-1"));
```

---

## 4. `BTWGraph` low-level API

**Header:** `btwgraph.h`

`GraphLayout::setBTWManualMarkerSeries()` is a fan-out wrapper. If you manage a
standalone `BTWGraph` directly (e.g. outside `GraphLayout`), set the binding per
instance:

```cpp
btwGraph->setManualMarkerSeries(QStringLiteral("ADOPTED"));
QString bound = btwGraph->manualMarkerSeries();   // query current binding
```

---

## 5. Placement behaviour (click path)

When the operator clicks empty space on a BTW graph (`BTWGraph::onMouseClick`):

1. **Timestamp** — taken from the click **Y** position (`mapScreenToTime`).
2. **System start guard** — placement is blocked if the timestamp is before
   `getApplicationStartTime()`.
3. **Range / X position:**
   - **Series bound** (`m_manualMarkerSeries` non-empty) **and** the BTW data source
     has samples for that series:
     - `range = interpolateSeriesRangeAtTime(series, timestamp)`
     - Screen position is recomputed: `mapDataToScreen(range, timestamp)`
     - The click's **X is ignored**.
     - Marker series label stored as the bound series (e.g. `"ADOPTED"`).
   - **No series bound**, or interpolation returns no samples:
     - `range = mapScreenXToRange(click X)` (raw horizontal click)
     - Marker series label is `"BTW-Click"`.
4. **Marker created** on the interactive overlay with horizontal-only drag
   (`constrainY=true` — marker moves along range, not time).

### Interpolation details

`WaterfallData::interpolateSeriesRangeAtTime(seriesLabel, targetTime, outRange)`:

- Returns `false` if the series does not exist or has no samples → click handler
  **falls back** to raw X placement.
- Before the first sample → uses the first sample's range.
- After the last sample → uses the last sample's range.
- Between two samples → linear interpolation in time.

```cpp
bool interpolateSeriesRangeAtTime(const QString& seriesLabel,
                                  const QDateTime& targetTime,
                                  qreal& outRange) const;
```

---

## 6. Main-system signals

Click placement propagates through the usual marker signal chain:

```
BTWGraph::manualMarkerPlaced(timestamp, scenePos)
  → GraphContainer::BTWManualMarkerPlaced
  → GraphLayout::onBTWManualMarkerPlaced  (adds magenta BTW symbols to other graphs)
  → GraphLayout::BTWManualMarkerPlaced    (forwarded to integrator)
```

Additional signals with the **resolved range** (after series binding):

```cpp
// On GraphLayout — timestamp + range value (not scene position).
void markerTimestampValueChanged(const QDateTime &timestamp, qreal value);

// On GraphLayout — full marker data when an existing marker is clicked.
void markerClickedWithData(const QDateTime &timestamp, qreal rangeValue, qreal bearingRate);
```

| Signal | When | Payload notes |
|--------|------|---------------|
| `BTWManualMarkerPlaced` | Click places a marker | `timestamp`, `position` (scene coords) |
| `markerTimestampValueChanged` | Click places or clicks a marker | `timestamp`, `value` = **bound-series range** when series binding is active |
| `markerClickedWithData` | Existing marker clicked | `timestamp`, `rangeValue`, `bearingRate` |
| `BTWManualMarkerClicked` | Existing marker clicked | `timestamp`, `position` |

> **Integrator tip:** Connect to `markerTimestampValueChanged` or `markerClickedWithData`
> when you need the **data-space range** after series binding. `BTWManualMarkerPlaced`
> carries scene coordinates; `GraphLayout::onBTWManualMarkerPlaced` still derives range
> from `position.x()` for magenta-symbol fan-out, which may differ from the bound-series
> range when series binding is active. Prefer the value-bearing signals for backend logic.

Programmatic `addBTWManualMarker()` also emits `markerTimestampValueChanged` and
`manualMarkerPlaced`, but **does not** go through series binding.

---

## 7. Integrating with the main system

### 7.1 Typical setup

The main system usually feeds multiple BTW series (e.g. computed tracks `BTW-1`…`BTW-3`
and a measured `ADOPTED` series). To make operator-placed markers follow the measured
track:

```cpp
// 1. Populate data (see GRAPH_LAYOUT_INTEGRATION_API.md).
layout->setDataToDataSource(GraphType::BTW, QStringLiteral("ADOPTED"), yData, timestamps);
layout->setDataToDataSource(GraphType::BTW, QStringLiteral("BTW-1"),   yData, timestamps);

// 2. Bind click markers to the measured series.
layout->setBTWManualMarkerSeries(QStringLiteral("ADOPTED"));

// 3. Receive resolved (timestamp, range) pairs.
connect(layout, &GraphLayout::markerTimestampValueChanged,
        backend, &Backend::onBtwMarkerPlaced);
```

### 7.2 Switch binding when the active solution changes

If the operator selects a different computed track, rebind at runtime:

```cpp
void onActiveTrackChanged(const QString &trackLabel)
{
    layout->setBTWManualMarkerSeries(trackLabel);   // e.g. "BTW-2"
}
```

No redraw is required — the binding is consulted only on the next click.

### 7.3 Programmatic markers with explicit range

When placing markers from code (e.g. replaying a saved solution), use
`addBTWManualMarker()` and pass the range directly. Series binding does not apply:

```cpp
layout->addBTWManualMarker(timestamp, rangeValue, bearingRate);
```

### 7.4 Clearing markers and bindings

```cpp
layout->clearBTWManualMarkers();                  // remove overlay markers
layout->setBTWManualMarkerSeries(QString());      // revert to raw X clicks
```

These are independent: clearing markers does not clear the series binding, and clearing
the binding does not remove existing markers.

---

## 8. Marker metadata

Each overlay marker stores:

| Data key | Content |
|----------|---------|
| `0` | `QDateTime` timestamp |
| `1` | Initial range value (`float`) |
| `2` | `QUuid` sync id |
| `3` | Source series label (`"ADOPTED"`, `"BTW-Click"`, `"BTW-API"`, …) |

- Click + series binding → key `3` = bound series name.
- Click, no binding → key `3` = `"BTW-Click"`.
- `addBTWManualMarker()` → key `3` = `"BTW-API"`.

---

## 9. Constraints and edge cases

| Case | Behaviour |
|------|-----------|
| Series label empty | Raw X placement (default). |
| Series label set but series has no data | Falls back to raw X; label `"BTW-Click"`. |
| Click before system start time | Placement blocked (no marker). |
| Click on existing marker / ruler / horizontal line | Handled by that item; no new marker. |
| BTW graph not currently visible | Binding still stored; applies when graph is shown. |
| Multiple container panels | Binding applied to **every** BTW graph in the layout. |
| Drag after placement | Marker moves horizontally (range only); timestamp fixed. Series binding is **not** re-evaluated on drag. |

---

## 10. Comparison with similar APIs

| Feature | API | What it controls |
|---------|-----|------------------|
| History selection valid range | `setHistorySelectionReferenceSeries(graphType, label)` | Time strip selection bounds |
| **BTW manual marker placement** | **`setBTWManualMarkerSeries(label)`** | **Click marker range (X) at a given time** |
| BTW symbol (magenta circle) | `addBTWMarker()` / click fan-out | Synced circles on all graph types |
| Horizontal time line | `setHorizontalLineMode()` | Constant-time overlay lines |

---

## 11. Quick reference

| Goal | Call |
|------|------|
| Bind click markers to a series | `layout->setBTWManualMarkerSeries("ADOPTED")` |
| Revert to raw X clicks | `layout->setBTWManualMarkerSeries(QString())` |
| Place marker at explicit range | `layout->addBTWManualMarker(ts, range)` |
| Remove all overlay markers | `layout->clearBTWManualMarkers()` |
| Receive (time, range) on placement | `connect(..., markerTimestampValueChanged, ...)` |
| Per-graph binding (no layout) | `btwGraph->setManualMarkerSeries(label)` |
| Query per-graph binding | `btwGraph->manualMarkerSeries()` |
