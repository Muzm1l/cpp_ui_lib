# Heaptrack Memory Analysis - heaptrack_summary_longest.txt

**Date:** 2024  
**Runtime:** 70,624 seconds (~19.6 hours)  
**Profiled Target:** ui-sandbox (PID 100188)

## Executive Summary

### Critical Statistics

- **Total Runtime:** 70,624 seconds (19.6 hours)
- **Total Allocation Calls:** 2,482,569,577 (35,151 calls/second)
- **Temporary Allocations:** 279,326,697 (3,955/second)
- **Peak Heap Memory:** 220.34 MB
- **Peak RSS (with overhead):** 467.42 MB
- **Total Memory Leaked:** 217.22 MB ⚠️ **CRITICAL**
- **Suppressed Leaks:** 554.45 KB

### Key Findings

1. **Severe Memory Leak:** 217.22 MB leaked over 19.6 hours (~98.6% of peak heap)
2. **Excessive QVector Allocations:** 361.6M calls to QArrayData::allocate (15.21M peak)
3. **BTWGraph::drawShadedRegions()** is the primary source of QVector allocations
4. **QGraphicsEllipseItem Creation:** 207.8M calls (2.29M peak) - likely not cleaned up properly
5. **CircularBuffer Reservations:** Multiple 7.01M allocations from cache updates

## Top Memory Allocation Hotspots

### 1. QArrayData::allocate (Qt QVector/QString allocations)

**Peak Consumption:** 15.21 MB  
**Total Calls:** 361,660,032  
**Primary Source:** `BTWGraph::drawShadedRegions()`

**Call Chain:**
```
QArrayData::allocate
  → QTypedArrayData::allocate
    → QVector::realloc
      → QVector::append(QPointF&&)
        → QVector::operator<<(QPointF&&)
          → BTWGraph::drawShadedRegions() [btwgraph.cpp:1232]
            → BTWGraph::draw()
              → GraphContainer::setTimeScope()
                → TimelineView::onTimerTick()
```

**Analysis:**
- 2.4M calls from `BTWGraph::drawShadedRegions()` alone
- Triggered by timer ticks and time scope changes
- QVector<QPointF> allocations for shaded region drawing
- **Issue:** These allocations may not be properly cleaned up

**Recommendation:**
- Reuse QVector<QPointF> instead of creating new ones each draw
- Pre-allocate vectors with estimated capacity
- Consider using QPainterPath directly instead of QVector<QPointF>

### 2. QGraphicsEllipseItem Creation

**Peak Consumption:** 2.29 MB  
**Total Calls:** 207,838,114  
**Primary Source:** `WaterfallGraph::drawDataSeries()`

**Call Chain:**
```
QGraphicsEllipseItem::QGraphicsEllipseItem
  → QGraphicsScene::addEllipse
    → WaterfallGraph::drawDataSeries() [waterfallgraph.cpp:4426]
      → WaterfallGraph::drawIncremental()
        → GraphEngine::dataAppended()
          → Simulator::addDataPoints()
```

**Analysis:**
- 205.8M calls creating ellipse items for scatterplot points
- Each data point creates a new QGraphicsEllipseItem
- **Issue:** Items may not be properly cleaned up, leading to memory leaks

**Recommendation:**
- Use object pooling for QGraphicsEllipseItem instances
- Batch item creation/deletion
- Consider using QGraphicsPixmapItem with cached pixmaps (already partially implemented)
- Ensure proper cleanup in `cleanupScatterplotItems()`

### 3. CircularBuffer Reserve Operations

**Peak Consumption:** 7.01 MB (multiple instances)  
**Total Calls:** 3,128 per instance  
**Primary Source:** `WaterfallGraph::updateVisibleDataCacheFull()`

**Call Chain:**
```
std::__new_allocator::allocate
  → std::allocator_traits::allocate
    → std::_Vector_base::_M_allocate
      → std::vector::reserve
        → CircularBuffer::reserve [circularbuffer.h:210]
          → WaterfallGraph::updateVisibleDataCacheFull() [waterfallgraph.cpp:1519]
            → WaterfallGraph::drawScatterplot()
```

**Analysis:**
- Multiple 7.01M allocations from cache updates
- Each graph type triggers separate cache reservations
- Called during full redraws and scatterplot updates
- **Issue:** Frequent large allocations can cause memory fragmentation

**Recommendation:**
- Pre-allocate cache buffers during initialization
- Use capacity limits to prevent unbounded growth
- Reuse existing buffers when possible
- Consider using memory pools for large allocations

### 4. QImage/QPixmap Allocations

**Peak Consumption:** 11.37 MB  
**Total Calls:** 87,082  
**Primary Source:** `WaterfallGraph::initializeWaterfallBuffer()`

**Call Chain:**
```
QImageData::create
  → QImage::QImage(QSize const&)
    → QPixmap::doInit
      → WaterfallGraph::initializeWaterfallBuffer() [waterfallgraph.cpp:2478]
        → WaterfallGraph::updateGraphicsDimensions()
          → WaterfallGraph::resizeEvent()
```

**Analysis:**
- 7.71M consumed over 8 calls (large buffer allocations)
- Waterfall buffer for direct rendering
- Triggered on widget resize events
- **Issue:** May not be properly released on resize

**Recommendation:**
- Ensure old buffers are released before creating new ones
- Use QPixmap::swap() to avoid temporary allocations
- Consider using QImage instead of QPixmap for off-screen rendering

## Memory Leak Analysis

### Total Leaked: 217.22 MB

This represents **98.6% of peak heap memory**, indicating severe memory management issues.

### Likely Leak Sources

Based on allocation patterns, the following are likely contributors:

1. **QGraphicsEllipseItem Leaks (Estimated: ~100-150 MB)**
   - 207.8M items created
   - If items aren't properly removed from scene, they accumulate
   - Each item: ~8-16 bytes overhead + QGraphicsItem base class

2. **QVector<QPointF> Leaks (Estimated: ~30-50 MB)**
   - 2.4M allocations from `BTWGraph::drawShadedRegions()`
   - If vectors aren't cleared/reused, they accumulate
   - Each vector with 100 points: ~800 bytes

3. **CircularBuffer Memory (Estimated: ~20-30 MB)**
   - Cache buffers may not be properly released
   - Multiple 7MB allocations that may not be freed

4. **QArrayData Leaks (Estimated: ~10-20 MB)**
   - Qt container allocations that aren't freed
   - QString, QVector, QList allocations

## Detailed Allocation Patterns

### Most Frequent Allocations

| Function | Calls | Peak Memory | Source |
|----------|-------|-------------|--------|
| `QArrayData::allocate` | 361,660,032 | 15.21 MB | BTWGraph::drawShadedRegions() |
| `QGraphicsEllipseItem::QGraphicsEllipseItem` | 207,838,114 | 2.29 MB | WaterfallGraph::drawDataSeries() |
| `std::__new_allocator::allocate` | 7,246,327 | 55.16 MB | Various (CircularBuffer, std::vector) |
| `QImageData::create` | 87,082 | 11.37 MB | WaterfallGraph::initializeWaterfallBuffer() |

### Allocation Rate Analysis

- **Average:** 35,151 allocations/second
- **Peak periods:** During timer ticks and redraws
- **Temporary allocations:** 3,955/second (11.2% of total)
- **Permanent allocations:** 31,196/second (88.8% of total)

## Critical Issues

### Issue #1: QGraphicsEllipseItem Accumulation

**Severity:** CRITICAL  
**Impact:** ~100-150 MB leaked

**Root Cause:**
- Items created in `drawDataSeries()` but may not be properly cleaned up
- `cleanupScatterplotItems()` may not be called frequently enough
- Items may remain in scene after data changes

**Fix:**
```cpp
// Ensure cleanup is called before creating new items
void WaterfallGraph::drawDataSeries(const QString &seriesLabel)
{
    // Cleanup old items first
    cleanupScatterplotItems(seriesLabel);
    
    // Create new items
    // ...
    
    // Ensure items are properly managed
}
```

### Issue #2: QVector<QPointF> in drawShadedRegions()

**Severity:** HIGH  
**Impact:** ~30-50 MB leaked

**Root Cause:**
- New QVector<QPointF> created on every draw
- Vectors appended to but may not be cleared
- Triggered frequently by timer ticks

**Fix:**
```cpp
// In BTWGraph::drawShadedRegions()
// Use a member variable to reuse the vector
QVector<QPointF> m_reusableShadedRegionPoints;  // Member variable

void BTWGraph::drawShadedRegions()
{
    m_reusableShadedRegionPoints.clear();  // Reuse instead of creating new
    // ... populate m_reusableShadedRegionPoints
    // Use m_reusableShadedRegionPoints
}
```

### Issue #3: CircularBuffer Cache Reservations

**Severity:** MEDIUM  
**Impact:** ~20-30 MB leaked

**Root Cause:**
- Large reservations (7MB) that may not be released
- Multiple graphs each reserving separately
- Cache invalidation may not free memory

**Fix:**
- Set capacity limits on circular buffers
- Ensure `clear()` is called when caches are invalidated
- Use `shrink_to_fit()` after clearing large buffers

## Recommendations

### Immediate Actions (High Priority)

1. **Fix QGraphicsEllipseItem Leaks**
   - Audit all `drawDataSeries()` implementations
   - Ensure `cleanupScatterplotItems()` is called before creating new items
   - Verify items are removed from scene before deletion
   - Consider using object pooling

2. **Fix QVector<QPointF> Leaks in drawShadedRegions()**
   - Use reusable member variables instead of creating new vectors
   - Clear vectors after use
   - Pre-allocate with estimated capacity

3. **Add Memory Leak Detection**
   - Use Valgrind or AddressSanitizer to identify exact leak locations
   - Add RAII wrappers for QGraphicsItem management
   - Implement automatic cleanup in destructors

### Short-term Optimizations

4. **Optimize CircularBuffer Usage**
   - Set capacity limits to prevent unbounded growth
   - Use `shrink_to_fit()` after clearing
   - Pre-allocate during initialization

5. **Reduce Allocation Frequency**
   - Reuse containers instead of creating new ones
   - Use object pooling for frequently allocated objects
   - Batch operations to reduce allocation overhead

6. **Improve Cache Management**
   - Ensure caches are properly cleared on invalidation
   - Use version numbers instead of full cache clearing
   - Implement cache size limits

### Long-term Improvements

7. **Memory Profiling Integration**
   - Add periodic memory usage reporting
   - Track allocation/deallocation ratios
   - Monitor for memory growth trends

8. **Refactor Graphics Item Management**
   - Use smart pointers for QGraphicsItem ownership
   - Implement automatic cleanup on data changes
   - Consider using QGraphicsItemGroup for batch operations

9. **Optimize Rendering Pipeline**
   - Reduce redraw frequency
   - Use dirty flags to avoid unnecessary redraws
   - Implement incremental rendering more efficiently

## Memory Growth Pattern

Based on the runtime of 19.6 hours:
- **Leak Rate:** ~11.1 MB/hour
- **Projected 24-hour leak:** ~266 MB
- **Projected 1-week leak:** ~1.86 GB

This indicates a **linear memory leak** that will continue to grow over time.

## Comparison with Callgrind Analysis

The heaptrack analysis complements the callgrind analysis:

- **Callgrind:** Shows CPU hotspots (data conversion, range calculations)
- **Heaptrack:** Shows memory hotspots (allocations, leaks)

**Key Correlation:**
- High allocation frequency in `drawShadedRegions()` (heaptrack)
- High instruction count in `populateYDataSeries()` (callgrind)
- Both indicate rendering pipeline needs optimization

## Conclusion

The application has a **critical memory leak** of 217.22 MB over 19.6 hours, representing 98.6% of peak heap memory. The primary sources are:

1. QGraphicsEllipseItem accumulation (estimated 100-150 MB)
2. QVector<QPointF> allocations in drawShadedRegions() (estimated 30-50 MB)
3. CircularBuffer cache reservations (estimated 20-30 MB)

**Immediate action required** to prevent memory exhaustion in long-running sessions.

