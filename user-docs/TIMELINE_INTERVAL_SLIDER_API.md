# Manual: timeline interval and slider time-range API

This document describes the **`GraphLayout`** APIs for reading the current timeline
**interval** (e.g. 15 / 30 minutes) and the **slider visible window** (start / end),
plus the signals that notify when either changes.

**Header:** `graphlayout.h`  
**See also:** [`SYSTEM_START_TIME_API.md`](./SYSTEM_START_TIME_API.md) (session start /
timeline end), [`GRAPH_LAYOUT_INTEGRATION_API.md`](./GRAPH_LAYOUT_INTEGRATION_API.md).

All examples assume you hold a `GraphLayout *layout` created by your host application.

---

## 1. Purpose

| Concept | Meaning |
|--------|---------|
| **Time interval** | Length of the visible timeline window (15 min, 30 min, 1 h, …). Set by the operator via the timeline interval control; shared across all containers. |
| **Slider time range** | The absolute `[start, end]` window currently shown by the vertical timeline slider. Moves when the user drags the slider, on release, in follow mode, and on programmatic seeks. |

Use the **getters** for a one-shot read; use the **signals** to stay in sync without polling.

---

## 2. Time interval API

### 2.1 Getter

```cpp
TimeInterval getTimeInterval() const;
```

Returns the shared timeline length. The enum’s underlying integer is **minutes**:

| Enum value | Minutes |
|------------|---------|
| `TimeInterval::FifteenMinutes` | 15 |
| `TimeInterval::ThirtyMinutes` | 30 |
| `TimeInterval::OneHour` | 60 |
| `TimeInterval::TwoHours` | 120 |
| … | … |

Until the user (or an internal path) has set an interval, returns
`TimeInterval::FifteenMinutes` (the UI default).

Valid selectable intervals are listed by `getValidTimeIntervals()` in `timelineutils.h`.

### 2.2 Signal

```cpp
void TimeIntervalChanged(TimeInterval interval);
```

Emitted once whenever the shared interval changes (operator selects a new length).
All graph containers are already synced before the signal fires.

### 2.3 Examples

**Read current interval as minutes:**

```cpp
TimeInterval interval = layout->getTimeInterval();
int minutes = static_cast<int>(interval);   // e.g. 15 or 30

qDebug() << "Current interval:" << minutes << "min"
         << "(" << timeIntervalToString(interval) << ")";
```

**React when the operator changes the interval:**

```cpp
connect(layout, &GraphLayout::TimeIntervalChanged,
        this, [](TimeInterval interval) {
    int minutes = static_cast<int>(interval);
    // e.g. refresh host-side buffers sized to the visible window
    qDebug() << "Interval changed to" << minutes << "minutes";
});
```

**Branch on a specific value:**

```cpp
connect(layout, &GraphLayout::TimeIntervalChanged,
        this, [this](TimeInterval interval) {
    if (interval == TimeInterval::FifteenMinutes ||
        interval == TimeInterval::ThirtyMinutes) {
        // short windows
    } else {
        // hour-scale windows
    }
});
```

---

## 3. Slider time-range API

### 3.1 Getters

```cpp
std::pair<QDateTime, QDateTime> getSliderTimeRange() const;
TimeSelectionSpan               getSliderTimeScope() const;
```

Both return the same shared visible window that every container follows.

| Method | Return |
|--------|--------|
| `getSliderTimeRange()` | `{ startTime, endTime }` pair |
| `getSliderTimeScope()` | `TimeSelectionSpan` with the same `startTime` / `endTime` |

Either boundary may be **invalid** if no scope has been published yet (e.g. right after
construction, before the first slider / follow-mode update). Always check
`QDateTime::isValid()` before using the values.

### 3.2 Signal

```cpp
void SliderTimeRangeChanged(const QDateTime &startTime, const QDateTime &endTime);
```

Emitted whenever the visible window changes:

- **While dragging** the slider (throttled via `TimeScopeBus`, ~16 ms)
- **On mouse release** (committed window)
- **Follow mode** ticks and **programmatic** seeks that publish into the scope bus

You do **not** need to distinguish drag vs commit for a simple “what is on screen?”
listener; both are covered by this signal.

### 3.3 Examples

**One-shot read of the current window:**

```cpp
auto [start, end] = layout->getSliderTimeRange();

if (start.isValid() && end.isValid()) {
    qDebug() << "Slider window:"
             << start.toString(Qt::ISODate)
             << "→"
             << end.toString(Qt::ISODate);
}
```

**Same data as a span:**

```cpp
TimeSelectionSpan scope = layout->getSliderTimeScope();
if (scope.startTime.isValid() && scope.endTime.isValid()) {
    // use scope.startTime / scope.endTime
}
```

**Live updates while the operator drags the slider:**

```cpp
connect(layout, &GraphLayout::SliderTimeRangeChanged,
        this, [](const QDateTime &startTime, const QDateTime &endTime) {
    if (!startTime.isValid() || !endTime.isValid())
        return;

    // e.g. update a host overlay, status bar, or query window
    qDebug() << "Slider:"
             << startTime.toString("HH:mm:ss")
             << "–"
             << endTime.toString("HH:mm:ss");
});
```

**Query duration of the visible window:**

```cpp
connect(layout, &GraphLayout::SliderTimeRangeChanged,
        this, [](const QDateTime &startTime, const QDateTime &endTime) {
    if (!startTime.isValid() || !endTime.isValid())
        return;

    const qint64 durationMs = startTime.msecsTo(endTime);
    qDebug() << "Visible duration:" << (durationMs / 1000.0) << "s";
});
```

---

## 4. Wiring both together

Typical host setup after creating the layout:

```cpp
// Initial snapshot
TimeInterval interval = layout->getTimeInterval();
auto [start, end] = layout->getSliderTimeRange();

qDebug() << "Boot:" << static_cast<int>(interval) << "min window"
         << start.toString(Qt::ISODate) << "→" << end.toString(Qt::ISODate);

// Stay in sync
connect(layout, &GraphLayout::TimeIntervalChanged,
        this, &HostController::onTimelineIntervalChanged);

connect(layout, &GraphLayout::SliderTimeRangeChanged,
        this, &HostController::onSliderTimeRangeChanged);
```

```cpp
void HostController::onTimelineIntervalChanged(TimeInterval interval)
{
    m_visibleMinutes = static_cast<int>(interval);
}

void HostController::onSliderTimeRangeChanged(const QDateTime &start,
                                              const QDateTime &end)
{
    if (!start.isValid() || !end.isValid())
        return;
    m_windowStart = start;
    m_windowEnd   = end;
    // refresh anything that depends on the visible [start, end]
}
```

---

## 5. Notes

- These APIs are **read / notify only**. Changing the interval or seeking the slider
  from the host still goes through the existing timeline / `TimeScopeBus` paths
  (or UI controls), not through these getters.
- Interval and slider window are **layout-wide**: one value is shared by all
  containers in the current `GraphLayout`.
- `SliderTimeRangeChanged` is driven by the layout’s subscription to
  `TimeScopeBus`; do not also write `GraphContainerSyncState::currentTimeScope`
  yourself — the layout is the sole writer of that field.
- Related lower-level access (usually unnecessary for hosts):
  - `layout->getSyncState()->currentInterval` / `hasInterval`
  - `layout->getSyncState()->currentTimeScope` / `hasTimeScope`
  - `layout->getScopeBus()->subscribe(...)`
