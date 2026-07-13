# Manual: Tactical Solution View (TSV) — integration and recent updates

This document describes the **Tactical Solution View** (`TacticalSolutionView`, widget name
`tsv`) and how to replicate display updates in a host ("integrated") system:

1. **Own-ship-only display** — TSV draws as soon as own-ship data is available; track
   vectors are optional.
2. **Own-ship base marker** — hollow cyan right triangle (right angle at base).
3. **Vector base points on the bearing line** — own ship, selected track, and adopted
   track all start on the green sensor bearing line.

**See also:** [`ARCHITECTURE_AND_IMPLEMENTATION_METHODOLOGY.md`](./ARCHITECTURE_AND_IMPLEMENTATION_METHODOLOGY.md)
for where TSV fits in the overall library.

**Troubleshooting:** §8 covers scroll bars appearing when TSV is embedded at a fixed size
(e.g. **226 × 240**).

---

## 1. What TSV is

`TacticalSolutionView` is a `QGraphicsView` subclass that renders a tactical vector plot:

| Element | Colour | Meaning |
|---------|--------|---------|
| Green line | `Qt::green` | Sensor **bearing line** through own ship |
| Cyan vector | `Qt::cyan` | Own-ship course (speed + bearing) |
| Yellow vector | `Qt::yellow` | Selected track course |
| Red vector | `Qt::red` | Adopted track course |
| White hatched region | `Qt::BDiagPattern` | Half-plane shading relative to own-ship course |

In the sandbox, the widget lives on the **Tactical View** tab (`mainwindow.ui`, name `tsv`)
and is fed by `MainWindow::updateSimulation()` via `setData(...)`.

---

## 2. Public API

### 2.1 Own-ship only (start display immediately)

```cpp
void setOwnShipData(
    const qreal &ownShipSpeed,
    const qreal &ownShipBearing,
    const qreal &sensorBearing);
```

Call this as soon as own-ship and sensor-bearing data are available — **do not wait** for a
computed track solution. TSV draws:

- green bearing line,
- cyan own-ship vector with **hollow triangle** base,
- white hatched half-plane.

No yellow (selected) or red (adopted) vectors are drawn.

**Integrated-system wiring (own-ship phase):**

```cpp
// As soon as own ship is known — before any track solution exists:
tsv->setOwnShipData(ownShipSpeed, ownShipBearing, sensorBearing);
```

Call again whenever own-ship speed, course, or sensor bearing changes.

### 2.2 Full or partial solution

```cpp
void setData(
    const qreal &ownShipSpeed,
    const qreal &ownShipBearing,
    const qreal &sensorBearing,
    const qreal &adoptedTrackRange,
    const qreal &adoptedTrackSpeed,
    const qreal &adoptedTrackBearing,
    const qreal &selectedTrackRange,
    const qreal &selectedTrackSpeed,
    const qreal &selectedTrackBearing,
    const qreal &adoptedTrackCourse,
    const qreal &selectedTrackCourse,
    bool showSelectedTrack = true,
    bool showAdoptedTrack = true);
```

Call when a computed solution arrives. Use the `show*` flags to control which track vectors
are drawn:

| Situation | Call |
|-----------|------|
| Full solution (both tracks) | `setData(...)` — defaults show both |
| Selected track only | `setData(..., true, false)` |
| Adopted track only | `setData(..., false, true)` |
| Own ship only | Prefer `setOwnShipData(...)` |

**Typical integrated lifecycle:**

```cpp
// Phase 1 — own ship available
tsv->setOwnShipData(ownShipSpeed, ownShipBearing, sensorBearing);

// Phase 2 — selected solution computed
tsv->setData(
    ownShipSpeed, ownShipBearing, sensorBearing,
    adoptedRange, adoptedSpeed, adoptedBearing,
    selectedRange, selectedSpeed, selectedBearing,
    adoptedCourse, selectedCourse,
    true,   // show selected
    false); // adopted not yet available

// Phase 3 — both solutions available
tsv->setData(
    ownShipSpeed, ownShipBearing, sensorBearing,
    adoptedRange, adoptedSpeed, adoptedBearing,
    selectedRange, selectedSpeed, selectedBearing,
    adoptedCourse, selectedCourse);  // both default true
```

### 2.3 Common integrator mistake

**Do not** gate all TSV updates on "computed solution ready". That leaves the widget blank
until tracks exist. Instead:

1. Call `setOwnShipData()` at startup / when own ship first appears.
2. Switch to `setData()` when track solutions arrive (with appropriate `show*` flags).

Call either method whenever tactical inputs change. The view normalizes range/speed to the
current scene size and redraws automatically.

**Bearing parameters used for drawing**

| Parameter | Used for |
|-----------|----------|
| `sensorBearing` | Green bearing line; **base-point placement** for all three vectors |
| `ownShipBearing` | Own-ship vector direction |
| `selectedTrackCourse` / `adoptedTrackCourse` | Track vector directions |
| `selectedTrackBearing` / `adoptedTrackBearing` | Accepted by API but **not** used for base-point position after the bearing-line update (see §4) |

---

## 3. Own-ship base marker: hollow cyan triangle

### Behaviour

- **Own ship** base point: **hollow right triangle** fixed in screen orientation (right angle
  at base, vertical leg up, horizontal leg right). Does **not** rotate with course — only the
  cyan vector line rotates. Outline colour: `Qt::cyan`, `Qt::NoBrush`.
- **Selected (computed) and adopted tracks** use a **hollow circle** at their base points
  (yellow and red respectively).
- The hollow triangle is **automatic** — no host configuration flag is required.

### How to set / enable in the integrated system

The hollow triangle is built into the library. The integrator does **not** pass a colour or
style flag. To get it:

1. **Link this library revision** (or port the drawing code below).
2. Call `setOwnShipData()` or `setData()` — own ship always renders with the hollow triangle.
3. Do **not** override drawing in the host; the marker is drawn inside `drawVectorsFromPointStore`.

**What the library does internally** (`tacticalsolutionview.cpp`):

```cpp
// Own ship only — always hollow triangle:
drawCourseVectorFromEndpoints(
    pointStore.ownShipPoints.first,
    pointStore.ownShipPoints.second,
    Qt::cyan,
    true);  // hollowTriangleMarker = true

// Tracks — hollow circle (hollowTriangleMarker defaults to false):
drawCourseVectorFromEndpoints(start, end, Qt::yellow);
drawCourseVectorFromEndpoints(start, end, Qt::red);
```

### How to replicate manually in another tree

1. Add `bool hollowTriangleMarker = false` to your vector-drawing helper.
2. When `hollowTriangleMarker` is **true** (own ship only):
   - Draw a **fixed-orientation** right triangle at `startPoint` (does not use course angle):
     - Right angle at `startPoint`
     - `topLeft = (startPoint.x(), startPoint.y() - triHeight)`
     - `bottomRight = (startPoint.x() + triWidth, startPoint.y())`
   - Draw with `QPen(Qt::cyan, 2)` and `Qt::NoBrush`.
   - The course vector line is drawn separately and **does** rotate.
3. When **false** (tracks): draw a **hollow circle** (`Qt::NoBrush` ellipse).
4. Never pass `hollowTriangleMarker = true` for track vectors.

| Vector | Base marker | Colour | Style |
|--------|-------------|--------|-------|
| Own ship | Hollow right triangle (fixed orientation) | `Qt::cyan` | Right angle at base; vertical up; horizontal right; vector rotates independently |
| Selected (computed) track | Hollow circle | `Qt::yellow` | `Qt::NoBrush` ellipse |
| Adopted track | Hollow circle | `Qt::red` | `Qt::NoBrush` ellipse |

### Where it lives

| File | Symbol |
|------|--------|
| `tacticalsolutionview.cpp` | `drawCourseVectorFromEndpoints(..., hollowTriangleMarker)` |
| `tacticalsolutionview.cpp` | `drawVectorsFromPointStore` — passes `hollowTriangleMarker = true` for own ship only |

**Reference implementation** (`tacticalsolutionview.cpp`, `drawCourseVectorFromEndpoints`):

```cpp
if (hollowTriangleMarker) {
    const qreal triHeight = 12;
    const qreal triWidth = 8;

    QPointF topLeft(anchor.x(), anchor.y() - triHeight);
    QPointF bottomRight(anchor.x() + triWidth, anchor.y());

    QPolygonF triangle;
    triangle << startPoint << topLeft << bottomRight;
    pen.setWidth(2);
    scene->addPolygon(triangle, pen, Qt::NoBrush);
} else {
    pen.setWidth(2);
    scene->addEllipse(startPoint.x() - radius, startPoint.y() - radius,
                      radius * 2, radius * 2, pen, Qt::NoBrush);
}
```

---

## 4. Vector base points on the bearing line

### Problem (before update)

Track base points were placed with `DrawUtils::bearingToCartesian(range, trackBearing, sceneRect)`.
That scatters contacts in 2D by each track's individual bearing, so bases often **do not** lie
on the green sensor bearing line.

### Behaviour (after update)

All three vector **base points** lie on the **sensor bearing line**:

| Vector | Base point |
|--------|------------|
| Own ship | Scene centre (range 0 on bearing line) |
| Selected track | `range = selectedTrackRange` along `sensorBearing` from own ship |
| Adopted track | `range = adoptedTrackRange` along `sensorBearing` from own ship |

Course arrows still use `selectedTrackCourse` / `adoptedTrackCourse` (and `ownShipBearing`)
from each base point — only **where** the base sits changed.

### Where it lives

`TacticalSolutionView::getGuideBox()` in `tacticalsolutionview.cpp`.

### How to replicate in the integrated system

**Step 1 — Own-ship origin (unchanged)**

```cpp
QPointF ownShipPosition = DrawUtils::bearingToCartesian(0, 0, scene->sceneRect());
```

**Step 2 — Replace track base placement**

*Before (do not use for integrated TSV):*

```cpp
QPointF selectedTrackPosition = DrawUtils::bearingToCartesian(
    selectedTrackRange, selectedTrackBearing, scene->sceneRect());
QPointF adoptedTrackPosition = DrawUtils::bearingToCartesian(
    adoptedTrackRange, adoptedTrackBearing, scene->sceneRect());
```

*After (bearing-line alignment):*

```cpp
QPointF selectedTrackPosition = DrawUtils::calculateEndpoint(
    ownShipPosition, selectedTrackRange, sensorBearing);
QPointF adoptedTrackPosition = DrawUtils::calculateEndpoint(
    ownShipPosition, adoptedTrackRange, sensorBearing);
```

**Step 3 — Endpoints unchanged**

```cpp
auto selectedEndpoint = DrawUtils::calculateEndpoint(
    selectedTrackPosition, selectedTrackSpeed, selectedTrackCourse);
auto adoptedEndpoint = DrawUtils::calculateEndpoint(
    adoptedTrackPosition, adoptedTrackSpeed, adoptedTrackCourse);
```

**Step 4 — Pass `sensorBearing` into geometry**

Ensure `getGuideBox` (or your equivalent layout function) receives `sensorBearing` and uses
it for track base placement, not `selectedTrackBearing` / `adoptedTrackBearing`.

**Step 5 — Normalization**

`setData` already scales `selectedTrackRange` / `adoptedTrackRange` before `draw()`. Base
points use those **normalized** range values, so spacing along the bearing line matches the
zoomed view.

### Visual check

After integration:

- Own ship: hollow cyan right triangle at the centre of the green bearing line.
- Yellow and red **hollow circles** on the **same** green line at distances proportional to range.
- Course arrows may leave the bearing line; only **bases** are constrained to it.

---

## 5. Wiring in a host system

```cpp
TacticalSolutionView *tsv = ...;

// 1. Own ship first — do not wait for track solution
tsv->setOwnShipData(ownShipSpeed, ownShipBearing, sensorBearing);

// 2. When solution(s) arrive
tsv->setData(
    ownShipSpeed, ownShipBearing, sensorBearing,
    adoptedTrackRange, adoptedTrackSpeed, adoptedTrackBearing,
    selectedTrackRange, selectedTrackSpeed, selectedTrackBearing,
    adoptedTrackCourse, selectedTrackCourse);
```

Hollow triangle, bearing-line base points, and own-ship-only mode are all internal to
`draw()` once this library revision is linked.

---

## 6. File checklist for porting

When merging TSV updates into another tree, touch:

| File | Changes |
|------|---------|
| `tacticalsolutionview.h` | `setOwnShipData()`; `setData(..., showSelectedTrack, showAdoptedTrack)`; `applyDataAndDraw()` |
| `tacticalsolutionview.cpp` | Own-ship-only draw path; conditional track drawing; hollow triangle; `getGuideBox` bearing-line bases; minimum guide-box fallback |

`DrawUtils::calculateEndpoint` and `DrawUtils::bearingToCartesian` are unchanged; reuse
existing `drawutils` from the library.

---

## 7. Own-ship-only display (implementation notes)

### Problem (before update)

Integrators typically called `setData()` only when a computed solution existed. With no
call, TSV stayed blank. Passing zero track values to `setData()` still drew degenerate
yellow/red markers on top of own ship.

### Behaviour (after update)

| API | Tracks drawn | Normalization uses |
|-----|--------------|-------------------|
| `setOwnShipData()` | None | Own-ship speed only |
| `setData(..., false, false)` | None | Own-ship speed only |
| `setData(..., true, false)` | Selected only | Own ship + selected range/speed |
| `setData(..., true, true)` | Both | All three entities |

When only own ship is shown and speed is zero, a minimum guide box prevents a degenerate
(empty) scene.

---

## 8. Scroll bars at fixed size (e.g. 226 × 240)

### Symptom

When `TacticalSolutionView` is placed in the integrated system's scene/layout at a small
fixed size (commonly **226 × 240**, matching `setMinimumSize(226, 240)` in the constructor),
horizontal or vertical **scroll bars** appear even though the widget should be a snug,
non-scrollable panel.

The scroll bar may belong to TSV itself (`QGraphicsView`) or to a **parent** container
(`QScrollArea`, scrollable layout).

---

### How TSV manages size today

| Mechanism | Location | Behaviour |
|-----------|----------|-----------|
| Scroll bar policy | Constructor | `ScrollBarAlwaysOff` on both axes |
| Scene rect update | `draw()` only | `scene->setSceneRect(0, 0, width(), height())` |
| Resize handling | **Not implemented** | `resizeEvent` is commented out in `tacticalsolutionview.h` |
| Data / redraw | `setData()` | Calls `draw()` after normalizing range/speed |

`QGraphicsView` computes its scroll range from **`sceneRect()`**, not only from visible
items. If the scene rect is larger than the viewport, scroll bars appear (when policy is
`ScrollBarAsNeeded`, the Qt default).

---

### Root causes (most likely first)

#### 8.1 Scene rect larger than the widget (most common)

Typical failure sequence in a host system:

1. `setData()` / `draw()` runs while the widget is still **large** (e.g. 800 × 600 copied
   from sandbox `.ui`, or before the layout assigns the final geometry).
2. `draw()` sets `sceneRect` to that larger size.
3. The layout then constrains TSV to **226 × 240**.
4. Viewport is 226 × 240 but `sceneRect` is still 800 × 600 → overflow → scroll bars.

Because there is **no `resizeEvent`**, a resize after the first draw does **not** update
the scene rect or trigger a redraw.

#### 8.2 Scroll bar policy not applied in the integrated build

The library constructor sets `ScrollBarAlwaysOff`. Scroll bars still appear if:

- the integrated tree uses an **older TSV** without those lines;
- host code resets scroll policies after construction;
- TSV is wrapped in a **`QScrollArea`** or other scrollable parent (the parent scroll bar
  is not controlled by TSV's policy).

Default `QGraphicsView` policy is `Qt::ScrollBarAsNeeded`.

#### 8.3 Oversized scene items

The green bearing line is drawn very long before the zoom transform:

```cpp
auto bearingLineRange = std::max(sceneRect.width(), sceneRect.height());
auto p1 = DrawUtils::calculateEndpoint(ownShipPosition, bearingLineRange * 5, sensorBearing);
```

At 226 × 240, that extends roughly **1200 px** from centre. `DrawUtils::computeTransformationMatrix`
currently applies **translation only** — the scale step is commented out in `drawutils.cpp` —
so items are re-centred but not shrunk. Content can extend beyond `sceneRect`, which triggers
`ScrollBarAsNeeded` on builds that do not force `AlwaysOff`.

#### 8.4 `setData()` called before final geometry

`setData()` normalizes range/speed using the current scene dimensions:

```cpp
qreal sceneDim = std::min(scene->sceneRect().width(), scene->sceneRect().height());
```

If called while `width()` / `height()` are still 0 or stale, normalization and the scene
rect are wrong until the next `draw()`. Resize alone does not schedule that redraw.

#### 8.5 Unusually tall embedding (e.g. 226 × 2240)

If height is **2240** rather than **240**, `bearingLineRange * 5` becomes ~11 200 px and
overflow risk increases sharply. Confirm the intended size with the layout designer.

---

### Diagnostic checklist

Run these **after** the host layout has finished (e.g. after `show()` or `QTimer::singleShot(0, ...)`):

| Check | What to log / inspect | Indicates problem when |
|-------|----------------------|-------------------------|
| Widget size | `tsv->size()` | Differs from expected 226 × 240 |
| Scene rect | `tsv->sceneRect()` (via scene) | **Larger** than widget/viewport |
| Viewport | `tsv->viewport()->size()` | Smaller than scene rect |
| Scroll policy | `horizontalScrollBarPolicy()`, `verticalScrollBarPolicy()` | Not `ScrollBarAlwaysOff` |
| Parent scroll | Walk parent chain for `QScrollArea` | Parent provides the scroll bar |
| Draw timing | When first `setData()` is called | Before final layout geometry |

**Smoking gun:** `sceneRect().width/height` > `viewport()->width/height`.

Example debug snippet:

```cpp
QTimer::singleShot(0, tsv, [tsv]() {
    qDebug() << "TSV size:" << tsv->size()
             << "viewport:" << tsv->viewport()->size()
             << "sceneRect:" << tsv->scene()->sceneRect()
             << "hScroll:" << tsv->horizontalScrollBarPolicy()
             << "vScroll:" << tsv->verticalScrollBarPolicy();
});
```

---

### Integrator workarounds (no library change)

**1. Force scroll bars off** (if the bar is on TSV itself):

```cpp
tsv->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
tsv->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
```

**2. Call `setData()` after layout** — not only in the constructor:

```cpp
// After show() or when layout has settled:
tsv->setData(ownShipSpeed, ownShipBearing, sensorBearing, ...);
```

**3. Avoid wrapping TSV in `QScrollArea`** unless scrolling is intentional.

**4. Match `.ui` geometry to the deployed size** (226 × 240) so the first `draw()` does not
lock in a larger scene rect.

---

### Recommended library fix (for a future revision)

Add `resizeEvent` (same pattern as `ManoeuvreOverlay`):

```cpp
void TacticalSolutionView::resizeEvent(QResizeEvent *event)
{
    QGraphicsView::resizeEvent(event);
    if (scene)
        scene->setSceneRect(0, 0, event->size().width(), event->size().height());
    draw();  // or redraw only when tactical data has been set at least once
}
```

Optional hardening:

- Clip or shorten the bearing line to scene bounds instead of `bearingLineRange * 5`.
- Re-enable uniform scaling in `DrawUtils::computeTransformationMatrix`, or call
  `fitInView(scene->sceneRect(), Qt::KeepAspectRatio)` after drawing.

---

### Quick decision guide

| Observation | Likely cause | Action |
|-------------|--------------|--------|
| `sceneRect` > viewport, first draw at 800×600 | §8.1 stale scene rect | `setData()` after layout; add `resizeEvent` |
| Scroll bar on parent, not TSV | §8.2 `QScrollArea` / layout | Remove parent scroll; size parent to 226×240 |
| `ScrollBarAsNeeded` on TSV | §8.2 old build or policy reset | Set `AlwaysOff`; update library |
| Huge bearing line, policy `AsNeeded` | §8.3 item overflow | Shorten line or enable scale transform |
| Wrong vector scale, size was 0 at first `setData` | §8.4 early `setData` | Defer `setData` until geometry is valid |
