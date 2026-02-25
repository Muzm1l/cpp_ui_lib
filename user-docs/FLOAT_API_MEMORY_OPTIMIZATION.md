# Float API Memory Optimization

**Last Updated:** 2024

## Overview

All `addData` APIs have been changed to accept `float` instead of `qreal` (which is typically `double`). This optimization reduces memory usage for callers by allowing them to pass 4-byte `float` values instead of 8-byte `double` values, while maintaining internal precision by converting to `double` for storage.

## Motivation

### Memory Savings

- **Before:** Callers had to allocate `std::vector<double>` (8 bytes per element)
- **After:** Callers can allocate `std::vector<float>` (4 bytes per element)
- **Savings:** 50% reduction in memory for data vectors passed to the API

### Example Memory Impact

For a typical data stream with 10,000 points:
- **Before:** 10,000 × 8 bytes = 80 KB per vector
- **After:** 10,000 × 4 bytes = 40 KB per vector
- **Savings:** 40 KB per vector (50% reduction)

For applications with multiple series and frequent updates, this can result in significant memory savings.

## API Changes

### WaterfallData

#### Before
```cpp
void addDataSeries(const QString& seriesLabel, 
                   const std::vector<qreal>& yData, 
                   const std::vector<QDateTime>& timestamps);

void addDataPointToSeries(const QString& seriesLabel, 
                          qreal yValue, 
                          const QDateTime& timestamp);

void addDataPointsToSeries(const QString& seriesLabel, 
                           const std::vector<qreal>& yValues, 
                           const std::vector<QDateTime>& timestamps);
```

#### After
```cpp
void addDataSeries(const QString& seriesLabel, 
                   const std::vector<float>& yData, 
                   const std::vector<QDateTime>& timestamps);

void addDataPointToSeries(const QString& seriesLabel, 
                          float yValue, 
                          const QDateTime& timestamp);

void addDataPointsToSeries(const QString& seriesLabel, 
                           const std::vector<float>& yValues, 
                           const std::vector<QDateTime>& timestamps);
```

### GraphEngine

#### Before
```cpp
void addDataPoint(const QString &seriesLabel, 
                  qreal yValue, 
                  const QDateTime &timestamp);

void addDataPoints(const QString &seriesLabel, 
                   const std::vector<qreal> &yValues, 
                   const std::vector<QDateTime> &timestamps);
```

#### After
```cpp
void addDataPoint(const QString &seriesLabel, 
                  float yValue, 
                  const QDateTime &timestamp);

void addDataPoints(const QString &seriesLabel, 
                   const std::vector<float> &yValues, 
                   const std::vector<QDateTime> &timestamps);
```

### WaterfallGraph

#### Before
```cpp
void addDataPoint(const QString &seriesLabel, 
                  qreal yValue, 
                  const QDateTime &timestamp);

void addDataPoints(const QString &seriesLabel, 
                   const std::vector<qreal> &yValues, 
                   const std::vector<QDateTime> &timestamps);
```

#### After
```cpp
void addDataPoint(const QString &seriesLabel, 
                  float yValue, 
                  const QDateTime &timestamp);

void addDataPoints(const QString &seriesLabel, 
                   const std::vector<float> &yValues, 
                   const std::vector<QDateTime> &timestamps);
```

### GraphLayout

#### Before
```cpp
void addDataPointToDataSource(const GraphType &graphType, 
                               const QString &seriesLabel, 
                               qreal yValue, 
                               const QDateTime &timestamp);

void addDataPointsToDataSource(const GraphType &graphType, 
                                const QString &seriesLabel, 
                                const std::vector<qreal> &yValues, 
                                const std::vector<QDateTime> &timestamps);
```

#### After
```cpp
void addDataPointToDataSource(const GraphType &graphType, 
                               const QString &seriesLabel, 
                               float yValue, 
                               const QDateTime &timestamp);

void addDataPointsToDataSource(const GraphType &graphType, 
                                const QString &seriesLabel, 
                                const std::vector<float> &yValues, 
                                const std::vector<QDateTime> &timestamps);
```

### SCWWindow

#### Before
```cpp
void addDataPoints(SCW_SERIES_ADOPTED series, 
                   const std::vector<qreal> &yData, 
                   const std::vector<QDateTime> &timestamps);

void addDataPoints(SCW_SERIES_R series, 
                   const std::vector<qreal> &yData, 
                   const std::vector<QDateTime> &timestamps);

void addDataPoints(SCW_SERIES_B series, 
                   const std::vector<qreal> &yData, 
                   const std::vector<QDateTime> &timestamps);

void addDataPoints(SCW_SERIES_A series, 
                   const std::vector<qreal> &yData, 
                   const std::vector<QDateTime> &timestamps);

void addDataPoints(SCW_SERIES_E series, 
                   const std::vector<qreal> &yData, 
                   const std::vector<QDateTime> &timestamps);
```

#### After
```cpp
void addDataPoints(SCW_SERIES_ADOPTED series, 
                   const std::vector<float> &yData, 
                   const std::vector<QDateTime> &timestamps);

void addDataPoints(SCW_SERIES_R series, 
                   const std::vector<float> &yData, 
                   const std::vector<QDateTime> &timestamps);

void addDataPoints(SCW_SERIES_B series, 
                   const std::vector<float> &yData, 
                   const std::vector<QDateTime> &timestamps);

void addDataPoints(SCW_SERIES_A series, 
                   const std::vector<float> &yData, 
                   const std::vector<QDateTime> &timestamps);

void addDataPoints(SCW_SERIES_E series, 
                   const std::vector<float> &yData, 
                   const std::vector<QDateTime> &timestamps);
```

### BTWInteractiveOverlay

#### Before
```cpp
InteractiveGraphicsItem* addDataPointMarker(const QPointF &position, 
                                             const QDateTime &timestamp, 
                                             qreal value, 
                                             const QString &seriesLabel);
```

#### After
```cpp
InteractiveGraphicsItem* addDataPointMarker(const QPointF &position, 
                                             const QDateTime &timestamp, 
                                             float value, 
                                             const QString &seriesLabel);
```

## Internal Implementation

### Conversion Strategy

The library converts `float` to `double` (qreal) internally for storage. This ensures:

1. **Precision Preservation:** Internal storage uses `double` precision
2. **Memory Efficiency:** Callers can use `float` to reduce memory footprint
3. **Transparent Conversion:** Conversion happens automatically, no manual casting required

### Conversion Implementation

#### Single Values
```cpp
// In WaterfallData::addDataPointToSeries()
qreal yValueDouble = static_cast<qreal>(yValue);  // float -> double
dataSeriesYData[seriesLabel].push_back(yValueDouble);
```

#### Vectors
```cpp
// In WaterfallData::addDataPointsToSeries()
std::vector<qreal> yValuesDouble(yValues.begin(), yValues.end());  // float[] -> double[]
dataSeriesYData[seriesLabel].push_back(yValuesDouble);
```

## Migration Guide

### For Existing Code

If your code currently uses `qreal` or `double`, you need to convert to `float`:

#### Single Values

**Before:**
```cpp
qreal value = 42.5;
graph->addDataPoint("Series1", value, timestamp);
```

**After:**
```cpp
qreal value = 42.5;
graph->addDataPoint("Series1", static_cast<float>(value), timestamp);
```

Or better yet:
```cpp
float value = 42.5f;
graph->addDataPoint("Series1", value, timestamp);
```

#### Vectors

**Before:**
```cpp
std::vector<qreal> data = {1.0, 2.0, 3.0, 4.0};
graph->addDataPoints("Series1", data, timestamps);
```

**After:**
```cpp
std::vector<qreal> data = {1.0, 2.0, 3.0, 4.0};
std::vector<float> dataFloat(data.begin(), data.end());
graph->addDataPoints("Series1", dataFloat, timestamps);
```

Or better yet:
```cpp
std::vector<float> data = {1.0f, 2.0f, 3.0f, 4.0f};
graph->addDataPoints("Series1", data, timestamps);
```

### Example: Complete Migration

**Before:**
```cpp
void addSensorData(WaterfallGraph* graph, const QString& seriesLabel) {
    QDateTime timestamp = QDateTime::currentDateTime();
    std::vector<qreal> sensorReadings;
    
    for (int i = 0; i < 1000; ++i) {
        qreal reading = getSensorValue(i);
        sensorReadings.push_back(reading);
    }
    
    graph->addDataPoints(seriesLabel, sensorReadings, timestamps);
}
```

**After:**
```cpp
void addSensorData(WaterfallGraph* graph, const QString& seriesLabel) {
    QDateTime timestamp = QDateTime::currentDateTime();
    std::vector<float> sensorReadings;  // Changed to float
    sensorReadings.reserve(1000);  // Optional: pre-allocate for efficiency
    
    for (int i = 0; i < 1000; ++i) {
        float reading = static_cast<float>(getSensorValue(i));  // Convert to float
        sensorReadings.push_back(reading);
    }
    
    graph->addDataPoints(seriesLabel, sensorReadings, timestamps);
}
```

## Files Modified

### Core Library Files

1. **waterfalldata.h/cpp** - Core data storage APIs
2. **graphengine.h/cpp** - Graph engine APIs
3. **waterfallgraph.h/cpp** - Graph widget APIs
4. **graphlayout.h/cpp** - Layout management APIs
5. **scwwindow.h/cpp** - SCW window APIs
6. **btwinteractiveoverlay.h/cpp** - Interactive overlay APIs

### Application Files (Updated for Compatibility)

1. **simulator.cpp** - Test data generation
2. **mainwindow.cpp** - Main window test code
3. **scwsimulator.cpp** - SCW simulator

## Precision Considerations

### Float vs Double Precision

- **Float:** ~7 decimal digits of precision, range ±3.4×10³⁸
- **Double:** ~15-17 decimal digits of precision, range ±1.7×10³⁰⁸

### When Float is Sufficient

For most graph data applications, `float` precision is sufficient:
- Sensor readings (typically 3-4 significant digits)
- Time-series data (rarely needs more than 6-7 digits)
- Scientific measurements (most instruments have limited precision)

### When You Might Need More Precision

If your application requires very high precision (more than 7 significant digits), you can:
1. Convert to `float` at the API boundary (as we do internally)
2. Use `double` in your calculations
3. Convert to `float` only when calling the API

Example:
```cpp
// Use double for calculations
double preciseValue = calculateWithHighPrecision();

// Convert to float for API call
graph->addDataPoint("Series1", static_cast<float>(preciseValue), timestamp);
```

## Performance Impact

### Memory Allocation

- **Reduced allocations:** Smaller vectors mean less memory pressure
- **Cache efficiency:** More data fits in CPU cache with smaller types
- **Conversion overhead:** Minimal - simple type conversion, no complex operations

### Benchmarking

The conversion from `float` to `double` is a simple type cast operation with negligible performance impact. The memory savings from using `float` vectors typically outweigh any conversion overhead.

## Backward Compatibility

### Breaking Changes

⚠️ **This is a breaking change.** Code that passes `qreal` or `double` values directly will not compile.

### Compilation Errors

You may see errors like:
```
error: cannot convert 'std::vector<double>' to 'const std::vector<float>&'
```

**Solution:** Convert your vectors to `float` as shown in the Migration Guide above.

## Testing

All existing tests have been updated to use `float` types. The internal conversion to `double` ensures that precision is maintained for all stored data.

## Summary

- ✅ All `addData` APIs now accept `float` instead of `qreal`
- ✅ Internal storage remains `double` for precision
- ✅ 50% memory reduction for data vectors passed to APIs
- ✅ Automatic conversion from `float` to `double` internally
- ⚠️ Breaking change - existing code must be updated

## Related Documentation

- [Memory Leak Analysis](./memory-leak-analysis.md) - Overall memory optimization strategy
- [Circular Buffer Guide](./circular-buffer-guide.md) - Internal data storage mechanisms
- [Performance Optimization Report](./performance-optimization-report.md) - Additional performance improvements

