# Manual: Tactical Solution View (TSV) — integration and recent updates

This document describes the **Tactical Solution View** (`TacticalSolutionView`, widget name
`tsv`) and how to replicate two display updates in a host ("integrated") system:

1. **Own-ship base marker** — hollow cyan triangle (not a filled circle).
2. **Vector base points on the bearing line** — own ship, selected track, and adopted
   track all start on the green sensor bearing line.

**See also:** [`ARCHITECTURE_AND_IMPLEMENTATION_METHODOLOGY.md`](./ARCHITECTURE_AND_IMPLEMENTATION_METHODOLOGY.md)
for where TSV fits in the overall library.

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
    const qreal &selectedTrackCourse);
```

Call `setData` whenever tactical inputs change. The view normalizes range/speed to the
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

- **Own ship** base point: **hollow** triangle, **cyan** outline (`Qt::NoBrush`), tip pointing
  along the own-ship course vector.
- **Selected / adopted tracks** keep the existing **filled circle** at their base points
  (yellow and red respectively).

### Where it lives

| File | Symbol |
|------|--------|
| `tacticalsolutionview.cpp` | `drawCourseVectorFromEndpoints(..., hollowTriangleMarker)` |
| `tacticalsolutionview.cpp` | `drawVectorsFromPointStore` — passes `hollowTriangleMarker = true` for own ship only |

### How to replicate in the integrated system

1. Add a `bool hollowTriangleMarker = false` parameter to your vector-drawing helper (or
   equivalent).
2. When `hollowTriangleMarker` is true:
   - Compute course angle from base to endpoint: `qAtan2(dy, dx)`.
   - Build a small triangle (tip toward endpoint, base vertices perpendicular to course).
   - Draw with `QPen(Qt::cyan, 2)` and `Qt::NoBrush`.
3. When false, keep the existing filled-circle base marker.
4. Call with `hollowTriangleMarker = true` **only** for the own-ship vector.

**Reference implementation** (`tacticalsolutionview.cpp`, `drawCourseVectorFromEndpoints`):

```cpp
if (hollowTriangleMarker) {
    qreal triLen = 6;
    qreal triHalfWidth = 4;
    qreal angle = qAtan2(endPoint.y() - startPoint.y(), endPoint.x() - startPoint.x());
    qreal perpAngle = angle + M_PI_2;

    QPointF tip(startPoint.x() + triLen * qCos(angle),
                startPoint.y() + triLen * qSin(angle));
    QPointF base1(startPoint.x() + triHalfWidth * qCos(perpAngle),
                  startPoint.y() + triHalfWidth * qSin(perpAngle));
    QPointF base2(startPoint.x() - triHalfWidth * qCos(perpAngle),
                  startPoint.y() - triHalfWidth * qSin(perpAngle));

    QPolygonF triangle;
    triangle << tip << base1 << base2;
    pen.setWidth(2);
    scene->addPolygon(triangle, pen, Qt::NoBrush);
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

- Own ship: hollow cyan triangle at the centre of the green bearing line.
- Yellow and red filled circles on the **same** green line at distances proportional to range.
- Course arrows may leave the bearing line; only **bases** are constrained to it.

---

## 5. Wiring in a host system

Minimal integration pattern (mirrors sandbox):

```cpp
// Construct / obtain widget (e.g. from .ui or programmatically)
TacticalSolutionView *tsv = ...;

// On each tactical update:
tsv->setData(
    ownShipSpeed, ownShipBearing, sensorBearing,
    adoptedTrackRange, adoptedTrackSpeed, adoptedTrackBearing,
    selectedTrackRange, selectedTrackSpeed, selectedTrackBearing,
    adoptedTrackCourse, selectedTrackCourse);
```

No extra calls are required for the hollow triangle or bearing-line alignment — both are
internal to `draw()` / `getGuideBox()` once this library revision is linked.

---

## 6. File checklist for porting

When merging TSV updates into another tree, touch only:

| File | Changes |
|------|---------|
| `tacticalsolutionview.h` | `drawCourseVectorFromEndpoints(..., bool hollowTriangleMarker = false)` |
| `tacticalsolutionview.cpp` | `getGuideBox` base-point logic; hollow triangle in `drawCourseVectorFromEndpoints`; `drawVectorsFromPointStore` own-ship flag |

`DrawUtils::calculateEndpoint` and `DrawUtils::bearingToCartesian` are unchanged; reuse
existing `drawutils` from the library.
