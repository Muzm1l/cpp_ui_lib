# Manual: Ruler indicators (RTW and BTW)

This document describes the **ruler** system — up to four numbered circle
indicators drawn on the **RTW** and **BTW** graphs — and the public API for
driving them.

State is **view-local** to each graph widget (`RTWGraph` / `BTWGraph`); the
**`GraphLayout`** API forwards commands to the graph view(s) and re-emits the
selection signal.

**See also:** [`SYMBOL_API.md`](./SYMBOL_API.md) for general BTW/RTW symbol APIs.

---

## 1. Concepts

| Term | Meaning |
|------|---------|
| **Ruler** | One of four (index `0..3`) indicators owned by the main system. Each ruler is positioned by a **`timestamp`** (time axis) and a **`range`** (range/bearing axis). |
| **Active** | A ruler that is currently shown on the graph. There can be **0, 1, 2, 3 or 4** active rulers at any time. |
| **Selected** | The single highlighted ruler. **At most one** ruler (or zero) may be selected at a time. |
| **Indicator glyph** | A numbered circle: **yellow** (`YellowCircle1..4`) when **selected**, **white** (`WhiteCircle1..4`) when **active but unselected**. RTW uses `RTWSymbolDrawing`; BTW uses `BTWSymbolDrawing`. |

Visual states per ruler:

- **Inactive** → nothing is drawn.
- **Active, not selected** → white circle with the ruler's digit.
- **Active and selected** → yellow circle with the ruler's digit.

> Index is **0-based** in the API (`0..3`), while the digit drawn in the circle
> is **1-based** (`1..4`). Ruler index `0` draws digit `1`, etc.

---

## 2. RTW — `GraphLayout` API

```cpp
void setRtwRulerActive(int index, const QDateTime &timestamp, qreal range);
void clearRtwRuler(int index);
void clearAllRtwRulers();
void setSelectedRtwRuler(int index);
int selectedRtwRuler() const;

signals:
    void RtwRulerSelected(int index, const QDateTime &timestamp, qreal range);
```

Chain: `RTWGraph::rulerSelected` → `GraphContainer::RtwRulerSelected` →
`GraphLayout::RtwRulerSelected`.

### RTW example

```cpp
layout->setRtwRulerActive(0, t0.addSecs(-300), 10.0);
layout->setRtwRulerActive(1, t0.addSecs(-180), 15.0);
layout->setSelectedRtwRuler(1);
connect(layout, &GraphLayout::RtwRulerSelected, ...);
layout->clearAllRtwRulers();
```

---

## 3. BTW — `GraphLayout` API

Same semantics as RTW; forwards to `BTWGraph` views
(`container->getWaterfallGraph(GraphType::BTW)`). On BTW, **`range`** is the
bearing / horizontal (X) axis value.

```cpp
void setBtwRulerActive(int index, const QDateTime &timestamp, qreal range);
void clearBtwRuler(int index);
void clearAllBtwRulers();
void setSelectedBtwRuler(int index);
int selectedBtwRuler() const;

signals:
    void BtwRulerSelected(int index, const QDateTime &timestamp, qreal range);
```

Chain: `BTWGraph::rulerSelected` → `GraphContainer::BtwRulerSelected` →
`GraphLayout::BtwRulerSelected`.

### BTW example

```cpp
layout->setBtwRulerActive(0, t0.addSecs(-300), 45.0);
layout->setBtwRulerActive(1, t0.addSecs(-180), 90.0);
layout->setSelectedBtwRuler(1);
connect(layout, &GraphLayout::BtwRulerSelected, ...);
layout->clearAllBtwRulers();
```

---

## 4. Direct graph API (`RTWGraph` / `BTWGraph`)

```cpp
static constexpr int RulerCount = 4;

void setRulerActive(int index, const QDateTime &timestamp, qreal range);
void clearRuler(int index);
void clearAllRulers();
void setSelectedRuler(int index);   // -1 clears; inactive index ignored
int  selectedRuler() const;         // -1 if none
bool isRulerActive(int index) const;

signals:
    void rulerSelected(int index, const QDateTime &timestamp, qreal range);
```

Ruler state (`RulerState` in `rulerstate.h`) lives on each graph instance:

```cpp
struct RulerState {
    bool      active = false;
    QDateTime timestamp;
    qreal     range = 0.0;
};
```

---

## 5. Rules and behavior

- **Single selection invariant:** `setSelectedRuler` / `setSelectedRtwRuler` is
  the only way selection changes. Selecting one ruler deselects all others.
- **Selecting an inactive ruler is ignored.** Activate it first with
  `setRulerActive` if you want it selectable.
- **Clicking** an active ruler on the graph selects it and emits the selection
  signal.
- **Off-range rulers are not drawn.** If a ruler's mapped position falls outside
  the visible drawing area (zoom / time window), its glyph is skipped; it
  reappears when back in range. Its active/selected state is preserved.
- **Z-order:** RTW rulers at z `1001` (above symbols at `1000`); BTW rulers at
  z `1004` (above data symbols at `1003`).

---

## 6. Where things live

| Concern | File |
|---------|------|
| Shared `RulerState` struct | `rulerstate.h` |
| RTW circle glyphs | `rtwsymboldrawing.h` / `rtwsymboldrawing.cpp` |
| BTW circle glyphs | `btwsymboldrawing.h` / `btwsymboldrawing.cpp` |
| RTW ruler state + rendering | `rtwgraph.h` / `rtwgraph.cpp` |
| BTW ruler state + rendering | `btwgraph.h` / `btwgraph.cpp` |
| Container relay slots + signals | `graphcontainer.h` / `graphcontainer.cpp` |
| Layout-level API + signals | `graphlayout.h` / `graphlayout.cpp` |
