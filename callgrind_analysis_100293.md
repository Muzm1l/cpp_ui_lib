# Callgrind Performance Analysis - callgrind.out.100293

**Date:** 2024  
**Total Instructions:** 5,394,413,897,762 (5.39 trillion)  
**Profiled Target:** ui-sandbox (PID 100293)

## Executive Summary

The profiling data reveals several critical performance hotspots:

1. **Data Population Methods** (38.85% combined):
   - `populateYDataSeries`: 19.17% (1.03 trillion instructions)
   - `populateTimestampsSeries`: 19.68% (1.06 trillion instructions)

2. **Range Calculations** (12.48% combined):
   - `getCombinedYRange`: 8.66% (466 billion instructions)
   - Called 1,034,915 times

3. **Rendering Operations** (15.52% combined):
   - `drawScatterplot`: 7.38% (398 billion instructions)
   - `drawDataLine`: 2.19% (118 billion instructions)
   - `drawIncremental`: 4.14% (223 billion instructions)

4. **Data Access** (11.12% combined):
   - `CircularBuffer<float>::operator[]`: 7.27% (392 billion instructions)
   - `std::vector<double>::emplace_back`: 8.03% (433 billion instructions)

## Top 20 Hotspots

| Rank | Function | Instructions | % of Total | Calls |
|------|----------|-------------|------------|-------|
| 1 | `populateTimestampsSeries` | 1,061,785,604,580 | 19.68% | 2,217,059 |
| 2 | `populateYDataSeries` | 1,034,269,515,658 | 19.17% | 2,217,059 |
| 3 | `getCombinedYRange` | 466,911,851,803 | 8.66% | 1,034,915 |
| 4 | `std::vector<double>::emplace_back` | 433,148,113,472 | 8.03% | - |
| 5 | `CircularBuffer<float>::operator[]` | 392,179,580,928 | 7.27% | - |
| 6 | `drawScatterplot` | 398,351,310,009 | 7.38% | 238,866 |
| 7 | `CircularBuffer<float>::size()` | 174,240,660,504 | 3.23% | - |
| 8 | `getCombinedTimeRange` | 38,718,001,719 | 0.72% | - |
| 9 | `mapDataToScreen(qreal, qint64)` | 63,651,453,838 | 1.18% | 471,768,922 |
| 10 | `updateVisibleDataCacheFull` | 53,448,188,572 | 0.99% | 57,537 |
| 11 | `drawDataLine` | 118,245,938,208 | 2.19% | 57,514 |
| 12 | `drawDataSeries` | 198,988,422,824 | 3.69% | 109,439 |
| 13 | `drawIncremental` | 223,373,031,262 | 4.14% | 56,758 |
| 14 | `updateDataRanges` | 27,006,833,819 | 0.50% | 57,235 |
| 15 | `CircularBuffer<QDateTime>::operator[]` | 196,942,250,106 | 3.65% | - |
| 16 | `std::vector<QDateTime>::push_back` | 202,281,116,898 | 3.75% | - |
| 17 | `populateTimestampsEpochSeries` | 31,022,259,210 | 0.58% | 606,072 |
| 18 | `updateScatterplotItemsFull` | 29,855,303,091 | 0.55% | - |
| 19 | `getLatestTime` | 27,658,858,110 | 0.51% | - |
| 20 | `isVisibleDataCacheValid` | 798,200,564,750 | 14.80% | 2,217,035 |

## Critical Performance Issues

### 1. Float-to-Double Conversion Overhead (19.17%)

**Issue:** `populateYDataSeries` converts `float` to `double` (qreal) for every data point.

**Location:** `waterfalldata.cpp:WaterfallData::populateYDataSeries()`

**Impact:**
- 1.03 trillion instructions (19.17% of total)
- Called 2,217,059 times
- Each call processes potentially thousands of data points

**Current Implementation:**
```cpp
void WaterfallData::populateYDataSeries(const QString& seriesLabel, std::vector<qreal>& output) const
{
    const CircularBuffer<float>& buffer = it->second;
    for (size_t i = 0; i < buffer.size(); ++i)
    {
        output.push_back(static_cast<qreal>(buffer[i]));  // Conversion on every element
    }
}
```

**Recommendation:**
- Consider providing a `populateYDataSeriesFloat()` method that returns `std::vector<float>` directly
- Only convert to `qreal` when absolutely necessary for calculations
- Cache converted results if the same data is accessed multiple times

### 2. QDateTime Operations (19.68%)

**Issue:** `populateTimestampsSeries` copies `QDateTime` objects, which is expensive.

**Location:** `waterfalldata.cpp:WaterfallData::populateTimestampsSeries()`

**Impact:**
- 1.06 trillion instructions (19.68% of total)
- Called 2,217,059 times
- QDateTime copy operations are expensive (timezone handling, etc.)

**Recommendation:**
- Use epoch milliseconds (`qint64`) instead of `QDateTime` in hot paths
- Already have `populateTimestampsEpochSeries()` - use it more frequently
- Cache `QDateTime` conversions if needed

### 3. Range Calculation Frequency (8.66%)

**Issue:** `getCombinedYRange()` is called 1,034,915 times, iterating through all data series.

**Location:** `waterfalldata.cpp:WaterfallData::getCombinedYRange()`

**Impact:**
- 466 billion instructions (8.66% of total)
- Called over 1 million times
- Each call iterates through all series and all data points

**Current Implementation:**
```cpp
std::pair<qreal, qreal> WaterfallData::getCombinedYRange() const
{
    for (const auto& pair : dataSeriesYData) {
        if (!pair.second.empty()) {
            // Find min/max - iterates through entire buffer
            for (size_t i = 1; i < pair.second.size(); ++i) {
                // ...
            }
        }
    }
}
```

**Recommendation:**
- Cache range calculations and invalidate only when data changes
- Use incremental min/max tracking during data insertion
- Consider maintaining running min/max values in the data structure

### 4. Circular Buffer Indexing (7.27%)

**Issue:** `CircularBuffer<float>::operator[]` is called very frequently.

**Location:** `circularbuffer.h:CircularBuffer<float>::operator[]`

**Impact:**
- 392 billion instructions (7.27% of total)
- Called millions of times during data access

**Recommendation:**
- Consider using direct vector access when capacity is unlimited
- Optimize the circular buffer indexing calculation
- Cache frequently accessed indices

### 5. Cache Validation Overhead (14.80%)

**Issue:** `isVisibleDataCacheValid()` is called 2,217,035 times.

**Location:** `waterfallgraph.cpp:WaterfallGraph::isVisibleDataCacheValid()`

**Impact:**
- 798 billion instructions (14.80% of total)
- Called before every data access

**Recommendation:**
- Simplify cache validation logic
- Use version numbers instead of complex checks
- Cache validation results

## Performance Optimization Opportunities

### High Priority

1. **Reduce Float-to-Double Conversions**
   - Provide float-based APIs where possible
   - Only convert when necessary for calculations
   - Cache converted results

2. **Cache Range Calculations**
   - Maintain running min/max during data insertion
   - Invalidate cache only when data changes
   - Use incremental updates

3. **Optimize QDateTime Usage**
   - Use epoch milliseconds in hot paths
   - Minimize QDateTime copies
   - Cache QDateTime conversions

4. **Simplify Cache Validation**
   - Use version numbers for cache invalidation
   - Reduce validation overhead
   - Cache validation results

### Medium Priority

5. **Optimize Circular Buffer Access**
   - Direct vector access when capacity is unlimited
   - Optimize indexing calculations
   - Cache frequently accessed indices

6. **Reduce Rendering Overhead**
   - Batch rendering operations
   - Use more efficient graphics primitives
   - Reduce redundant drawing calls

### Low Priority

7. **Optimize Vector Operations**
   - Pre-allocate vectors when size is known
   - Use move semantics where possible
   - Reduce unnecessary allocations

## Call Frequency Analysis

### Most Frequently Called Functions

| Function | Calls | Avg Instructions/Call |
|----------|-------|----------------------|
| `mapDataToScreen(qreal, qint64)` | 471,768,922 | 210 |
| `CircularBuffer<QPointF>::push_back` | 469,892,342 | 97 |
| `CircularBuffer<std::pair<double, long long>>::push_back` | 124,466,127 | 97 |
| `isVisibleDataCacheValid` | 2,217,035 | 360,000 |
| `populateYDataSeries` | 2,217,059 | 466,000 |
| `populateTimestampsSeries` | 2,217,059 | 479,000 |
| `getCombinedYRange` | 1,034,915 | 451,000 |
| `drawScatterplot` | 238,866 | 1,667,000 |
| `drawDataLine` | 57,514 | 2,056,000 |

## Memory Allocation Patterns

### Allocation Hotspots

1. **Vector Allocations:**
   - `std::vector<double>::emplace_back`: 8.03%
   - `std::vector<QDateTime>::push_back`: 3.75%
   - `std::vector<QPointF>::push_back`: 0.26%

2. **Memory Management:**
   - `operator new`: 3.44%
   - `malloc`: 0.21%
   - `free`: 0.13%

## Recommendations Summary

1. **Immediate Actions:**
   - Cache `getCombinedYRange()` results
   - Use epoch milliseconds instead of QDateTime in hot paths
   - Provide float-based APIs to avoid conversions

2. **Short-term Optimizations:**
   - Implement incremental min/max tracking
   - Simplify cache validation
   - Optimize circular buffer access

3. **Long-term Improvements:**
   - Refactor to reduce data copying
   - Implement more efficient data structures
   - Consider SIMD optimizations for bulk operations

## Notes

- The profiling shows that data access and conversion operations dominate the execution time
- Rendering operations, while significant, are less of a bottleneck than data preparation
- The float-to-double conversion overhead suggests that maintaining float precision throughout more of the pipeline could improve performance
- QDateTime operations are expensive and should be minimized in performance-critical paths


