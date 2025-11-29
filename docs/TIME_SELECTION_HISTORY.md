# Time Selection History Guide

This document explains how to capture and store time selection timestamps from `GraphLayout` in your main project. The guide shows how to maintain a history of up to 5 time selections using a vector.

---

## Overview

When users create time selections on the timeline (by dragging on the timeline slider or using the time selection visualizer), the `GraphLayout` emits a `TimeSelectionCreated` signal. This signal contains a `TimeSelectionSpan` object with both start and end timestamps as `QDateTime` values.

---

## TimeSelectionSpan Structure

The `TimeSelectionSpan` struct contains:

```cpp
struct TimeSelectionSpan
{
    QDateTime startTime;  // Start timestamp of the selection
    QDateTime endTime;    // End timestamp of the selection
    
    TimeSelectionSpan() = default;
    TimeSelectionSpan(const QDateTime &start, const QDateTime &end) 
        : startTime(start), endTime(end) {}
};
```

**Note**: Both `startTime` and `endTime` are `QDateTime` objects (not `QTime`), so they include full date and time information.

---

## Basic Integration

### Step 1: Include Required Headers

```cpp
#include "graphlayout.h"
#include "timelineutils.h"  // For TimeSelectionSpan
#include <vector>
#include <QDebug>
```

### Step 2: Declare Storage Vector

In your class header file:

```cpp
class MyApplication : public QMainWindow
{
    Q_OBJECT

private:
    GraphLayout* graphLayout;
    
    // Time selection history storage (max 5 selections)
    std::vector<TimeSelectionSpan> timeSelectionHistory;
    
    // ... other members
};
```

### Step 3: Connect to Signal

In your constructor or initialization function:

```cpp
MyApplication::MyApplication(QWidget *parent)
    : QMainWindow(parent)
{
    // ... create graphLayout ...
    
    // Connect time selection signal
    connect(graphLayout, &GraphLayout::TimeSelectionCreated,
            this, &MyApplication::onTimeSelectionCreated);
}
```

### Step 4: Implement Handler Slot

```cpp
void MyApplication::onTimeSelectionCreated(const TimeSelectionSpan &selection)
{
    qDebug() << "Time selection created:";
    qDebug() << "  Start:" << selection.startTime.toString("yyyy-MM-dd hh:mm:ss.zzz");
    qDebug() << "  End:" << selection.endTime.toString("yyyy-MM-dd hh:mm:ss.zzz");
    
    // Store the selection timestamps
    // If we already have 5, remove the oldest (FIFO)
    if (timeSelectionHistory.size() >= 5)
    {
        timeSelectionHistory.erase(timeSelectionHistory.begin());
        qDebug() << "Time selection history full, removed oldest entry";
    }
    
    // Add the new selection to the end
    timeSelectionHistory.push_back(selection);
    
    qDebug() << "Time selection stored. History size:" << timeSelectionHistory.size();
}
```

---

## Complete Example

Here's a complete example showing how to integrate time selection history in your main project:

### Header File (`myapplication.h`)

```cpp
#ifndef MYAPPLICATION_H
#define MYAPPLICATION_H

#include <QMainWindow>
#include "graphlayout.h"
#include "timelineutils.h"
#include <vector>

QT_BEGIN_NAMESPACE
namespace Ui { class MyApplication; }
QT_END_NAMESPACE

class MyApplication : public QMainWindow
{
    Q_OBJECT

public:
    explicit MyApplication(QWidget *parent = nullptr);
    ~MyApplication();

private slots:
    /**
     * @brief Handles time selection created events
     *
     * Called when a time selection is created on the timeline.
     * Stores the selection timestamps in history (max 5).
     */
    void onTimeSelectionCreated(const TimeSelectionSpan &selection);
    
    /**
     * @brief Get the time selection history
     * @return Vector of TimeSelectionSpan objects (up to 5)
     */
    std::vector<TimeSelectionSpan> getTimeSelectionHistory() const { return timeSelectionHistory; }
    
    /**
     * @brief Clear the time selection history
     */
    void clearTimeSelectionHistory() { timeSelectionHistory.clear(); }

private:
    Ui::MyApplication *ui;
    
    GraphLayout* graphLayout;
    QTimer* timeUpdateTimer;
    
    // Time selection history storage (max 5 selections)
    std::vector<TimeSelectionSpan> timeSelectionHistory;
    
    void setupGraphLayout();
    void processTimeSelection(const TimeSelectionSpan &selection);
};

#endif // MYAPPLICATION_H
```

### Implementation File (`myapplication.cpp`)

```cpp
#include "myapplication.h"
#include "ui_myapplication.h"
#include <QDebug>
#include <QTimer>

MyApplication::MyApplication(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MyApplication)
{
    ui->setupUi(this);
    
    // Create timer for time updates
    timeUpdateTimer = new QTimer(this);
    timeUpdateTimer->setInterval(1000); // 1 second
    timeUpdateTimer->start();
    
    // Setup graph layout
    setupGraphLayout();
}

MyApplication::~MyApplication()
{
    delete ui;
}

void MyApplication::setupGraphLayout()
{
    // Initialize series labels map (example)
    std::map<GraphType, std::vector<QPair<QString, QColor>>> seriesLabelsMap;
    seriesLabelsMap[GraphType::RTW] = {{"RTW-1", QColor(Qt::red)}, {"ADOPTED", QColor(Qt::yellow)}};
    seriesLabelsMap[GraphType::BTW] = {{"BTW-1", QColor(Qt::red)}, {"BTW-2", QColor(Qt::green)}, {"ADOPTED", QColor(Qt::yellow)}};
    // ... add other graph types ...
    
    // Create GraphLayout
    graphLayout = new GraphLayout(ui->centralWidget, LayoutType::GPW4W, timeUpdateTimer, seriesLabelsMap);
    graphLayout->setObjectName("graphLayout");
    graphLayout->setGeometry(QRect(100, 100, 900, 900));
    
    // Connect time selection signal
    connect(graphLayout, &GraphLayout::TimeSelectionCreated,
            this, &MyApplication::onTimeSelectionCreated);
    
    qDebug() << "GraphLayout created and time selection signal connected";
}

void MyApplication::onTimeSelectionCreated(const TimeSelectionSpan &selection)
{
    qDebug() << "========================================";
    qDebug() << "TIME SELECTION CREATED";
    qDebug() << "========================================";
    qDebug() << "Start:" << selection.startTime.toString("yyyy-MM-dd hh:mm:ss.zzz");
    qDebug() << "End:" << selection.endTime.toString("yyyy-MM-dd hh:mm:ss.zzz");
    
    // Store the selection timestamps
    // If we already have 5, remove the oldest (FIFO)
    if (timeSelectionHistory.size() >= 5)
    {
        TimeSelectionSpan oldest = timeSelectionHistory.front();
        timeSelectionHistory.erase(timeSelectionHistory.begin());
        qDebug() << "History full - removed oldest selection:";
        qDebug() << "  Start:" << oldest.startTime.toString("yyyy-MM-dd hh:mm:ss.zzz");
        qDebug() << "  End:" << oldest.endTime.toString("yyyy-MM-dd hh:mm:ss.zzz");
    }
    
    // Add the new selection to the end
    timeSelectionHistory.push_back(selection);
    
    qDebug() << "Time selection stored. History size:" << timeSelectionHistory.size();
    qDebug() << "All stored selections:";
    for (size_t i = 0; i < timeSelectionHistory.size(); ++i)
    {
        qDebug() << "  [" << i << "] Start:" << timeSelectionHistory[i].startTime.toString("yyyy-MM-dd hh:mm:ss.zzz")
                 << "End:" << timeSelectionHistory[i].endTime.toString("yyyy-MM-dd hh:mm:ss.zzz");
    }
    qDebug() << "========================================";
    
    // Process the selection (e.g., update UI, save to file, etc.)
    processTimeSelection(selection);
}

void MyApplication::processTimeSelection(const TimeSelectionSpan &selection)
{
    // Example: Calculate duration
    qint64 durationMs = selection.startTime.msecsTo(selection.endTime);
    qint64 durationSeconds = durationMs / 1000;
    
    qDebug() << "Selection duration:" << durationSeconds << "seconds";
    
    // Example: Use the timestamps for your application logic
    // - Filter data within the time range
    // - Update UI elements
    // - Save to configuration
    // - Send to external systems
    // etc.
}
```

---

## Accessing Stored Selections

### Get All Selections

```cpp
std::vector<TimeSelectionSpan> selections = getTimeSelectionHistory();

for (const auto& selection : selections)
{
    qDebug() << "Start:" << selection.startTime.toString("yyyy-MM-dd hh:mm:ss.zzz");
    qDebug() << "End:" << selection.endTime.toString("yyyy-MM-dd hh:mm:ss.zzz");
}
```

### Get Most Recent Selection

```cpp
if (!timeSelectionHistory.empty())
{
    TimeSelectionSpan mostRecent = timeSelectionHistory.back();
    qDebug() << "Most recent selection:";
    qDebug() << "  Start:" << mostRecent.startTime.toString("yyyy-MM-dd hh:mm:ss.zzz");
    qDebug() << "  End:" << mostRecent.endTime.toString("yyyy-MM-dd hh:mm:ss.zzz");
}
```

### Get Oldest Selection

```cpp
if (!timeSelectionHistory.empty())
{
    TimeSelectionSpan oldest = timeSelectionHistory.front();
    qDebug() << "Oldest selection:";
    qDebug() << "  Start:" << oldest.startTime.toString("yyyy-MM-dd hh:mm:ss.zzz");
    qDebug() << "  End:" << oldest.endTime.toString("yyyy-MM-dd hh:mm:ss.zzz");
}
```

### Get Selection by Index

```cpp
if (timeSelectionHistory.size() > index)
{
    TimeSelectionSpan selection = timeSelectionHistory[index];
    // Use the selection...
}
```

### Clear History

```cpp
timeSelectionHistory.clear();
qDebug() << "Time selection history cleared";
```

---

## Advanced Usage

### Filter Selections by Date Range

```cpp
QDateTime filterStart = QDateTime::currentDateTime().addDays(-1);
QDateTime filterEnd = QDateTime::currentDateTime();

std::vector<TimeSelectionSpan> filteredSelections;
for (const auto& selection : timeSelectionHistory)
{
    if (selection.startTime >= filterStart && selection.endTime <= filterEnd)
    {
        filteredSelections.push_back(selection);
    }
}
```

### Calculate Total Duration

```cpp
qint64 totalDurationMs = 0;
for (const auto& selection : timeSelectionHistory)
{
    totalDurationMs += selection.startTime.msecsTo(selection.endTime);
}
qint64 totalDurationSeconds = totalDurationMs / 1000;
qDebug() << "Total duration of all selections:" << totalDurationSeconds << "seconds";
```

### Find Overlapping Selections

```cpp
bool hasOverlap = false;
for (size_t i = 0; i < timeSelectionHistory.size(); ++i)
{
    for (size_t j = i + 1; j < timeSelectionHistory.size(); ++j)
    {
        const auto& sel1 = timeSelectionHistory[i];
        const auto& sel2 = timeSelectionHistory[j];
        
        // Check if selections overlap
        if (!(sel1.endTime < sel2.startTime || sel2.endTime < sel1.startTime))
        {
            hasOverlap = true;
            qDebug() << "Found overlapping selections at indices" << i << "and" << j;
        }
    }
}
```

---

## Signal Flow

The time selection signal flows through the system as follows:

```
User creates time selection
    ↓
WaterfallGraph::endSelection()
    ↓ emits SelectionCreated
GraphContainer::onSelectionCreated()
    ↓ emits TimeSelectionCreated
GraphLayout::onTimeSelectionCreated()
    ↓ emits TimeSelectionCreated (external signal)
YourApplication::onTimeSelectionCreated()
    ↓
Store in timeSelectionHistory vector
```

---

## Important Notes

1. **Maximum History**: The vector maintains a maximum of 5 selections using FIFO (First In, First Out) behavior. When the 6th selection is added, the oldest is automatically removed.

2. **QDateTime vs QTime**: The `TimeSelectionSpan` contains `QDateTime` objects, not `QTime`. This means you have full date and time information, not just time-of-day.

3. **Thread Safety**: The signal/slot mechanism is thread-safe, but if you access `timeSelectionHistory` from multiple threads, you should add appropriate synchronization.

4. **Memory Management**: The `TimeSelectionSpan` objects are stored by value in the vector, so they are automatically cleaned up when the vector is destroyed.

5. **Signal Frequency**: The signal is emitted every time a user creates a time selection. If users create selections rapidly, the history will update accordingly.

---

## Troubleshooting

### Signal Not Received

- Ensure `GraphLayout` is created before connecting the signal
- Verify the connection is made in the constructor or initialization function
- Check that time selections are actually being created (try dragging on the timeline)

### History Not Updating

- Verify the slot is being called (add debug output)
- Check that the vector size limit logic is correct
- Ensure `TimeSelectionSpan` objects are valid (check `isValid()` on the QDateTime objects)

### Incorrect Timestamps

- Verify the graph's time range is properly set
- Check that the data source has valid timestamps
- Ensure the timeline view is properly configured

---

## Related Documentation

- [Timestamp Integration Guide](./TIMESTAMP_INTEGRATION.md) - General timestamp capture guide
- [Timestamp Capture Guide](./TIMESTAMP_CAPTURE.md) - Detailed timestamp capture documentation
- [Components Guide](./COMPONENTS.md) - Overview of all components

