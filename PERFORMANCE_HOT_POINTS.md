# Performance Hot Points Analysis

## Problem
System works fine with 15-minute intervals but slows down tremendously with 1 hour or more intervals.

## Root Causes

### 1. **CRITICAL: `addBTWSymbolToOtherGraphs()` - O(n²) Complexity**
**Location:** `btwgraph.cpp:683-782`

**Issues:**
- **Line 727**: `getBTWSymbols()` returns a full vector copy - O(n) memory copy
- **Lines 729-738**: Linear search through ALL existing symbols for each new symbol - O(n²) worst case
- **Lines 754-763**: Linear search through ALL timestamps for each series - O(n²) worst case
- Called during marker placement/drawing operations

**Impact:** With 1 hour of data (potentially thousands of symbols and timestamps), this becomes extremely slow.

**Fix:**
- Use binary search or hash map for symbol deduplication
- Use binary search for timestamp lookup (timestamps are sorted)
- Cache symbol existence checks
- Consider using `std::unordered_set` or `std::set` for symbol lookup

### 2. **`drawCustomCircleMarkers()` - Inefficient Filtering**
**Location:** `btwgraph.cpp:284-390`

**Issues:**
- **Line 292**: `getBTWMarkers()` returns a full vector copy
- **Lines 303-307**: Linear filtering through ALL markers, even those outside visible range
- No early exit or binary search optimization

**Impact:** With 1 hour of data, processes hundreds/thousands of invisible markers every draw call.

**Fix:**
- Add `getBTWMarkersWithinTimeRange()` method to WaterfallData
- Use binary search for time range filtering (timestamps are sorted)
- Filter at data source level, not in drawing code

### 3. **`drawBTWSymbols()` - Same Filtering Issue**
**Location:** `btwgraph.cpp:612-681`

**Issues:**
- **Line 621**: `getBTWSymbols()` returns a full vector copy
- **Lines 634-640**: Linear filtering through ALL symbols, even those outside visible range

**Impact:** Same as #2 - processes invisible symbols unnecessarily.

**Fix:**
- Add `getBTWSymbolsWithinTimeRange()` method to WaterfallData
- Use binary search for time range filtering

### 4. **Vector Copying Overhead**
**Location:** `waterfalldata.cpp:817-819, 866-868`

**Issues:**
- `getBTWSymbols()` and `getBTWMarkers()` return copies instead of const references
- With large datasets, copying is expensive

**Fix:**
- Return `const std::vector<BTWSymbolData>&` instead of `std::vector<BTWSymbolData>`
- Or provide both const reference and filtered versions

### 5. **Timestamp Search in `addBTWSymbolToOtherGraphs()`**
**Location:** `btwgraph.cpp:754-763`

**Issues:**
- Linear search through all timestamps for each symbol
- No early exit optimization
- Could use binary search since timestamps are sorted

**Fix:**
- Use `std::lower_bound` for binary search on sorted timestamps
- Early exit when time difference becomes too large

### 6. **`getDataSeriesWithinTimeRange()` - Linear Scan**
**Location:** `waterfalldata.cpp:346-362`

**Issues:**
- O(n) linear scan through all timestamps
- No binary search optimization

**Fix:**
- Use binary search (`std::lower_bound`/`std::upper_bound`) for sorted timestamps
- Find start and end indices, then copy range

## Recommended Priority Order

1. **HIGH PRIORITY**: Fix `addBTWSymbolToOtherGraphs()` - This is the biggest bottleneck
2. **HIGH PRIORITY**: Add time-range filtering methods to WaterfallData
3. **MEDIUM PRIORITY**: Optimize `drawCustomCircleMarkers()` and `drawBTWSymbols()` to use filtered data
4. **MEDIUM PRIORITY**: Change return types to const references where safe
5. **LOW PRIORITY**: Optimize `getDataSeriesWithinTimeRange()` with binary search

## Expected Performance Improvement

- **15-minute intervals**: Minimal impact (already fast)
- **1-hour intervals**: Should see 10-100x improvement depending on data density
- **Multi-hour intervals**: Critical for usability

## Implementation Notes

- Timestamps in WaterfallData are stored in vectors - verify they are sorted
- Consider maintaining sorted indices or using `std::set`/`std::map` for time-based lookups
- Profile before and after changes to measure improvement

