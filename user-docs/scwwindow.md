# SCWWindow Implementation Guide

## Overview

`SCWWindow` is a specialized Qt widget that displays a horizontal layout containing:
- One `TimelineView` (with history slider and chevron disabled)
- Seven vertical layouts, each containing:
  - A label button
  - A `WaterfallGraph` for displaying time-series data

The widget manages multiple data series organized into four categories (R, B, A, E) and provides APIs for adding and setting data points. Windows 1-4 display fixed RULER series, while windows 5-7 can cycle through different series when their buttons are clicked.

## Table of Contents

1. [Architecture](#architecture)
2. [Series Types and Enums](#series-types-and-enums)
3. [API Reference](#api-reference)
4. [Events and Signals](#events-and-signals)
5. [Usage Guide](#usage-guide)
6. [Integration with MainWindow](#integration-with-mainwindow)
7. [Simulator Integration](#simulator-integration)
8. [Window Selection](#window-selection)

---

## Architecture

### Layout Structure

```
SCWWindow
├── QHBoxLayout (main layout)
    ├── TimelineView (fixed width, expanding height)
    └── QFrame[0-6] (7 container frames, each with expanding size policy)
        └── QVBoxLayout
            ├── QPushButton (label button, fixed height 30px)
            └── WaterfallGraph (expanding, fills remaining space)
```

### Data Source Management

Each series type has its own `QMap` storing `WaterfallData*` instances:
- `m_dataSourcesR`: Maps `SCW_SERIES_R` → `WaterfallData*` (4 series)
- `m_dataSourcesB`: Maps `SCW_SERIES_B` → `WaterfallData*` (5 series)
- `m_dataSourcesA`: Maps `SCW_SERIES_A` → `WaterfallData*` (2 series)
- `m_dataSourcesE`: Maps `SCW_SERIES_E` → `WaterfallData*` (5 series)

**Total: 16 data sources** (4 + 5 + 2 + 5)

### Window Configuration

| Window | Series Type | Behavior | Series Options |
|--------|-------------|----------|----------------|
| 1 | RULER_1 | Fixed | RULER_1 only |
| 2 | RULER_2 | Fixed | RULER_2 only |
| 3 | RULER_3 | Fixed | RULER_3 only |
| 4 | RULER_4 | Fixed | RULER_4 only |
| 5 | SCW_SERIES_B | Cycling | BRAT, BOT, BFT, BOPT, BOTC |
| 6 | SCW_SERIES_A | Cycling | ATMA, ATMAF |
| 7 | SCW_SERIES_E | Cycling | EXTERNAL1-5 |

---

## Series Types and Enums

### SCW_SERIES_R (RULER Series)

Fixed series displayed in windows 1-4.

```cpp
enum class SCW_SERIES_R
{
    RULER_1,  // Window 1
    RULER_2,  // Window 2
    RULER_3,  // Window 3
    RULER_4,  // Window 4
};
```

**Helper Functions:**
- `QString scwSeriesRToString(SCW_SERIES_R series)` - Converts enum to string
- `SCW_SERIES_R stringToScwSeriesR(const QString &str)` - Converts string to enum

### SCW_SERIES_B

Cycling series for window 5.

```cpp
enum class SCW_SERIES_B
{
    BRAT,   // Bit Rate Average Time
    BOT,    // Bit Over Time
    BFT,    // Bit Frequency Time
    BOPT,   // Bit Optimal Time
    BOTC,   // Bit Over Time Count
};
```

**Helper Functions:**
- `QString scwSeriesBToString(SCW_SERIES_B series)`
- `SCW_SERIES_B stringToScwSeriesB(const QString &str)`

### SCW_SERIES_A

Cycling series for window 6.

```cpp
enum class SCW_SERIES_A
{
    ATMA,   // ATM Average
    ATMAF,  // ATM Average Frequency
};
```

**Helper Functions:**
- `QString scwSeriesAToString(SCW_SERIES_A series)`
- `SCW_SERIES_A stringToScwSeriesA(const QString &str)`

### SCW_SERIES_E (External Series)

Cycling series for window 7.

```cpp
enum class SCW_SERIES_E
{
    EXTERNAL1,
    EXTERNAL2,
    EXTERNAL3,
    EXTERNAL4,
    EXTERNAL5,
};
```

**Helper Functions:**
- `QString scwSeriesEToString(SCW_SERIES_E series)`
- `SCW_SERIES_E stringToScwSeriesE(const QString &str)`

---

## API Reference

### Constructor

```cpp
explicit SCWWindow(QWidget *parent = nullptr, QTimer *timer = nullptr);
```

**Parameters:**
- `parent`: Parent widget (typically a tab widget)
- `timer`: QTimer instance for TimelineView synchronization (optional, can be nullptr)

**Description:**
Creates a new `SCWWindow` instance, initializes all data sources, sets up the layout, and configures all waterfall graphs. The widget is configured to expand and fill available space.

**Example:**
```cpp
QTimer* timer = new QTimer(this);
timer->setInterval(1000); // 1 second
SCWWindow* scwWindow = new SCWWindow(parentWidget, timer);
```

---

### Data Management APIs

#### setDataPoints (RULER Series)

```cpp
void setDataPoints(SCW_SERIES_R series, 
                   const std::vector<qreal> &yData, 
                   const std::vector<QDateTime> &timestamps);
```

**Parameters:**
- `series`: The RULER series enum (RULER_1, RULER_2, RULER_3, or RULER_4)
- `yData`: Vector of data values (qreal)
- `timestamps`: Vector of QDateTime timestamps (must match yData size)

**Description:**
Replaces all existing data for the specified RULER series with the new data. The corresponding WaterfallGraph is updated and redrawn.

**Example:**
```cpp
std::vector<qreal> values = {10.5, 20.3, 30.7, 40.2};
std::vector<QDateTime> times = {
    QDateTime::currentDateTime().addSecs(-30),
    QDateTime::currentDateTime().addSecs(-20),
    QDateTime::currentDateTime().addSecs(-10),
    QDateTime::currentDateTime()
};
scwWindow->setDataPoints(SCW_SERIES_R::RULER_1, values, times);
```

#### addDataPoints (RULER Series)

```cpp
void addDataPoints(SCW_SERIES_R series, 
                   const std::vector<qreal> &yData, 
                   const std::vector<QDateTime> &timestamps);
```

**Parameters:**
- `series`: The RULER series enum
- `yData`: Vector of new data values to append
- `timestamps`: Vector of QDateTime timestamps

**Description:**
Appends new data points to the existing data for the specified RULER series. The graph performs an incremental redraw.

**Example:**
```cpp
std::vector<qreal> newValues = {50.1};
std::vector<QDateTime> newTimes = {QDateTime::currentDateTime()};
scwWindow->addDataPoints(SCW_SERIES_R::RULER_2, newValues, newTimes);
```

#### setDataPoints (SCW_SERIES_B)

```cpp
void setDataPoints(SCW_SERIES_B series, 
                   const std::vector<qreal> &yData, 
                   const std::vector<QDateTime> &timestamps);
```

**Parameters:**
- `series`: The B series enum (BRAT, BOT, BFT, BOPT, or BOTC)
- `yData`: Vector of data values
- `timestamps`: Vector of QDateTime timestamps

**Description:**
Replaces all existing data for the specified B series. If the series is currently displayed in window 5, the graph is updated immediately. Otherwise, the data source is updated but the graph won't refresh until that series is selected.

**Example:**
```cpp
std::vector<qreal> values = {15.0, 25.0, 35.0};
std::vector<QDateTime> times = {
    QDateTime::currentDateTime().addSecs(-20),
    QDateTime::currentDateTime().addSecs(-10),
    QDateTime::currentDateTime()
};
scwWindow->setDataPoints(SCW_SERIES_B::BRAT, values, times);
```

#### addDataPoints (SCW_SERIES_B)

```cpp
void addDataPoints(SCW_SERIES_B series, 
                   const std::vector<qreal> &yData, 
                   const std::vector<QDateTime> &timestamps);
```

**Description:**
Appends new data points to the specified B series. If the series is currently displayed in window 5, the graph performs an incremental redraw.

#### setDataPoints (SCW_SERIES_A)

```cpp
void setDataPoints(SCW_SERIES_A series, 
                   const std::vector<qreal> &yData, 
                   const std::vector<QDateTime> &timestamps);
```

**Parameters:**
- `series`: The A series enum (ATMA or ATMAF)
- `yData`: Vector of data values
- `timestamps`: Vector of QDateTime timestamps

**Description:**
Replaces all existing data for the specified A series. If the series is currently displayed in window 6, the graph is updated immediately.

#### addDataPoints (SCW_SERIES_A)

```cpp
void addDataPoints(SCW_SERIES_A series, 
                   const std::vector<qreal> &yData, 
                   const std::vector<QDateTime> &timestamps);
```

**Description:**
Appends new data points to the specified A series. If the series is currently displayed in window 6, the graph performs an incremental redraw.

#### setDataPoints (SCW_SERIES_E)

```cpp
void setDataPoints(SCW_SERIES_E series, 
                   const std::vector<qreal> &yData, 
                   const std::vector<QDateTime> &timestamps);
```

**Parameters:**
- `series`: The E series enum (EXTERNAL1 through EXTERNAL5)
- `yData`: Vector of data values
- `timestamps`: Vector of QDateTime timestamps

**Description:**
Replaces all existing data for the specified E series. If the series is currently displayed in window 7, the graph is updated immediately.

#### addDataPoints (SCW_SERIES_E)

```cpp
void addDataPoints(SCW_SERIES_E series, 
                   const std::vector<qreal> &yData, 
                   const std::vector<QDateTime> &timestamps);
```

**Description:**
Appends new data points to the specified E series. If the series is currently displayed in window 7, the graph performs an incremental redraw.

---

## Events and Signals

### seriesSelected Signal

```cpp
void seriesSelected(const QString &seriesName);
```

**Description:**
Emitted when a window is selected (either by clicking the graph or clicking the button for windows 1-4). The signal carries the name of the currently displayed series.

**Parameters:**
- `seriesName`: QString containing the series name (e.g., "RULER_1", "BRAT", "ATMA", "EXTERNAL1")

**Example Connection:**
```cpp
connect(scwWindow, &SCWWindow::seriesSelected, 
        [](const QString &seriesName) {
            qDebug() << "Selected series:" << seriesName;
            // Handle selection change
        });
```

**Series Name Values:**
- Windows 1-4: "RULER_1", "RULER_2", "RULER_3", "RULER_4"
- Window 5: "BRAT", "BOT", "BFT", "BOPT", or "BOTC" (depending on current cycle)
- Window 6: "ATMA" or "ATMAF" (depending on current cycle)
- Window 7: "EXTERNAL1", "EXTERNAL2", "EXTERNAL3", "EXTERNAL4", or "EXTERNAL5" (depending on current cycle)

---

## Usage Guide

### Basic Setup

#### Step 1: Create SCWWindow

```cpp
#include "scwwindow.h"

// In your MainWindow or parent widget
QTimer* timeUpdateTimer = new QTimer(this);
timeUpdateTimer->setInterval(1000); // 1 second
timeUpdateTimer->start();

SCWWindow* scwWindow = new SCWWindow(parentWidget, timeUpdateTimer);
scwWindow->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
```

#### Step 2: Add to Layout

```cpp
QVBoxLayout* layout = new QVBoxLayout(parentWidget);
layout->setContentsMargins(0, 0, 0, 0);
layout->addWidget(scwWindow);
```

#### Step 3: Connect to Selection Signal (Optional)

```cpp
connect(scwWindow, &SCWWindow::seriesSelected, 
        this, &MyClass::onSeriesSelected);

// Handler implementation
void MyClass::onSeriesSelected(const QString &seriesName)
{
    qDebug() << "User selected series:" << seriesName;
    // Update UI, log selection, etc.
}
```

### Adding Data Points

#### Single Data Point

```cpp
// Add a single point to RULER_1
std::vector<qreal> value = {42.5};
std::vector<QDateTime> timestamp = {QDateTime::currentDateTime()};
scwWindow->addDataPoints(SCW_SERIES_R::RULER_1, value, timestamp);
```

#### Multiple Data Points

```cpp
// Add multiple points at once
std::vector<qreal> values = {10.0, 20.0, 30.0, 40.0, 50.0};
std::vector<QDateTime> timestamps;
QDateTime baseTime = QDateTime::currentDateTime();
for (int i = 0; i < 5; ++i) {
    timestamps.push_back(baseTime.addSecs(-i * 10));
}
scwWindow->addDataPoints(SCW_SERIES_B::BRAT, values, timestamps);
```

#### Batch Data Update

```cpp
// Replace all data for a series
std::vector<qreal> allValues;
std::vector<QDateTime> allTimestamps;
QDateTime startTime = QDateTime::currentDateTime().addSecs(-900); // 15 minutes ago

for (int i = 0; i < 100; ++i) {
    allValues.push_back(25.0 + 10.0 * std::sin(i * 0.1)); // Sine wave pattern
    allTimestamps.push_back(startTime.addSecs(i * 9)); // 9 second intervals
}

scwWindow->setDataPoints(SCW_SERIES_R::RULER_3, allValues, allTimestamps);
```

### Cycling Through Series

Windows 5, 6, and 7 allow users to cycle through different series by clicking their label buttons:

**Window 5 (SCW_SERIES_B):**
- Clicking the button cycles through: BRAT → BOT → BFT → BOPT → BOTC → BRAT (loop)
- The button text updates to show the current series
- The graph switches to display the selected series' data

**Window 6 (SCW_SERIES_A):**
- Clicking the button cycles through: ATMA → ATMAF → ATMA (loop)

**Window 7 (SCW_SERIES_E):**
- Clicking the button cycles through: EXTERNAL1 → EXTERNAL2 → EXTERNAL3 → EXTERNAL4 → EXTERNAL5 → EXTERNAL1 (loop)

**Note:** When cycling, the button click does NOT select the window. Selection only occurs when clicking directly on the graph.

### Window Selection

Windows can be selected by:
1. **Windows 1-4:** Clicking either the button or the graph
2. **Windows 5-7:** Clicking the graph (button clicks only cycle series)

When a window is selected:
- A yellow border (3px solid) appears around the container frame
- The `seriesSelected` signal is emitted with the current series name
- The previously selected window's border is removed

**Example:**
```cpp
// Programmatically select window 3 (RULER_3)
// Note: This is an internal method, but you can trigger it by simulating a click
// or by connecting to the button's clicked signal for windows 1-4
```

---

## Integration with MainWindow

### Complete Setup Example

Refer to `mainwindow.cpp` for a complete integration example:

```cpp
void MainWindow::setupSCWWindow()
{
    // Create a new tab for SCWWindow
    QWidget* scwTab = new QWidget();
    scwTab->setObjectName("scwTab");
    
    // Create a layout for the tab
    QVBoxLayout* tabLayout = new QVBoxLayout(scwTab);
    tabLayout->setContentsMargins(0, 0, 0, 0);
    tabLayout->setSpacing(0);
    
    // Create SCWWindow in the new tab
    scwWindow = new SCWWindow(scwTab, timeUpdateTimer);
    scwWindow->setObjectName("scwWindow");
    scwWindow->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    
    // Add SCWWindow to the tab layout
    tabLayout->addWidget(scwWindow);
    scwTab->setLayout(tabLayout);
    
    // Add the tab to the tab widget
    ui->tabWidget->addTab(scwTab, "SCW Window");
    
    // Optional: Create and start simulator
    scwSimulator = new SCWSimulator(this, timeUpdateTimer, scwWindow);
    scwSimulator->start();
}
```

### Key Points

1. **Timer Sharing:** The same `timeUpdateTimer` can be shared between `SCWWindow` and `SCWSimulator` (and other components)
2. **Layout Management:** Use `QVBoxLayout` with zero margins to ensure `SCWWindow` fills the tab
3. **Size Policy:** Set `QSizePolicy::Expanding` on both axes to allow resizing

---

## Simulator Integration

### SCWSimulator Overview

`SCWSimulator` is a companion class that automatically populates all `SCWWindow` graphs with simulated data every second.

### Creating and Starting the Simulator

```cpp
#include "scwsimulator.h"

// Create simulator (uses the same timer as SCWWindow)
SCWSimulator* scwSimulator = new SCWSimulator(this, timeUpdateTimer, scwWindow);

// Start the simulator
scwSimulator->start();
```

### Simulator Behavior

- **Update Frequency:** Adds new data points every second (when timer fires)
- **Data Generation:** Generates random values with bounded variations
- **All Series:** Updates all 16 series simultaneously:
  - 4 RULER series (windows 1-4)
  - 5 B series (window 5 cycles through these)
  - 2 A series (window 6 cycles through these)
  - 5 E series (window 7 cycles through these)

### Value Ranges

The simulator uses the following value ranges:

| Series Type | Range | Initial Value | Delta |
|-------------|-------|---------------|-------|
| RULER (1-4) | 0-100 | 25, 50, 75, 100 | ±5.0 |
| SCW_SERIES_B | 0-100 | 20, 40, 60, 80, 100 | ±5.0 |
| SCW_SERIES_A | 0-50 | 15, 35 | ±3.0 |
| SCW_SERIES_E | 0-100 | 10, 30, 50, 70, 90 | ±5.0 |

### Stopping the Simulator

```cpp
scwSimulator->stop();
```

### Checking Simulator Status

```cpp
if (scwSimulator->isRunning()) {
    qDebug() << "Simulator is active";
}
```

---

## Window Selection

### Selection Behavior

| Window | Button Click | Graph Click |
|--------|--------------|-------------|
| 1-4 | Selects window | Selects window |
| 5-7 | Cycles series only | Selects window |

### Visual Feedback

- **Selected Window:** Yellow border (3px solid) around the container frame
- **Unselected Windows:** Transparent border (2px solid transparent)

### Programmatic Selection

While there's no public API to programmatically select a window, you can:
1. Connect to button signals for windows 1-4
2. Simulate mouse clicks on graphs (advanced)

### Selection Signal Usage

```cpp
// Connect to selection signal
connect(scwWindow, &SCWWindow::seriesSelected, 
        [](const QString &seriesName) {
            // Update external UI based on selection
            updateStatusBar("Selected: " + seriesName);
            
            // Log selection for debugging
            qDebug() << "SCWWindow selection changed to:" << seriesName;
            
            // Enable/disable controls based on selection
            if (seriesName.startsWith("RULER")) {
                enableRulerControls();
            } else if (seriesName.startsWith("EXTERNAL")) {
                enableExternalControls();
            }
        });
```

---

## Best Practices

### 1. Data Timestamps

Always ensure timestamps are in chronological order and match the data vector size:

```cpp
// Good: Matching sizes
std::vector<qreal> values = {10.0, 20.0, 30.0};
std::vector<QDateTime> times = {t1, t2, t3}; // 3 timestamps

// Bad: Mismatched sizes will cause issues
std::vector<qreal> values = {10.0, 20.0};
std::vector<QDateTime> times = {t1, t2, t3}; // 3 timestamps, but only 2 values
```

### 2. Incremental Updates

Use `addDataPoints` for real-time streaming data:

```cpp
// In a timer callback or data stream handler
void onNewDataReceived(qreal value) {
    std::vector<qreal> singleValue = {value};
    std::vector<QDateTime> singleTime = {QDateTime::currentDateTime()};
    scwWindow->addDataPoints(SCW_SERIES_R::RULER_1, singleValue, singleTime);
}
```

### 3. Batch Updates

Use `setDataPoints` when replacing all data (e.g., loading historical data):

```cpp
// Load historical data
void loadHistoricalData() {
    auto historicalData = fetchFromDatabase();
    scwWindow->setDataPoints(SCW_SERIES_R::RULER_1, 
                             historicalData.values, 
                             historicalData.timestamps);
}
```

### 4. Cycling Series Data

When using cycling windows (5-7), ensure data is added to all series in the cycle, not just the currently displayed one:

```cpp
// Add data to all B series (window 5 cycles through these)
scwWindow->addDataPoints(SCW_SERIES_B::BRAT, values, times);
scwWindow->addDataPoints(SCW_SERIES_B::BOT, values, times);
scwWindow->addDataPoints(SCW_SERIES_B::BFT, values, times);
scwWindow->addDataPoints(SCW_SERIES_B::BOPT, values, times);
scwWindow->addDataPoints(SCW_SERIES_B::BOTC, values, times);
```

### 5. Memory Management

`SCWWindow` manages all `WaterfallData` instances internally. Do not delete them manually:

```cpp
// Good: Let SCWWindow manage data sources
scwWindow->addDataPoints(SCW_SERIES_R::RULER_1, values, times);

// Bad: Don't try to access or delete internal data sources
// WaterfallData* data = scwWindow->getDataSourceR(...); // No such public method
```

---

## Troubleshooting

### Graphs Not Updating

**Problem:** Data is added but graphs don't refresh.

**Solutions:**
1. Ensure timestamps are valid `QDateTime` objects
2. Check that data and timestamp vectors have matching sizes
3. For cycling windows (5-7), verify the series is currently displayed
4. Check debug output for error messages

### Selection Not Working

**Problem:** Clicking graphs doesn't select windows.

**Solutions:**
1. Verify event filters are installed (done automatically in constructor)
2. Check that graphs are not disabled or hidden
3. Ensure mouse events are not being intercepted by parent widgets

### Simulator Not Running

**Problem:** `SCWSimulator` doesn't add data.

**Solutions:**
1. Verify the timer is started: `timer->start()`
2. Check timer interval is set: `timer->setInterval(1000)`
3. Verify simulator is started: `scwSimulator->start()`
4. Check `scwSimulator->isRunning()` returns true

### Performance Issues

**Problem:** UI becomes sluggish with many data points.

**Solutions:**
1. Use `addDataPoints` for incremental updates (more efficient)
2. Limit the number of data points per series (WaterfallGraph has built-in limits)
3. Consider reducing update frequency if using a simulator

---

## Example: Complete Application Integration

```cpp
class MyApplication : public QMainWindow
{
    Q_OBJECT

private:
    QTimer* m_timer;
    SCWWindow* m_scwWindow;
    SCWSimulator* m_simulator;

public:
    MyApplication(QWidget* parent = nullptr)
        : QMainWindow(parent)
    {
        // Setup timer
        m_timer = new QTimer(this);
        m_timer->setInterval(1000);
        m_timer->start();
        
        // Create SCWWindow
        m_scwWindow = new SCWWindow(this, m_timer);
        setCentralWidget(m_scwWindow);
        
        // Connect to selection signal
        connect(m_scwWindow, &SCWWindow::seriesSelected,
                this, &MyApplication::onSeriesSelected);
        
        // Create and start simulator
        m_simulator = new SCWSimulator(this, m_timer, m_scwWindow);
        m_simulator->start();
    }

private slots:
    void onSeriesSelected(const QString& seriesName)
    {
        statusBar()->showMessage("Selected: " + seriesName);
    }
};
```

---

## Summary

`SCWWindow` provides a comprehensive solution for displaying multiple synchronized waterfall graphs with:
- **7 display windows** (4 fixed + 3 cycling)
- **16 data series** across 4 categories
- **Automatic layout management** with resizing support
- **Selection feedback** with visual indicators and signals
- **Easy integration** with timers and simulators

For questions or issues, refer to the source code in `scwwindow.h` and `scwwindow.cpp`, or check the integration example in `mainwindow.cpp`.

