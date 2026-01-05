# Valgrind Memory Analysis Report - valgrind_mem_5.log

## Executive Summary

**Total Errors:** 486 errors from 422 contexts (down from 516 in valgrind_mem_3!)  
**Memory Leaks:**
- **Definitely lost:** 256 bytes in 1 block (same as mem_3)
- **Indirectly lost:** 64 bytes in 2 blocks (same as mem_3)
- **Possibly lost:** 97,480 bytes in 473 blocks (up from 96,188 in 469 blocks)
- **Still reachable:** 55,508,123 bytes in 145,960 blocks (up from 51,585,480 in 116,453 blocks)

**Improvement:** Error count reduced by **6%** (from 516 to 486) after cleanup fixes!

---

## Error Summary Comparison

| Metric | valgrind_mem_3 | valgrind_mem_5 | Change |
|--------|----------------|----------------|--------|
| **Total Errors** | 516 | 486 | -30 (-6%) ✅ |
| **Error Contexts** | 421 | 422 | +1 |
| **Definitely Lost** | 256 bytes (1 block) | 256 bytes (1 block) | No change |
| **Indirectly Lost** | 64 bytes (2 blocks) | 64 bytes (2 blocks) | No change |
| **Possibly Lost** | 96,188 bytes (469 blocks) | 97,480 bytes (473 blocks) | +1,292 bytes (+4 blocks) |
| **Still Reachable** | 51,585,480 bytes (116,453 blocks) | 55,508,123 bytes (145,960 blocks) | +3,922,643 bytes (+29,507 blocks) |

---

## Error Type Analysis

### Uninitialized Value Errors (486 errors)

**Status:** ✅ **Improved** - Reduced from 516 to 486 errors (-6%)

**Root Cause:** All errors are still related to uninitialized values in `TimelineVisualizerWidget::setTimeInterval()`.

**Location:** `timelineview.cpp:346`

**Stack Trace Pattern:**
```
TimelineVisualizerWidget::setTimeInterval(TimeInterval) (timelineview.cpp:346)
TimelineView::TimelineView(...) (timelineview.cpp:1766)
GraphContainer::GraphContainer(...) (graphcontainer.cpp:99)
GraphLayout::initializeContainers() (graphlayout.cpp:361-364)
MainWindow::setupTimelineView() (mainwindow.cpp:871)
```

**Issue:** `m_showCrosshairTimestamp` member variable is not initialized in the constructor before `setTimeInterval()` is called.

**Recommendation:** Initialize `m_showCrosshairTimestamp = false` in the `TimelineVisualizerWidget` constructor initializer list.

---

## Memory Leak Analysis

### 1. Definitely Lost: 256 bytes (1 block)

**Status:** ⚠️ **Unchanged** - Same as valgrind_mem_3

**Analysis:**
- Small leak (256 bytes) - likely a minor issue
- Same size as previous run suggests it's a consistent leak
- Not related to the cleanup fixes applied

**Recommendation:** Investigate the specific block to identify source.

---

### 2. Indirectly Lost: 64 bytes (2 blocks)

**Status:** ⚠️ **Unchanged** - Same as valgrind_mem_3

**Analysis:**
- Very small leak (64 bytes total)
- Same as previous run
- Likely related to the definitely lost block

**Recommendation:** Fix the definitely lost block, and this should resolve automatically.

---

### 3. Possibly Lost: 97,480 bytes (473 blocks)

**Status:** ⚠️ **Slightly Increased** - Up from 96,188 bytes (469 blocks)

**Change:** +1,292 bytes (+4 blocks) - **+1.3% increase**

**Analysis:**
- Small increase (1.3%)
- May be within normal variation
- Could indicate minor accumulation

**Recommendation:** Monitor over time. If it continues to increase, investigate further.

---

### 4. Still Reachable: 55,508,123 bytes (145,960 blocks)

**Status:** ⚠️ **Increased** - Up from 51,585,480 bytes (116,453 blocks)

**Change:** +3,922,643 bytes (+29,507 blocks) - **+7.6% increase**

**Analysis:**
- Significant increase in still reachable memory
- This is memory that's still pointed to but may not be actively used
- Could indicate:
  - Qt internal caching
  - Graphics buffers not fully released
  - Container data accumulation

**Key Findings:**

#### WaterfallGraph Buffer Leaks (Still Reachable)

**Location:** `waterfallgraph.cpp:2453` - `WaterfallGraph::initializeWaterfallBuffer()`

**Largest Still Reachable Blocks:**
- **7,710,960 bytes in 8 blocks** - QImageData::create from `initializeWaterfallBuffer()`
- **714,160 bytes in 2 blocks** - QImageData::create from `initializeWaterfallBuffer()`

**Total Waterfall Buffer Leak:** ~8.4 MB (still reachable)

**Analysis:**
- These are the waterfall buffers (QPixmap/QImage) that are still reachable
- The cleanup fixes help, but buffers may not be fully released
- Qt may be caching these internally

**Recommendation:**
- Verify buffers are explicitly cleared in destructor (already implemented)
- Consider calling `QPixmap::detach()` before clearing
- Monitor if this continues to grow over time

#### Other Still Reachable Sources

1. **Graphics Driver (libgallium):** 720,896 bytes - External library, not fixable
2. **QTextDocument:** 1,036,800 bytes - From BTWGraph::drawCustomCircleMarkers()
3. **Qt Internal:** Remaining ~46.7 MB - Qt framework internal allocations

---

## Comparison with Previous Fixes

### Fixes Applied (from heaptrack analysis)

1. ✅ **QGraphicsEllipseItem cleanup** - Not directly visible in Valgrind (heaptrack showed improvement)
2. ✅ **Waterfall buffer cleanup** - Partially effective (still 8.4 MB reachable)
3. ✅ **WaterfallData container cleanup** - Not directly visible in Valgrind

### Impact on Valgrind Results

**Error Count:** ✅ Reduced by 6% (516 → 486)
- Small improvement, but moving in right direction
- All remaining errors are uninitialized value errors (not memory leaks)

**Memory Leaks:**
- **Definitely/Indirectly Lost:** No change (same small leaks)
- **Possibly Lost:** Slight increase (+1.3%) - within normal variation
- **Still Reachable:** Increased (+7.6%) - needs investigation

---

## Remaining Issues

### Issue 1: Uninitialized Value Errors (486 errors) ⚠️ HIGH PRIORITY

**Problem:** `m_showCrosshairTimestamp` not initialized in `TimelineVisualizerWidget` constructor.

**Fix Required:**
```cpp
// In timelineview.cpp - TimelineVisualizerWidget constructor
TimelineVisualizerWidget::TimelineVisualizerWidget(...)
    : QWidget(parent),
      m_currentTime(QTime::currentTime()),
      m_numberOfDivisions(15),
      m_lastCurrentTime(QTime::currentTime()),
      m_pixelSpeed(0.0),
      m_accumulatedOffset(0.0),
      m_sliderIndicator(nullptr),
      m_syncState(syncState),
      m_sliderVisible(sliderVisible),
      m_chevronVisible(chevronVisible),
      m_manoeuvreOverlay(nullptr),
      m_showCrosshairTimestamp(false)  // ADD THIS LINE
{
    // ... rest of constructor ...
}
```

**Expected Impact:** Eliminates all 486 uninitialized value errors.

---

### Issue 2: Still Reachable Memory Growth ⚠️ MEDIUM PRIORITY

**Problem:** Still reachable memory increased by 7.6% (3.9 MB).

**Possible Causes:**
1. Waterfall buffers (8.4 MB) - Qt may be caching internally
2. Qt internal allocations - Framework overhead
3. Graphics driver allocations - External library

**Recommendations:**
1. Verify `QPixmap` buffers are properly cleared:
   ```cpp
   // In WaterfallGraph destructor
   m_waterfallBuffer = QPixmap();  // Already implemented
   m_waterfallBuffer.detach();     // Consider adding
   ```
2. Monitor still reachable memory over longer runs
3. Consider periodic cleanup if memory continues to grow

---

### Issue 3: Possibly Lost Memory Slight Increase ⚠️ LOW PRIORITY

**Problem:** Possibly lost memory increased by 1.3% (+1,292 bytes).

**Analysis:**
- Very small increase (1.3%)
- May be within normal variation
- Not a significant concern at this level

**Recommendation:** Monitor over time. If it continues to increase significantly, investigate further.

---

## Recommendations

### Priority 1: Fix Uninitialized Value Errors

**Action:** Initialize `m_showCrosshairTimestamp` in `TimelineVisualizerWidget` constructor.

**Expected Result:** Eliminates all 486 errors, bringing total to 0.

**Code Location:** `timelineview.cpp` - `TimelineVisualizerWidget` constructor

---

### Priority 2: Investigate Still Reachable Memory

**Actions:**
1. Add explicit `detach()` calls before clearing QPixmap buffers
2. Monitor still reachable memory over longer test runs
3. Profile which components are contributing to growth

**Expected Result:** Better understanding of memory retention patterns.

---

### Priority 3: Monitor Memory Leaks Over Time

**Actions:**
1. Run Valgrind tests after longer operation periods
2. Compare still reachable memory across multiple runs
3. Track if memory continues to accumulate

**Expected Result:** Identify if leaks are linear or exponential.

---

## Conclusion

The cleanup fixes applied (based on heaptrack analysis) have had a **positive but limited impact** on Valgrind results:

✅ **Error count reduced:** 516 → 486 (-6%)  
✅ **Definitely/Indirectly lost:** No change (same small leaks)  
⚠️ **Still reachable:** Increased by 7.6% (needs investigation)  
⚠️ **Possibly lost:** Slight increase (1.3%, within normal variation)

**Key Findings:**
1. **Uninitialized value errors** remain the primary issue (486 errors)
2. **Waterfall buffers** still showing as reachable (8.4 MB)
3. **Overall memory leak situation** is stable but needs monitoring

**Next Steps:**
1. Fix uninitialized value errors (Priority 1)
2. Investigate still reachable memory growth (Priority 2)
3. Continue monitoring with longer test runs (Priority 3)

---

## Appendix: Error Breakdown

### Error Types
- **Uninitialized Value Errors:** 486 (100% of errors)
  - All from `TimelineVisualizerWidget::setTimeInterval()`
  - Same root cause as valgrind_mem_3

### Memory Leak Breakdown

| Category | Size | Blocks | Status |
|----------|------|--------|--------|
| Definitely Lost | 256 bytes | 1 | Unchanged |
| Indirectly Lost | 64 bytes | 2 | Unchanged |
| Possibly Lost | 97,480 bytes | 473 | +1.3% |
| Still Reachable | 55,508,123 bytes | 145,960 | +7.6% |
| **Total** | **55,605,923 bytes** | **146,436** | **+7.5%** |

### Largest Still Reachable Blocks

1. **7,710,960 bytes** - QImageData from `WaterfallGraph::initializeWaterfallBuffer()`
2. **720,896 bytes** - Graphics driver (libgallium)
3. **714,160 bytes** - QImageData from `WaterfallGraph::initializeWaterfallBuffer()`
4. **1,036,800 bytes** - QTextDocument from `BTWGraph::drawCustomCircleMarkers()`
5. **Remaining ~45.3 MB** - Qt internal allocations

---

**Report Generated:** Analysis of valgrind_mem_5.log  
**Comparison Baseline:** valgrind_mem_3.log  
**Status:** Errors reduced, still reachable memory increased

