# Manual: BTW and RTW symbol APIs

This document describes how to **add BTW symbols** and **remove BTW / RTW symbols** using the public APIs on **`GraphLayout`**. Lower-level access via **`GraphEngine`**, **`WaterfallData`**, and graph widgets is noted where useful.

**See also:** [`SYSTEM_START_TIME_API.md`](./SYSTEM_START_TIME_API.md) for timeline / session time APIs.

---

## 1. Concepts

| Term | Meaning |
|------|---------|
| **Symbol** | A small pixmap glyph stored in the graph’s **`WaterfallData`** and drawn on the waterfall overlay. Positioned by **`timestamp`** (vertical / time axis) and **`range`** (horizontal axis value for that graph). |
| **BTW symbol** | Circle-style markers defined in **`BTWSymbolDrawing`** (magenta, yellow numbered, white numbered). Stored per graph engine; visible on BTW and on other graph types that have BTW symbol data. |
| **RTW symbol** | Tactical glyphs defined in **`RTWSymbolDrawing`** (TM, DP, triangles, rects, etc.). Stored and drawn on **RTW** graphs. |
| **Marker** | Separate from symbols — e.g. **`addBTWMarker`** (blue delta markers) or **`addRTWRMarker`** (R markers). Marker APIs are not covered here except where they auto-add BTW symbols. |

**Recommended entry point:** `GraphLayout` — it updates the engine, triggers **`redrawGraph()`**, and keeps UI in sync.

---

## 2. Adding BTW symbols

### 2.1 `GraphLayout` API (preferred)

Two overloads; both take the target graph type (usually **`GraphType::BTW`**), a timestamp, and a **range** (horizontal position / bearing on the BTW X axis).

```cpp
// By registered name (case-insensitive; underscores/spaces ignored)
void addBTWSymbol(const GraphType &graphType,
                  const QString &symbolName,
                  const QDateTime &timestamp,
                  float range);

// By enum (avoids string typos)
void addBTWSymbol(const GraphType &graphType,
                  BTWSymbolDrawing::SymbolType symbolType,
                  const QDateTime &timestamp,
                  float range);
```

**Example — from application code with a `GraphLayout *layout`:**

```cpp
const QDateTime ts = QDateTime::currentDateTime().addSecs(-60);

// Typed enum overload
layout->addBTWSymbol(GraphType::BTW,
                     BTWSymbolDrawing::SymbolType::YellowCircle1,
                     ts,
                     42.0f);

// String overload (same symbol)
layout->addBTWSymbol(GraphType::BTW,
                     QStringLiteral("WhiteCircle2"),
                     ts.addSecs(-30),
                     50.0f);
```

`GraphLayout::addBTWSymbol` stores the symbol in that graph type’s **`GraphEngine`** and calls **`redrawGraph(graphType)`** automatically. You do not need a separate redraw for layout-level calls.

### 2.2 Registered BTW symbol names

Canonical names (from **`BTWSymbolDrawing::registeredSymbolNames()`**):

| Name | Appearance |
|------|------------|
| `MagentaCircle` | Hollow magenta circle (default if name unknown) |
| `MagentaCircleSynced` | Filled magenta circle |
| `YellowCircle1` … `YellowCircle4` | Yellow filled circle, white digit 1–4 |
| `WhiteCircle1` … `WhiteCircle4` | White filled circle, black digit 1–4 |

**Aliases** (normalized to the same type): e.g. `YC1` → `YellowCircle1`, `WC2` → `WhiteCircle2`, `MAGENTA_CIRCLE` → `MagentaCircle`.

Use the enum overload or **`BTWSymbolDrawing::symbolTypeToName()`** / **`symbolNameToType()`** when converting between names and types.

### 2.3 Synced magenta circles

The BTW marker symbol (**`MagentaCircle`**) is always drawn as a **hollow** circle — identically on the BTW graph, other waterfall graph types, and SCW graphs. The internal **`isSynced`** flag (set e.g. via **`GraphLayout::addBTWSymbolToAllGraphs()`** after a BTW marker) **no longer changes its appearance**; **`BTWSymbolDrawing::resolveDisplayType()`** keeps it hollow.

The filled variant is available only by explicitly requesting the **`MagentaCircleSynced`** name/type.

Direct **`addBTWSymbol`** calls use **`isSynced = false`** unless you write to **`WaterfallData`** yourself.

### 2.4 Adding a magenta circle to all non-BTW graphs

When a **BTW marker** is placed, the layout propagates a synced magenta symbol to every other graph that has data at that time:

```cpp
layout->addBTWMarker(GraphType::BTW, timestamp, range, delta);
// → also calls addBTWSymbolToAllGraphs(timestamp, range) internally
```

To add synced symbols without a marker:

```cpp
layout->addBTWSymbolToAllGraphs(timestamp, range);
```

This looks up the Y/range value from each graph’s data at **`timestamp`** and adds **`MagentaCircle`** with **`isSynced = true`**. Skips **`GraphType::BTW`** and empty data sources.

### 2.5 Lower-level APIs (advanced)

| Layer | Add API |
|-------|---------|
| **`GraphEngine`** | `addBTWSymbol(name, ts, range)` / `addBTWSymbol(SymbolType, ts, range)` |
| **`WaterfallData`** | `addBTWSymbol(name, ts, range, isSynced = false)` |
| **`BTWGraph`** (widget) | Same as engine via `dataSource`; does **not** auto-redraw all graphs |

**Example — direct on the BTW widget** (caller must redraw):

```cpp
BTWGraph *btw = qobject_cast<BTWGraph *>(container->getWaterfallGraph(GraphType::BTW));
btw->addBTWSymbol(BTWSymbolDrawing::SymbolType::YellowCircle3, ts, 32.0);
layout->redrawGraph(GraphType::BTW);
```

### 2.6 Parameters

| Parameter | Description |
|-----------|-------------|
| **`graphType`** | Usually **`GraphType::BTW`**. Symbols are stored on that engine; rendering also occurs on graphs that read BTW symbol data from their datasource. |
| **`timestamp`** | **`QDateTime`** of the event. Must fall within (or near) the visible time window to be seen. |
| **`range`** | Horizontal axis value where the symbol is drawn (bearing / range on BTW). Unknown symbol names still accept any float. |

---

## 3. Removing BTW and RTW symbols

### 3.1 RTW — remove one symbol

```cpp
bool removeRTWSymbol(const GraphType &graphType,
                     const QString &symbolName,
                     const QDateTime &timestamp,
                     float range,
                     float toleranceMs = 1000,
                     float rangeTolerance = 0.1f);
```

**Returns:** `true` if a matching symbol was found and removed; `false` otherwise.

Matching rules (see **`WaterfallData::removeRTWSymbol`**):

- **`symbolName`** must match **exactly** (case-sensitive).
- **`timestamp`** must be within **`toleranceMs`** milliseconds (default **1000 ms**).
- **`range`** must be within **`rangeTolerance`** (default **0.1**).

**Example:**

```cpp
const QDateTime ts = QDateTime::currentDateTime().addSecs(-120);
layout->addRTWSymbol(GraphType::RTW, QStringLiteral("TM"), ts, 15.0f);

// Remove the same logical symbol (tolerances allow small float / clock drift)
bool ok = layout->removeRTWSymbol(GraphType::RTW,
                                  QStringLiteral("TM"),
                                  ts,
                                  15.0f);
// ok == true → graph redrawn automatically
```

Use the **same `symbolName`** string that was passed to **`addRTWSymbol`**. RTW names are mapped with **`RTWGraph::symbolNameToType()`** (e.g. `"TM"`, `"YELLOW_CIRCLE_1"`, `"MAX"`); removal compares the stored name, not aliases.

### 3.2 RTW — clear all symbols

```cpp
void clearRTWSymbols(const GraphType &graphType);
```

Removes every RTW symbol for that graph type and redraws.

### 3.3 BTW — remove one symbol

```cpp
bool removeBTWSymbol(const GraphType &graphType,
                     const QString &symbolName,
                     const QDateTime &timestamp,
                     float range,
                     float toleranceMs = 1000,
                     float rangeTolerance = 0.1f);

bool removeBTWSymbol(const GraphType &graphType,
                     BTWSymbolDrawing::SymbolType symbolType,
                     const QDateTime &timestamp,
                     float range,
                     float toleranceMs = 1000,
                     float rangeTolerance = 0.1f);
```

**Returns:** `true` if a matching symbol was found and removed; `false` otherwise.

Matching rules (see **`WaterfallData::removeBTWSymbol`**):

- **`symbolName`** matches the stored name **exactly**, or resolves to the same **`BTWSymbolDrawing::SymbolType`** (aliases such as `YC1` / `YellowCircle1` are equivalent).
- **`timestamp`** must be within **`toleranceMs`** milliseconds (default **1000 ms**).
- **`range`** must be within **`rangeTolerance`** (default **0.1**).

**Example:**

```cpp
const QDateTime ts = QDateTime::currentDateTime().addSecs(-90);
layout->addBTWSymbol(GraphType::BTW,
                     BTWSymbolDrawing::SymbolType::YellowCircle2,
                     ts,
                     44.0f);

// Enum overload
bool ok = layout->removeBTWSymbol(GraphType::BTW,
                                  BTWSymbolDrawing::SymbolType::YellowCircle2,
                                  ts,
                                  44.0f);

// String overload with alias (same symbol)
ok = layout->removeBTWSymbol(GraphType::BTW,
                             QStringLiteral("YC2"),
                             ts,
                             44.0f);
// ok == true → graph redrawn automatically
```

**Note:** `MagentaCircle` and `MagentaCircleSynced` are distinct types — use the name/type that was used when adding.

### 3.4 BTW — clear all symbols

```cpp
void clearBTWSymbols(const GraphType &graphType);
```

**Example:**

```cpp
layout->clearBTWSymbols(GraphType::BTW);
```

This clears symbols on that engine only. Other graph types that received synced magenta circles via **`addBTWSymbolToAllGraphs`** keep their own copies in their engines — clear those graph types separately if needed:

```cpp
layout->clearBTWSymbols(GraphType::BDW);
layout->clearBTWSymbols(GraphType::RTW);
// … per graph type that received synced symbols
```

Or use **`clearAllGraphs()`** to reset all data, symbols, and markers on every graph (see §3.6).

### 3.5 BTW vs marker removal

Do not confuse **symbols** with **BTW markers** (blue manual markers):

| API | Removes |
|-----|---------|
| **`clearBTWSymbols` / `clearRTWSymbols`** | Stored symbol glyphs |
| **`removeBTWMarker` / `clearBTWMarkers`** | BTW blue markers (not numbered circle symbols) |
| **`removeRTWRMarker` / `clearRTWRMarkers`** | RTW “R” markers (not RTW tactical symbols) |

### 3.6 Clear everything (all graph types)

```cpp
void clearAllGraphs();   // all engines: data + RTW/BTW symbols + all markers
void clearGraph(const GraphType &graphType);  // one graph type’s data (see implementation for markers/symbols)
```

**`clearAllGraphs()`** explicitly calls **`clearRTWSymbols()`**, **`clearBTWSymbols()`**, **`clearBTWMarkers()`**, and **`clearRTWRMarkers()`** on every engine.

---

## 4. Adding RTW symbols (reference)

Removal docs above assume symbols were added first. RTW add API for completeness:

```cpp
void addRTWSymbol(const GraphType &graphType,
                  const QString &symbolName,
                  const QDateTime &timestamp,
                  float range);
```

**Example:**

```cpp
layout->addRTWSymbol(GraphType::RTW,
                     QStringLiteral("Triangle"),
                     QDateTime::currentDateTime(),
                     20.0f);
```

Common RTW **`symbolName`** values: `TM`, `DP`, `LY`, `Triangle`, `RectR`, `R`, `L`, `BOT`, `YellowCircle1`–`4`, `Max`, `Min`, and variants listed in **`RTWGraph::symbolNameToType()`** (`rtwgraph.cpp`). Unrecognized names default to drawing **`R`**.

---

## 5. Query / inspect

Via **`GraphEngine`** (from layout’s internal engines) or **`WaterfallData`**:

```cpp
size_t n = engine->getBTWSymbolsCount();
size_t m = engine->getRTWSymbolsCount();
std::vector<BTWSymbolData> btw = engine->getBTWSymbols();
std::vector<RTWSymbolData> rtw = engine->getRTWSymbols();
```

Symbol records contain **`symbolName`**, **`timestamp`**, **`range`**, and for BTW **`isSynced`**.

---

## 6. Sandbox examples

**`MainWindow::testBTWSymbolsAPI()`** (`mainwindow.cpp`) demonstrates:

- **`GraphLayout::addBTWSymbol`** with every **`BTWSymbolDrawing::SymbolType`**
- String name **`MagentaCircleSynced`**
- Direct **`BTWGraph::addBTWSymbol`** on the live widget

Run the ui-sandbox and check debug output for `"MainWindow::testBTWSymbolsAPI"` after startup (~5 s delay).

---

## 7. Quick reference

| Action | API | Graph type |
|--------|-----|------------|
| Add BTW symbol | `addBTWSymbol(...)` | Usually `GraphType::BTW` |
| Add synced magenta to all graphs | `addBTWSymbolToAllGraphs(ts, range)` | All except BTW |
| Remove one BTW symbol | `removeBTWSymbol(...)` | Per graph type (usually `GraphType::BTW`) |
| Remove one RTW symbol | `removeRTWSymbol(...)` | `GraphType::RTW` |
| Clear all BTW symbols | `clearBTWSymbols(...)` | Per graph type |
| Clear all RTW symbols | `clearRTWSymbols(...)` | Per graph type |

---

## 8. Document history

| Date | Change |
|------|--------|
| 2026-05-22 | Added `removeBTWSymbol` per-symbol removal API and documentation. |
| 2026-05-22 | Initial manual for BTW add and BTW/RTW symbol removal APIs. |
