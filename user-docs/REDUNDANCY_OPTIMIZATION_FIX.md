# Redundancy and Duplicate Reference Optimization Fix

**Date:** 2025  
**Issue:** Multiple redundancies and duplicate references causing unnecessary overhead

---

## Problems Identified and Fixed

### 1. **`drawDataSeries()` Using Unoptimized Methods** ✅ FIXED

**Location:** `waterfallgraph.cpp:4329-4341`

**Problem:**
- Called `populateYDataSeries()` (qreal version) - unoptimized
- Called `populateTimestampsSeries()` (QDateTime version) - unoptimized
- Data was immediately discarded and cache was used instead
- Same issue as `drawScatterplot()` that was already fixed

**Fix:**
- Removed unnecessary `populateYDataSeries()` and `populateTimestampsSeries()` calls
- Now uses cached visible data directly (same pattern as `drawScatterplot()`)

**Impact:**
- Eliminates additional unnecessary populate calls
- Reduces float-to-double and QDateTime conversion overhead

---

### 2. **Redundant timeMin/timeMax Epoch Conversion** ✅ FIXED

**Location:** 
- `updateVisibleDataCacheFull()` (line 1594-1595)
- `updateVisibleDataCacheIncremental()` (line 1727-1728)

**Problem:**
- `timeMin.toMSecsSinceEpoch()` and `timeMax.toMSecsSinceEpoch()` called on every cache update
- Even when `timeMin`/`timeMax` haven't changed
- We already cache `m_cachedTimeMaxEpoch` but not `m_cachedTimeMinEpoch`

**Fix:**
- Added `m_cachedTimeMinEpoch` member to `waterfallgraph.h`
- Cache both `timeMinEpoch` and `timeMaxEpoch` in `updateCoordinateMappingCaches()`
- Use cached values in `updateVisibleDataCacheFull()` and `updateVisibleDataCacheIncremental()`

**Impact:**
- Eliminates 2 QDateTime conversions per cache update
- Reduces timezone lookup overhead

---

### 3. **Duplicate Cache Validity Check Pattern** ✅ FIXED

**Location:** Multiple functions (`drawScatterplot`, `drawDataLine`, `drawDataSeries`)

**Problem:**
All three functions had identical cache validity check code:
```cpp
if (!isVisibleDataCacheValid(seriesLabel))
{
    auto rangeIt = m_cachedTimeRange.find(seriesLabel);
    if (rangeIt != m_cachedTimeRange.end() &&
        rangeIt->second.first == timeMin && rangeIt->second.second == timeMax)
    {
        updateVisibleDataCacheIncremental(seriesLabel);
    }
    else
    {
        updateVisibleDataCacheFull(seriesLabel);
    }
}
```

**Fix:**
- Created helper method `ensureVisibleDataCacheValid(seriesLabel)`
- Encapsulates the cache validity check and update logic
- All three functions now call this helper method

**Impact:**
- Eliminates code duplication
- Easier to maintain and modify cache update logic
- Consistent behavior across all rendering functions

---

### 4. **Redundant Visible Data Copying** ✅ FIXED

**Location:** Multiple functions copying from `CircularBuffer` to `m_reusableVisibleData`

**Problem:**
The same copying pattern appeared in:
- `drawScatterplot()` (line 4155-4160)
- `drawDataLine()` (line 3158-3164)
- `drawDataSeries()` (line 4362-4368)
- `removeScatterplotItemsOutsideRange()` (line 1916-1921)

**Fix:**
- Created helper method `getVisibleDataVector(seriesLabel)`
- Handles copying from `CircularBuffer` to reusable vector
- All functions now call this helper method

**Impact:**
- Eliminates code duplication
- Consistent copying behavior
- Easier to optimize copying logic in the future

---

### 5. **Redundant Range Check in `updateVisibleDataCacheIncremental()`** ✅ FIXED

**Location:** `updateVisibleDataCacheIncremental()` (line 1640-1645)

**Problem:**
```cpp
// Check if time range changed - requires full refilter
auto rangeIt = m_cachedTimeRange.find(seriesLabel);
if (rangeIt == m_cachedTimeRange.end() ||
    rangeIt->second.first != timeMin || rangeIt->second.second != timeMax)
{
    updateVisibleDataCacheFull(seriesLabel);
    return;
}
```

This same check is already done in `isVisibleDataCacheValid()` (line 1476-1479), so callers that check validity then call incremental update are checking twice.

**Fix:**
- Removed redundant range check
- Trust `isVisibleDataCacheValid()` - if cache is valid, time range matches
- Added comment explaining the optimization

**Impact:**
- Eliminates redundant map lookup and QDateTime comparison
- Faster incremental cache updates

---

## Summary of Changes

### Files Modified:

1. **`waterfallgraph.h`**:
   - Added `m_cachedTimeMinEpoch` member
   - Added `ensureVisibleDataCacheValid()` method declaration
   - Added `getVisibleDataVector()` method declaration

2. **`waterfallgraph.cpp`**:
   - Removed unoptimized populate calls from `drawDataSeries()`
   - Added `m_cachedTimeMinEpoch` caching in `updateCoordinateMappingCaches()`
   - Updated `updateVisibleDataCacheFull()` to use cached `timeMinEpoch`/`timeMaxEpoch`
   - Updated `updateVisibleDataCacheIncremental()` to use cached `timeMinEpoch`/`timeMaxEpoch`
   - Removed redundant range check from `updateVisibleDataCacheIncremental()`
   - Implemented `ensureVisibleDataCacheValid()` helper method
   - Implemented `getVisibleDataVector()` helper method
   - Updated `drawScatterplot()`, `drawDataLine()`, and `drawDataSeries()` to use helper methods
   - Updated `removeScatterplotItemsOutsideRange()` to use `getVisibleDataVector()`

---

## Expected Performance Impact

1. **Eliminated Unnecessary Populate Calls**:
   - `drawDataSeries()` no longer calls unoptimized populate methods
   - Reduces float-to-double and QDateTime conversion overhead

2. **Reduced Time Conversions**:
   - Cached `timeMinEpoch` eliminates repeated `toMSecsSinceEpoch()` calls
   - 2 fewer conversions per cache update

3. **Eliminated Redundant Checks**:
   - Removed duplicate time range check in `updateVisibleDataCacheIncremental()`
   - Faster incremental cache updates

4. **Code Quality Improvements**:
   - Reduced code duplication
   - Easier to maintain
   - Consistent behavior across functions

---

## Verification

To verify the fixes:
1. Run a new callgrind profile
2. Check that `populateYDataSeries` and `populateTimestampsSeries` call counts are reduced
3. Verify that `timeMin.toMSecsSinceEpoch()` is no longer called in hot paths
4. Confirm that cache update functions are faster

---

## Related Optimizations

These fixes complement other optimizations:
- **Float-to-double conversion elimination**: Using `populateYDataSeriesFloat()` instead of `populateYDataSeries()`
- **Epoch milliseconds optimization**: Using `populateTimestampsEpochSeries()` instead of `populateTimestampsSeries()`
- **Cached time max epoch**: Already implemented, now extended to `timeMinEpoch`

---

## Summary

All identified redundancies and duplicate references have been fixed:
- ✅ Removed unoptimized populate calls from `drawDataSeries()`
- ✅ Cached `timeMinEpoch` to avoid repeated conversions
- ✅ Created helper methods to eliminate code duplication
- ✅ Removed redundant range check in incremental cache update

The code is now more efficient, maintainable, and consistent across all rendering functions.



