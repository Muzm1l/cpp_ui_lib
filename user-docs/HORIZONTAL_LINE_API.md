# Manual: Synchronized horizontal time lines

This document describes **horizontal time lines** — full-width constant-time overlays on
waterfall graphs — and the public API for drawing, moving, and syncing them across all
graph types and containers.

A horizontal line is positioned by a single **`timestamp`** (Y axis = time). The same
line appears on **every** waterfall graph (BTW, BDW, RTW, FTW, FDW, BRW, LTW) in **every**
container panel.

**See also:** [`GRAPH_LAYOUT_INTEGRATION_API.md`](./GRAPH_LAYOUT_INTEGRATION_API.md),
[`ARCHITECTURE_AND_IMPLEMENTATION_METHODOLOGY.md`](./ARCHITECTURE_AND_IMPLEMENTATION_METHODOLOGY.md).

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
| **DrawLine** | Add a new line | Drag to move; click without drag deletes |
| **DeleteLine** | Ignored | Click deletes |

Lines can be drawn or moved from **any** visible waterfall graph when the layout is in
the appropriate mode.

---

## 2. Architecture (low overhead)

```
User gesture on any WaterfallGraph
  → emit horizontalLineSync* signal
  → GraphContainer relay
  → GraphLayout hub (updates m_syncState)
  → every WaterfallGraph mirror: update cached QGraphicsLineItem only
```

**Performance rules:**

- **No `redrawAllGraphs()` during drag** — only `lineItem->setLine()` position updates.
- **Drag sync throttled** to ~16 ms (`INTERACTIVE_UPDATE_THROTTLE_MS`).
- **Yellow highlight** via `draggingLineSyncId` in sync state (set once at drag start, cleared at release).
- **Stable identity** via `syncId`, not timestamp matching.

---

## 3. `GraphLayout` public API

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

// Legacy BTW-named wrappers (forward to the API above)
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
// ...
layout->removeHorizontalLine(id);
layout->clearHorizontalLines();
```

### Example: query sync state

```cpp
for (const auto &line : layout->getActiveHorizontalLines()) {
    qDebug() << line.syncId << line.timestamp;
}
```

---

## 4. Signals

Sync is internal (graph → container → layout). External listeners can use legacy BTW
placement signals or query `getActiveHorizontalLines()` after changes.

---

## 5. Implementation map

| Component | Role |
|-----------|------|
| `sharedsyncstate.h` | `HorizontalLineMode`, `HorizontalLineSyncData`, `GraphContainerSyncState` helpers |
| `WaterfallGraph` | Mirror storage, `drawHorizontalLines()`, mouse interaction, sync signals |
| `BTWGraph` | BTW-specific overlays; defers line interaction to base class |
| `GraphContainer` | Applies sync payloads to every graph in `m_waterfallGraphs` |
| `GraphLayout` | Public API, sync hub, mode broadcast |

---

## 6. Distinction from other overlays

| Feature | Horizontal lines | BTW rulers | BTW symbols |
|---------|------------------|------------|-------------|
| Axis | Time (Y) | Time + range | Time + range |
| Sync | All graph types | RTW/BTW view-local | Stored in `WaterfallData` |
| API | `setHorizontalLineMode` | `setBtwRulerActive` | `addBTWSymbol` |
