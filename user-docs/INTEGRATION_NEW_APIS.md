# Manual: New integration APIs (magenta-circle placement, line/scatter per series, RTW boxed symbols)

This document covers the APIs added most recently and how a host ("integrated")
system wires them up. Three independent features:

1. **Magenta BTW sync circle placement + re-sync** — where the magenta circle lands on
   each graph, and how to refresh it when the solution changes.
2. **Per-series line vs scatterplot** — force any single series to render as a
   continuous line or a scatterplot.
3. **RTW boxed symbols on click** — every RTW symbol (and the yellow R marker) has a
   plain and a "boxed" (rectangle) variant; clicking selects/boxes it.

**See also:** [`BTW_MAGENTA_CIRCLE_SYNC.md`](./BTW_MAGENTA_CIRCLE_SYNC.md),
[`BTW_MANUAL_MARKER_SERIES_API.md`](./BTW_MANUAL_MARKER_SERIES_API.md).

All entry points marked **`GraphLayout`** are the main host-facing surface; the
`WaterfallGraph` / `RTWSymbolDrawing` methods are for code that manages a single graph
directly.

---

## 1. Magenta BTW sync circle: placement + re-sync

### 1.1 What changed

When a BTW marker (automatic or manual) is placed, magenta circles are synced onto the
other graphs. Placement is now rule-based:

| Graph family | Middle line? | Circle is drawn on… |
|--------------|--------------|---------------------|
| SCW (8 graphs), BDW, BRW, FDW | Yes | the **middle line** (zero-axis X), tracked live through zoom/pan |
| RTW, LTW, FTW | No | the **solution series** trace (default `"ADOPTED"`), interpolated at the marker time |

Middle-line graphs snap to the line automatically at draw time (no host action needed).
For the others, the circle is placed on the solution series at add time, and you refresh
it when the solution changes.

### 1.2 API (`GraphLayout`)

```cpp
// Choose which series the circle follows on graphs without a middle line.
// Default is "ADOPTED". Empty string -> fall back to nearest data point.
void setMagentaCircleSolutionSeries(const QString &seriesLabel);
QString magentaCircleSolutionSeries() const;

// Recompute stored circle ranges from the CURRENT solution series and redraw.
// O(number of circles); does NOT re-run the full fan-out — safe to call often.
void resyncBTWSymbols();
```

### 1.3 Example

```cpp
// Once, after data sources exist (optional — "ADOPTED" is the default):
layout->setMagentaCircleSolutionSeries(QStringLiteral("ADOPTED"));

// Place a BTW marker (fans magenta circles onto the other graphs):
layout->addBTWMarker(GraphType::BTW, timestamp, rangeValue);

// Later, when a new solution is computed and pushed into the ADOPTED series:
layout->setDataToDataSource(GraphType::RTW, "ADOPTED", newYData, newTimestamps);
layout->resyncBTWSymbols();   // circles on RTW/LTW/FTW move onto the updated trace
```

> **When to call `resyncBTWSymbols()`:** after you update the solution-series data.
> Do **not** call it every frame — the circle stores its range and only recomputes on
> demand (that is the whole point of the API). Middle-line graphs are unaffected by
> solution changes and need no re-sync.

---

## 2. Per-series line vs scatterplot

### 2.1 What changed

Any single series can be forced to render as a continuous line or a scatterplot,
independent of the whole-graph mode. `"ADOPTED"` still defaults to a line. This replaces
the previous hardcoded, graph-by-graph `== "ADOPTED"` checks with one shared rule.

Precedence (highest first): whole-graph line mode → per-series override → `"ADOPTED"`
default → scatterplot.

### 2.2 API (`GraphLayout`) — main entry point

```cpp
// Render one series of a graph type as a line (asLine=true) or scatterplot (false).
// Applies to that graph type in every container (visible or hidden).
void setSeriesRenderMode(const GraphType &graphType, const QString &seriesLabel, bool asLine);

// Remove all per-series overrides for a graph type (reverts to defaults).
void clearSeriesRenderModes(const GraphType &graphType);
```

### 2.3 API (`WaterfallGraph`) — single graph

```cpp
void setSeriesRenderMode(const QString &seriesLabel, bool asLine);
bool isSeriesRenderedAsLine(const QString &seriesLabel) const;  // effective mode
void clearSeriesRenderModes();

// Whole graph (unchanged): render EVERY series as a line.
void setUseLineDrawing(bool useLines);
```

### 2.4 Examples

```cpp
// Make RTW-1 a continuous line:
layout->setSeriesRenderMode(GraphType::RTW, QStringLiteral("RTW-1"), true);

// Revert RTW-1 to a scatterplot:
layout->setSeriesRenderMode(GraphType::RTW, QStringLiteral("RTW-1"), false);

// Force ADOPTED (normally a line) to draw as points on BDW:
layout->setSeriesRenderMode(GraphType::BDW, QStringLiteral("ADOPTED"), false);

// Drop all RTW overrides:
layout->clearSeriesRenderModes(GraphType::RTW);
```

Notes:
- Series labels match exactly (as keyed in the data source), e.g. `"RTW-1"`, `"ADOPTED"`.
- Takes effect immediately; cached geometry is rebuilt on the change.
- The setting persists on the (pre-created) per-type graph instances, so it still
  applies when a graph is later shown in a container.

---

## 3. RTW boxed symbols on click

### 3.1 What changed

Every RTW symbol has two cached variants:

- **plain** — drawn normally,
- **boxed** — the same glyph enclosed in a selection rectangle.

Symbols draw **plain** by default. **Clicking** a symbol selects it and swaps it to its
**boxed** variant (click again to clear). This applies to all atlas symbols *including
the `R` symbol*, and to the separate yellow **R marker**. At most one symbol and one R
marker are boxed at a time.

Also fixed/tuned:
- **Reliable symbol clicks** — symbols now hit-test against their bounding rect (tightened
  by a small inset) instead of opaque-pixels-only, so clicks on the hollow centre of
  outline/letter symbols register.
- **Smaller R-marker click zone** — R markers are picked within a small radius of the
  marker centre instead of the oversized text bounding box.

### 3.2 Host-facing API (`GraphLayout` / `RTWGraph`)

The boxing is automatic on click — no new setter is required. The relevant host surface:

```cpp
// Add an RTW symbol (drawn plain; boxes on click). GraphLayout wrapper:
void addRTWSymbol(const GraphType &graphType, const QString &symbolName,
                  const QDateTime &timestamp, float range);

// RTWGraph signals emitted on click (unchanged names):
void rtwSymbolTimestampCaptured(const QDateTime &timestamp,
                                const QPointF &position,
                                const QString &symbolName);   // symbol clicked
void rMarkerTimestampCaptured(const QDateTime &timestamp,
                              const QPointF &position);       // yellow R marker clicked
```

Supported symbol names (case-insensitive) include: `TM, DP, LY, CircleI, Triangle,
RectR, EllipsePP, RectX, RectA, RectAPurple, RectK, CircleRYellow, DoubleBarYellow, R,
L, BOT, BOTC, BOTF, BOTD, …` (see `RTWGraph::symbolNameToType`).

### 3.3 Low-level atlas API (`RTWSymbolDrawing`)

If you render RTW symbols yourself:

```cpp
// boxed = false -> plain glyph; boxed = true -> glyph enclosed in a rectangle.
const QPixmap& get(SymbolType type, bool boxed = false) const;
void draw(QPainter* p, QPointF pos, SymbolType type, bool boxed = false);
```

### 3.4 Example

```cpp
// Add symbols; they render plain until the operator clicks one.
layout->addRTWSymbol(GraphType::RTW, "TM", t1, 12.0f);
layout->addRTWSymbol(GraphType::RTW, "R",  t2, 8.0f);

// React to a symbol / R-marker selection in the host:
connect(rtwGraph, &RTWGraph::rtwSymbolTimestampCaptured,
        backend, [](const QDateTime &ts, const QPointF &, const QString &name){
            // name = clicked symbol; it is now shown boxed until clicked again.
        });
connect(rtwGraph, &RTWGraph::rMarkerTimestampCaptured,
        backend, [](const QDateTime &ts, const QPointF &){ /* R marker selected */ });
```

### 3.5 Tuning (in `rtwgraph.cpp`)

These are internal constants, adjust if the defaults do not suit the deployment:

| Constant | Default | Meaning |
|----------|---------|---------|
| `kRtwSymbolClickInsetPx` | `3.0` | Inset shrinking the symbol clickable area inside the pixmap. |
| `kRMarkerClickRadiusPx` | `8.0` | Click radius (px) around an R-marker centre. |

The selection rectangle style (white, 1px) is set in
`RTWSymbolDrawing::addSelectionRectangle()` (symbols) and in `RTWGraph::drawCustomRMarkers()`
(R marker).

---

## 4. Quick reference

| Goal | Call |
|------|------|
| Set solution series for magenta circles | `layout->setMagentaCircleSolutionSeries("ADOPTED")` |
| Refresh circles after solution changes | `layout->resyncBTWSymbols()` |
| Make a series a line | `layout->setSeriesRenderMode(type, label, true)` |
| Make a series a scatterplot | `layout->setSeriesRenderMode(type, label, false)` |
| Whole graph as lines | `waterfallGraph->setUseLineDrawing(true)` |
| Clear per-series overrides | `layout->clearSeriesRenderModes(type)` |
| Add an RTW symbol (boxes on click) | `layout->addRTWSymbol(type, name, ts, range)` |
| React to RTW symbol click | connect `RTWGraph::rtwSymbolTimestampCaptured` |
| React to R-marker click | connect `RTWGraph::rMarkerTimestampCaptured` |
| Draw a boxed RTW glyph manually | `rtwSymbols.get(type, /*boxed=*/true)` |
