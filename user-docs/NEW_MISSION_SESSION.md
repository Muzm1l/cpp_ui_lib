# Manual: starting a new mission session

This document describes how to **stop data**, **clear the UI**, **set a new system start time**, and **load new data** when a mission ends and a fresh one begins.

It complements [`SYSTEM_START_TIME_API.md`](SYSTEM_START_TIME_API.md) (what `setSystemStartTime` does) and [`GRAPH_LAYOUT_INTEGRATION_API.md`](GRAPH_LAYOUT_INTEGRATION_API.md) (data load/clear APIs).

All examples assume you hold a `GraphLayout *layout` created by your host application.

---

## 1. Use case

| Phase | What happens |
|-------|----------------|
| **Old mission ends** | Live feed stops; you clear all graph data and session overlays. |
| **New mission begins** | Backend supplies a new **mission start time** and new sample timestamps. |
| **Expected UI state** | Timelines anchor at the new start; sliders span **new start → now**; graphs show only the new data. |

This API path does **not** rewrite existing sample timestamps. Your backend must pass data with timestamps appropriate for the new mission.

---

## 2. APIs involved

| API | Role in new session |
|-----|---------------------|
| `clearAllGraphs()` | Clears all graph engines: data series, RTW/BTW symbols, BTW/RTW markers. |
| `clearTimelineEndOverride()` | Returns the timeline “now” edge to wall clock (exits replay/pause). |
| `setSystemStartTime(t)` | Sets shared mission start; propagates to all timelines and waterfall graphs. |
| `clearHorizontalLines()` | Removes synced horizontal lines. |
| `clearAllShadedRegions()` | Removes synced shaded regions. |
| `clearManoeuvres()` | Removes manoeuvre overlays. |
| `clearAllBtwRulers()` / `clearAllRtwRulers()` | Removes ruler overlays. |
| `deleteInteractiveMarkers()` | Removes interactive markers on all containers. |
| `GraphContainer::clearTimeSelections()` | Clears history-selection bands (call on each container). |
| `setDataToDataSource(...)` / `addDataPointsToDataSource(...)` | Load new data after reset. |

**Header:** `graphlayout.h` (container methods: `graphcontainer.h`)

---

## 3. Recommended sequence

Always follow this order:

```
1. Stop live data feed
2. Clear graph data (clearAllGraphs)
3. Clear session overlays + timeline override
4. setSystemStartTime(newMissionStart)   ← before loading new data
5. Load new data (valid timestamps)
6. Resume live feed (if applicable)
```

### Why this order matters

1. **Stop feed first** — avoids new points arriving while you are clearing.
2. **`clearAllGraphs()`** — wipes engines but **not** lines, rulers, manoeuvres, time selections, or replay override.
3. **`setSystemStartTime` before data** — timelines snap to the new anchor immediately; marker and slider rules use the new start from the first interaction.
4. **New data last** — timestamps must already reflect the new mission; the library does not remap old timestamps.

---

## 4. Full example

```cpp
#include "graphlayout.h"
#include "graphcontainer.h"
#include "graphtype.h"

// Optional: your live feed handle (e.g. Simulator in ui-sandbox)
class DataFeed {
public:
    virtual void stop() = 0;
    virtual void start() = 0;
    virtual bool isRunning() const = 0;
};

void beginNewMission(GraphLayout *layout,
                     DataFeed *feed,
                     const QDateTime &newMissionStart,
                     const MissionData &missionData)
{
    if (!layout || !newMissionStart.isValid())
        return;

    // ── 1. Stop live feed ──────────────────────────────────────────────
    if (feed && feed->isRunning())
        feed->stop();

    // ── 2. Clear all graph data, symbols, and markers ────────────────
    layout->clearAllGraphs();

    // ── 3. Clear session overlays (not covered by clearAllGraphs) ────
    layout->clearTimelineEndOverride();
    layout->clearHorizontalLines();
    layout->clearAllShadedRegions();
    layout->clearManoeuvres();
    layout->clearAllBtwRulers();
    layout->clearAllRtwRulers();
    layout->deleteInteractiveMarkers();

    for (GraphContainer *container : layout->findChildren<GraphContainer *>())
    {
        if (container)
            container->clearTimeSelections();
    }

    // ── 4. Set new system / mission start ────────────────────────────
    layout->setSystemStartTime(newMissionStart);

    // ── 5. Load new data (timestamps >= newMissionStart) ─────────────
    layout->setDataToDataSource(GraphType::BTW, QStringLiteral("BTW-1"),
                                missionData.btwY, missionData.btwTimestamps);
    layout->setDataToDataSource(GraphType::RTW, QStringLiteral("RTW-1"),
                                missionData.rtwY, missionData.rtwTimestamps);
    // ... other graph types as needed

    // ── 6. Resume live feed ──────────────────────────────────────────
    if (feed)
        feed->start();
}
```

### Minimal example (data and overlays already clean)

When you only need to reset the time anchor after `clearAllGraphs()`:

```cpp
feed->stop();
layout->clearAllGraphs();
layout->clearTimelineEndOverride();
layout->setSystemStartTime(newMissionStart);
layout->setDataToDataSource(GraphType::BTW, "BTW-1", yData, timestamps);
feed->start();
```

---

## 5. System start time

Use **`setSystemStartTime`**, not **`clearSystemStartTime`**, when your backend provides an explicit mission start:

```cpp
// Correct for a new mission with a known start from the backend
layout->setSystemStartTime(newMissionStart);
```

| Call | When to use |
|------|-------------|
| `setSystemStartTime(t)` | New mission with a known start time (typical integration path). |
| `clearSystemStartTime()` | Drop fixed mission start; anchor falls back to wall-clock **now**. Rarely needed for a deliberate new mission. |

After `setSystemStartTime`:

- **`GraphContainerSyncState::applicationStartTime`** is updated for all containers.
- Timelines in **FOLLOW_MODE** snap to `[effectiveTimelineEnd − interval, effectiveTimelineEnd]`.
- All waterfall graphs receive **`setApplicationStartTime(newMissionStart)`**.
- BTW manual markers before `newMissionStart` are blocked.

See [`SYSTEM_START_TIME_API.md`](SYSTEM_START_TIME_API.md) for timeline slider and replay-override details.

---

## 6. Data timestamp requirements

New samples must use timestamps **on or after** the mission start you set:

```cpp
const QDateTime missionStart = backend.missionStartTime();  // e.g. from mission record

layout->setSystemStartTime(missionStart);

std::vector<QDateTime> timestamps = {
    missionStart,
    missionStart.addSecs(60),
    missionStart.addSecs(120),
};
std::vector<float> yValues = { 22.0f, 22.5f, 23.0f };

layout->setDataToDataSource(GraphType::BTW, "BTW-1", yValues, timestamps);
```

| Situation | Result |
|-----------|--------|
| Timestamps aligned with `newMissionStart` | Graphs and timelines behave as expected. |
| Timestamps still from the **old** mission | Data may render off-window or look empty; start-time reset alone does not fix this. |
| Timestamps before `newMissionStart` | BTW marker placement is blocked; slider drag clamps to start. |

For bulk historical load, prefer **`setDataToDataSource`**. For live streaming after the initial load, use **`addDataPointToDataSource`** or **`addDataPointsToDataSource`**.

---

## 7. What `clearAllGraphs()` clears vs. what it does not

### Cleared by `clearAllGraphs()`

- All data series in every `GraphEngine`
- RTW symbols, BTW symbols
- BTW markers, RTW R markers
- Triggers redraw on all containers

### Not cleared — call explicitly in step 3

| State | API |
|-------|-----|
| Replay / paused “now” edge | `clearTimelineEndOverride()` |
| Horizontal lines | `clearHorizontalLines()` |
| Shaded regions | `clearAllShadedRegions()` |
| Manoeuvres | `clearManoeuvres()` |
| BTW / RTW rulers | `clearAllBtwRulers()`, `clearAllRtwRulers()` |
| Interactive markers (non-engine) | `deleteInteractiveMarkers()` |
| History time selections | `container->clearTimeSelections()` per container |

---

## 8. Replay and paused sessions

If the previous mission used a timeline end override (replay paused at time **T**):

```cpp
layout->clearTimelineEndOverride();   // required before new live mission
layout->setSystemStartTime(newMissionStart);
```

Without clearing the override, **`effectiveTimelineEnd()`** stays at **T** instead of wall clock, and follow mode will not track real time.

---

## 9. ui-sandbox reference

In the sandbox app, live data is driven by **`Simulator`**:

```cpp
simulator->stop();
layout->clearAllGraphs();
layout->clearTimelineEndOverride();
layout->setSystemStartTime(newMissionStart);
// load data via layout->setDataToDataSource(...) or simulator bulk helpers
simulator->start();
```

Initial construction uses the same start-time concept:

```cpp
const QDateTime systemStartTime = QDateTime::currentDateTime().addSecs(-4 * 3600);
auto *layout = new GraphLayout(parent, LayoutType::GPW4W, timer, seriesLabelsMap, systemStartTime);
```

For a **runtime** new mission, call **`setSystemStartTime`** again with the new value — you do not need to recreate `GraphLayout`.

---

## 10. Pitfalls

1. **Invalid `QDateTime`** — `setSystemStartTime(invalid)` is ignored. Validate before calling.
2. **Loading data before setting start** — timelines and marker rules may briefly use the old anchor.
3. **Forgetting overlay cleanup** — stale lines, rulers, or selections can remain after `clearAllGraphs()`.
4. **Stale replay override** — follow mode stays pinned to old time **T** until `clearTimelineEndOverride()`.
5. **Old timestamps in new data** — the library does not remap samples; your backend must supply correct times.
6. **Race with live feed** — stop the feed before clear; restart only after start time and initial data are set.

---

## 11. Quick reference

| Step | Call |
|------|------|
| Stop feed | `feed->stop()` (your backend) |
| Clear data | `layout->clearAllGraphs()` |
| Exit replay | `layout->clearTimelineEndOverride()` |
| Clear overlays | lines, shaded regions, manoeuvres, rulers, markers, time selections (see §7) |
| New start time | `layout->setSystemStartTime(newMissionStart)` |
| Load data | `layout->setDataToDataSource(...)` / `addDataPointsToDataSource(...)` |
| Resume feed | `feed->start()` (your backend) |
| Query start | `layout->systemStartTime()` |

---

## 12. Related documents

| Document | Topic |
|----------|-------|
| [`SYSTEM_START_TIME_API.md`](SYSTEM_START_TIME_API.md) | `setSystemStartTime`, timeline end override, slider behavior |
| [`GRAPH_LAYOUT_INTEGRATION_API.md`](GRAPH_LAYOUT_INTEGRATION_API.md) | `setDataToDataSource`, `clearDataSource`, series management |
| [`SYMBOL_API.md`](SYMBOL_API.md) | `clearAllGraphs`, symbol and marker clearing |
| [`HORIZONTAL_LINE_API.md`](HORIZONTAL_LINE_API.md) | Horizontal line clear API |
