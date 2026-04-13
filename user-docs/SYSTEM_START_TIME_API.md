# Manual: system start time and timeline end API

This document describes the APIs on **`GraphLayout`** and **`GraphContainerSyncState`**, how they interact with **timelines** and **waterfall graphs**, and what changes at runtime.

---

## 1. Purpose

| Concept | Meaning |
|--------|---------|
| **System / session start time** | Earliest time that should anchor the **vertical timeline slider** mapping. While dragging, the slider spans from this time to the **effective timeline end** (see below). Data before this time is still clamped in slider math when a shared start is set. |
| **Effective timeline end** | The **right edge of “live” time** for the UI: normally **wall-clock now**, unless you set a **timeline end override** (e.g. paused replay). |

Together they replace scattered use of `QDateTime::currentDateTime()` for timeline follow mode, slider position-to-time mapping, and some “is this window near now?” checks in **`GraphContainer`**.

---

## 2. `GraphLayout` API

### 2.1 Set at construction (optional)

```cpp
GraphLayout *layout = new GraphLayout(
    parent,
    LayoutType::GPW4W,
    timer,
    seriesLabelsMap,
    missionStart   // QDateTime — optional 5th argument; default = invalid
);
```

- If **`missionStart` is valid**, it is written to shared sync **before** any **`GraphContainer`** / **`TimelineView`** is created.
- If omitted (invalid), **`initializeContainers()`** sets shared start to **`QDateTime::currentDateTime()`** the first time (same as legacy behavior).

### 2.2 Set after construction

```cpp
void setSystemStartTime(const QDateTime &t);
```

- **`t` must be valid**; invalid `QDateTime` is ignored.
- Sets **`GraphContainerSyncState::applicationStartTime`** and **`hasApplicationStartTime = true`**.
- Calls **`propagateSystemStartTimeToContainers()`**: every container updates its **timeline** and **all waterfall graphs**’ `setApplicationStartTime(...)`.

```cpp
QDateTime systemStartTime() const;
```

- Returns the shared start if **`hasApplicationStartTime`** and the stored time is valid; otherwise an **invalid** `QDateTime`.

```cpp
void clearSystemStartTime();
```

- Clears the shared flag and stored start.
- Propagates: timelines get **`setSystemStartTime(QDateTime::currentDateTime())`** as local anchor; waterfalls get the same for **`setApplicationStartTime`**.

### 2.3 Timeline “now” edge (replay / override)

```cpp
void setTimelineEndOverride(const QDateTime &t);
void clearTimelineEndOverride();
```

- **`setTimelineEndOverride`**: if **`t` is valid**, **`effectiveTimelineEnd()`** returns **`t`** instead of wall clock everywhere sync state is used (slider follow mode, drag mapping clamp “now”, relative labels, some graph “near live” checks).
- **`clearTimelineEndOverride`**: back to wall clock.

Direct sync access (if you do not use the wrappers):

```cpp
GraphContainerSyncState *s = graphLayout->getSyncState();
s->effectiveTimelineEnd();
s->setTimelineEndOverride(t);
s->clearTimelineEndOverride();
```

---

## 3. Lower-level / widget API (usually via `GraphLayout`)

| API | Role |
|-----|------|
| **`TimelineView::setSystemStartTime(const QDateTime &t)`** | Forwards to the internal visualizer; normally you only need **`GraphLayout::setSystemStartTime`**. |
| **`TimelineVisualizerWidget::effectiveTimelineEnd()`** | Current right edge (override or wall clock). |
| **`TimelineVisualizerWidget::setSystemStartTime`** | Updates local start + in **FOLLOW_MODE** snaps the visible window to **`[end − interval, end]`** with **`end = effectiveTimelineEnd()`**. |

---

## 4. Effects in the UI and data path

### 4.1 Timeline slider

- **Drag mapping** uses **`[systemStart, effectiveTimelineEnd]`** when a valid application/system start applies (same idea as before, but **end** can be overridden).
- **Follow mode** advances the visible window so its **end** tracks **`effectiveTimelineEnd()`**, not only raw wall clock when an override is active.
- **Relative timestamp labels** use **`effectiveTimelineEnd()`** as “now” for offsets.

### 4.2 Waterfall graphs (e.g. BTW)

- **`WaterfallGraph::setApplicationStartTime`** is kept in line with the session start when you use **`GraphLayout::setSystemStartTime`** / **`clearSystemStartTime`** / construction parameter.
- Existing BTW logic that reads **`getApplicationStartTime()`** continues to see a consistent value.

### 4.3 `GraphContainer` “recent data” behavior

- Places that compared the graph’s time range to “current time” now use **`m_syncState->effectiveTimelineEnd()`** (with fallback to wall clock if there is no sync).
- So with a **timeline end override**, “within ~1 minute of live edge” follows the **override**, not the real clock.

---

## 5. Typical usage scenarios

### 5.1 Mission started in the past (live app starts later)

```cpp
QDateTime missionStart = /* e.g. three hours ago, from your backend */;

// Either:
auto *layout = new GraphLayout(parent, LayoutType::GPW4W, timer, map, missionStart);

// Or after `new GraphLayout(...)` without the 5th arg:
layout->setSystemStartTime(missionStart);
```

**Effect:** Slider span reflects **mission start → now** (or override). Timelines and waterfalls share the same start. Live data should still use **real** timestamps in that range.

### 5.2 Change mission mid-session

```cpp
layout->setSystemStartTime(newMissionStart);
```

**Effect:** Shared start updates; **FOLLOW_MODE** timelines snap to a window ending at **`effectiveTimelineEnd()`**; frozen timelines keep their window but the **start anchor** used for labels/clamping updates.

### 5.3 Replay paused at time T

```cpp
layout->setTimelineEndOverride(T);
// Optionally still setSystemStartTime(recordStart) if needed
```

**Effect:** The UI treats **`T`** as the live right edge until you **`clearTimelineEndOverride()`**.

### 5.4 Back to default “wall clock live”

```cpp
layout->clearTimelineEndOverride();
// Optional: layout->clearSystemStartTime();  // if you want default anchor = “now” only
```

---

## 6. Pitfalls and notes

1. **Invalid datetimes** – **`setSystemStartTime(invalid)`** does nothing. Always validate before calling.
2. **Order** – Using the **5th constructor argument** avoids one frame where start was “now” before you set a past mission time.
3. **Data timestamps** – This API does **not** rewrite your samples; it only changes **how the timeline and some range heuristics** interpret time. Historical/replay data must still carry correct **`QDateTime`** values.
4. **Override vs clock** – If **`setTimelineEndOverride`** is stale while wall clock moves, follow mode stays pinned to **`T`** until you clear or update the override.
5. **`getSyncState()`** – Gives the same **`GraphContainerSyncState`** all containers share; use it if you integrate from outside **`GraphLayout`** but keep a pointer to the layout’s sync object.

---

## 7. Quick reference

| Call | Effect |
|------|--------|
| `GraphLayout(..., systemStartTimeAtInit)` | Initial shared start before widgets exist |
| `setSystemStartTime(t)` | Shared start + propagate to all containers |
| `systemStartTime()` | Query shared start |
| `clearSystemStartTime()` | Drop shared start; anchor propagation uses **now** |
| `setTimelineEndOverride(t)` | “Now” for timeline/slider/heuristics = **t** |
| `clearTimelineEndOverride()` | “Now” = wall clock again |
| `getSyncState()->effectiveTimelineEnd()` | Read current effective end |


  // Session / system start: four hours before wall-clock now (timeline slider span anchor)
    const QDateTime systemStartTime = QDateTime::currentDateTime().addSecs(-4 * 3600);
    graphgrid = new GraphLayout(ui->originalTab, LayoutType::GPW4W, timeUpdateTimer, seriesLabelsMap, systemStartTime);
    DEBUG_OUT() << "MainWindow: system start time (4h before now):" << systemStartTime.toString(Qt::ISODate);