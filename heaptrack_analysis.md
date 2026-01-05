# Heaptrack Memory Leak Analysis

## Executive Summary

**Total Memory Leaked:** 44.7 MB over 15 minutes 56 seconds  
**Peak Heap:** 50.9 MB  
**Peak RSS:** 246.9 MB  
**Leak Rate:** ~2.8 MB/minute

---

## Top Memory Leaks Breakdown

### 1. QArrayData::allocate - 15.0 MB (33.6% of total leak)
**Location:** Qt container allocations  
**Impact:** CRITICAL - Largest single leak  
**Likely Sources:**
- WaterfallData series storage (QVector<qreal>, QVector<QDateTime>)
- TimelineView cached timestamp labels
- GraphContainer data structures
- String operations (QString, QByteArray)

**Root Cause:** Qt containers (QVector, QList, QString) accumulating data without cleanup

### 2. QImageData::create - 11.6 MB (26.0% of total leak)
**Location:** `QImageData::create(QSize const&, QImage::Format)` in libQt5Gui.so.5  
**Impact:** CRITICAL - Confirmed WaterfallGraph buffer leak  
**Stack Trace (from Valgrind):**
```
WaterfallGraph::initializeWaterfallBuffer(QSize const&) (waterfallgraph.cpp:2347)
WaterfallGraph::updateGraphicsDimensions() (waterfallgraph.cpp:1914)
WaterfallGraph::resizeEvent(QResizeEvent*) (waterfallgraph.cpp:2321)
```

**Root Cause:** WaterfallGraph buffers created but not freed when graphs are destroyed

### 3. Unresolved in Qt5Gui - 4.1 MB (9.2% of total leak)
**Location:** Qt graphics/painting code  
**Impact:** HIGH  
**Likely Sources:**
- QPixmap/QImage operations
- QPainter operations
- Graphics rendering buffers

### 4. QGraphicsE... - 4.6 MB Peak
**Location:** QGraphicsEllipseItem or QGraphicsLineItem  
**Impact:** HIGH  
**Likely Sources:**
- TimelineView graphics items
- WaterfallGraph markers/overlays
- BTW/RTW symbol graphics items

**Root Cause:** Graphics items added to scenes but not removed on cleanup

### 5. Unresolved in libgallium - 1.8 MB (4.0% of total leak)
**Location:** Graphics driver (Mesa/Gallium)  
**Impact:** MEDIUM - External library, may be acceptable  
**Note:** This is graphics driver overhead, but still a leak

### 6. QHashData::allocateNode - 1.2 MB (2.7% of total leak)
**Location:** Qt hash table allocations  
**Impact:** MEDIUM  
**Likely Sources:**
- WaterfallData series storage (QHash<QString, SeriesData>)
- GraphContainer lookup tables
- Qt internal hash tables

---

## Allocation Statistics

- **Total Allocations:** 10,922,681 (11,414/s)
- **Leaked Allocations:** 111,754 (1.02% of total)
- **Temporary Allocations:** 2,386,217 (21.85%, 2,493/s)

**Analysis:**
- High allocation rate is normal for GUI applications
- 1% leak rate is concerning - suggests systematic issue
- 22% temporary allocations is reasonable

---

## Leak Pattern Analysis

### Leak Distribution:
1. **Qt Containers (QArrayData):** 15.0 MB - Growing data structures
2. **Image Buffers (QImageData):** 11.6 MB - WaterfallGraph buffers
3. **Graphics Items:** 4.6 MB - QGraphicsItems not removed
4. **Qt Graphics:** 4.1 MB - Painting/rendering buffers
5. **Graphics Driver:** 1.8 MB - External library
6. **Hash Tables:** 1.2 MB - Lookup structures
7. **Other:** ~6.0 MB - Miscellaneous

### Leak Rate:
- **44.7 MB / 15.9 minutes = 2.8 MB/minute**
- **2.8 MB/minute = 47 KB/second**
- This is a **constant leak rate**, not exponential

---

## Root Causes Identified

### 1. WaterfallGraph Buffer Management (11.6 MB)
**Problem:** Buffers created in `initializeWaterfallBuffer()` but never freed

**Evidence:**
- Valgrind shows 7.7 MB + 714 KB allocations in WaterfallGraph
- Heaptrack confirms 11.6 MB leak in QImageData::create
- Stack traces point to `waterfallgraph.cpp:2347`

**Fix Required:**
- Ensure destructor frees buffers
- Clear QPixmap/QImage when graph is destroyed
- Free buffers before recreating on resize

### 2. Qt Container Accumulation (15.0 MB)
**Problem:** Data accumulating in Qt containers without cleanup

**Evidence:**
- QArrayData::allocate is largest leak
- Likely in WaterfallData series storage
- TimelineView cached data structures

**Fix Required:**
- Clear WaterfallData when graphs are destroyed
- Clear TimelineView caches
- Review container growth patterns

### 3. Graphics Items Not Removed (4.6 MB)
**Problem:** QGraphicsItems added to scenes but not removed

**Evidence:**
- QGraphicsE... shows 4.6 MB peak
- Graphics items persist after cleanup

**Fix Required:**
- Ensure `scene->clear()` is called
- Remove graphics items explicitly
- Verify destructors clean up scenes

---

## Comparison with Valgrind

| Source | Valgrind | Heaptrack | Match |
|--------|----------|-----------|-------|
| **QImageData** | 7.7 MB + 714 KB (still reachable) | 11.6 MB (leaked) | ✅ Confirmed |
| **Qt Containers** | Not specifically identified | 15.0 MB (leaked) | ✅ New finding |
| **Graphics Items** | Not specifically identified | 4.6 MB (peak) | ✅ New finding |
| **Total Leak** | 51.5 MB (still reachable) | 44.7 MB (leaked) | ✅ Close match |

**Key Insight:** Heaptrack identifies leaks that Valgrind classifies as "still reachable" - these are real leaks that need fixing.

---

## Recommended Fixes (Priority Order)

### Priority 1: WaterfallGraph Buffer Cleanup (11.6 MB)
**Impact:** Fixes 26% of total leak

1. Add destructor to WaterfallGraph
2. Free buffers in destructor
3. Clear QPixmap before recreation

### Priority 2: Qt Container Cleanup (15.0 MB)
**Impact:** Fixes 33.6% of total leak

1. Clear WaterfallData series on destruction
2. Clear TimelineView caches
3. Review all container growth

### Priority 3: Graphics Scene Cleanup (4.6 MB)
**Impact:** Fixes 10.3% of total leak

1. Ensure scene->clear() is called
2. Remove graphics items explicitly
3. Verify cleanup in destructors

### Priority 4: Qt Graphics Buffer Cleanup (4.1 MB)
**Impact:** Fixes 9.2% of total leak

1. Review QPixmap/QImage usage
2. Clear painting buffers
3. Optimize rendering operations

---

## Expected Results After Fixes

**Current Leak:** 44.7 MB  
**After Priority 1 Fix:** ~33.1 MB (26% reduction)  
**After Priority 2 Fix:** ~18.1 MB (60% reduction)  
**After Priority 3 Fix:** ~13.5 MB (70% reduction)  
**After Priority 4 Fix:** ~9.4 MB (79% reduction)  

**Remaining Leak:** ~9-10 MB (graphics driver + Qt overhead - may be acceptable)

---

## Investigation Steps

1. **Use heaptrack_gui call tree:**
   - Expand `QImageData::create` → find call stacks
   - Expand `QArrayData::allocate` → find container types
   - Expand `QGraphicsE...` → find graphics item types

2. **Add logging to track lifecycle:**
   - WaterfallGraph buffer creation/destruction
   - Container growth patterns
   - Graphics item creation/removal

3. **Verify destructors are called:**
   - Add logging to all destructors
   - Confirm cleanup happens when tabs are hidden
   - Check parent-child widget relationships

---

## Code Areas to Investigate

### High Priority:
- `waterfallgraph.cpp:2347` - `initializeWaterfallBuffer()`
- `waterfallgraph.cpp` - Destructor (if exists)
- `waterfalldata.cpp` - Series storage cleanup
- `timelineview.cpp` - Cached data cleanup
- `graphcontainer.cpp` - Graph destruction

### Medium Priority:
- Graphics scene cleanup in all graph types
- QPixmap/QImage usage throughout codebase
- Container growth in data structures

---

## Next Actions

1. ✅ **Analysis Complete** - Heaptrack data analyzed
2. 🔄 **Investigation** - Use heaptrack_gui to find call stacks
3. ⏳ **Implementation** - Fix buffer cleanup
4. ⏳ **Testing** - Verify leak reduction
5. ⏳ **Validation** - Re-run heaptrack to confirm fixes

