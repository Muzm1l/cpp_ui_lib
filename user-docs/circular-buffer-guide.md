# Circular Buffer Implementation Guide

## Overview

The graph system uses **circular buffers** to prevent unbounded memory growth when processing continuous data streams. This document explains how circular buffers work and how to configure their sizes.

## What is a Circular Buffer?

A circular buffer is a data structure that maintains a fixed maximum size. When the buffer reaches its capacity, new elements **overwrite the oldest elements** (First-In-First-Out, or FIFO behavior). This ensures that:

- Memory usage remains bounded
- Recent data is always preserved
- Old data is automatically discarded when capacity is exceeded

### Example

If you have a circular buffer with capacity 1000:
- Adding 500 points: buffer contains 500 points
- Adding 500 more points: buffer contains 1000 points (full)
- Adding 100 more points: buffer still contains 1000 points, but the **oldest 100 points are overwritten**

## Where Circular Buffers Are Used

Circular buffers are implemented in the following data structures:

### 1. Data Series (`WaterfallData`)
- **Y Data**: `dataSeriesYData` - stores Y-axis values for each series
- **Timestamps**: `dataSeriesTimestamps` - stores QDateTime timestamps
- **Epoch Timestamps**: `dataSeriesTimestampsEpoch` - stores epoch milliseconds (performance optimization)

### 2. Symbols (`WaterfallData`)
- **RTW Symbols**: `rtwSymbols` - Range-Time-Waterfall symbols
- **BTW Symbols**: `btwSymbols` - Bearing-Time-Waterfall symbols

### 3. Markers (`WaterfallData`)
- **BTW Markers**: `btwMarkers` - manually placed BTW markers
- **RTW R Markers**: `rtwRMarkers` - manually placed RTW R markers

### 4. Rendering Caches (`WaterfallGraph`)
- **Scatter Points**: `m_scatterPoints` - cached scatter plot points for rendering
- **Batched Line Paths**: `m_batchedLinePaths` - cached line paths for large datasets
- **Cached Visible Data**: `m_cachedVisibleData` - filtered visible data points

## How Circular Buffers Work

### Internal Structure

The `CircularBuffer` template class maintains:
- **m_data**: Underlying storage vector
- **m_capacity**: Maximum number of elements (0 = unlimited)
- **m_startIndex**: Index of the oldest element (for circular wrapping)
- **m_size**: Current number of elements

### Access Pattern

Circular buffers provide **chronological access**:
- Index 0 = oldest element
- Index (size-1) = newest element
- Elements are accessed in chronological order regardless of internal storage order

### Key Methods

```cpp
// Add elements (automatically handles capacity limits)
buffer.push_back(value);
buffer.push_back(vectorOfValues);

// Get element at index (0 = oldest, size-1 = newest)
const T& value = buffer[index];

// Get all elements as vector (chronological order)
std::vector<T> allData = buffer.toVector();

// Set capacity (0 = unlimited)
buffer.setCapacity(1000);

// Check status
size_t currentSize = buffer.size();
size_t maxCapacity = buffer.capacity();
bool isEmpty = buffer.empty();
bool isFull = buffer.full();
```

## Setting Buffer Sizes

### API Methods

All capacity management is done through `GraphLayout` or directly through `WaterfallData` and `WaterfallGraph`.

#### 1. Data Series Capacity

Set capacity for all data series in a graph type:

```cpp
// Through GraphLayout
graphLayout->setDataSeriesCapacity(capacity);

// Directly through WaterfallData
waterfallData->setAllDataSeriesCapacity(capacity);

// For a specific series
waterfallData->setDataSeriesCapacity(seriesLabel, capacity);
```

**Example:**
```cpp
// Limit each data series to 10,000 points
graphLayout->setDataSeriesCapacity(10000);
```

#### 2. Symbols Capacity

Set capacity for RTW and BTW symbols:

```cpp
// Through GraphLayout
graphLayout->setSymbolsCapacity(capacity);

// Directly through WaterfallData
waterfallData->setRTWSymbolsCapacity(capacity);
waterfallData->setBTWSymbolsCapacity(capacity);
waterfallData->setAllSymbolsAndMarkersCapacity(symbolsCapacity, markersCapacity);
```

**Example:**
```cpp
// Limit symbols to 1,000 entries
graphLayout->setSymbolsCapacity(1000);
```

#### 3. Markers Capacity

Set capacity for BTW and RTW R markers:

```cpp
// Through GraphLayout
graphLayout->setMarkersCapacity(capacity);

// Directly through WaterfallData
waterfallData->setBTWMarkersCapacity(capacity);
waterfallData->setRTWRMarkersCapacity(capacity);
```

**Example:**
```cpp
// Limit markers to 500 entries
graphLayout->setMarkersCapacity(500);
```

#### 4. Rendering Caches Capacity

Set capacity for rendering caches (scatter points, line paths, cached visible data):

```cpp
// Through GraphLayout
graphLayout->setRenderingCachesCapacity(scatterCapacity, linePathsCapacity, cachedDataCapacity);

// Directly through WaterfallGraph
waterfallGraph->reserveAllScatterPointsCapacity(scatterCapacity);
waterfallGraph->reserveAllCachedVisibleDataCapacity(cachedDataCapacity);
```

**Example:**
```cpp
// Set rendering cache capacities
graphLayout->setRenderingCachesCapacity(
    5000,   // scatter points
    1000,   // line paths
    10000   // cached visible data
);
```

### Capacity Values

- **0 (zero)**: Unlimited capacity (not recommended for production)
- **Positive number**: Maximum number of elements
- **When capacity is reduced**: Oldest elements are automatically removed

### Setting Capacity at Initialization

The best practice is to set capacities early, before data starts flowing:

```cpp
// In your initialization code
GraphLayout* layout = new GraphLayout();

// Set capacities before adding data
layout->setDataSeriesCapacity(10000);      // 10K points per series
layout->setSymbolsCapacity(1000);         // 1K symbols
layout->setMarkersCapacity(500);          // 500 markers
layout->setRenderingCachesCapacity(5000, 1000, 10000);

// Now start adding data
// ...
```

## Memory Considerations

### Calculating Memory Usage

Approximate memory per element:
- **qreal (double)**: 8 bytes
- **QDateTime**: ~24 bytes
- **qint64**: 8 bytes
- **QPointF**: 16 bytes
- **QPainterPath**: Variable (depends on complexity)

**Example calculation for data series:**
```
Capacity: 10,000 points
Memory per point:
  - Y value (qreal): 8 bytes
  - Timestamp (QDateTime): 24 bytes
  - Epoch timestamp (qint64): 8 bytes
  Total: 40 bytes per point

Total memory: 10,000 × 40 = 400 KB per series
```

### Recommended Capacities

Based on typical usage patterns:

| Data Type | Recommended Capacity | Notes |
|-----------|---------------------|-------|
| Data Series | 10,000 - 50,000 | Depends on update rate and time window |
| Symbols | 1,000 - 5,000 | Usually much smaller than data series |
| Markers | 500 - 2,000 | User-placed markers are infrequent |
| Scatter Points | 5,000 - 20,000 | Should match visible data range |
| Line Paths | 1,000 - 5,000 | Batched paths for large datasets |
| Cached Visible Data | 10,000 - 50,000 | Should match data series capacity |

## Behavior When Capacity is Exceeded

When a circular buffer reaches capacity:

1. **New elements overwrite oldest elements** (FIFO)
2. **Buffer size remains constant** (equal to capacity)
3. **Chronological order is maintained** (oldest at index 0, newest at size-1)
4. **No memory allocation** occurs (memory is reused)

### Example Timeline

```
Time 0:   Buffer empty (0/1000)
Time 1:   Add 500 points → Buffer: 500/1000
Time 2:   Add 500 points → Buffer: 1000/1000 (full)
Time 3:   Add 100 points → Buffer: 1000/1000
          (Oldest 100 points overwritten)
Time 4:   Add 200 points → Buffer: 1000/1000
          (Oldest 200 points overwritten)
```

## Accessing Data

### Direct Access

```cpp
// Get element at index (0 = oldest)
qreal yValue = dataSeriesYData[seriesLabel][index];
QDateTime timestamp = dataSeriesTimestamps[seriesLabel][index];

// Get all data as vector (chronological order)
std::vector<qreal> allYData = dataSeriesYData[seriesLabel].toVector();
std::vector<QDateTime> allTimestamps = dataSeriesTimestamps[seriesLabel].toVector();
```

### Through WaterfallData API

```cpp
// Get data series (returns vector, converted from circular buffer)
std::vector<qreal> yData = waterfallData->getYDataSeries(seriesLabel);
std::vector<QDateTime> timestamps = waterfallData->getTimestampsSeries(seriesLabel);

// Get size
size_t size = waterfallData->getDataSeriesSize(seriesLabel);
```

## Best Practices

1. **Set capacities early**: Configure buffer sizes before data starts flowing
2. **Monitor memory usage**: Use appropriate capacities based on available memory
3. **Match related capacities**: Keep rendering cache capacities aligned with data series capacities
4. **Consider update rates**: Higher update rates may require larger capacities
5. **Test with realistic data**: Verify behavior with expected data volumes

## Troubleshooting

### Issue: Data disappears unexpectedly
**Cause**: Buffer capacity is too small, oldest data is being overwritten  
**Solution**: Increase capacity or reduce data update rate

### Issue: High memory usage
**Cause**: Buffer capacities are too large  
**Solution**: Reduce capacities to match actual needs

### Issue: Performance degradation
**Cause**: Converting circular buffers to vectors too frequently  
**Solution**: Cache converted vectors when possible, or access elements directly by index

## Implementation Details

The circular buffer implementation (`CircularBuffer<T>`) is located in:
- **Header**: `circularbuffer.h`
- **Usage**: All data structures in `WaterfallData` and rendering caches in `WaterfallGraph`

For unlimited capacity (not recommended), set capacity to 0. The buffer will behave like a regular vector but without automatic memory management.

