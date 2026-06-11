# Manual: RTW Ruler indicators

This document describes the **RTW ruler** system — up to four numbered circle
indicators drawn on the RTW graph — and the public API for driving them.

State is **view-local** to the **`RTWGraph`** widget; the **`GraphLayout`** API
forwards commands to the RTW graph view(s) and re-emits the selection signal,
mirroring the existing BTW horizontal-line / shaded-region pattern.

**See also:** [`SYMBOL_API.md`](./SYMBOL_API.md) for general BTW/RTW symbol APIs.

---

## 1. Concepts

| Term | Meaning |
|------|---------|
| **Ruler** | One of four (index `0..3`) indicators owned by the main system. Each ruler is positioned by a **`timestamp`** (time axis) and a **`range`** (range axis). |
| **Active** | A ruler that is currently shown on the graph. There can be **0, 1, 2, 3 or 4** active rulers at any time. |
| **Selected** | The single highlighted ruler. **At most one** ruler (or zero) may be selected at a time. |
| **Indicator glyph** | A numbered circle from **`RTWSymbolDrawing`**: a **yellow** circle (`YellowCircle1..4`) when **selected**, a **white** circle (`WhiteCircle1..4`) when **active but unselected**. |

Visual states per ruler:

- **Inactive** → nothing is drawn.
- **Active, not selected** → white circle with the ruler's digit.
- **Active and selected** → yellow circle with the ruler's digit.

> Index is **0-based** in the API (`0..3`), while the digit drawn in the circle
> is **1-based** (`1..4`). Ruler index `0` draws digit `1`, etc.

---

## 2. `GraphLayout` API (preferred entry point)

```cpp
// Activate/position a ruler (index 0..3). Draws it as a white circle
// unless it is the selected ruler.
void setRtwRulerActive(int index, const QDateTime &timestamp, qreal range);

// Deactivate a single ruler (removes it). Clears selection if it was selected.
void clearRtwRuler(int index);

// Deactivate all rulers and clear the selection.
void clearAllRtwRulers();

// Select a ruler (turns it yellow) and deselect all others.
// Pass -1 to clear the selection. Selecting an inactive ruler is ignored.
void setSelectedRtwRuler(int index);

// Index of the currently selected ruler, or -1 if none.
int selectedRtwRuler() const;
```

These iterate every container, fetch the RTW graph view
(`container->getWaterfallGraph(GraphType::RTW)`), and forward the call. A redraw
is triggered automatically by the graph.

### Selection signal

```cpp
signals:
    // Emitted when a ruler indicator is clicked on the graph (and selected).
    void RtwRulerSelected(int index, const QDateTime &timestamp, qreal range);
```

Chain: `RTWGraph::rulerSelected` → `GraphContainer::RtwRulerSelected` →
`GraphLayout::RtwRulerSelected`.

### Example

```cpp
// Given GraphLayout *layout
const QDateTime t0 = QDateTime::currentDateTime();

// Activate three rulers
layout->setRtwRulerActive(0, t0.addSecs(-300), 10.0);  // white "1"
layout->setRtwRulerActive(1, t0.addSecs(-180), 15.0);  // white "2"
layout->setRtwRulerActive(2, t0.addSecs(-60),  20.0);  // white "3"

// Select ruler 2 -> it turns yellow, others stay white
layout->setSelectedRtwRuler(1);

// React to user clicks on a ruler
connect(layout, &GraphLayout::RtwRulerSelected,
        [](int index, const QDateTime &ts, qreal range) {
            qDebug() << "Ruler" << index << "selected at" << ts << range;
        });

// Clear selection (no yellow circle, all active rulers shown white)
layout->setSelectedRtwRuler(-1);

// Remove everything
layout->clearAllRtwRulers();
```

---

## 3. `RTWGraph` API (direct, view-local)

If you hold an `RTWGraph *` directly, the same operations are available:

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

Ruler state lives on each `RTWGraph` instance:

```cpp
struct RulerState {
    bool      active = false;
    QDateTime timestamp;
    qreal     range = 0.0;
};
```

---

## 4. Rules and behavior

- **Single selection invariant:** `setSelectedRuler` / `setSelectedRtwRuler` is
  the only way selection changes. Selecting one ruler deselects all others.
- **Selecting an inactive ruler is ignored.** Activate it first with
  `setRulerActive` if you want it selectable.
- **Clicking** an active ruler on the graph selects it and emits the selection
  signal.
- **Off-range rulers are not drawn.** If a ruler's mapped position falls outside
  the visible drawing area (zoom / time window), its glyph is skipped; it
  reappears when back in range. Its active/selected state is preserved.
- **Z-order:** ruler glyphs are drawn at z-value `1001`, above ordinary RTW
  symbols (z `1000`), so they remain visible and clickable.

---

## 5. Where things live

| Concern | File |
|---------|------|
| Circle glyphs (`YellowCircle1..4`, `WhiteCircle1..4`, `makeNumberedCircle`) | `rtwsymboldrawing.h` / `rtwsymboldrawing.cpp` |
| Ruler state, `drawRulers()`, click handling, API | `rtwgraph.h` / `rtwgraph.cpp` |
| Container relay slot + signal | `graphcontainer.h` / `graphcontainer.cpp` |
| Layout-level API + signal | `graphlayout.h` / `graphlayout.cpp` |
