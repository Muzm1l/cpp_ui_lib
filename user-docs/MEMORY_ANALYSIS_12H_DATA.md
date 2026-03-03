# Memory Consumption Analysis: 12-Hour Data

**Purpose:** Estimate system memory usage when the graph/UI system holds 12 hours of time-series data (TimelineView window).

**Reference:** 12 hours = 720 minutes (`TimeInterval::TwelveHours`, `SliderGeometry::TWELVE_HOURS_IN_MINUTES`).

---

## 1. Data scale for 12 hours

| Sampling rate   | Points in 12 h | Typical use      |
|-----------------|----------------|------------------|
| 1 per minute    | 720            | Low-rate logs    |
| 1 per 10 s      | 4,320          | Moderate         |
| 1 per second    | 43,200         | High-rate (1 Hz)  |
| 10 per second   | 432,000        | Very high rate   |

Recommended capacity range from [circular-buffer-guide.md](circular-buffer-guide.md): **10,000–50,000** for data series (aligns with ~1 sample/sec for 12 h).

---

## 2. Structures that scale with 12-hour data

### 2.1 Data series (WaterfallData)

**Location:** `WaterfallData`: `dataSeriesYData`, `dataSeriesTimestamps`, `dataSeriesTimestampsEpoch` (per series).

**Per-point size:**

| Field    | Type       | Size (approx.) |
|----------|------------|-----------------|
| Y value  | `float`    | 4 B             |
| Timestamp| `QDateTime`| ~24 B           |
| Epoch    | `qint64`   | 8 B             |
| **Total**|            | **~36 B/point** |

**Series count (from MainWindow):**

| Graph type | Series labels                          | Series count |
|------------|----------------------------------------|--------------|
| BDW        | BDW-1, ADOPTED                         | 2            |
| BRW        | BRW-1, BRW-2, ADOPTED                  | 3            |
| BTW        | BTW-1, BTW-2, BTW-3, ADOPTED           | 4            |
| FDW        | FDW-1, FDW-2, ADOPTED                  | 2            |
| FTW        | FTW-1, FTW-2, ADOPTED                  | 2            |
| LTW        | LTW-1, ADOPTED                         | 2            |
| RTW        | RTW-1, ADOPTED                         | 2            |
| **Total**  |                                        | **17**       |

**Data sources:** One `WaterfallData` per `GraphType` (7 types), so 7 data sources, 17 series in total.

**Memory (data series only) for capacity N points per series:**

- Per series: `N × 36` bytes.
- All 17 series: **17 × N × 36 bytes**.

| Capacity N (points) | Per series | All 17 series |
|---------------------|------------|----------------|
| 720 (1/min)         | ~25 KB     | **~0.4 MB**    |
| 4,320 (1/10 s)      | ~152 KB    | **~2.6 MB**     |
| 43,200 (1/s)        | ~1.52 MB   | **~25.9 MB**    |
| 50,000 (recomm. max)| ~1.76 MB   | **~30 MB**      |

---

### 2.2 Symbols and markers (WaterfallData)

Bounded by capacity; not proportional to “12 hours” of samples, but to number of symbols/markers over that window.

**Approximate sizes:**

- `RTWSymbolData`: QString + QDateTime + float → order ~100–200 B per entry.
- `BTWSymbolData`: QString + QDateTime + qint64 + float + bool → order ~120–220 B.
- `BTWMarkerData`: QDateTime + 2× float → ~40 B.
- `RTWRMarkerData`: QDateTime + float → ~32 B.

Recommended capacities (from guide): symbols 1,000–5,000, markers 500–2,000.

**Rough total:** ~0.5–2 MB for symbols and markers at recommended caps (all 7 data sources).

---

### 2.3 Rendering caches (WaterfallGraph)

**Location:** `WaterfallGraph`: `m_scatterPoints`, `m_batchedLinePaths`, `m_cachedVisibleData` (per series).

**Per-point (approx.):**

- Scatter: `QPointF` → 16 B.
- Cached visible: `std::pair<float, qint64>` → 12 B.
- Line paths: `QPainterPath` → variable (often dominant for many segments).

There is one `WaterfallGraph` per graph type (7). Each graph has one cache set per series it displays (2–4 series depending on type). So total cache memory scales with **visible data size** (e.g. 12-hour window) and number of series.

**Example (43,200 points, recommended cache caps):**

- Scatter: 5,000–20,000 pts × 16 B → 80–320 KB per series.
- Cached visible: 10,000–50,000 × 12 B → 120–600 KB per series.
- Line paths: batch count × path size (variable).

**Order of magnitude:** ~1–5 MB per graph type for scatter + cached visible at recommended capacities; line paths can add more.

---

### 2.4 TimelineView / slider

- Background cache: one `QPixmap(rect().size())` (widget size).
- No storage of 12-hour *data*; only current range and geometry. Memory is small (e.g. hundreds of KB to low MB) relative to data buffers.

---

## 3. Total memory estimate (12-hour data, 1 sample/sec)

Using **43,200 points per series** and recommended caps for symbols, markers, and caches:

| Component              | Estimate      |
|------------------------|---------------|
| Data series (17 × 43.2k × 36 B) | **~26 MB**   |
| Symbols + markers (all sources) | **~1–2 MB**  |
| Rendering caches (7 graphs)     | **~5–15 MB** |
| TimelineView / pixmaps          | **~1–2 MB**  |
| **Subtotal (bounded storage)**  | **~33–45 MB**|

**Note:** This assumes **capacity limits are set** (e.g. via `GraphLayout::setDataSeriesCapacity`, `setRenderingCachesCapacity`, etc.). In the current codebase, **MainWindow does not call these**; buffers default to **unlimited (0)**. With unlimited capacity, 12 hours of high-rate data can grow far beyond the above (e.g. hundreds of MB if many series and no cap).

---

## 4. Heaptrack findings (long runs) and 12-hour impact

From [heaptrack_analysis_longest.md](heaptrack_analysis_longest.md):

- **Peak heap:** ~220 MB over ~19.6 h; **~217 MB reported as leaked**.
- Main contributors: `QGraphicsEllipseItem` creation, `QVector<QPointF>` in `BTWGraph::drawShadedRegions()`, CircularBuffer reserve (e.g. 7 MB), QImage/QPixmap.

So over a 12-hour (or longer) run:

1. **Bounded 12-hour data** (with capacities set): data + caches should stay in the **~33–45 MB** range above.
2. **Unbounded data**: data series and caches can grow with time; 12-hour at 1 Hz already implies **~26 MB** for series alone; longer runs and higher rates increase this.
3. **Leaks** (ellipse items, QVector<QPointF>, cache reserves, pixmaps) are **additive over time** and not strictly “12-hour data”; they can dominate total process memory in long sessions.

---

## 5. Recommendations

1. **Set capacities for 12-hour operation**  
   Call early (e.g. after creating `GraphLayout`):
   - `setDataSeriesCapacity(43200)` (or 720 / 4320 for lower rates).
   - `setSymbolsCapacity(1000)`, `setMarkersCapacity(500)` (or per your needs).
   - `setRenderingCachesCapacity(5000, 1000, 10000)` (or aligned to data series capacity).

   Then **12-hour data** stays within the **~33–45 MB** bounded estimate above.

2. **Avoid unlimited buffers**  
   With capacity 0, 12-hour high-rate data plus leaks can push the process into hundreds of MB.

3. **Fix known leaks**  
   Address `BTWGraph::drawShadedRegions()` (reuse/preallocate `QVector<QPointF>`), ellipse item lifecycle, and cache reserve/release so long runs (including 12-hour views) do not grow without bound.

4. **Tune by sampling rate**  
   For 1 sample/minute over 12 h use 720; for 1/s use 43,200; scale rendering cache capacities accordingly (see [circular-buffer-guide.md](circular-buffer-guide.md)).

---

## 6. Summary table (12-hour data, capacity set)

| Scenario              | Points/series | Data series (17) | Total (approx.) |
|-----------------------|---------------|------------------|------------------|
| 1 sample/min           | 720           | ~0.4 MB          | **~2–5 MB**      |
| 1 sample/10 s          | 4,320         | ~2.6 MB          | **~8–12 MB**     |
| 1 sample/s (1 Hz)      | 43,200        | ~26 MB           | **~33–45 MB**    |
| 10 samples/s (unusual) | 432,000       | ~260 MB          | **~270–290 MB**  |

“Total” includes data series + symbols/markers + rendering caches + small UI caches. Actual process size will also include Qt/widget overhead and any leaks from the heaptrack report.
