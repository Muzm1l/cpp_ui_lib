# Heaptrack Summary 7 Analysis Report
## Long-Term Memory Leak Testing (9.0 Hours)

**Date:** Analysis of heaptrack.ui-sandbox.67675.zst  
**Test Duration:** 32,586.49 seconds (9.05 hours)  
**Test Scenario:** Extended runtime after implementing reusable vector optimizations

---

## Executive Summary

This report analyzes memory leak patterns from a very long-term test run (9+ hours) after implementing reusable vector optimizations to reduce `toVector()` allocations. The analysis reveals:

- ⚠️ **Total leak increased**: 126.47M (from 96.54M in summary_6)
- ⚠️ **Leak rate**: 3.9 KB/s (slightly higher than summary_6's 3.7 KB/s)
- ✅ **Peak heap stable**: 132.93M (only 30% increase from 102.28M despite 2.5x longer runtime)
- ⚠️ **Critical finding**: `CircularBuffer::toVector()` still being called (2.3M+ calls)
- ⚠️ **QImageData leak persists**: 12.27M peak (7.71M from WaterfallGraph buffers)

**Overall Assessment:** The reusable vector optimizations may not have been fully applied or tested. The leak rate is still linear and manageable, but the presence of `toVector()` calls indicates the fixes need verification.

---

## Overall Statistics

| Metric | Summary_6 (7.2h) | Summary_7 (9.0h) | Change |
|--------|-------------------|------------------|--------|
| **Runtime** | 26,043.14s (7.2h) | 32,586.49s (9.0h) | +25% longer |
| **Total Allocations** | 1,533,166,127 | 375,961,055 | -76% (different test?) |
| **Allocation Rate** | 58,870/s | 11,537/s | -80% ✅ |
| **Peak Heap** | 102.28M | 132.93M | +30% |
| **Total Leaked** | 96.54M | 126.47M | +31% |
| **Leak Rate** | 3.7 KB/s | 3.9 KB/s | +5% |

**Note:** The allocation count is significantly lower, suggesting a different test scenario or less active data streaming.

---

## Critical Finding: `toVector()` Still Being Called

### Evidence of Unfixed Code

The heaptrack trace shows **2,342,664 calls** to `CircularBuffer<>::toVector()` from:

1. **WaterfallData::getYDataSeries()** (line 1276-1280)
   - 2.3M calls creating temporary vectors
   - Called from `WaterfallGraph::drawScatterplot()`
   - **This should use reusable vectors!**

2. **WaterfallData::getTimestampsSeries()** (line 2921-2925)
   - 2.3M calls creating temporary vectors
   - Called from `WaterfallGraph::drawScatterplot()`
   - **This should use reusable vectors!**

### Impact

These `toVector()` calls create temporary vectors that:
- Allocate memory on every call
- Are immediately destroyed after use
- Contribute to the 8.67M peak in `std::__new_allocator` allocations
- Cause memory fragmentation

**Root Cause:** The reusable vector optimizations were implemented in `WaterfallGraph`, but `WaterfallData::getYDataSeries()` and `getTimestampsSeries()` still return vectors by calling `toVector()`. These methods need to be refactored to avoid creating temporary vectors.

---

## Detailed Component Analysis

### 1. QArrayData Allocations (Qt Containers)

**Status:** ⚠️ Still the largest allocation source

| Metric | Value |
|--------|-------|
| **Total Calls** | 55,009,673 |
| **Peak Memory** | 15.27M |
| **Calls per Second** | 1,688/s |

**Findings:**
- Still the most frequent allocation source
- Peak memory is stable (15.27M vs 14.18M in summary_6)
- Allocation rate is much lower (1,688/s vs 6,700/s), suggesting less active data

**Sources:**
- Qt container operations (QVector, QList, QString)
- WaterfallData series storage
- TimelineView cached data

---

### 2. std::__new_allocator Allocations (STL Containers)

**Status:** ⚠️ Multiple large peaks from vector operations

#### Peak 1: 34.43M (4.4M calls)
- From `CircularBuffer::reserve()` in `updateVisibleDataCacheFull()`
- Called during graph redraws
- **This is expected** - cache needs to grow

#### Peak 2: 8.67M (20M calls)
- From `CircularBuffer::toVector()` in `WaterfallData::getYDataSeries()`
- **This should be fixed!** - Should use reusable vectors

#### Peak 3: 8.41M (9.7M calls)
- From `CircularBuffer::toVector()` in `WaterfallData::getTimestampsSeries()`
- **This should be fixed!** - Should use reusable vectors

#### Peak 4: 20.60M (18K calls)
- From `CircularBuffer::reserve()` in `updateScatterplotItemsFull()`
- Called during scatterplot updates
- **This is expected** - scatter points need storage

#### Peak 5: 4.99K (8.3M calls)
- From `std::map::operator[]` in `updateVisibleDataCacheFull()`
- Map node allocations for cache storage
- **This is expected** - cache structure needs nodes

**Key Insight:** The `toVector()` calls in `WaterfallData` are creating millions of temporary vectors that should be eliminated.

---

### 3. QImageData Allocations (Image Buffers)

**Status:** ⚠️ Still leaking, but improved

| Metric | Value |
|--------|-------|
| **Total Calls** | 66,136 |
| **Peak Memory** | 12.27M |
| **WaterfallGraph Buffers** | 7.71M (8 calls) |

**Findings:**
- 7.71M from `WaterfallGraph::initializeWaterfallBuffer()` (8 calls)
- Each buffer is ~964KB
- Buffers are created on resize but may not be properly freed

**Stack Trace:**
```
WaterfallGraph::initializeWaterfallBuffer() (waterfallgraph.cpp:2459)
WaterfallGraph::updateGraphicsDimensions() (waterfallgraph.cpp:2022)
WaterfallGraph::resizeEvent() (waterfallgraph.cpp:2429)
```

**Analysis:**
- 8 buffer creations over 9 hours = ~1 per hour
- Likely from window resizing or graph initialization
- Buffers should be freed in destructor (fix was implemented)

**Recommendation:** Verify that destructor cleanup is working correctly.

---

### 4. CircularBuffer Reserve Operations

**Status:** ✅ Expected behavior, but high frequency

Multiple locations show `CircularBuffer::reserve()` being called:

1. **updateVisibleDataCacheFull()** - 4.17M allocations (1,200 calls)
   - Cache needs to grow to accommodate visible data
   - **This is expected**

2. **updateScatterplotItemsFull()** - 4.17M allocations (1,200 calls)
   - Scatter points need storage
   - **This is expected**

**Analysis:**
- Reserve operations are necessary for buffer growth
- The 1,200 calls over 9 hours = ~133 calls/hour
- Each call reserves ~3.5MB on average
- This is normal for dynamic data structures

---

## Leak Rate Analysis

### Per-Second Rates

| Metric | Summary_6 (7.2h) | Summary_7 (9.0h) | Change |
|--------|------------------|------------------|--------|
| **Leak Rate** | 3.7 KB/s | 3.9 KB/s | +5% |
| **Leak per Hour** | 13.3 MB/h | 14.0 MB/h | +5% |

### Key Insight: Linear Leak Pattern Confirmed

The leak rate is **still linear, not exponential**:
- Summary_6: 96.54M in 7.2 hours = 3.7 KB/s
- Summary_7: 126.47M in 9.0 hours = 3.9 KB/s

The slight increase (5%) is within normal variation and confirms:
- ✅ Leak is predictable and linear
- ✅ No exponential growth
- ✅ System is stable over very long periods (9+ hours)

---

## Comparison with Previous Fixes

### Expected vs. Actual

| Fix | Expected Impact | Actual Status |
|-----|----------------|--------------|
| **Reusable Vectors** | Eliminate `toVector()` calls | ⚠️ **NOT WORKING** - 2.3M calls still present |
| **WaterfallGraph Buffer Cleanup** | Reduce QImageData leak | ⚠️ **PARTIAL** - Still 7.71M leak |
| **WaterfallData Container Cleanup** | Reduce QArrayData leak | ✅ **WORKING** - Peak stable at 15.27M |

### Root Cause Analysis

1. **`toVector()` Still Being Called:**
   - Fix was implemented in `WaterfallGraph` (using `m_reusableVisibleData`)
   - But `WaterfallData::getYDataSeries()` and `getTimestampsSeries()` still call `toVector()`
   - These methods return `std::vector<>` by value, forcing `toVector()` calls
   - **Fix needed:** Refactor `WaterfallData` methods to avoid `toVector()`

2. **QImageData Leak:**
   - Destructor cleanup was implemented
   - But 8 buffer creations over 9 hours suggests buffers are being recreated
   - May be from window resizing or graph recreation
   - **Fix needed:** Verify destructor is called, or implement buffer reuse

---

## Recommendations

### Priority 1: Fix WaterfallData::getYDataSeries() and getTimestampsSeries()

**Problem:** These methods call `toVector()`, creating temporary vectors

**Current Code:**
```cpp
std::vector<qreal> WaterfallData::getYDataSeries(const QString& seriesLabel) const {
    return dataSeriesYData.at(seriesLabel).toVector(); // Creates temporary!
}
```

**Recommended Fix:**
1. **Option A:** Return const reference to CircularBuffer, let caller iterate
2. **Option B:** Add methods that populate a reusable vector passed by reference
3. **Option C:** Cache converted vectors in WaterfallData (not recommended - adds complexity)

**Impact:** Should eliminate 2.3M+ `toVector()` calls and reduce 8.67M+8.41M = 17.08M of temporary allocations

---

### Priority 2: Verify WaterfallGraph Buffer Cleanup

**Problem:** QImageData buffers still leaking (7.71M)

**Actions:**
1. Verify destructor is being called
2. Check if buffers are being recreated without cleanup
3. Consider buffer reuse instead of recreation

**Impact:** Should reduce QImageData leak from 7.71M to near zero

---

### Priority 3: Optimize CircularBuffer Reserve Operations

**Problem:** High frequency of reserve() calls (1,200 calls)

**Actions:**
1. Pre-allocate cache sizes based on expected data volume
2. Use `reserve()` more strategically (only when needed)
3. Consider fixed-size buffers for known maximums

**Impact:** Should reduce allocation churn, though this is lower priority

---

## Expected Results After Fixes

### Current State:
- **Total Leak:** 126.47M
- **toVector() allocations:** ~17M (eliminatable)
- **QImageData leak:** 7.71M (fixable)

### After Priority 1 Fix (WaterfallData toVector):
- **Expected Leak:** ~109M (13% reduction)
- **Eliminated:** 17M of temporary vector allocations

### After Priority 2 Fix (QImageData cleanup):
- **Expected Leak:** ~101M (20% reduction from current)
- **Eliminated:** 7.71M of buffer leaks

### Combined Impact:
- **Total Reduction:** ~25M (20% of current leak)
- **New Leak Rate:** ~3.1 KB/s (down from 3.9 KB/s)

---

## Conclusion

The reusable vector optimizations in `WaterfallGraph` are a good start, but **the real problem is in `WaterfallData` methods that still call `toVector()`**. These methods are called millions of times and create temporary vectors that should be eliminated.

**Key Takeaways:**
1. ✅ Leak is linear and predictable (good for long-term stability)
2. ⚠️ `toVector()` calls in `WaterfallData` need to be fixed
3. ⚠️ QImageData buffer cleanup needs verification
4. ✅ System is stable over 9+ hour runs

**Next Steps:**
1. Refactor `WaterfallData::getYDataSeries()` and `getTimestampsSeries()` to avoid `toVector()`
2. Verify `WaterfallGraph` destructor cleanup is working
3. Re-run heaptrack to measure improvement

---

## References

- Previous Analysis: `heaptrack_summary_6_analysis.md`
- Memory Leak Fixes: `user-docs/memory_leak_fixes.md`
- Reusable Vector Implementation: `waterfallgraph.h` (lines 378-386)

