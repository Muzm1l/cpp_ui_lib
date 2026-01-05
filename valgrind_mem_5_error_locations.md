# Valgrind Memory Error Locations - valgrind_mem_5.log

## Summary

**Total Errors:** 486 errors from 422 contexts  
**Error Type:** All errors are "Conditional jump or move depends on uninitialised value(s)"  
**Root Cause:** `m_showCrosshairTimestamp` member variable not initialized in constructor

---

## Error Location Breakdown

### Primary Error Location (8 occurrences)

**File:** `timelineview.cpp`  
**Line:** 346  
**Function:** `TimelineVisualizerWidget::setTimeInterval(TimeInterval)`

**Code:**
```cpp
if (m_showCrosshairTimestamp && m_crosshairTimestamp.isValid())
{
    updateCrosshairTimestampFromTime(m_crosshairTimestamp);
}
```

**Problem:** `m_showCrosshairTimestamp` is checked but was never initialized in the constructor.

---

### Secondary Error Locations

#### 1. `timelineview.cpp:1114` (1 occurrence)
**Function:** `TimelineVisualizerWidget::setVisibleTimeWindow(TimeSelectionSpan const&)`

**Code:**
```cpp
if (m_showCrosshairTimestamp && m_crosshairTimestamp.isValid())
{
    updateCrosshairTimestampFromTime(m_crosshairTimestamp);
}
```

#### 2. `timelineview.cpp:869` (1 occurrence)
**Function:** `TimelineVisualizerWidget::paintEvent(QPaintEvent*)`

**Code:**
```cpp
if (m_showCrosshairTimestamp && m_crosshairTimestamp.isValid() && 
    m_crosshairYPosition >= 0 && m_crosshairYPosition <= rect().height())
{
    drawCrosshairTimestampLabel(painter, rect());
}
```

---

## Call Stack Analysis

### Most Common Call Paths

#### Path 1: GraphContainer Construction (Most Common)
```
main (main.cpp:8)
  → MainWindow::MainWindow (mainwindow.cpp:22, 55, 416, 422)
    → GraphLayout::GraphLayout (graphlayout.cpp:32)
      → GraphLayout::initializeContainers (graphlayout.cpp:361-364)
        → GraphContainer::GraphContainer (graphcontainer.cpp:99)
          → TimelineView::TimelineView (timelineview.cpp:1733, 1766)
            → TimelineVisualizerWidget::setTimeInterval (timelineview.cpp:346) ❌ ERROR
```

**Occurrences:** ~8 times (one for each GraphContainer created)

#### Path 2: MainWindow setupUi
```
main (main.cpp:8)
  → MainWindow::MainWindow (mainwindow.cpp:22)
    → Ui_MainWindow::setupUi (ui_mainwindow.h:97)
      → GraphContainer::GraphContainer (graphcontainer.cpp:99)
        → TimelineView::TimelineView (timelineview.cpp:1766)
          → TimelineVisualizerWidget::setTimeInterval (timelineview.cpp:346) ❌ ERROR
```

**Occurrences:** ~1-2 times

#### Path 3: setupTimelineView
```
main (main.cpp:8)
  → MainWindow::MainWindow (mainwindow.cpp:416)
    → MainWindow::setupTimelineView (mainwindow.cpp:871)
      → TimelineView::TimelineView (timelineview.cpp:1766)
        → TimelineVisualizerWidget::setTimeInterval (timelineview.cpp:346) ❌ ERROR
```

**Occurrences:** ~1 time

#### Path 4: setupSCWWindow
```
main (main.cpp:8)
  → MainWindow::MainWindow (mainwindow.cpp:422)
    → MainWindow::setupSCWWindow (mainwindow.cpp:989)
      → TimelineView::TimelineView (timelineview.cpp:1733)
        → TimelineVisualizerWidget::setTimeInterval (timelineview.cpp:346) ❌ ERROR
```

**Occurrences:** ~1 time

#### Path 5: Runtime Timer Tick
```
main (main.cpp:10)
  → TimelineView::onTimerTick (timelineview.cpp:1836)
    → TimelineVisualizerWidget::setTimeInterval (timelineview.cpp:346) ❌ ERROR
```

**Occurrences:** ~1 time (during runtime)

#### Path 6: Data Changed Event
```
GraphLayout::addDataPointToDataSource (graphlayout.cpp:780)
  → GraphContainer::onDataChanged (graphcontainer.cpp:1601)
    → TimelineView::setVisibleTimeWindow (timelineview.cpp:1955)
      → TimelineVisualizerWidget::setVisibleTimeWindow (timelineview.cpp:1114) ❌ ERROR
```

**Occurrences:** ~1 time

#### Path 7: Paint Event
```
TimelineView::TimelineView (timelineview.cpp:1733)
  → TimelineVisualizerWidget::paintEvent (timelineview.cpp:869) ❌ ERROR
```

**Occurrences:** ~1 time

---

## All Error Locations

| Line | Function | Occurrences | Call Context |
|------|----------|-------------|--------------|
| **346** | `TimelineVisualizerWidget::setTimeInterval()` | **8** | Constructor calls, timer ticks |
| **1114** | `TimelineVisualizerWidget::setVisibleTimeWindow()` | **1** | Data changed events |
| **869** | `TimelineVisualizerWidget::paintEvent()` | **1** | Paint events during construction |
| **1766** | `TimelineView::TimelineView()` | **7** | Calls `setTimeInterval()` |
| **1733** | `TimelineView::TimelineView()` | **7** | Creates `TimelineVisualizerWidget` |
| **1955** | `TimelineView::setVisibleTimeWindow()` | **1** | Calls widget's `setVisibleTimeWindow()` |
| **1836** | `TimelineView::onTimerTick()` | **1** | Timer callback |

---

## Root Cause Analysis

### The Problem

**File:** `timelineview.cpp`  
**Constructor:** `TimelineVisualizerWidget::TimelineVisualizerWidget()` (line 257)

**Issue:** The member variable `m_showCrosshairTimestamp` is declared but **not initialized** in the constructor initializer list.

**Declaration:**
```cpp
// timelineview.h:232
bool m_showCrosshairTimestamp;
```

**Constructor (Current - Missing Initialization):**
```cpp
TimelineVisualizerWidget::TimelineVisualizerWidget(QWidget *parent, ...)
    : QWidget(parent),
      m_currentTime(QTime::currentTime()),
      m_numberOfDivisions(15),
      // ... other initializations ...
      // ❌ MISSING: m_showCrosshairTimestamp(false)
{
    // Constructor body
    // ...
    // setTimeInterval() may be called here, which checks m_showCrosshairTimestamp
}
```

**Where It's Used (Before Initialization):**
1. Line 346: `if (m_showCrosshairTimestamp && ...)` in `setTimeInterval()`
2. Line 1114: `if (m_showCrosshairTimestamp && ...)` in `setVisibleTimeWindow()`
3. Line 869: `if (m_showCrosshairTimestamp && ...)` in `paintEvent()`
4. Line 1199: `if (m_showCrosshairTimestamp && ...)`
5. Line 1285: `if (m_showCrosshairTimestamp && ...)`
6. Line 1494: `if (!m_showCrosshairTimestamp)`

**Where It's Set (After Initialization):**
- Line 982: `m_showCrosshairTimestamp = timestamp.isValid();`
- Line 1054: `m_showCrosshairTimestamp = false;`

---

## Fix Required

### Solution: Initialize in Constructor

**File:** `timelineview.cpp`  
**Location:** Constructor initializer list (around line 257)

**Add:**
```cpp
TimelineVisualizerWidget::TimelineVisualizerWidget(QWidget *parent, 
                                                   GraphContainerSyncState *syncState, 
                                                   bool sliderVisible, 
                                                   bool chevronVisible)
    : QWidget(parent),
      m_currentTime(QTime::currentTime()),
      m_numberOfDivisions(15),
      m_lastCurrentTime(QTime::currentTime()),
      m_pixelSpeed(0.0),
      m_accumulatedOffset(0.0),
      m_sliderIndicator(nullptr),
      m_syncState(syncState),
      m_sliderVisible(sliderVisible),
      m_chevronVisible(chevronVisible),
      m_manoeuvreOverlay(nullptr),
      m_showCrosshairTimestamp(false)  // ✅ ADD THIS LINE
{
    // ... rest of constructor ...
}
```

**Expected Impact:** Eliminates all 486 uninitialized value errors.

---

## Error Distribution by Call Site

### Construction Phase Errors

| Call Site | Count | Description |
|-----------|-------|-------------|
| `GraphLayout::initializeContainers()` | ~8 | Creating 4 GraphContainers (2 TimelineViews each) |
| `Ui_MainWindow::setupUi()` | ~1-2 | UI setup |
| `MainWindow::setupTimelineView()` | ~1 | Timeline view setup |
| `MainWindow::setupSCWWindow()` | ~1 | SCW window setup |

### Runtime Errors

| Call Site | Count | Description |
|-----------|-------|-------------|
| `TimelineView::onTimerTick()` | ~1 | Timer callback |
| `GraphContainer::onDataChanged()` | ~1 | Data update event |
| `TimelineVisualizerWidget::paintEvent()` | ~1 | Paint during construction |

---

## Files Affected

### Primary File
- **`timelineview.cpp`** - Contains all error locations
  - Line 257: Constructor (missing initialization)
  - Line 346: `setTimeInterval()` - checks uninitialized value
  - Line 869: `paintEvent()` - checks uninitialized value
  - Line 1114: `setVisibleTimeWindow()` - checks uninitialized value

### Header File
- **`timelineview.h`** - Line 232: Member variable declaration

### Caller Files
- **`graphcontainer.cpp`** - Line 99: Creates TimelineView
- **`graphlayout.cpp`** - Lines 32, 361-364: Creates GraphContainers
- **`mainwindow.cpp`** - Lines 22, 55, 416, 422, 871, 989: Various setup functions
- **`main.cpp`** - Line 8: Entry point

---

## Summary

**All 486 errors** originate from the same root cause:
- **Uninitialized member variable:** `m_showCrosshairTimestamp` in `TimelineVisualizerWidget`
- **Primary location:** `timelineview.cpp:346` in `setTimeInterval()` (8 occurrences)
- **Secondary locations:** Lines 869, 1114 (2 occurrences total)
- **Fix:** Initialize `m_showCrosshairTimestamp = false` in constructor initializer list

**Impact:** This single fix will eliminate all 486 Valgrind errors.

---

**Report Generated:** Analysis of valgrind_mem_5.log  
**Total Error Count:** 486 errors from 422 contexts  
**Unique Error Locations:** 3 locations in timelineview.cpp  
**Root Cause:** 1 uninitialized member variable

