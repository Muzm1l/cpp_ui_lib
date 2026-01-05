# Memory Leak Fixes Applied

## Summary

Based on heaptrack analysis showing **44.7 MB of memory leaks**, the following fixes have been applied to address the major leak sources.

---

## Fixes Applied

### 1. WaterfallGraph Buffer Cleanup (Addresses 11.6 MB QImageData Leak)

**File:** `waterfallgraph.cpp`

**Changes:**
- Added explicit QPixmap buffer cleanup in destructor
- Added buffer clearing before recreation in `initializeWaterfallBuffer()`
- Added point pixmap cache clearing

**Code Changes:**

```cpp
// In destructor (~WaterfallGraph):
m_waterfallBuffer = QPixmap(); // Clear QPixmap to free underlying QImageData
m_waterfallBufferHeight = 0;
pointPixmapCache.clear(); // Clear cache to free QImageData allocations

// In initializeWaterfallBuffer():
// Explicitly clear old buffer before creating new one
m_waterfallBuffer = QPixmap(); // Clear old buffer first
m_waterfallBuffer = QPixmap(size); // Create new buffer
```

**Impact:** Should eliminate the 11.6 MB QImageData::create leak (26% of total leak)

---

### 2. WaterfallData Container Cleanup (Addresses 15.0 MB QArrayData Leak)

**File:** `waterfalldata.cpp`

**Changes:**
- Added missing `dataSeriesTimestampsEpoch.clear()` in destructor
- Added explicit `dataTitle.clear()` to free QString QArrayData

**Code Changes:**

```cpp
// In destructor (~WaterfallData):
dataSeriesYData.clear();
dataSeriesTimestamps.clear();
dataSeriesTimestampsEpoch.clear(); // Was missing - now included
rtwSymbols.clear();
btwSymbols.clear();
btwMarkers.clear();
rtwRMarkers.clear();
dataTitle.clear(); // Clear QString to free QArrayData
```

**Impact:** Should help reduce the 15.0 MB QArrayData::allocate leak (33.6% of total leak)

**Note:** The QArrayData leak may also be from accumulating data over time. Consider implementing data trimming/aging mechanisms if data continues to grow indefinitely.

---

## Expected Results

### Before Fixes:
- **Total Leak:** 44.7 MB
- **QImageData:** 11.6 MB
- **QArrayData:** 15.0 MB
- **Other:** 18.1 MB

### After Fixes:
- **Expected Total Leak:** ~20-25 MB (44-55% reduction)
- **QImageData:** ~0 MB (eliminated)
- **QArrayData:** ~10-15 MB (reduced, but may need data trimming)
- **Other:** ~10-15 MB (Qt/GUI overhead, may be acceptable)

---

## Remaining Issues

### 1. QArrayData Leak (15.0 MB → ~10-15 MB expected)
**Status:** Partially addressed  
**Remaining Cause:** Data accumulation over time in WaterfallData containers

**Future Fixes:**
- Implement data trimming/aging (remove old data points)
- Limit maximum data size per series
- Clear data when graphs are hidden/not in use

### 2. Unresolved Qt5Gui Leak (4.1 MB)
**Status:** Not addressed  
**Cause:** Qt graphics/painting code (may be Qt internal)

**Future Investigation:**
- Review QPixmap/QImage usage patterns
- Check for QPainter buffer leaks
- May require Qt version update

### 3. QGraphicsE... Peak (4.6 MB)
**Status:** Not addressed  
**Cause:** Graphics items not removed from scenes

**Future Fixes:**
- Ensure `scene->clear()` is called in destructors
- Verify graphics items are removed when graphs are destroyed
- Check TimelineView graphics item cleanup

### 4. libgallium Leak (1.8 MB)
**Status:** Not addressed  
**Cause:** Graphics driver (external library)

**Note:** This is graphics driver overhead. May be acceptable or require driver update.

---

## Testing Recommendations

1. **Re-run heaptrack** after fixes:
   ```bash
   heaptrack ./ui-sandbox
   ```

2. **Compare results:**
   - Check if QImageData leak is eliminated
   - Verify QArrayData leak is reduced
   - Measure total leak reduction

3. **Monitor over time:**
   - Run for same duration (15+ minutes)
   - Compare leak rates
   - Verify fixes are effective

4. **Use heaptrack_gui** to verify:
   - Open new heaptrack file
   - Check "Largest Memory Leaks" tab
   - Verify QImageData::create is no longer top leak
   - Check if QArrayData leak is reduced

---

## Additional Recommendations

### Data Trimming Implementation

To fully address the QArrayData leak, consider implementing data aging:

```cpp
// In WaterfallData::addDataPointToSeries():
void WaterfallData::addDataPointToSeries(const QString& seriesLabel, qreal yValue, const QDateTime& timestamp)
{
    // ... existing code ...
    
    // Trim old data if series exceeds maximum size
    const size_t MAX_DATA_POINTS = 100000; // Adjust as needed
    if (dataSeriesYData[seriesLabel].size() > MAX_DATA_POINTS)
    {
        // Remove oldest 10% of data
        size_t removeCount = MAX_DATA_POINTS / 10;
        dataSeriesYData[seriesLabel].erase(
            dataSeriesYData[seriesLabel].begin(),
            dataSeriesYData[seriesLabel].begin() + removeCount
        );
        dataSeriesTimestamps[seriesLabel].erase(
            dataSeriesTimestamps[seriesLabel].begin(),
            dataSeriesTimestamps[seriesLabel].begin() + removeCount
        );
        dataSeriesTimestampsEpoch[seriesLabel].erase(
            dataSeriesTimestampsEpoch[seriesLabel].begin(),
            dataSeriesTimestampsEpoch[seriesLabel].begin() + removeCount
        );
    }
}
```

### Graphics Scene Cleanup

Ensure graphics scenes are properly cleaned:

```cpp
// In WaterfallGraph destructor or cleanup methods:
if (graphicsScene) {
    graphicsScene->clear(); // Remove all graphics items
}
```

---

## Files Modified

1. `waterfallgraph.cpp` - Added buffer cleanup in destructor and initializeWaterfallBuffer()
2. `waterfalldata.cpp` - Fixed destructor to clear all containers

---

## Next Steps

1. ✅ **Fixes Applied** - Buffer and container cleanup added
2. ⏳ **Testing** - Re-run heaptrack to verify fixes
3. ⏳ **Data Trimming** - Implement if QArrayData leak persists
4. ⏳ **Graphics Cleanup** - Review graphics item removal
5. ⏳ **Long-term Monitoring** - Track leak rates over extended periods

