# Manual: BTW magenta circle — hollow rendering & sync

This document describes the **BTW marker symbol** (the magenta circle), guarantees it
is drawn as a **hollow** circle **everywhere** (BTW graph, other waterfall graph types,
and SCW graphs), and explains the synchronization logic so you can verify it keeps
working.

**See also:** [`SYMBOL_API.md`](./SYMBOL_API.md) (symbol names/types),
[`SCW_WINDOW_API.md`](./SCW_WINDOW_API.md) (SCW sync path),
[`BTW_MARKER_SIZES.md`](./BTW_MARKER_SIZES.md) (marker sizing).

---

## 1. The rule

> The `MagentaCircle` symbol is **always hollow** — same appearance on the BTW graph,
> other waterfall graphs, and SCW graphs. Its synced state does **not** change the fill.

A separate **filled** variant (`MagentaCircleSynced`) still exists, but it is used
**only** when explicitly requested by name/type. The automatic BTW-marker sync never
uses it.

---

## 1a. Where the circle is placed (X / range)

The circle's **time (Y)** always comes from the BTW marker timestamp. Its **range (X)**
now follows a per-graph rule:

| Graph family | Has middle line? | Circle X is drawn on… |
|--------------|------------------|-----------------------|
| SCW (8 graphs), BDW, BRW, FDW | **Yes** (dashed middle line) | the **middle line** (zero-axis X), tracked live through zoom/pan |
| BTW *(source)*, RTW, LTW, FTW | No | the **solution series** trace (default `"ADOPTED"`), interpolated at the marker time |

**How it is implemented**

- **Middle-line graphs** snap the circle to the middle line at *draw* time, so no data
  lookup or re-sync is needed — it always sits on the line even while zooming/panning.
  `WaterfallGraph::drawBTWSymbols()` overrides the symbol's X with the zero-axis X when
  `magentaCircleOnMiddleLine()` returns `true`. That hook returns `m_zeroAxisEnabled`
  (true for SCW graphs) and is overridden to `true` in `BDWGraph` / `BRWGraph` /
  `FDWGraph`.
- **Graphs without a middle line** store the range resolved from the **solution series**
  at add time (`GraphLayout::resolveMagentaCircleRange()` →
  `WaterfallData::interpolateSeriesRangeAtTime("ADOPTED", …)`), falling back to the
  nearest data point across series if the solution series has no data.

### Re-syncing when the solution changes

Solution-series values change whenever the computed solution is updated. Because the
circle stores a resolved range (rather than recomputing every frame — that would
"overload the system"), the host must ask for a refresh:

```cpp
// Optional: change which series is the "solution" (default "ADOPTED").
layout->setMagentaCircleSolutionSeries(QStringLiteral("ADOPTED"));

// After feeding a new solution into the ADOPTED series, move the circles onto it:
layout->resyncBTWSymbols();
```

`resyncBTWSymbols()` only recomputes stored ranges for existing `MagentaCircle` symbols
(O(number of circles)) and redraws once — it does **not** re-run the full fan-out.
Middle-line graphs are unaffected by solution changes (they stay on the line), so calling
it is cheap and safe to invoke each time a new solution arrives.

---

## 2. Where hollow vs filled is decided

All waterfall-family graphs (BTW, RTW, and the 8 SCW graphs are `WaterfallGraph`
instances) render stored BTW symbols through **one** shared point:

**`waterfallgraph.cpp`**

```cpp
const BTWSymbolDrawing::SymbolType symbolType =
    BTWSymbolDrawing::resolveDisplayType(symbolData.symbolName, symbolData.isSynced);
const QPixmap &symbolPixmap = m_btwSymbols.get(symbolType);
```

Because every view funnels through `resolveDisplayType()`, that function is the single
source of truth for appearance.

**`btwsymboldrawing.cpp`**

```cpp
BTWSymbolDrawing::SymbolType BTWSymbolDrawing::resolveDisplayType(const QString &symbolName, bool isSynced)
{
    // MagentaCircle always draws hollow, regardless of synced state.
    Q_UNUSED(isSynced);
    return symbolNameToType(symbolName);
}
```

The two pixmaps:

| Type | Brush | Appearance |
|------|-------|------------|
| `MagentaCircle` | `Qt::NoBrush` | **Hollow** magenta ring |
| `MagentaCircleSynced` | filled magenta | Solid magenta disc |

> **Why this matters:** previously `resolveDisplayType()` upgraded a synced
> `MagentaCircle` to `MagentaCircleSynced` (filled). That made cross-graph synced
> markers appear filled while SCW/BTW stayed hollow — an inconsistency. The upgrade
> has been removed, so all three views match.

---

## 3. Synchronization chain

The stored symbol name is **always** `"MagentaCircle"`. Only the `isSynced` flag
differs by source, and (as of this change) that flag no longer affects appearance.

```
User places a BTW manual marker        (or host calls GraphLayout::addBTWMarker)
  │
  ▼
BTWGraph::onMarkerAdded / onMarkerMoved ── emit manualMarkerPlaced
  │
  ▼
GraphContainer::onBTWManualMarkerPlaced ── emit BTWManualMarkerPlaced
  │
  ▼
GraphLayout::onBTWManualMarkerPlaced  /  GraphLayout::addBTWMarker()
  │
  ▼
GraphLayout::addBTWSymbolToAllGraphs(timestamp, range)
  │   • for every NON-BTW graph type with a data point at `timestamp`:
  │       dataSource->addBTWSymbol("MagentaCircle", timestamp, range, /*isSynced=*/true)
  │   • redraws; if any added, emit BTWSymbolAddedToAllGraphs(timestamp)
  │
  ├─────────────────────────────► (waterfall graphs render hollow via resolveDisplayType)
  │
  ▼
[host connection] scwWindow->addBTWSymbolToAllGraphs(timestamp)
      • for each of the 8 SCW graphs: find nearest data point (±1s),
        dataSource->addBTWSymbol("MagentaCircle", timestamp, range)  // isSynced=false
      • forceFullRedraw()  (SCW graphs render hollow via resolveDisplayType)
```

### Source of the `isSynced` flag

| Producer | File / function | `isSynced` | Appearance |
|----------|-----------------|-----------|------------|
| BTW graph self-add | `btwgraph.cpp` — `addBTWSymbolToOtherGraphs` | `false` | hollow |
| Cross-graph layout sync | `graphlayout.cpp` — `addBTWSymbolToAllGraphs` | `true` | hollow |
| SCW graphs | `scwwindow.cpp` — `addBTWSymbolToAllGraphs` | `false` | hollow |

All rows now render identically (hollow).

### Deduplication

Each producer skips adding if a `MagentaCircle` already exists within **±100 ms** at
that timestamp (`getBTWSymbolsWithinTimeRange`). This prevents duplicate rings when
markers are moved or the sync fires more than once.

---

## 4. Key file references

| Concern | File / symbol |
|---------|---------------|
| Shared render / hollow-vs-filled decision | `waterfallgraph.cpp` (BTW symbol draw loop) → `BTWSymbolDrawing::resolveDisplayType()` |
| Middle-line snap (X override) | `waterfallgraph.cpp` — `drawBTWSymbols()` + `magentaCircleOnMiddleLine()` (overridden in `bdwgraph.h`/`brwgraph.h`/`fdwgraph.h`) |
| Solution-series range resolution | `graphlayout.cpp` — `resolveMagentaCircleRange()`; `setMagentaCircleSolutionSeries()` |
| Re-sync circles to updated solution | `graphlayout.cpp` — `resyncBTWSymbols()` |
| Hollow pixmap | `btwsymboldrawing.cpp` — `makeMagentaCircle()` (`Qt::NoBrush`) |
| Filled pixmap (explicit only) | `btwsymboldrawing.cpp` — `makeMagentaCircleSynced()` |
| Symbol types | `btwsymboldrawing.h` — `SymbolType::MagentaCircle`, `MagentaCircleSynced` |
| Cross-graph sync + signal | `graphlayout.cpp` — `addBTWSymbolToAllGraphs()`, signal `BTWSymbolAddedToAllGraphs` |
| SCW sync consumer | `scwwindow.cpp` — `addBTWSymbolToAllGraphs()` |
| Storage flag | `waterfalldata.h` — `BTWSymbolData::isSynced`; `addBTWSymbol(..., bool isSynced = false)` |

---

## 5. Verifying it works

### Visual check

1. Run the sandbox with both the main graph layout and the SCW window open.
2. Place a BTW manual marker on the BTW graph (click, or the BTW marker API).
3. Confirm the magenta circle appears as a **hollow ring** in **all** of:
   - the BTW graph itself,
   - the other waterfall graph types in the layout (that have data at that time),
   - the SCW graphs.
4. Move the marker; the synced rings should track and stay hollow (no duplicates).

### Guard against regressions

- Do **not** reintroduce a synced→filled upgrade inside `resolveDisplayType()`. If a
  filled circle is ever needed, request `MagentaCircleSynced` explicitly by name — do
  not overload `MagentaCircle`.
- Keep `makeMagentaCircle()` using `Qt::NoBrush`.
- If you add a new view that renders BTW symbols, route it through
  `BTWSymbolDrawing::resolveDisplayType()` / `get()` so it inherits the hollow rule
  automatically.

### Quick code assertions (optional)

```cpp
// Hollow regardless of synced flag:
Q_ASSERT(BTWSymbolDrawing::resolveDisplayType("MagentaCircle", true)
         == BTWSymbolDrawing::SymbolType::MagentaCircle);
Q_ASSERT(BTWSymbolDrawing::resolveDisplayType("MagentaCircle", false)
         == BTWSymbolDrawing::SymbolType::MagentaCircle);
```

---

## 5a. Removing a circle by timestamp

To delete **one** manual marker **and** its fanned-out magenta circles:

```cpp
layout->removeBTWManualMarker(timestamp);           // default ±1000 ms
layout->removeBTWManualMarker(timestamp, 250);      // tighter window
```

This uses `GraphLayout::removeBTWSymbolFromAllGraphs()` internally to strip
`MagentaCircle` symbols from every non-BTW data source in the time window, then
redraws. Full details: [`BTW_MANUAL_MARKER_SERIES_API.md`](./BTW_MANUAL_MARKER_SERIES_API.md) §3a.

**Note:** `clearBTWManualMarkers()` only clears the pink overlay markers — it does
**not** remove the magenta circles. Prefer `removeBTWManualMarker` when you need
both cleaned up for a specific time.

---

## 6. Summary

- One render path (`resolveDisplayType`) → consistent look across BTW, waterfall, SCW.
- BTW marker always stored as `"MagentaCircle"` → always hollow now.
- `isSynced` retained for API/dedup compatibility but no longer changes appearance.
- Filled `MagentaCircleSynced` remains available only on explicit request.
