# GraphLayout & SCW — complete API reference

Host-facing APIs on **`GraphLayout`** (`graphlayout.h`) and **`SCWWindow`**
(`scwwindow.h`), grouped by concern and by graph type wherever that applies.

| Surface | Role |
|---------|------|
| **`GraphLayout`** | Main GPW grid: up to 4 containers, graph types BDW / BRW / BTW / FDW / FTW / LTW / RTW, shared timeline, markers, lines, selections |
| **`SCWWindow`** | Separate 8-pane SCW view; not owned by `GraphLayout`; shares sync via `GraphContainerSyncState` |

**Detailed feature manuals** (examples, edge cases):

| Topic | Doc |
|-------|-----|
| History selection | [`HISTORY_SELECTION_API.md`](./HISTORY_SELECTION_API.md) |
| Timeline interval / slider | [`TIMELINE_INTERVAL_SLIDER_API.md`](./TIMELINE_INTERVAL_SLIDER_API.md) |
| System start / timeline end | [`SYSTEM_START_TIME_API.md`](./SYSTEM_START_TIME_API.md) |
| Horizontal lines | [`HORIZONTAL_LINE_API.md`](./HORIZONTAL_LINE_API.md) |
| RTW rulers | [`RTW_RULER_API.md`](./RTW_RULER_API.md) |
| Symbols | [`SYMBOL_API.md`](./SYMBOL_API.md) |
| BTW magenta sync | [`BTW_MAGENTA_CIRCLE_SYNC.md`](./BTW_MAGENTA_CIRCLE_SYNC.md) |
| BTW manual marker series | [`BTW_MANUAL_MARKER_SERIES_API.md`](./BTW_MANUAL_MARKER_SERIES_API.md) |
| SCW integration notes | [`SCW_WINDOW_API.md`](./SCW_WINDOW_API.md) |
| Layout integration snippets | [`GRAPH_LAYOUT_INTEGRATION_API.md`](./GRAPH_LAYOUT_INTEGRATION_API.md) |
| Recent integration APIs | [`INTEGRATION_NEW_APIS.md`](./INTEGRATION_NEW_APIS.md) |

---

## Table of contents

1. [Graph types & layout modes](#1-graph-types--layout-modes)
2. [GraphLayout — construction](#2-graphlayout--construction)
3. [GraphLayout — layout & containers](#3-graphlayout--layout--containers)
4. [GraphLayout — data sources (all graph types)](#4-graphlayout--data-sources-all-graph-types)
5. [GraphLayout — timeline, slider, sync](#5-graphlayout--timeline-slider-sync)
6. [GraphLayout — history selection](#6-graphlayout--history-selection)
7. [GraphLayout — manoeuvres](#7-graphlayout--manoeuvres)
8. [GraphLayout — horizontal lines (all graphs)](#8-graphlayout--horizontal-lines-all-graphs)
9. [BTW](#9-btw)
10. [RTW](#10-rtw)
11. [LTW / FTW](#11-ltw--ftw)
12. [BDW / BRW / FDW](#12-bdw--brw--fdw)
13. [GraphLayout — chrome, capacity, redraw](#13-graphlayout--chrome-capacity-redraw)
14. [GraphLayout — signals & public slots](#14-graphlayout--signals--public-slots)
15. [SCWWindow](#15-scwwindow)
16. [Wiring GraphLayout + SCW](#16-wiring-graphlayout--scw)
17. [Magenta circles on SCW — required host wiring](#17-magenta-circles-on-scw--required-host-wiring)

---

## 1. Graph types & layout modes

### Graph types (`graphtype.h`)

| `GraphType` | Typical use |
|-------------|-------------|
| `BTW` | Bearing-time waterfall — markers, shaded regions, rulers, magenta sync source |
| `RTW` | Range-time — symbols, R markers, rulers |
| `LTW` | Long-term waterfall |
| `FTW` | Frequency-time waterfall |
| `BDW` | Bearing-deviation — middle line; magenta circle on middle line |
| `BRW` | Bearing-rate — middle line |
| `FDW` | Frequency-deviation — middle line |

`SCW` is **not** a `GraphType`; it is a separate widget (`SCWWindow`).

### Layout modes (`LayoutType`)

| Value | Arrangement |
|-------|-------------|
| `GPW1W` | 1 window |
| `GPW4W` | 2×2 (0=TL, 1=TR, 2=BL, 3=BR) |
| `GPW2WV` | 2 vertical |
| `GPW2WH` | 2 horizontal |
| `GPW4WH` | 4 horizontal |
| `NOGPW2WH` | 2 horizontal, full screen |
| `HIDDEN` | Hidden |

---

## 2. GraphLayout — construction

```cpp
GraphLayout(
    QWidget *parent,
    LayoutType layoutType,
    QTimer *timer = nullptr,   // layout creates a 1s timer if null
    std::map<GraphType, std::vector<QPair<QString, QColor>>> seriesLabelsMap = {},
    const QDateTime &systemStartTimeAtInit = QDateTime()
);
```

- `seriesLabelsMap` seeds per-graph series labels/colors and engines.
- Valid `systemStartTimeAtInit` anchors the timeline slider before containers are created.

---

## 3. GraphLayout — layout & containers

| API | Purpose |
|-----|---------|
| `void setLayoutType(LayoutType)` / `LayoutType getLayoutType() const` | Switch / read grid mode |
| `void setGraphViewSize(int width, int height)` | Preferred graph view size |
| `void updateLayoutSizing()` | Recompute geometry |
| `std::vector<QString> getContainerLabels() const` | Container labels |
| `bool hasContainer(const GraphType &) const` | Container present for type |
| `std::vector<QString> getVisibleGraphNames() const` | On-screen graph names (hidden containers excluded) |
| `void linkHorizontalContainers()` | Re-wire container linking for current layout |
| `GraphContainerSyncState* getSyncState()` | Shared sync state (pass to SCW) |
| `TimeScopeBus* getScopeBus()` | Visible-window bus (layout is sole writer of `currentTimeScope`) |

### Which graph is shown

**By container label**

```cpp
void addDataOption(const QString &containerLabel, const GraphType &, WaterfallData &);
void removeDataOption(const QString &containerLabel, const GraphType &);
void clearDataOptions(const QString &containerLabel);
void setCurrentDataOption(const QString &containerLabel, const GraphType &);
GraphType getCurrentDataOption(const QString &containerLabel) const;
std::vector<GraphType> getAvailableDataOptions(const QString &containerLabel) const;
WaterfallData *getDataOption(const QString &containerLabel, const GraphType &);
bool hasDataOption(const QString &containerLabel, const GraphType &) const;
```

**By index (0–3)**

```cpp
void setContainerGraphType(int containerIndex, const GraphType &);
GraphType getContainerGraphType(int containerIndex) const;
```

**All visible containers**

```cpp
void addDataOption(const GraphType &, WaterfallData &);
void removeDataOption(const GraphType &);
void clearDataOptions();
void setCurrentDataOption(const GraphType &);
```

**Signal:** `VisibleGraphsChanged()` — call `getVisibleGraphNames()` to refresh.

---

## 4. GraphLayout — data sources (all graph types)

Every method takes `GraphType` so it targets that graph’s engine/data across containers.

### Feed / clear

```cpp
void addDataPointToDataSource(GraphType, const QString &seriesLabel, float y, const QDateTime &t);
void addDataPointsToDataSource(GraphType, const QString &seriesLabel,
                               const std::vector<float> &y, const std::vector<QDateTime> &t);
void setDataToDataSource(GraphType, const QString &seriesLabel,
                         const std::vector<float> &y, const std::vector<QDateTime> &t);
void setDataToDataSource(GraphType, const QString &seriesLabel, const WaterfallData &data);
void clearDataSource(GraphType, const QString &seriesLabel);
void clearGraph(const GraphType &);   // force clear + redraw one type
void clearAllGraphs();                // everything
```

### Interactive drag (e.g. while a ruler moves)

```cpp
void setDataToDataSourceInteractive(GraphType, const QString &seriesLabel,
                                    const std::vector<float> &y, const std::vector<QDateTime> &t);
void endInteractiveDrag(const GraphType &);
void flushPendingInteractiveUpdates(const GraphType &);
```

### Series / engine lookup

```cpp
WaterfallData *getDataSource(const GraphType &);
bool hasDataSource(const GraphType &) const;
std::vector<GraphType> getDataSourceLabels() const;
GraphEngine *getEngine(const GraphType &);
bool hasSeriesInDataSource(GraphType, const QString &seriesLabel) const;
std::vector<QString> getSeriesLabelsInDataSource(GraphType) const;
void addSeriesToDataSource(GraphType, const QString &seriesLabel);
void removeSeriesFromDataSource(GraphType, const QString &seriesLabel);
```

### Line vs scatter (per series)

```cpp
void setSeriesRenderMode(GraphType, const QString &seriesLabel, bool asLine);
void clearSeriesRenderModes(GraphType);
// "ADOPTED" defaults to line even without an explicit call
```

### Hard Y limits

```cpp
void setHardRangeLimits(GraphType, qreal yMin, qreal yMax);
void removeHardRangeLimits(GraphType);
void clearAllHardRangeLimits();
bool hasHardRangeLimits(GraphType) const;
std::pair<qreal, qreal> getHardRangeLimits(GraphType) const;
```

### Redraw / track switch

```cpp
void redrawGraph(const GraphType &);
void redrawAllGraphs();
void markTrackChanged();   // multi-track: clear caches, visible-window-first render
```

---

## 5. GraphLayout — timeline, slider, sync

| API | Purpose |
|-----|---------|
| `TimeInterval getTimeInterval() const` | Window length (enum int = minutes; default 15) |
| `std::pair<QDateTime,QDateTime> getSliderTimeRange() const` | Slider `[start, end]` |
| `TimeSelectionSpan getSliderTimeScope() const` | Same as span |
| `void setSystemStartTime(const QDateTime &)` / `QDateTime systemStartTime() const` / `clearSystemStartTime()` | Session start for slider mapping |
| `void setTimelineEndOverride(const QDateTime &)` / `clearTimelineEndOverride()` | Replay / paused “now” |
| `void setCurrentTime(const QTime &)` | Push current time to containers |
| `void syncAllTimelineViews()` | Sync internal timelines |
| `void syncExternalTimelineView(TimelineView *)` | Sync SCW (or other) timeline |

**Signals**

```cpp
void TimeIntervalChanged(TimeInterval interval);
void SliderTimeRangeChanged(const QDateTime &startTime, const QDateTime &endTime);
```

```cpp
connect(layout, &GraphLayout::TimeIntervalChanged, this, [](TimeInterval i) {
    int minutes = static_cast<int>(i);  // 15, 30, …
});
connect(layout, &GraphLayout::SliderTimeRangeChanged, this,
        [](const QDateTime &a, const QDateTime &b) { /* live while dragging */ });
```

---

## 6. GraphLayout — history selection

Applies to the history-selection bar on each container (not graph-type-specific).

```cpp
bool highlightHistorySelectionRegion(const QDateTime &start, const QDateTime &end);
std::vector<TimeSelectionSpan> getActiveTimeSelections() const;
void setHistorySelectionReferenceSeries(GraphType graphType, const QString &seriesLabel);
// e.g. setHistorySelectionReferenceSeries(GraphType::BTW, "ADOPTED");
```

**Preferred signal**

```cpp
void TimeSelectionsChanged(const std::vector<TimeSelectionSpan> &selections, int changedIndex);
// changedIndex == -1 when cleared
```

Deprecated (prefer `TimeSelectionsChanged`): `TimeSelectionCreated`, `TimeSelectionModified`, `TimeSelectionsCleared`.

---

## 7. GraphLayout — manoeuvres

Propagated to all containers / graphs that draw manoeuvres.

```cpp
void addManoeuvre(const Manoeuvre &);
void setManoeuvres(const std::vector<Manoeuvre> &);
void clearManoeuvres();
std::vector<Manoeuvre> getManoeuvres() const;
void startManoeuvreDrawing(const QDateTime &start, int bearing, int speed, int depth);
void endManoeuvreDrawing(const QDateTime &end);
```

---

## 8. GraphLayout — horizontal lines (all graphs)

Constant-time overlays mirrored across **every** graph type in the layout.

```cpp
void setHorizontalLineMode(HorizontalLineMode mode);  // Normal | DrawLine | DeleteLine
HorizontalLineMode horizontalLineMode() const;
QUuid addHorizontalLine(const QDateTime &t, const QColor & = Qt::white, qreal width = 2.0);
QDateTime getHorizontalLineTimestamp(const QUuid &syncId) const;
bool removeHorizontalLine(const QUuid &syncId);
void clearHorizontalLines();
std::vector<HorizontalLineSyncData> getActiveHorizontalLines() const;
```

**User-action signals** (not emitted by programmatic add/remove):

```cpp
void HorizontalLineAdded(const QUuid &syncId, const QDateTime &timestamp);
void HorizontalLineRemoved(const QUuid &syncId, const QDateTime &timestamp);
```

Deprecated aliases (`graphType` ignored): `setBTWHorizontalLineMode`, `addBTWHorizontalLine`, …

---

## 9. BTW

Bearing-time graph: markers, manual markers, rulers, shaded regions, and the **source** of magenta-circle sync to other graphs (and SCW when wired).

### Markers & symbols

```cpp
void addBTWMarker(GraphType, const QDateTime &t, float range, float delta);
bool removeBTWMarker(GraphType, const QDateTime &t, float range,
                     float toleranceMs = 1000, float rangeTolerance = 0.1f);
void clearBTWMarkers(GraphType);

void addBTWSymbol(GraphType, const QString &symbolName, const QDateTime &t, float range);
void addBTWSymbol(GraphType, BTWSymbolDrawing::SymbolType, const QDateTime &t, float range);
bool removeBTWSymbol(/* name or SymbolType */, …);
void clearBTWSymbols(GraphType);
```

Usually pass `GraphType::BTW`. Placing a BTW marker fans magenta circles onto other layout graphs (and SCW if connected — see §16).

### Manual markers (interactive overlay)

```cpp
bool addBTWManualMarker(const QDateTime &t, float rangeValue, float bearingRate = 0.0f);
void clearBTWManualMarkers();
void setBTWManualMarkerSeries(const QString &seriesLabel);  // empty = use click X
void deleteInteractiveMarkers();  // all containers
```

### Rulers (indices 0..3)

```cpp
void setBtwRulerActive(int index, const QDateTime &t, qreal range);
void clearBtwRuler(int index);
void clearAllBtwRulers();
void setSelectedBtwRuler(int index);
int selectedBtwRuler() const;
```

### Shaded regions (BTW only)

```cpp
QUuid addShadedRegionToAllBTW(qreal startX, qreal endX);
bool removeShadedRegionFromAllBTW(const QUuid &syncId);
void clearAllShadedRegions();
std::vector<ShadedRegionSyncData> getAllShadedRegions() const;
```

### Magenta sync circles (placement policy)

| Graph family | Circle X |
|--------------|----------|
| BTW, RTW, LTW, FTW | Solution series (default `"ADOPTED"`), interpolated at marker time |
| BDW, BRW, FDW (+ SCW graphs) | Dashed **middle line** (zero axis) |

```cpp
void setMagentaCircleSolutionSeries(const QString &seriesLabel);  // default "ADOPTED"
QString magentaCircleSolutionSeries() const;
void resyncBTWSymbols();  // after solution series data changes
```

### BTW signals

```cpp
void BTWManualMarkerPlaced(const QDateTime &, const QPointF &);
void BTWManualMarkerClicked(const QDateTime &, const QPointF &);
void BTWSymbolAddedToAllGraphs(const QDateTime &timestamp);  // fan-out done — wire to SCW
void BtwRulerClicked(int index, const QDateTime &, qreal range);
void markerTimestampValueChanged(const QDateTime &, qreal value);
void markerClickedWithData(const QDateTime &, qreal rangeValue, qreal bearingRate);
```

---

## 10. RTW

Range-time graph: atlas symbols, yellow R markers, rulers, boxed-on-click selection.

### Symbols & R markers

```cpp
void addRTWSymbol(GraphType, const QString &symbolName, const QDateTime &t, float range);
bool removeRTWSymbol(GraphType, const QString &symbolName, const QDateTime &t, float range, …);
void clearRTWSymbols(GraphType);

void addRTWRMarker(GraphType, const QDateTime &t, float range);
bool removeRTWRMarker(GraphType, const QDateTime &t, float range, …);
void clearRTWRMarkers(GraphType);
```

Usually `GraphType::RTW`. Symbol names are defined in [`SYMBOL_API.md`](./SYMBOL_API.md).

### Rulers (indices 0..3)

```cpp
void setRtwRulerActive(int index, const QDateTime &t, qreal range);
void clearRtwRuler(int index);
void clearAllRtwRulers();
void setSelectedRtwRuler(int index);
int selectedRtwRuler() const;
```

### RTW signals

```cpp
void RTWSymbolTimestampCaptured(const QDateTime &, const QPointF &, const QString &symbolName);
void RTWRMarkerTimestampCaptured(const QDateTime &, const QPointF &);
void RtwRulerClicked(int index, const QDateTime &, qreal range);
```

### Example — line mode for a track series

```cpp
layout->setSeriesRenderMode(GraphType::RTW, QStringLiteral("RTW-1"), true);
layout->setDataToDataSource(GraphType::RTW, "RTW-1", y, times);
```

---

## 11. LTW / FTW

Same **data / series / clear / redraw / horizontal-line / magenta-circle** surface as other types. No dedicated marker/ruler/shaded APIs on `GraphLayout`.

```cpp
layout->setDataToDataSource(GraphType::LTW, "SERIES", y, times);
layout->setDataToDataSource(GraphType::FTW, "SERIES", y, times);
layout->setHardRangeLimits(GraphType::LTW, ymin, ymax);
layout->clearGraph(GraphType::FTW);
```

Magenta circles on LTW/FTW follow the **solution series** (see §9), not a middle line.

---

## 12. BDW / BRW / FDW

Deviation / rate graphs with a **dashed middle line**. Magenta BTW sync circles are drawn **on the middle line** (no host re-sync needed for X).

```cpp
layout->setDataToDataSource(GraphType::BDW, "ADOPTED", y, times);
layout->setDataToDataSource(GraphType::BRW, "ADOPTED", y, times);
layout->setDataToDataSource(GraphType::FDW, "ADOPTED", y, times);
layout->setSeriesRenderMode(GraphType::BDW, "ADOPTED", true);
```

Use the shared horizontal-line, manoeuvre, timeline, and history-selection APIs (§5–§8).

---

## 13. GraphLayout — chrome, capacity, redraw

### Chevron labels & dropdown indicator

```cpp
void setChevronLabel1/2/3(const QString &);           // all visible
QString getChevronLabel1/2/3() const;
void setChevronLabel1/2/3(const QString &containerLabel, const QString &);
QString getChevronLabel1/2/3(const QString &containerLabel) const;

void setDropdownArrowColor(const QColor &);
QColor dropdownArrowColor() const;
```

### Capacity (startup)

```cpp
void setDataSeriesCapacity(size_t);
void setSymbolsCapacity(size_t);
void setMarkersCapacity(size_t);
void setRenderingCachesCapacity(size_t scatter, size_t linePaths, size_t cachedData);
void setAllArraysCapacity(size_t data, size_t symbols, size_t markers,
                          size_t scatter, size_t linePaths, size_t cachedData);
```

---

## 14. GraphLayout — signals & public slots

### Signals (host-facing)

| Signal | Relates to |
|--------|------------|
| `VisibleGraphsChanged()` | Layout / container graph switch |
| `TimeIntervalChanged(TimeInterval)` | Timeline interval |
| `SliderTimeRangeChanged(QDateTime, QDateTime)` | Slider window |
| `TimeSelectionsChanged(vector, int)` | History selection |
| `RTWSymbolTimestampCaptured` / `RTWRMarkerTimestampCaptured` / `RtwRulerClicked` | RTW |
| `BTWManualMarkerPlaced` / `Clicked` / `BtwRulerClicked` / `marker*` / `BTWSymbolAddedToAllGraphs` | BTW |
| `HorizontalLineAdded` / `HorizontalLineRemoved` | Horizontal lines (user only) |

### Public slots (fan-out / sync; usually not called by hosts)

`onTimerTick`, `onTimeSelectionCreated/Modified/Cleared`, `onBTWManualMarkerPlaced`,
`onHorizontalLineSync*`, `onBTWMarkerSync*`, `onShadedRegionSync*`,
`onContainerIntervalChanged`.

---

## 15. SCWWindow

Standalone 8-pane view. **Not** a `GraphLayout` child.

### Construction

```cpp
SCWWindow(QWidget *parent = nullptr,
          QTimer *timer = nullptr,
          GraphContainerSyncState *syncState = nullptr);
```

Pass the layout’s timer and `layout->getSyncState()` so interval / scope stay shared.

### Window map (index → series)

| Index | Label | Series | Button |
|------:|-------|--------|--------|
| 0 | Window 1 | `ADOPTED` | Select |
| 1 | Window 2 | `RULER_1` | Select |
| 2 | Window 3 | `RULER_2` | Select |
| 3 | Window 4 | `RULER_3` | Select |
| 4 | Window 5 | `RULER_4` | Select |
| 5 | Window 6 | Cycles `BRAT`→`BOT`→`BFT`→`BOPT`→`BOTC` | Cycle |
| 6 | Window 7 | Cycles `ATMA`→`ATMAF` | Cycle |
| 7 | Window 8 | Cycles `EXTERNAL1`…`EXTERNAL5` | Cycle |

Hover selects a window (yellow frame). Graphs use a white border, yellow crosshair, Y range ±20, middle/zero axis, line drawing.

### Series enums & converters

```cpp
enum class SCW_SERIES_ADOPTED { ADOPTED };
enum class SCW_SERIES_R { RULER_1, RULER_2, RULER_3, RULER_4 };
enum class SCW_SERIES_B { BRAT, BOT, BFT, BOPT, BOTC };
enum class SCW_SERIES_A { ATMA, ATMAF };
enum class SCW_SERIES_E { EXTERNAL1, EXTERNAL2, EXTERNAL3, EXTERNAL4, EXTERNAL5 };

QString scwSeriesRToString(SCW_SERIES_R);
SCW_SERIES_R stringToScwSeriesR(const QString &);
// … same pattern for Adopted / B / A / E
```

### Data APIs

```cpp
void setDataPoints(SCW_SERIES_ADOPTED, const std::vector<float> &y, const std::vector<QDateTime> &t);
void addDataPoints(SCW_SERIES_ADOPTED, …);
void setDataPoints(SCW_SERIES_R, …);   void addDataPoints(SCW_SERIES_R, …);
void setDataPoints(SCW_SERIES_B, …);   void addDataPoints(SCW_SERIES_B, …);
void setDataPoints(SCW_SERIES_A, …);   void addDataPoints(SCW_SERIES_A, …);
void setDataPoints(SCW_SERIES_E, …);   void addDataPoints(SCW_SERIES_E, …);

void clearAllGraphs();  // all series + magenta symbols
```

For cycling windows (6–8), data for a non-current series is stored; the visible graph updates when that series is selected.

### Timeline / hover / magenta

```cpp
TimelineView *getTimelineView() const;

int getHoveredWindowIndex() const;      // 0..7 or -1
QString getHoveredSeriesName() const;

void addBTWSymbolToAllGraphs(const QDateTime &timestamp);  // hollow MagentaCircle on all 8
```

### Signals

```cpp
void seriesSelected(const QString &seriesName);
void windowHovered(int windowIndex, const QString &seriesName);  // -1 / "" on leave
```

### Example — feed SCW

```cpp
scw->setDataPoints(SCW_SERIES_ADOPTED::ADOPTED, yAdopted, times);
scw->addDataPoints(SCW_SERIES_R::RULER_1, yR1, times);
scw->setDataPoints(SCW_SERIES_B::BRAT, yBrat, times);
```

---

## 16. Wiring GraphLayout + SCW

Minimal host setup:

```cpp
auto *layout = new GraphLayout(parent, LayoutType::GPW4W, timer, seriesMap, missionStart);
auto *scw = new SCWWindow(parent, timer, layout->getSyncState());

// Shared timeline (interval + slider scope)
layout->syncExternalTimelineView(scw->getTimelineView());

// Magenta circles from BTW markers → SCW panes
connect(layout, &GraphLayout::BTWSymbolAddedToAllGraphs,
        scw, &SCWWindow::addBTWSymbolToAllGraphs, Qt::UniqueConnection);

// Optional: interval / slider observers on the layout
connect(layout, &GraphLayout::TimeIntervalChanged, …);
connect(layout, &GraphLayout::SliderTimeRangeChanged, …);
```

After updating the solution series used by magenta circles:

```cpp
layout->setDataToDataSource(GraphType::RTW, "ADOPTED", y, times);
layout->resyncBTWSymbols();
```

---

## 17. Magenta circles on SCW — required host wiring

> **Common integration miss:** circles appear on **RTW** (and other layout graphs) but
> **SCW shows 0 circles**. Layout fan-out is working; only the **SCW host link** is missing.

Circles on **RTW / BDW / LTW / …** inside `GraphLayout` do **not** automatically appear on
**SCW**. `SCWWindow` is a sibling widget; the library **never** connects it for you.

### Symptom

| Where | Result |
|-------|--------|
| RTW (and other layout graphs) | Magenta circles **do** draw |
| SCW (all 8 panes) | **0** circles |

### Required signal → API

| | |
|--|--|
| **Signal** | `GraphLayout::BTWSymbolAddedToAllGraphs(const QDateTime &timestamp)` |
| **API / slot** | `SCWWindow::addBTWSymbolToAllGraphs(const QDateTime &timestamp)` |

```cpp
SCWWindow *scw = new SCWWindow(parent, timer, layout->getSyncState());
layout->syncExternalTimelineView(scw->getTimelineView());

connect(layout, &GraphLayout::BTWSymbolAddedToAllGraphs,
        scw, &SCWWindow::addBTWSymbolToAllGraphs,
        Qt::UniqueConnection);
```

Because RTW already received a circle, `anySymbolAdded` is true and
`BTWSymbolAddedToAllGraphs` **is** emitted — if SCW still has 0 circles, this `connect`
is missing (or pointed at the wrong / null `SCWWindow` instance).

### Also required for placement

Use the fan-out path so the signal can fire:

```cpp
layout->addBTWMarker(GraphType::BTW, timestamp, range, delta);
// or interactive BTW manual-marker placement
```

Do **not** expect SCW updates from `addBTWSymbol(GraphType::BTW, …)` alone (no fan-out,
no signal).

### Fallback (interactive only)

If the layout signal is unreliable in your layout (e.g. BTW-only / empty peer graphs —
signal is gated on at least one non-BTW layout graph receiving a circle):

```cpp
connect(layout, &GraphLayout::BTWManualMarkerPlaced,
        this, [scw](const QDateTime &t, const QPointF &) {
            if (t.isValid())
                scw->addBTWSymbolToAllGraphs(t);
        });
```

For programmatic `addBTWMarker()`, call `scw->addBTWSymbolToAllGraphs(timestamp)` yourself
when the layout signal did not fire.

### Checklist

- [ ] `SCWWindow` constructed with `layout->getSyncState()`
- [ ] `layout->syncExternalTimelineView(scw->getTimelineView())`
- [ ] `BTWSymbolAddedToAllGraphs` → `SCWWindow::addBTWSymbolToAllGraphs` connected once (`Qt::UniqueConnection`)
- [ ] Markers placed via `addBTWMarker` / interactive placement

**See also:** [`SCW_WINDOW_API.md`](./SCW_WINDOW_API.md) §4–§5,
[`BTW_MAGENTA_CIRCLE_SYNC.md`](./BTW_MAGENTA_CIRCLE_SYNC.md).

---

## Quick “where do I look?” index

| I need to… | Use |
|------------|-----|
| Switch 1/2/4-window layout | `setLayoutType` |
| Show BTW in a pane | `setContainerGraphType` / `setCurrentDataOption` |
| Push track data | `setDataToDataSource` / `addDataPointsToDataSource` |
| Know 15 vs 30 min | `getTimeInterval` + `TimeIntervalChanged` |
| Know slider start/end | `getSliderTimeRange` + `SliderTimeRangeChanged` |
| Place BTW marker + sync circles | `addBTWMarker` (+ SCW connect in §16 / §17) |
| Magenta on RTW but **not** SCW | §17 — connect `BTWSymbolAddedToAllGraphs` |
| RTW symbol / R / ruler | §10 |
| Shaded band on BTW | `addShadedRegionToAllBTW` |
| Time line overlays | §8 |
| History highlight | §6 |
| SCW series data | §15 `setDataPoints` / `addDataPoints` |
| SCW hover | `getHoveredWindowIndex` / `windowHovered` |
