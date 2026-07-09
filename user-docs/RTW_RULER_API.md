# Manual: Ruler indicators (BTW + RTW unified)

This document describes the **ruler** system — up to four numbered circle
indicators drawn on **both BTW and RTW** graphs — and the public API for
driving them.

One `GraphLayout` call positions a ruler on **both** graph types. Selection
(yellow highlight) is **API-only**; clicking a ruler returns its timestamp
without changing selection.

**See also:** [`SYMBOL_API.md`](./SYMBOL_API.md) for general BTW/RTW symbol APIs.

---

## 1. Concepts

| Term | Meaning |
|------|---------|
| **Ruler** | One of four (index `0..3`) indicators owned by the main system. Each ruler is positioned by a **`timestamp`** (time axis) and a **`range`** (range/bearing axis). |
| **Active** | A ruler that is currently shown on the graph. There can be **0, 1, 2, 3 or 4** active rulers at any time. |
| **Selected** | The single highlighted ruler. **At most one** ruler (or zero) may be selected at a time. Changed **only via API** (`setSelectedRuler`). |
| **Indicator glyph** | A numbered circle: **yellow** (`YellowCircle1..4`) when **selected**, **white** (`WhiteCircle1..4`) when **active but unselected**. |

Visual states per ruler:

- **Inactive** → nothing is drawn.
- **Active, not selected** → white circle with the ruler's digit.
- **Active and selected** → yellow circle with the ruler's digit.

> Index is **0-based** in the API (`0..3`), while the digit drawn in the circle
> is **1-based** (`1..4`). Ruler index `0` draws digit `1`, etc.

---

## 2. `GraphLayout` API (unified BTW + RTW)

```cpp
void setRulerActive(int index, const QDateTime &timestamp, qreal range);
void clearRuler(int index);
void clearAllRulers();
void setSelectedRuler(int index);   // -1 clears; inactive index ignored
int  selectedRuler() const;         // -1 if none

signals:
    void RulerClicked(int index, const QDateTime &timestamp, qreal range, GraphType graphType);
```

- `setRulerActive` / `clearRuler` / `clearAllRulers` / `setSelectedRuler` apply to
  **every BTW and RTW graph** in every container.
- `RulerClicked` is emitted when the operator clicks an active ruler glyph. It
  returns the ruler's **timestamp** (plus index, range, and which graph was
  clicked). It does **not** change selection.

Chain:

```
BTWGraph::rulerClicked / RTWGraph::rulerClicked
  → GraphContainer::RulerClicked
  → GraphLayout::RulerClicked
```

### Example

```cpp
const QDateTime t0 = QDateTime::currentDateTime();

layout->setRulerActive(0, t0.addSecs(-300), 12.0);
layout->setRulerActive(1, t0.addSecs(-180), 30.0);
layout->setSelectedRuler(1);   // yellow highlight on ruler 2 — API only

connect(layout, &GraphLayout::RulerClicked,
        backend, [](int index, const QDateTime &timestamp, qreal range, GraphType graphType) {
    backend->onRulerClicked(index, timestamp, range, graphType);
});

layout->clearAllRulers();
```

---

## 3. Direct graph API (`RTWGraph` / `BTWGraph`)

For standalone graph instances outside `GraphLayout`:

```cpp
static constexpr int RulerCount = 4;

void setRulerActive(int index, const QDateTime &timestamp, qreal range);
void clearRuler(int index);
void clearAllRulers();
void setSelectedRuler(int index);
int  selectedRuler() const;
bool isRulerActive(int index) const;

signals:
    void rulerClicked(int index, const QDateTime &timestamp, qreal range);
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

## 4. Rules and behavior

- **Unified positioning:** One `setRulerActive` call places the ruler on both BTW
  and RTW with the same timestamp and range value.
- **Selection is API-only:** `setSelectedRuler` is the only way to change the
  yellow highlight. Clicking a ruler does **not** call `setSelectedRuler`.
- **Click returns timestamp:** `RulerClicked` carries `index`, `timestamp`,
  `range`, and `graphType` (which graph the operator clicked).
- **Selecting an inactive ruler is ignored.** Activate it first with
  `setRulerActive`.
- **Off-range rulers are not drawn.** If a ruler's mapped position falls outside
  the visible drawing area (zoom / time window), its glyph is skipped; it
  reappears when back in range. Its active/selected state is preserved.
- **Z-order:** RTW rulers at z `1001` (above symbols at `1000`); BTW rulers at
  z `1004` (above data symbols at `1003`).

---

## 5. Where things live

| Concern | File |
|---------|------|
| Shared `RulerState` struct | `rulerstate.h` |
| RTW circle glyphs | `rtwsymboldrawing.h` / `rtwsymboldrawing.cpp` |
| BTW circle glyphs | `btwsymboldrawing.h` / `btwsymboldrawing.cpp` |
| RTW ruler state + rendering | `rtwgraph.h` / `rtwgraph.cpp` |
| BTW ruler state + rendering | `btwgraph.h` / `btwgraph.cpp` |
| Container relay slots + signals | `graphcontainer.h` / `graphcontainer.cpp` |
| Layout-level API + signals | `graphlayout.h` / `graphlayout.cpp` |

---

## 6. Quick reference

| Goal | Call / connect to |
|------|-------------------|
| Activate ruler on BTW + RTW | `layout->setRulerActive(index, timestamp, range)` |
| Highlight a ruler (yellow) | `layout->setSelectedRuler(index)` |
| Clear one / all rulers | `layout->clearRuler(index)` / `layout->clearAllRulers()` |
| Receive timestamp on click | `connect(..., RulerClicked, ...)` |
| Query selected ruler | `layout->selectedRuler()` |
