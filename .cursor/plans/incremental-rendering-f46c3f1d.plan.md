<!-- f46c3f1d-2df0-4eed-a915-cbf93088cc40 e476fefb-3ec8-4192-907e-647be24595d0 -->
# Plan: Incremental Cached Data Filtering for WaterfallGraph

## Problem

Current `drawDataSeries()` method filters data by time range on EVERY draw call, creating a new `visibleData` vector each time. In a logging system that continuously adds new data, this causes:

- Unnecessary memory allocations on every draw
- O(n) filtering cost on every frame where n = total data points
- Re-filtering of already-visible data points that haven't changed
- Wasted CPU cycles filtering data that was already filtered

## Solution

Implement incremental filtering with caching:

- Cache filtered visible data per series
- Track last processed data index per series
- Incrementally filter only new data points and merge with cache
- Use binary search for efficient time range boundary finding
- Retain cached data for up to 12 hours (sliding window)
- Automatically remove data older than 12 hours from cache

## Implementation Steps

### 1. Add Filtered Data Cache Member Variables to Header

**File**: `waterfallgraph.h`

- Add private member variables:
- `std::map<QString, std::vector<std::pair<qreal, QDateTime>>> m_cachedVisibleData` - Cached filtered data per series
- `std::map<QString, std::pair<QDateTime, QDateTime>> m_cachedTimeRange` - Time range used for each series cache
- `std::map<QString, size_t> m_lastProcessedIndex` - Last data index processed for each series
- `std::map<QString, size_t> m_cachedDataSize` - Data size when cache was last updated

### 2. Add Cache Management Method Declarations

**File**: `waterfallgraph.h`

- Add private helper methods:
- `void invalidateVisibleDataCache(const QString& seriesLabel)`
- `void invalidateAllVisibleDataCache()`
- `void updateVisibleDataCacheIncremental(const QString& seriesLabel)`
- `void updateVisibleDataCacheFull(const QString& seriesLabel)`
- `bool isVisibleDataCacheValid(const QString& seriesLabel) const`
- `size_t findFirstVisibleIndex(const std::vector<QDateTime>& timestamps, const QDateTime& timeMin) const`
- `size_t findLastVisibleIndex(const std::vector<QDateTime>& timestamps, const QDateTime& timeMax) const`

### 3. Initialize Cache in Constructor

**File**: `waterfallgraph.cpp` (constructor)

- Cache maps are automatically initialized as empty (no explicit initialization needed)
- Cache starts invalid (which is correct - will be populated on first use)
- `m_lastProcessedIndex` starts at 0 for all series

### 4. Implement Cache Invalidation Methods

**File**: `waterfallgraph.cpp`

- `invalidateVisibleDataCache(seriesLabel)`:
- Clear `m_cachedVisibleData[seriesLabel]`
- Reset `m_lastProcessedIndex[seriesLabel] = 0`
- Reset `m_cachedDataSize[seriesLabel] = 0`

- `invalidateAllVisibleDataCache()`:
- Clear all cached data maps
- Reset all last processed indices
- Reset all cached data sizes

### 5. Implement Binary Search Helpers for Time Range Boundaries

**File**: `waterfallgraph.cpp`

- `findFirstVisibleIndex(timestamps, timeMin)`:
- Use `std::lower_bound` to find first timestamp >= timeMin
- Return index of first visible data point
- O(log n) instead of O(n) linear search

- `findLastVisibleIndex(timestamps, timeMax)`:
- Use `std::upper_bound` to find first timestamp > timeMax
- Return index of last visible data point (exclusive)
- O(log n) instead of O(n) linear search

### 6. Implement Incremental Cache Update Method

**File**: `waterfallgraph.cpp`

- `updateVisibleDataCacheIncremental(seriesLabel)`:
- Get current data size from dataSource
- Check if data size changed (new data added)
- If time range changed, call `updateVisibleDataCacheFull()` instead
- Get last processed index for this series
- Only filter data points from `m_lastProcessedIndex[seriesLabel]` to end
- Filter new points within time range
- Append new visible points to `m_cachedVisibleData[seriesLabel]`
- Update `m_lastProcessedIndex[seriesLabel]` to current data size
- Update `m_cachedDataSize[seriesLabel]`
- Remove points that fall outside time range (if time range moved backward)

### 7. Implement Full Cache Update Method

**File**: `waterfallgraph.cpp`

- `updateVisibleDataCacheFull(seriesLabel)`:
- Get all data from dataSource
- Use binary search to find first and last visible indices
- Filter only the visible range (not entire dataset)
- Store filtered data in `m_cachedVisibleData[seriesLabel]`
- Store current time range in `m_cachedTimeRange[seriesLabel]`
- Update `m_lastProcessedIndex[seriesLabel]` to current data size
- Update `m_cachedDataSize[seriesLabel]`

### 8. Implement Cache Validation Check

**File**: `waterfallgraph.cpp`

- `isVisibleDataCacheValid(seriesLabel)`:
- Check if cache exists for series
- Verify cached time range matches current `timeMin` and `timeMax`
- Verify cached data size matches current data size (no new data added)
- Return `true` only if all conditions are met

### 9. Refactor drawDataSeries() to Use Cached Data

**File**: `waterfallgraph.cpp` (lines 1805-1855)

- Replace inline filtering logic with:
- Check if cache is valid using `isVisibleDataCacheValid()`
- If invalid:
- If time range changed: call `updateVisibleDataCacheFull(seriesLabel)`
- Else if data size changed: call `updateVisibleDataCacheIncremental(seriesLabel)`
- Else: call `updateVisibleDataCacheFull(seriesLabel)` (fallback)
- Use `m_cachedVisibleData[seriesLabel] `instead of local `visibleData`
- Remove the local `visibleData` vector creation and filtering loop

### 10. Invalidate Cache When Time Range Changes

**File**: `waterfallgraph.cpp`

- `setTimeRange()` (line ~2141): Call `invalidateAllVisibleDataCache()` before setting new range
- `setTimeMax()` (line ~2168): Call `invalidateAllVisibleDataCache()` before setting new max
- `setTimeMin()` (line ~2207): Call `invalidateAllVisibleDataCache()` before setting new min
- `setTimeRangeFromData()`: Call `invalidateAllVisibleDataCache()` if time range changes
- `updateDataRanges()`: If time range changes, invalidate cache

### 11. Handle Incremental Updates When Data is Added

**File**: `waterfallgraph.cpp`

- In `drawIncremental()` method, when drawing dirty series:
- Cache validation will detect data size change
- `updateVisibleDataCacheIncremental()` will be called automatically
- Only new data points will be filtered and merged

### 12. Optimize for Time Range That Moves Forward (Sliding Window)

**File**: `waterfallgraph.cpp`

- When time range moves forward (common in logging systems):
- Remove points that are now outside the range (older than new timeMin)
- Use binary search to find cutoff point
- Only filter new points that entered the range
- More efficient than full refilter

### 13. Invalidate Cache When Data Source Changes

**File**: `waterfallgraph.cpp`

- `setDataSource()` (line ~188): Call `invalidateAllVisibleDataCache()` when new data source is set

### 14. Update Other Methods That Use Filtering

**File**: `waterfallgraph.cpp`

- `drawDataLine()` (line ~1201): Refactor to use same caching mechanism
- Search for other occurrences of similar filtering patterns
- Apply same incremental filtering approach

### 15. Handle Edge Cases

**File**: `waterfallgraph.cpp`

- Empty data: Cache should handle empty series gracefully
- Time range with no data: Cache should store empty vector
- Multiple series: Each series has independent cache
- Data removed: If data size decreases, invalidate cache (data was cleared)
- Time range moves backward: Full refilter required

### 16. Testing and Verification

- Test incremental update: Add single point, verify only that point is filtered
- Test time range change: Change range, verify full refilter
- Test sliding window: Move time range forward, verify old points removed efficiently
- Test multiple series: Verify each series has independent cache
- Test binary search: Verify correct boundary finding
- Performance test: Measure reduction in filtering operations
- Memory test: Verify cache doesn't leak memory

## Key Implementation Details

### Incremental Filtering Strategy

- **New data added**: Only filter new points (from last processed index to end)
- **Time range unchanged**: Merge new visible points with existing cache
- **Time range changed**: Full refilter using binary search for boundaries
- **Time range moves forward**: Remove old points, add new points efficiently

### Binary Search Optimization

- Use `std::lower_bound` and `std::upper_bound` for O(log n) boundary finding
- Only filter the visible range, not entire dataset
- Significant improvement when visible data is small subset of total data

### Cache Invalidation Strategy

- **Time range changes**: Invalidate all series caches (time range affects all series)
- **Data source changes**: Invalidate all caches
- **Data size decreases**: Invalidate cache (data was cleared/removed)

### Performance Improvements

- **Before**: O(n) filtering on every `drawDataSeries()` call where n = total data points
- **After**: 
- O(k) incremental filtering where k = new data points (typically 1-2 points)
- O(log n) binary search for time boundaries
- O(m) full refilter where m = visible data points (usually << n)
- **Expected**: 10-100x faster for typical logging use case (adding 1-2 points per frame)

### Memory Considerations

- Cached data stores copies of filtered data points
- Memory usage is proportional to visible data points (not total data points)
- Incremental updates append to cache (may need periodic cleanup if time range doesn't slide)

### Backward Compatibility

- Public API remains unchanged
- Behavior should be identical to current implementation
- Only internal optimization