# Manual: Synchronized horizontal time lines

This document describes **horizontal time lines** — full-width constant-time overlays on
waterfall graphs — the public API, the internal drawing pipeline, and **where to emit
signals** for main-system integration when a line is drawn or removed.

A horizontal line is positioned by a single **`timestamp`** (Y axis = time). The same
line appears on **every** waterfall graph (BTW, BDW, RTW, FTW, FDW, BRW, LTW) in **every**
container panel.

**See also:** [`GRAPH_LAYOUT_INTEGRATION_API.md`](./GRAPH_LAYOUT_INTEGRATION_API.md),
[`NEW_MISSION_SESSION.md`](./NEW_MISSION_SESSION.md).

---

## 1. Concepts

| Term | Meaning |
|------|---------|
| **Horizontal line** | A constant-time overlay spanning the plot width. White by default; **yellow** while being dragged. |
| **syncId** | Global `QUuid` identifying one logical line across all graphs and containers. |
| **Mode** | Interaction state: **Normal**, **DrawLine**, or **DeleteLine**. |
| **Source of truth** | `GraphContainerSyncState::horizontalLines` in `GraphLayout`. Each graph holds a lightweight mirror. |

### Interaction modes

| Mode | Click empty area | Click existing line |
|------|------------------|---------------------|
| **Normal** | Default graph behaviour | Drag to move |
| **DrawLine** | Add a new line at click time | Drag to move; click without drag deletes |
| **DeleteLine** | Ignored | Click deletes |

Lines can be drawn or moved from **any** visible waterfall graph when the layout is in
the appropriate mode.

---

## 2. Architecture

```
User gesture on any WaterfallGraph
  → WaterfallGraph::emitHorizontalLineSync*()
  → GraphContainer relay (HorizontalLineSync* signals)
  → GraphLayout hub slots (onHorizontalLineSync*)
  → updates m_syncState + fans out to all containers
  → every WaterfallGraph mirror: update cached QGraphicsLineItem only
```

**Performance rules:**

- **No `redrawAllGraphs()` during drag** — only `lineItem->setLine()` position updates.
- **Drag sync throttled** to ~16 ms (`INTERACTIVE_UPDATE_THROTTLE_MS`).
- **Yellow highlight** via `draggingLineSyncId` in sync state (set once at drag start, cleared at release).
- **Stable identity** via `syncId`, not timestamp matching.

---

## 3. `GraphLayout` public API

**Header:** `graphlayout.h`, `sharedsyncstate.h`

```cpp
#include "sharedsyncstate.h"  // HorizontalLineMode, HorizontalLineSyncData

// Interaction mode (all graphs, all containers)
void setHorizontalLineMode(HorizontalLineMode mode);
HorizontalLineMode horizontalLineMode() const;

// Programmatic line management (returns global syncId)
QUuid addHorizontalLine(const QDateTime &timestamp,
                        const QColor &color = Qt::white, qreal width = 2.0);
bool removeHorizontalLine(const QUuid &syncId);
void clearHorizontalLines();
QDateTime getHorizontalLineTimestamp(const QUuid &syncId) const;
std::vector<HorizontalLineSyncData> getActiveHorizontalLines() const;

// Legacy BTW-named wrappers (forward to the API above; graphType is ignored)
void setBTWHorizontalLineMode(const GraphType &graphType, HorizontalLineMode mode);
QUuid addBTWHorizontalLine(const GraphType &graphType, const QDateTime &timestamp, ...);
```

### Example: draw-line mode

```cpp
layout->setHorizontalLineMode(HorizontalLineMode::DrawLine);
// User clicks any visible graph → line appears on all graphs
```

### Example: programmatic line

```cpp
const QUuid id = layout->addHorizontalLine(QDateTime::currentDateTime().addSecs(-120));
layout->removeHorizontalLine(id);
layout->clearHorizontalLines();
```

---

## 4. Main-system signals (user interaction only)

Public signals on **`GraphLayout`** — emitted **only when the operator clicks** to draw or
delete a line. **Not** emitted by `addHorizontalLine()`, `removeHorizontalLine()`, or
`clearHorizontalLines()`.

```cpp
void HorizontalLineAdded(const QUuid &syncId, const QDateTime &timestamp);
void HorizontalLineRemoved(const QUuid &syncId, const QDateTime &timestamp);
```

| User action | Signal | Payload |
|-------------|--------|---------|
| Click empty area in **DrawLine** mode | `HorizontalLineAdded` | `syncId`, `timestamp` |
| Click existing line in **DrawLine** / **DeleteLine** mode (no drag) | `HorizontalLineRemoved` | `syncId`, `timestamp` |

### How it works

User gestures flow:

```
WaterfallGraph (click)
  → horizontalLineSyncAdded / Removed
  → GraphContainer relay
  → GraphLayout::onHorizontalLineSync*
  → emit HorizontalLine*   (only when sender() is GraphContainer)
```

Programmatic API calls `onHorizontalLineSync*` directly (no Qt sender), so **no signal**.

| API | Updates UI | Emits signal |
|-----|------------|--------------|
| User click draw | Yes | `HorizontalLineAdded` |
| User click delete | Yes | `HorizontalLineRemoved` |
| `addHorizontalLine()` | Yes | No |
| `removeHorizontalLine()` | Yes | No |
| `clearHorizontalLines()` | Yes | No |

### Example: main-system connection

```cpp
connect(layout, &GraphLayout::HorizontalLineAdded,
        backend, [](const QUuid &syncId, const QDateTime &timestamp) {
    backend->onUserDrewHorizontalLine(syncId, timestamp);
});

connect(layout, &GraphLayout::HorizontalLineRemoved,
        backend, [](const QUuid &syncId, const QDateTime &timestamp) {
    backend->onUserRemovedHorizontalLine(syncId, timestamp);
});
```

**Legacy (unused):** `GraphContainer::BTWHorizontalLinePlaced` — do not connect.

---

## 5. Function reference — drawing pipeline

### 5.1 User interaction flow (`waterfallgraph.cpp`)

Entry points from mouse events:

| Mouse event | Handler | Calls |
|-------------|---------|-------|
| Click | `onMouseClick` | `handleHorizontalLinePress` |
| Drag | `onMouseDrag` | `handleHorizontalLineDrag` |
| Release | `mouseReleaseEvent` | `handleHorizontalLineRelease` |

#### `handleHorizontalLinePress(scenePos)`

```
hitTestHorizontalLine(y)
  ├─ HIT existing line
  │    → start drag (m_lineDragActive = true)
  │    → emit horizontalLineSyncDragStarted(syncId)
  │
  └─ MISS (empty area)
       ├─ mode == Normal     → return false (pass to graph)
       ├─ mode == DeleteLine → return false (ignore)
       └─ mode == DrawLine
            → mapScreenToTime(y) → timestamp
            → addHorizontalLine(timestamp)        // local mirror + new syncId
            → drawHorizontalLines()
            → emitHorizontalLineSyncAdded(lineId) // → hub → all graphs
            → return true
```

#### `handleHorizontalLineDrag(scenePos)`

```
if (!m_lineDragActive) return false
if moved > 2 px:
    moveHorizontalLineTo(lineId, y)               // local QGraphicsLineItem only
    emitHorizontalLineSyncUpdated(lineId)         // throttled at hub
```

#### `handleHorizontalLineRelease(button)`

```
if (!m_lineDragActive) return false   // new DrawLine click (no drag state) keeps line

emit horizontalLineSyncDragEnded()

if (dragged):
    emitHorizontalLineSyncUpdated(lineId)         // final timestamp commit
else if (mode != Normal):
    removeHorizontalLine(lineId)                  // click-without-drag on existing line
    drawHorizontalLines()
    emitHorizontalLineSyncRemoved(syncId)
```

### 5.2 Internal sync emitters (`waterfallgraph.cpp`)

| Function | When called | Qt signal emitted |
|----------|-------------|-------------------|
| `emitHorizontalLineSyncAdded(lineId)` | After user/API creates line on this graph | `horizontalLineSyncAdded(data)` |
| `emitHorizontalLineSyncUpdated(lineId)` | After drag move or final release | `horizontalLineSyncUpdated(data)` |
| `emitHorizontalLineSyncRemoved(syncId)` | After line deleted on this graph | `horizontalLineSyncRemoved(syncId)` |

Guard: all three skip emission when `m_applyingHorizontalLineSync == true` (mirror update
from hub, not a user gesture).

### 5.3 Rendering (`waterfallgraph.cpp`)

| Function | Role |
|----------|------|
| `drawHorizontalLines()` | Create/update/remove `QGraphicsLineItem` per line from `m_horizontalLines` and `mapTimeToY()`. Called from overlay pass and after line changes. |
| `moveHorizontalLineTo(lineId, sceneY)` | Fast drag: updates timestamp + `lineItem->setLine()` without full graph redraw. |
| `hitTestHorizontalLine(sceneY)` | 5 px Y tolerance hit test for press/release. |
| `refreshHorizontalLineVisuals()` | Thin wrapper → `drawHorizontalLines()`. |
| `invalidateHorizontalLineGraphicsItems()` | Delete all `QGraphicsLineItem` instances (used by `clearHorizontalLines`). |

### 5.4 Container relay (`graphcontainer.cpp`)

When a graph is registered, the container connects:

```
WaterfallGraph::horizontalLineSyncAdded    → GraphContainer::HorizontalLineSyncAdded
WaterfallGraph::horizontalLineSyncUpdated  → GraphContainer::HorizontalLineSyncUpdated
WaterfallGraph::horizontalLineSyncRemoved  → GraphContainer::HorizontalLineSyncRemoved
...
```

`GraphLayout` connects container signals to hub slots in `initializeContainers()`.

Container apply functions (called by hub fan-out):

| Slot | Effect on every graph in container |
|------|-------------------------------------|
| `onHorizontalLineSyncAdded` | `createHorizontalLineFromSyncData` or `updateHorizontalLineFromSyncData` |
| `onHorizontalLineSyncUpdated` | `updateHorizontalLineFromSyncData` |
| `onHorizontalLineSyncRemoved` | `deleteHorizontalLineBySyncId` |
| `onHorizontalLineSyncDragStarted/Ended` | `refreshHorizontalLineVisuals` (yellow highlight) |
| `onHorizontalLinesSyncCleared` | `clearHorizontalLines` |

### 5.5 Layout hub (`graphlayout.cpp`)

| Function | Role |
|----------|------|
| `setHorizontalLineMode(mode)` | Sets `m_syncState.horizontalLineMode`; calls `applyHorizontalLineModeToAllGraphs`. |
| `addHorizontalLine(...)` | Programmatic add: writes sync state + `applyHorizontalLineSyncToAllContainers`. |
| `removeHorizontalLine(syncId)` | Calls `onHorizontalLineSyncRemoved`. |
| `clearHorizontalLines()` | Calls `onHorizontalLinesSyncCleared`. |
| `onHorizontalLineSyncAdded` | **Hub:** update sync state → fan out → **emit main-system signal here**. |
| `onHorizontalLineSyncRemoved` | **Hub:** update sync state → fan out → **emit main-system signal here**. |
| `onHorizontalLineSyncUpdated` | Hub with 16 ms drag throttle; commits pending drag on `onHorizontalLineSyncDragEnded`. |
| `onHorizontalLinesSyncCleared` | Clears sync vector → fan out → **emit main-system signal here**. |
| `applyHorizontalLineModeToAllGraphs` | Pushes mode to every waterfall graph. |
| `applyHorizontalLineSyncToAllContainers` | Used by programmatic `addHorizontalLine` only. |

### 5.6 Sync state (`sharedsyncstate.h`)

| Type / member | Role |
|---------------|------|
| `HorizontalLineSyncData` | `syncId`, `timestamp`, `color`, `width`, `isDeleted` |
| `horizontalLines` | Vector of all lines (soft-delete via `isDeleted`) |
| `horizontalLineMode` | Current interaction mode |
| `draggingLineSyncId` | Line highlighted yellow during drag |
| `addOrUpdateHorizontalLine` | Insert or replace by `syncId` |
| `removeHorizontalLine` | Soft-delete one line |
| `getActiveHorizontalLines` | All lines where `!isDeleted` |
| `clearHorizontalLines` | Remove all lines + clear drag id |

---

## 6. End-to-end: user draws a line

```
1. Main system: layout->setHorizontalLineMode(DrawLine)

2. Operator clicks empty area on BRW graph
   WaterfallGraph::handleHorizontalLinePress
     → addHorizontalLine(timestamp)
     → emitHorizontalLineSyncAdded
     → horizontalLineSyncAdded(data)

3. GraphContainer relays → GraphLayout::onHorizontalLineSyncAdded
     → m_syncState.addOrUpdateHorizontalLine
     → fan out to all containers / all graph types
     → emit HorizontalLineAdded(syncId, timestamp)

4. All panels show white line at the same timestamp
```

---

## 7. End-to-end: user removes a line

```
1. Operator in DrawLine or DeleteLine mode clicks an existing line (no drag)

2. WaterfallGraph::handleHorizontalLineRelease
     → removeHorizontalLine(lineId)
     → emitHorizontalLineSyncRemoved(syncId)

3. GraphLayout::onHorizontalLineSyncRemoved
     → m_syncState.removeHorizontalLine
     → fan out delete to all mirrors
     → emit HorizontalLineRemoved(syncId, timestamp)
```

---

## 8. Data structure

```cpp
struct HorizontalLineSyncData
{
    QUuid syncId;           // global id (auto-generated in constructor)
    QDateTime timestamp;    // line position on time axis
    QColor color;           // default Qt::white
    qreal width;            // default 2.0
    bool isDeleted;         // soft-delete flag in sync state
};
```

---

## 9. Distinction from other overlays

| Feature | Horizontal lines | BTW rulers | BTW symbols |
|---------|------------------|------------|-------------|
| Axis | Time (Y) | Time + range | Time + range |
| Sync | All graph types | RTW/BTW view-local | Stored in `WaterfallData` |
| API | `setHorizontalLineMode` | `setBtwRulerActive` | `addBTWSymbol` |
| Main-system signal location | `GraphLayout` hub slots | `GraphLayout::BtwRulerClicked` | `GraphLayout::BTWManualMarkerPlaced` |

---

## 10. Quick reference

| Goal | Call / connect to |
|------|-------------------|
| Enable draw mode | `layout->setHorizontalLineMode(HorizontalLineMode::DrawLine)` |
| Add line from code | `layout->addHorizontalLine(timestamp)` |
| Remove one line | `layout->removeHorizontalLine(syncId)` |
| Clear all lines | `layout->clearHorizontalLines()` |
| Query active lines | `layout->getActiveHorizontalLines()` |
| Notify main system on user draw | `HorizontalLineAdded` (click only) |
| Notify main system on user remove | `HorizontalLineRemoved` (click only) |
