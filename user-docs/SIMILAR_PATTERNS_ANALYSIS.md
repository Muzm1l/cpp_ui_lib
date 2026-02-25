# Similar Patterns Analysis

**Date:** 2025  
**Purpose:** Identify duplicate code patterns and redundancies for optimization

---

## Patterns Identified

### 1. **Repeated Screen Point Validation** ⚠️ HIGH PRIORITY

**Pattern:**
```cpp
if (screenPoint.isNull() || !qIsFinite(screenPoint.x()) || !qIsFinite(screenPoint.y()))
{
    continue; // or return;
}
```

**Locations Found:**
- `waterfallgraph.cpp:1883` - `updateScatterplotItemsFull()`
- `waterfallgraph.cpp:1913` - `updateScatterplotItemPositions()`
- `waterfallgraph.cpp:2065` - `updateScatterplotItemsIncremental()`
- `waterfallgraph.cpp:2126` - `removeScatterplotItemsOutsideRange()`
- `waterfallgraph.cpp:3318` - `drawDataLine()` (single point case)
- `waterfallgraph.cpp:3370` - `drawDataLine()` (first point check)
- `waterfallgraph.cpp:3400` - `drawDataLine()` (loop)
- `waterfallgraph.cpp:3464` - `drawDataLine()` (plotPoints loop)
- `waterfallgraph.cpp:3482` - `drawDataLine()` (incremental update)
- `waterfallgraph.cpp:3500` - `drawDataLine()` (incremental update)
- `waterfallgraph.cpp:3536` - `drawDataLine()` (incremental update)
- `waterfallgraph.cpp:3586` - `buildBatchedLinePaths()` (first point check)

**Recommendation:**
Create helper method:
```cpp
bool isValidScreenPoint(const QPointF& point) const
{
    return !point.isNull() && qIsFinite(point.x()) && qIsFinite(point.y());
}
```

**Impact:** Eliminates 12+ duplicate validation checks, improves maintainability

---

### 2. **Repeated TimeMin/TimeMax Validation** ⚠️ MEDIUM PRIORITY

**Pattern:**
```cpp
if (!timeMin.isValid() || !timeMax.isValid())
{
    return; // or return QPointF(0, 0);
}
```

**Locations Found:**
- `waterfallgraph.cpp:1197` - `drawBTWSymbols()` (combined check)
- `waterfallgraph.cpp:3047` - `mapDataToScreen(QDateTime)` overload
- `waterfallgraph.cpp:5125` - `isTimeRangeValidForDrawing()`
- `waterfallgraph.cpp:3703` - `drawTimeMarkers()` (topTime/bottomTime)

**Recommendation:**
Create helper method:
```cpp
bool isTimeRangeValid() const
{
    return timeMin.isValid() && timeMax.isValid() && timeMin < timeMax;
}
```

**Impact:** Eliminates duplicate validation logic, consistent behavior

---

### 3. **Repeated DataSource isEmpty Checks** ⚠️ MEDIUM PRIORITY

**Pattern:**
```cpp
if (dataSource && !dataSource->isEmpty())
{
    // ... do work ...
}
```

**Locations Found:**
- `waterfallgraph.cpp:793` - `updateDataRanges()`
- `waterfallgraph.cpp:981` - `paintEvent()` (first check)
- `waterfallgraph.cpp:993` - `paintEvent()` (second check)
- `waterfallgraph.cpp:1001` - `paintEvent()` (third check)
- `waterfallgraph.cpp:1087` - `paintEvent()` (fourth check)
- `waterfallgraph.cpp:3781` - `mapScreenToTime()`

**Recommendation:**
Create helper method:
```cpp
bool hasData() const
{
    return dataSource && !dataSource->isEmpty();
}
```

**Impact:** Eliminates 6+ duplicate checks, clearer intent

---

### 4. **Repeated DrawingArea Validation** ⚠️ MEDIUM PRIORITY

**Pattern:**
```cpp
if (!dataRangesValid || drawingArea.isEmpty())
{
    return; // or return QPointF(0, 0);
}
```

**Locations Found:**
- `waterfallgraph.cpp:2221` - `drawGrid()`
- `waterfallgraph.cpp:3042` - `mapDataToScreen(QDateTime)` overload
- `waterfallgraph.cpp:3111` - `mapDataToScreen(qint64)` overload
- `waterfallgraph.cpp:3781` - `mapScreenToTime()`

**Recommendation:**
Create helper method:
```cpp
bool isDrawingAreaValid() const
{
    return dataRangesValid && !drawingArea.isEmpty();
}
```

**Impact:** Eliminates 4+ duplicate checks, consistent validation

---

### 5. **Repeated toMSecsSinceEpoch() Calls** ⚠️ HIGH PRIORITY

**Locations Found:**
- `waterfallgraph.cpp:1221` - `drawBTWSymbols()` - `symbolData.timestamp.toMSecsSinceEpoch()`
- `waterfallgraph.cpp:1963` - `removeScatterplotItemsOutsideRange()` - `newTimeMin.toMSecsSinceEpoch()`
- `waterfallgraph.cpp:3077` - `mapDataToScreen(QDateTime)` overload - `timestamp.toMSecsSinceEpoch()`
- `waterfallgraph.cpp:5598` - `mapScreenYToTime()` - `time.toMSecsSinceEpoch()`

**Analysis:**
- **Line 1221**: BTW symbols already have `timestamp` as `QDateTime` - could cache epoch in `BTWSymbolData` struct
- **Line 1963**: `newTimeMin` is a parameter - could pass epoch directly or cache if called frequently
- **Line 3077**: This is the `QDateTime` overload - should use the `qint64` overload instead when possible
- **Line 5598**: `time` parameter - could pass epoch directly

**Recommendation:**
1. Add `qint64 timestampEpoch` to `BTWSymbolData` struct (store once, use many times)
2. Update callers to use `mapDataToScreen(qint64)` overload when epoch is available
3. Consider caching epoch values for frequently accessed timestamps

**Impact:** Eliminates expensive timezone conversions in hot paths

---

### 6. **Repeated Data Size Validation** ⚠️ LOW PRIORITY

**Pattern:**
```cpp
if (yData.empty() || timestampsEpoch.empty() || yData.size() != timestampsEpoch.size())
{
    return; // or clear cache
}
```

**Locations Found:**
- `waterfallgraph.cpp:1629` - `updateVisibleDataCacheFull()`
- `waterfallgraph.cpp:1696` - `updateVisibleDataCacheIncremental()`

**Recommendation:**
Create helper method:
```cpp
bool isValidDataSize(const std::vector<float>& yData, const std::vector<qint64>& timestampsEpoch) const
{
    return !yData.empty() && !timestampsEpoch.empty() && yData.size() == timestampsEpoch.size();
}
```

**Impact:** Eliminates duplicate validation, clearer intent

---

### 7. **Repeated Static Cast to qreal** ⚠️ LOW PRIORITY

**Pattern:**
```cpp
static_cast<qreal>(dataPoint.first)
```

**Locations Found:**
- Appears 13+ times in `waterfallgraph.cpp`
- Always used when calling `mapDataToScreen()` with float data

**Analysis:**
This is necessary because `mapDataToScreen()` takes `qreal` but we store `float`. The cast is intentional and correct.

**Recommendation:**
- Keep as-is (necessary conversion)
- Could create inline helper if it becomes verbose:
```cpp
inline qreal toQReal(float val) { return static_cast<qreal>(val); }
```

**Impact:** Minimal - just code clarity, no performance impact

---

### 8. **Repeated Empty Checks Before Iteration** ⚠️ LOW PRIORITY

**Pattern:**
```cpp
if (timestamps.empty() || !minTime.isValid())
{
    return 0;
}
```

**Locations Found:**
- `waterfallgraph.cpp:1577` - `findFirstVisibleIndex()`
- `waterfallgraph.cpp:1597` - `findLastVisibleIndex()`

**Recommendation:**
These are already in helper methods, but could be combined into a single validation helper.

**Impact:** Low - already encapsulated

---

### 9. **Repeated Graphics Item Cleanup Pattern** ⚠️ MEDIUM PRIORITY

**Pattern:**
```cpp
if (item)
{
    // Check if item belongs to this scene before removing
    if (item->scene() == graphicsScene)
    {
        graphicsScene->removeItem(item);
    }
    delete item;
}
```

**Locations Found:**
- `waterfallgraph.cpp:1056-1064` - `paintEvent()` FULL_REDRAW (point items)
- `waterfallgraph.cpp:1820-1830` - `cleanupSeriesItems()` (point items)
- `waterfallgraph.cpp:3295-3305` - `drawDataLine()` FULL_REDRAW (point items)
- `waterfallgraph.cpp:3330-3340` - `drawDataLine()` single point cleanup
- `waterfallgraph.cpp:3443-3453` - `drawDataLine()` plotPoints cleanup
- Similar pattern for path items in multiple locations

**Analysis:**
The pattern of checking `item->scene() == graphicsScene` before removing is repeated many times. This is a safety check to prevent crashes when items have been removed from the scene already.

**Recommendation:**
Create helper method:
```cpp
void safeRemoveAndDeleteItem(QGraphicsItem* item, QGraphicsScene* scene)
{
    if (item)
    {
        if (item->scene() == scene)
        {
            scene->removeItem(item);
        }
        delete item;
    }
}
```

**Impact:** Eliminates 10+ duplicate cleanup blocks, reduces risk of bugs

---

## Priority Summary

### High Priority (Performance Impact)
1. **Repeated toMSecsSinceEpoch() Calls** - Expensive timezone conversions
2. **Repeated Screen Point Validation** - Many duplicate checks

### Medium Priority (Code Quality)
3. **Repeated TimeMin/TimeMax Validation** - Duplicate logic
4. **Repeated DataSource isEmpty Checks** - Multiple locations
5. **Repeated DrawingArea Validation** - Multiple locations

### Low Priority (Code Clarity)
6. **Repeated Data Size Validation** - Already in helper methods
7. **Repeated Static Cast to qreal** - Necessary conversion
8. **Repeated Empty Checks** - Already encapsulated

---

## Recommended Implementation Order

1. **Fix toMSecsSinceEpoch() calls** (High Priority)
   - Add `timestampEpoch` to `BTWSymbolData` struct
   - Update `drawBTWSymbols()` to use cached epoch
   - Update callers to use `qint64` overload when possible

2. **Create screen point validation helper** (High Priority)
   - Add `isValidScreenPoint()` method
   - Replace all duplicate checks

3. **Create time range validation helper** (Medium Priority)
   - Add `isTimeRangeValid()` method
   - Replace duplicate checks

4. **Create data source validation helper** (Medium Priority)
   - Add `hasData()` method
   - Replace duplicate checks

5. **Create drawing area validation helper** (Medium Priority)
   - Add `isDrawingAreaValid()` method
   - Replace duplicate checks

---

## Expected Impact

### Performance
- **Eliminate timezone conversions**: 4+ `toMSecsSinceEpoch()` calls in hot paths
- **Reduce validation overhead**: 12+ screen point validations consolidated

### Code Quality
- **Reduce duplication**: 30+ duplicate checks eliminated
- **Improve maintainability**: Single source of truth for validation logic
- **Better readability**: Clearer intent with named helper methods

---

## Notes

- Some patterns are intentional (e.g., `static_cast<qreal>` is necessary)
- Some patterns are already partially optimized (e.g., helper methods exist but aren't used everywhere)
- Focus on high-priority items first for maximum performance impact

