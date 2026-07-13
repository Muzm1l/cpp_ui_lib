# Manual: BTW marker symbol sizes

This document explains how to change the **visual size** of BTW markers — both the
**manual marker** (pink interactive circle + line) and the **automatic marker**
(magenta circle + angled line drawn from data).

There is no runtime API for marker geometry today. Sizes are controlled by constants
in the drawing code. After changing them, rebuild the library/app.

**See also:**
[`BTW_MANUAL_MARKER_SERIES_API.md`](./BTW_MANUAL_MARKER_SERIES_API.md) (series binding),
[`RTW_RULER_API.md`](./RTW_RULER_API.md) (ruler circles — separate system).

---

## 1. Overview

| Marker type | Color | Interactive? | Primary file |
|-------------|-------|--------------|--------------|
| **Manual marker** | Light pink (`#FFB6C1`) | Yes — drag / rotate on overlay | `btwinteractiveoverlay.cpp` |
| **Automatic marker** | Magenta | No — redrawn from data source | `btwgraph.cpp` |

Both markers share the same geometry model:

```
circle diameter = 2 × markerRadius
line half-length = 5 × markerRadius   (total span = 10 × markerRadius)
```

So shrinking the circle automatically shrinks the line **if** you change
`markerRadius` (or the cached equivalent) and keep the `5 * markerRadius`
multiplier.

---

## 2. Manual marker (pink, interactive)

### Where it is drawn

Manual markers are `InteractiveGraphicsItem` instances created in two places.
**Both code paths must stay in sync** when you change size:

| Function | Purpose |
|----------|---------|
| `BTWInteractiveOverlay::addDataPointMarker()` | User click or programmatic add |
| `BTWInteractiveOverlay::addMarkerFromData()` (sync restore path) | Cross-container marker sync |

**File:** `btwinteractiveoverlay.cpp`

### Constants to change

In **both** custom draw lambdas, update these together:

```cpp
marker->setSize(QSizeF(12, 12));   // item bounding box (width = height)

qreal markerRadius = 6.0;          // MUST be half of setSize width/height
qreal lineLength = 5 * markerRadius;
qreal headRadius = 2.0;            // filled dot at the line head end
```

Current defaults (as of this doc):

| Constant | Value | Effect |
|----------|-------|--------|
| `setSize` | `12 × 12` | Hit box, drag/rotate bounds |
| `markerRadius` | `6.0` | Circle radius |
| `lineLength` | `5 × markerRadius` → `30` | Half-length of the bearing line |
| `headRadius` | `2.0` | Head endpoint dot |

### Bearing-rate label offset

The pink value box to the left of the marker uses the same radius for spacing:

**Function:** `BTWInteractiveOverlay::updateBearingRateBox()`

```cpp
qreal markerRadius = 6.0; // Match the marker radius in addDataPointMarker
qreal textX = markerPos.x() - textRect.width() - markerRadius - 7;
```

If you change `markerRadius` in the draw functions, update this value too so the
label stays aligned with the circle.

### Line thickness and color (optional)

Stroke width comes from `InteractiveGraphicsItem`, default **`2.0`** px
(`interactivegraphicsitem.cpp`). Per-marker overrides are available via:

```cpp
BTWInteractiveOverlay::setMarkerStyle(marker, markerColor, lineColor, lineWidth);
BTWInteractiveOverlay::setAllMarkerStyles(markerColor, lineColor, lineWidth);
```

Circle/line **color** defaults: marker `#FFB6C1`, line same color, width `2.0`.

### Bearing-rate text size (optional)

The pink `R…` / `L…` label beside the manual marker uses a cached font:

**Function:** `BTWInteractiveOverlay::getCachedTextPixmap()`

```cpp
s_cachedFont.setPointSizeF(8.0);
s_cachedFont.setBold(true);
```

Changing this affects all bearing-rate boxes. Clear the pixmap cache or restart if
you tweak font size during development (cache key is the text string only).

### Rotation hit region (optional)

Rotation grab area size (head-only rotation):

```cpp
marker->setRotateRegionSize(QSizeF(12, 12));
```

Consider scaling this down if you make the marker much smaller, so the rotate handle
does not dominate the glyph.

### Example: make manual markers ~40% smaller

```cpp
marker->setSize(QSizeF(8, 8));
qreal markerRadius = 4.0;
qreal headRadius = 1.5;
// lineLength stays: 5 * markerRadius  → 20 px half-length
```

Also set `markerRadius = 4.0` in `updateBearingRateBox()`.

---

## 3. Automatic marker (magenta, data-driven)

### Where it is drawn

**Function:** `BTWGraph::drawCustomCircleMarkers()`  
**File:** `btwgraph.cpp`

Markers are rebuilt each draw pass from automatic marker data in the data source.
Circle, line, and label are separate `QGraphicsItem`s on the overlay scene.

### Circle and line size

Radius is **cached from window width** so markers scale slightly with panel size:

**Function:** `BTWGraph::updateWindowSizeCache()`

```cpp
m_cachedMarkerRadius = std::min(0.025 * m_cachedWindowSize.width(), 7.0);
```

| Parameter | Current | Meaning |
|-----------|---------|---------|
| `0.025` | 2.5% of graph width | Proportional term |
| `7.0` | px cap | Maximum radius |

In `drawCustomCircleMarkers()`:

```cpp
qreal markerRadius = m_cachedMarkerRadius;
qreal lineLength = 5 * markerRadius;   // same 5× rule as manual markers
```

### Stroke width

Both circle and line use a **2 px** magenta pen:

```cpp
circleOutline->setPen(QPen(Qt::magenta, 2));
angledLine->setPen(QPen(Qt::magenta, 2));
```

Reduce the second argument (e.g. `1`) for thinner strokes without changing radius.

### Label and outline (optional)

Automatic marker delta label (`R12.3` / `L4.5` style):

```cpp
font.setPointSizeF(8.0);
font.setBold(true);
textLabel->setPos(screenPos.x() - textRect.width() - markerRadius - 5, ...);
textOutline->setPen(QPen(Qt::magenta, 1));
```

Text position uses `markerRadius`; if you change the radius formula, spacing updates
automatically. Font size is independent — change `8.0` to scale the label.

### Example: smaller fixed automatic markers

To use a fixed radius instead of window scaling, replace the cache line with e.g.:

```cpp
m_cachedMarkerRadius = 5.0;
```

Or tighten scaling:

```cpp
m_cachedMarkerRadius = std::min(0.018 * m_cachedWindowSize.width(), 5.0);
```

The cache is refreshed in `resizeEvent()`, so live resize picks up new formulas.

---

## 4. Shared geometry reference

```
                    head (filled dot, manual only)
                         ●
                        /
                       /  lineLength = 5 × markerRadius
                      /
         ────────────●────────────  circle center
              markerRadius
```

| Quantity | Formula | Manual (current) | Automatic (current, typical) |
|----------|---------|------------------|------------------------------|
| Circle diameter | `2 × markerRadius` | 12 px | ≤ 14 px (cap 7) |
| Line total length | `10 × markerRadius` | 60 px | ≤ 70 px |
| Line half-length | `5 × markerRadius` | 30 px | ≤ 35 px |

---

## 5. Checklist after changing sizes

1. Update **both** manual marker draw lambdas in `btwinteractiveoverlay.cpp`.
2. Update **`updateBearingRateBox()`** `markerRadius` to match.
3. Update **`updateWindowSizeCache()`** (and/or pen widths) for automatic markers.
4. Rebuild and verify:
   - click placement and drag bounds (manual),
   - rotation at line head (manual),
   - bearing-rate box not overlapping the circle (manual),
   - automatic markers at several window widths,
   - markers still visible but not obscuring BTW traces.

---

## 6. Where things live

| Concern | File |
|---------|------|
| Manual marker create + draw | `btwinteractiveoverlay.cpp` — `addDataPointMarker()`, sync restore |
| Manual marker bearing-rate box | `btwinteractiveoverlay.cpp` — `updateBearingRateBox()`, `getCachedTextPixmap()` |
| Manual marker item bounds / rotation math | `interactivegraphicsitem.cpp` — `boundingRect()`, rotate hit tests |
| Automatic marker draw | `btwgraph.cpp` — `drawCustomCircleMarkers()` |
| Automatic marker radius cache | `btwgraph.cpp` — `updateWindowSizeCache()`, `resizeEvent()` |
| Cached radius member | `btwgraph.h` — `m_cachedMarkerRadius` |

---

## 7. Quick reference

| Goal | What to change |
|------|----------------|
| Smaller manual circle + line | `setSize`, `markerRadius`, `headRadius` in both overlay lambdas; `updateBearingRateBox()` radius |
| Thinner manual strokes | `InteractiveGraphicsItem` default `m_lineWidth` or `setMarkerStyle()` |
| Smaller manual value box text | `s_cachedFont.setPointSizeF(...)` in `getCachedTextPixmap()` |
| Smaller automatic circle + line | `m_cachedMarkerRadius` formula in `updateWindowSizeCache()` |
| Thinner automatic strokes | `QPen(Qt::magenta, …)` width in `drawCustomCircleMarkers()` |
| Smaller automatic delta label | `font.setPointSizeF(...)` in `drawCustomCircleMarkers()` |
