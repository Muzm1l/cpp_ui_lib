# Manual: History time selections

This document describes the **history selection** feature — the vertical strip with an
**H** button that sits beside each waterfall graph container — its public API, the
recent behavioural fixes, and **how the main system integrates** with it.

A history selection is a **time range** (`start → end`) drawn on the vertical strip.
Selections are synchronized across every visible container and reported to the main
system so it can recalculate solutions over the selected time span.

**See also:** [`HORIZONTAL_LINE_API.md`](./HORIZONTAL_LINE_API.md),
[`GRAPH_LAYOUT_INTEGRATION_API.md`](./GRAPH_LAYOUT_INTEGRATION_API.md).

---

## 1. Concepts

| Term | Meaning |
|------|---------|
| **History selection** | A time range `[start, end]` shown as a white rectangle on the strip. |
| **Strip** | The `TimeVisualizerWidget` — maps the vertical axis to time (top = now, bottom = now − interval). |
| **H button** | Creates a selection when there is none; clears all selections when some exist. |
| **Interval** | The timeline length currently shown (15 min, 30 min, …), i.e. the strip's full height in time. |
| **Valid range** | The `[start, end]` bounds a selection is clamped to. Driven by the data (or an explicit reference series). |
| **Selection id** | Stable `QUuid` on each selection, used to address a specific selection across containers. |

### Ownership / sync flow

```
User gesture on the strip (TimeVisualizerWidget)
  → TimeSelectionVisualizer relays signals
  → GraphContainer (onTimeSelectionMade / modified / cleared / fullSelectionRequested)
  → GraphLayout hub (onTimeSelectionCreated / Modified / Cleared)
  → updates m_syncState.timeSelections + fans out to all other containers
  → emits main-system signals
```

- Source of truth: `GraphContainerSyncState::timeSelections` (a `std::vector<TimeSelectionSpan>`).
- Each container mirrors the set in its own `TimeVisualizerWidget`.
- Max **5** selections (`MAX_TIME_SELECTIONS`). Overlapping selections are merged on add.

---

## 2. Data structure

```cpp
struct TimeSelectionSpan
{
    QDateTime startTime;   // range start (older)
    QDateTime endTime;     // range end   (newer)
    QUuid     id;          // stable identity; null until the selection is created
};
```

- `id` is **null by default** (the struct is reused for time windows/scopes that don't
  need one). A non-null `id` is assigned when a history selection is actually created,
  and is **preserved** through resize, drag, and cross-container sync.
- Use `id` — not the list index — when you need to track "the same selection" over time.
  Indices can differ between containers because of per-container merging/clamping.

---

## 3. `GraphLayout` public API (main entry point)

**Header:** `graphlayout.h`, `timelineutils.h`

```cpp
#include "timelineutils.h"   // TimeSelectionSpan
#include "graphtype.h"       // GraphType

// Query the full current set of selections (start/end + id).
std::vector<TimeSelectionSpan> getActiveTimeSelections() const;

// Programmatically add a selection region (clamped + merged like a user gesture).
bool highlightHistorySelectionRegion(const QDateTime &startTime, const QDateTime &endTime);

// Choose which graph + data series drives the valid range and the H selection.
// Pass an empty seriesLabel to clear the override.
void setHistorySelectionReferenceSeries(GraphType graphType, const QString &seriesLabel);
```

### Signals

```cpp
// PREFERRED — full set + which entry changed.
void TimeSelectionsChanged(const std::vector<TimeSelectionSpan> &selections, int changedIndex);

// DEPRECATED — kept for backward compatibility, removed after a stable release.
void TimeSelectionCreated(const TimeSelectionSpan &selection);
void TimeSelectionModified(int index, const TimeSelectionSpan &newSpan);
void TimeSelectionsCleared();
```

| Event | `TimeSelectionsChanged` payload |
|-------|---------------------------------|
| User/API creates a selection | `selections` = full set, `changedIndex` = index of the new one |
| User modifies a selection (resize/drag) | `selections` = full set, `changedIndex` = index of the modified one |
| Selections cleared (H button with existing selections) | `selections` = empty, `changedIndex` = `-1` |

> `TimeSelectionsChanged` fires alongside the deprecated signals for every change,
> so new integrations can rely on it exclusively.

---

## 4. Integrating with the main system

### 4.1 Receive selections (recommended path)

Connect to `TimeSelectionsChanged` and treat the vector as the source of truth. Use the
`id` to recalculate the specific selection that changed.

```cpp
connect(layout, &GraphLayout::TimeSelectionsChanged, backend,
    [backend](const std::vector<TimeSelectionSpan> &selections, int changedIndex) {
        if (changedIndex < 0) {
            backend->onHistorySelectionsCleared();      // all cleared
            return;
        }
        const TimeSelectionSpan &changed = selections[static_cast<size_t>(changedIndex)];
        // Recalculate only the selection the operator touched, addressed by stable id.
        backend->recalculateForSelection(changed.id, changed.startTime, changed.endTime);
        // 'selections' also carries the complete current set if you need it.
    });
```

### 4.2 (Optional) keep supporting the deprecated signals

Existing code that used the single-span signals keeps working unchanged:

```cpp
connect(layout, &GraphLayout::TimeSelectionCreated,  backend, &Backend::onSelectionCreated);
connect(layout, &GraphLayout::TimeSelectionModified, backend, &Backend::onSelectionModified);
connect(layout, &GraphLayout::TimeSelectionsCleared, backend, &Backend::onSelectionsCleared);
```

Plan: migrate to `TimeSelectionsChanged`, then drop these once a stable version ships.

### 4.3 Choose which series the selection considers

The main system typically sends two series: a **measured** series (consistent, longer)
and a **computed** series (changes per solution, often shorter). By default the valid
range / H selection uses the *combined* range of the current graph's data, which can
collapse to the shorter computed extent.

Call this once (after data sources exist) to anchor the range to the series you want:

```cpp
// Consider the "ADOPTED" (measured) series on the BTW graph for history selection.
layout->setHistorySelectionReferenceSeries(GraphType::BTW, QStringLiteral("ADOPTED"));

// Revert to the default combined-range behaviour:
layout->setHistorySelectionReferenceSeries(GraphType::BTW, QString());
```

- `graphType` — the graph whose `WaterfallData` holds the series.
- `seriesLabel` — the series key as added to the data source (e.g. `"ADOPTED"`, `"BTW-1"`).
- If the reference series isn't populated yet, the code falls back to the combined range.

### 4.4 Read the current set on demand

```cpp
const std::vector<TimeSelectionSpan> selections = layout->getActiveTimeSelections();
for (const TimeSelectionSpan &s : selections) {
    qDebug() << s.id << s.startTime << "→" << s.endTime;
}
```

---

## 5. H button behaviour (anchor policy)

The **H** button (`TimeSelectionVisualizer`) is context-sensitive:

| State when H pressed | Action |
|----------------------|--------|
| One or more selections exist | Clear all selections (`TimeSelectionsCleared` / `TimeSelectionsChanged(-1)`) |
| No selections, **no** BTW line | Create `[now − interval, now]` (exactly one interval long) |
| No selections, **BTW line present** | Create `[latestLine, now]` (line is the endpoint; latest line if several) |

Notes:
- "Interval" is the timeline length currently shown (15 min, 30 min, …).
- Both create cases end at the current time; a BTW horizontal line overrides only the
  **start**. See [`HORIZONTAL_LINE_API.md`](./HORIZONTAL_LINE_API.md) for lines.
- The created span is clamped to the valid range (Section 6).

---

## 6. Valid range (clamping)

Selections are clamped to a valid `[start, end]` range so they can't exceed available data.

- The range is stored and compared as full **`QDateTime`** (not time-of-day), so a long
  measured range is not truncated.
- The container recomputes the range from data whenever data or the reference series
  changes (`GraphContainer::refreshHistorySelectionValidRange`).
- `TimeSelectionVisualizer::setValidSelectionRange(start, end)` accepts `QDateTime`
  (preferred) or `QTime` (compatibility, composed with the current date).

Priority when computing the range:

```
reference series set & populated → getTimeRangeSeries(seriesLabel)
otherwise                        → getCombinedTimeRange()  (union of all series)
```

---

## 7. Interaction reference (the strip)

| Gesture | Result |
|---------|--------|
| Click-drag on empty strip | Create a new selection (rubber band → span on release) |
| Double-click | Create the full-range selection |
| Drag the middle of a selection | Move the whole span (duration preserved) |
| Drag the top/bottom edge | Resize that edge (only when the rectangle is tall enough) |
| Click a selection without moving | No-op (span is unchanged, nothing emitted) |

### Constants (`timeselectionvisualizer.h`)

| Constant | Default | Meaning |
|----------|---------|---------|
| `MAX_TIME_SELECTIONS` | 5 | Max simultaneous selections |
| `RESIZE_EDGE_THRESHOLD` | 4 px | Distance from an edge that counts as a resize grab |
| `MIN_SELECTION_SECONDS` | 1 | Minimum duration when resizing |

---

## 8. Behavioural fixes (why the API behaves the way it does)

These recent fixes keep behaviour stable; integrators should be aware of them.

| Symptom (before) | Fix | Where |
|------------------|-----|-------|
| Dragging past the strip edge kept **increasing** the selection time (worst in 2-row layouts) | Clamp the Y coordinate to `[0, height]`; drop the midnight-wrap hack | `TimeVisualizerWidget::yCoordinateToTime` |
| Clicking an existing short selection made its **age/duration grow** on every click | Edge-resize only when the rectangle is tall enough (`> 2 × RESIZE_EDGE_THRESHOLD`); otherwise treat as a center drag. Idempotency guard: a click with no movement changes nothing | `TimeVisualizerWidget::hitTest`, `mouseReleaseEvent` |
| H selected the whole data range | H now selects exactly one interval (or line → now) | `createIntervalSelection`, `GraphContainer::onHistoryFullSelectionRequested` |
| H considered only the shorter computed series | Explicit reference-series API + full-`QDateTime` valid range | `setHistorySelectionReferenceSeries`, `setValidSelectionRange(QDateTime,…)` |
| Main system couldn't tell which selection changed | Stable `id` + `TimeSelectionsChanged(all, changedIndex)` | `TimeSelectionSpan::id`, `GraphLayout` |

---

## 9. Quick reference

| Goal | Call / connect to |
|------|-------------------|
| Get all current selections | `layout->getActiveTimeSelections()` |
| Be notified of any change | `GraphLayout::TimeSelectionsChanged` |
| Recalculate a specific selection | Use `TimeSelectionSpan::id` from the changed entry |
| Add a selection from code | `layout->highlightHistorySelectionRegion(start, end)` |
| Pick which series drives the range | `layout->setHistorySelectionReferenceSeries(GraphType::BTW, "ADOPTED")` |
| Revert to default range | `layout->setHistorySelectionReferenceSeries(GraphType::BTW, QString())` |
| H (no line) selects | `[now − interval, now]` |
| H (BTW line) selects | `[latestLine, now]` |
