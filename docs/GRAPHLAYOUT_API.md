# GraphLayout API Documentation

This document provides comprehensive documentation for all public APIs in the `GraphLayout` class, which serves as the central integration point for managing multiple graph containers and their data sources.

## Table of Contents

1. [Initialization](#initialization)
2. [Layout Management](#layout-management)
3. [Data Source Management](#data-source-management)
4. [Data Options Management](#data-options-management)
5. [Data Point Management](#data-point-management)
6. [Series Management](#series-management)
7. [Container Management](#container-management)
8. [Range Limits](#range-limits)
9. [Marker and Symbol Management](#marker-and-symbol-management)
10. [Chevron Labels](#chevron-labels)
11. [Time Management](#time-management)
12. [Graph Redraw](#graph-redraw)
13. [Signals](#signals)
14. [Slots](#slots)

---

## Initialization

### Constructor

```cpp
GraphLayout(QWidget *parent, 
            LayoutType layoutType, 
            QTimer *timer = nullptr, 
            std::map<GraphType, std::vector<QPair<QString, QColor>>> seriesLabelsMap = {})
```

Creates a new `GraphLayout` instance.

**Parameters:**
- `parent`: Parent widget
- `layoutType`: Layout configuration (GPW1W, GPW4W, GPW2WV, etc.)
- `timer`: Optional timer for data updates (if nullptr, creates a default 1-second timer)
- `seriesLabelsMap`: Map of graph types to their series labels and colors

**Example:**
```cpp
// Create a 2x2 grid layout with custom series
std::map<GraphType, std::vector<QPair<QString, QColor>>> seriesMap;
seriesMap[GraphType::BTW] = {{"BTW-1", QColor(Qt::red)}, {"BTW-2", QColor(Qt::green)}};
seriesMap[GraphType::RTW] = {{"RTW-1", QColor(Qt::blue)}};

GraphLayout *layout = new GraphLayout(parent, LayoutType::GPW4W, nullptr, seriesMap);
```

---

## Layout Management

### setLayoutType

```cpp
void setLayoutType(LayoutType layoutType)
```

Changes the layout configuration.

**Example:**
```cpp
layout->setLayoutType(LayoutType::GPW2WH); // Switch to 2 windows horizontal
```

### getLayoutType

```cpp
LayoutType getLayoutType() const
```

Returns the current layout type.

**Example:**
```cpp
LayoutType current = layout->getLayoutType();
if (current == LayoutType::GPW4W) {
    // Handle 4-window layout
}
```

### setGraphViewSize

```cpp
void setGraphViewSize(int width, int height)
```

Sets the size for all graph views.

**Example:**
```cpp
layout->setGraphViewSize(800, 600);
```

### updateLayoutSizing

```cpp
void updateLayoutSizing()
```

Recalculates and updates the layout sizing based on available space.

**Example:**
```cpp
// After window resize
layout->updateLayoutSizing();
```

---

## Data Source Management

### getDataSource

```cpp
WaterfallData *getDataSource(const GraphType &graphType)
```

Gets the data source for a specific graph type.

**Returns:** Pointer to `WaterfallData`, or `nullptr` if not found.

**Example:**
```cpp
WaterfallData *btwData = layout->getDataSource(GraphType::BTW);
if (btwData) {
    // Access BTW data
    qDebug() << "BTW data range:" << btwData->getYRange().first 
             << "to" << btwData->getYRange().second;
}
```

### hasDataSource

```cpp
bool hasDataSource(const GraphType &graphType) const
```

Checks if a data source exists for the given graph type.

**Example:**
```cpp
if (layout->hasDataSource(GraphType::RTW)) {
    // RTW data source is available
}
```

### getDataSourceLabels

```cpp
std::vector<GraphType> getDataSourceLabels() const
```

Returns all graph types that have data sources.

**Example:**
```cpp
std::vector<GraphType> availableTypes = layout->getDataSourceLabels();
for (GraphType type : availableTypes) {
    qDebug() << "Available graph type:" << static_cast<int>(type);
}
```

---

## Data Options Management

### Per-Container Data Options

These methods operate on a specific container identified by its label.

#### addDataOption (container-specific)

```cpp
void addDataOption(const QString &containerLabel, 
                   const GraphType &graphType, 
                   WaterfallData &dataSource)
```

Adds a data option to a specific container.

**Example:**
```cpp
WaterfallData rtwData("RTW Data");
layout->addDataOption("Container 1", GraphType::RTW, rtwData);
```

#### removeDataOption (container-specific)

```cpp
void removeDataOption(const QString &containerLabel, const GraphType &graphType)
```

Removes a data option from a specific container.

**Example:**
```cpp
layout->removeDataOption("Container 1", GraphType::RTW);
```

#### clearDataOptions (container-specific)

```cpp
void clearDataOptions(const QString &containerLabel)
```

Clears all data options from a specific container.

**Example:**
```cpp
layout->clearDataOptions("Container 1");
```

#### setCurrentDataOption (container-specific)

```cpp
void setCurrentDataOption(const QString &containerLabel, const GraphType &graphType)
```

Sets the current data option for a specific container.

**Example:**
```cpp
layout->setCurrentDataOption("Container 1", GraphType::BTW);
```

#### getCurrentDataOption (container-specific)

```cpp
GraphType getCurrentDataOption(const QString &containerLabel) const
```

Gets the current data option for a specific container.

**Example:**
```cpp
GraphType current = layout->getCurrentDataOption("Container 1");
```

#### getAvailableDataOptions (container-specific)

```cpp
std::vector<GraphType> getAvailableDataOptions(const QString &containerLabel) const
```

Gets all available data options for a specific container.

**Example:**
```cpp
std::vector<GraphType> options = layout->getAvailableDataOptions("Container 1");
for (GraphType type : options) {
    qDebug() << "Available option:" << static_cast<int>(type);
}
```

#### getDataOption (container-specific)

```cpp
WaterfallData *getDataOption(const QString &containerLabel, const GraphType &graphType)
```

Gets the data source for a specific option in a container.

**Example:**
```cpp
WaterfallData *data = layout->getDataOption("Container 1", GraphType::RTW);
```

#### hasDataOption (container-specific)

```cpp
bool hasDataOption(const QString &containerLabel, const GraphType &graphType) const
```

Checks if a container has a specific data option.

**Example:**
```cpp
if (layout->hasDataOption("Container 1", GraphType::BTW)) {
    // Container 1 has BTW option
}
```

### Global Data Options

These methods operate on all visible containers.

#### addDataOption (global)

```cpp
void addDataOption(const GraphType &graphType, WaterfallData &dataSource)
```

Adds a data option to all visible containers.

**Example:**
```cpp
WaterfallData btwData("BTW Data");
layout->addDataOption(GraphType::BTW, btwData);
```

#### removeDataOption (global)

```cpp
void removeDataOption(const GraphType &graphType)
```

Removes a data option from all visible containers.

**Example:**
```cpp
layout->removeDataOption(GraphType::RTW);
```

#### clearDataOptions (global)

```cpp
void clearDataOptions()
```

Clears all data options from all visible containers.

**Example:**
```cpp
layout->clearDataOptions();
```

#### setCurrentDataOption (global)

```cpp
void setCurrentDataOption(const GraphType &graphType)
```

Sets the current data option for all visible containers.

**Example:**
```cpp
layout->setCurrentDataOption(GraphType::BTW);
```

---

## Data Point Management

### addDataPointToDataSource

```cpp
void addDataPointToDataSource(const GraphType &graphType, 
                              const QString &seriesLabel, 
                              qreal yValue, 
                              const QDateTime &timestamp)
```

Adds a single data point to a specific series in a data source.

**Example:**
```cpp
QDateTime timestamp = QDateTime::currentDateTime();
layout->addDataPointToDataSource(GraphType::BTW, "BTW-1", 45.5, timestamp);
```

### addDataPointsToDataSource

```cpp
void addDataPointsToDataSource(const GraphType &graphType, 
                               const QString &seriesLabel, 
                               const std::vector<qreal> &yValues, 
                               const std::vector<QDateTime> &timestamps)
```

Adds multiple data points to a specific series.

**Example:**
```cpp
std::vector<qreal> yValues = {10.0, 20.0, 30.0, 40.0};
std::vector<QDateTime> timestamps;
QDateTime baseTime = QDateTime::currentDateTime();
for (int i = 0; i < 4; ++i) {
    timestamps.push_back(baseTime.addSecs(i));
}
layout->addDataPointsToDataSource(GraphType::RTW, "RTW-1", yValues, timestamps);
```

### setDataToDataSource (vector version)

```cpp
void setDataToDataSource(const GraphType &graphType, 
                        const QString &seriesLabel, 
                        const std::vector<qreal> &yData, 
                        const std::vector<QDateTime> &timestamps)
```

Replaces all data in a series with new data.

**Example:**
```cpp
std::vector<qreal> newYData = {15.0, 25.0, 35.0, 45.0};
std::vector<QDateTime> newTimestamps;
QDateTime start = QDateTime::currentDateTime();
for (int i = 0; i < 4; ++i) {
    newTimestamps.push_back(start.addSecs(i * 2));
}
layout->setDataToDataSource(GraphType::BTW, "BTW-1", newYData, newTimestamps);
```

### setDataToDataSource (WaterfallData version)

```cpp
void setDataToDataSource(const GraphType &graphType, 
                        const QString &seriesLabel, 
                        const WaterfallData &data)
```

Copies data from a `WaterfallData` object to a specific series.

**Example:**
```cpp
WaterfallData sourceData("Source");
sourceData.addDataPointToSeries("BTW-1", 50.0, QDateTime::currentDateTime());
layout->setDataToDataSource(GraphType::BTW, "BTW-1", sourceData);
```

### clearDataSource

```cpp
void clearDataSource(const GraphType &graphType, const QString &seriesLabel)
```

Clears all data from a specific series.

**Example:**
```cpp
layout->clearDataSource(GraphType::BTW, "BTW-1");
```

---

## Series Management

### hasSeriesInDataSource

```cpp
bool hasSeriesInDataSource(const GraphType &graphType, const QString &seriesLabel) const
```

Checks if a series exists in a data source.

**Example:**
```cpp
if (layout->hasSeriesInDataSource(GraphType::BTW, "BTW-1")) {
    // Series exists
}
```

### getSeriesLabelsInDataSource

```cpp
std::vector<QString> getSeriesLabelsInDataSource(const GraphType &graphType) const
```

Gets all series labels in a data source.

**Example:**
```cpp
std::vector<QString> series = layout->getSeriesLabelsInDataSource(GraphType::BTW);
for (const QString &label : series) {
    qDebug() << "Series:" << label;
}
```

### addSeriesToDataSource

```cpp
void addSeriesToDataSource(const GraphType &graphType, const QString &seriesLabel)
```

Adds a new series to a data source (initializes empty series).

**Example:**
```cpp
layout->addSeriesToDataSource(GraphType::BTW, "BTW-New");
```

### removeSeriesFromDataSource

```cpp
void removeSeriesFromDataSource(const GraphType &graphType, const QString &seriesLabel)
```

Removes a series from a data source.

**Example:**
```cpp
layout->removeSeriesFromDataSource(GraphType::BTW, "BTW-Old");
```

---

## Container Management

### getContainerLabels

```cpp
std::vector<QString> getContainerLabels() const
```

Returns all container labels.

**Example:**
```cpp
std::vector<QString> labels = layout->getContainerLabels();
for (const QString &label : labels) {
    qDebug() << "Container:" << label;
}
```

### hasContainer

```cpp
bool hasContainer(const GraphType &graphType) const
```

Checks if a container exists for the given graph type.

**Example:**
```cpp
if (layout->hasContainer(GraphType::BTW)) {
    // BTW container exists
}
```

---

## Range Limits

### setHardRangeLimits

```cpp
void setHardRangeLimits(const GraphType graphType, qreal yMin, qreal yMax)
```

Sets hard range limits for a graph type (prevents auto-scaling beyond these limits).

**Example:**
```cpp
layout->setHardRangeLimits(GraphType::BTW, 5.0, 75.0);
```

### removeHardRangeLimits

```cpp
void removeHardRangeLimits(const GraphType graphType)
```

Removes hard range limits for a graph type.

**Example:**
```cpp
layout->removeHardRangeLimits(GraphType::BTW);
```

### clearAllHardRangeLimits

```cpp
void clearAllHardRangeLimits()
```

Clears all hard range limits from all graphs.

**Example:**
```cpp
layout->clearAllHardRangeLimits();
```

### hasHardRangeLimits

```cpp
bool hasHardRangeLimits(const GraphType graphType) const
```

Checks if a graph type has hard range limits.

**Example:**
```cpp
if (layout->hasHardRangeLimits(GraphType::BTW)) {
    // BTW has range limits
}
```

### getHardRangeLimits

```cpp
std::pair<qreal, qreal> getHardRangeLimits(const GraphType graphType) const
```

Gets the hard range limits for a graph type.

**Returns:** Pair of (min, max) values, or (0.0, 0.0) if no limits set.

**Example:**
```cpp
auto limits = layout->getHardRangeLimits(GraphType::BTW);
qDebug() << "BTW range limits:" << limits.first << "to" << limits.second;
```

---

## Marker and Symbol Management

### Adding Markers and Symbols

#### addRTWSymbol

```cpp
void addRTWSymbol(const GraphType &graphType, 
                  const QString &symbolName, 
                  const QDateTime &timestamp, 
                  qreal range)
```

Adds an RTW symbol to a graph.

**Example:**
```cpp
QDateTime timestamp = QDateTime::currentDateTime();
layout->addRTWSymbol(GraphType::RTW, "Circle", timestamp, 25.0);
```

#### addBTWSymbol

```cpp
void addBTWSymbol(const GraphType &graphType, 
                  const QString &symbolName, 
                  const QDateTime &timestamp, 
                  qreal range)
```

Adds a BTW symbol to a graph.

**Example:**
```cpp
QDateTime timestamp = QDateTime::currentDateTime();
layout->addBTWSymbol(GraphType::BTW, "MagentaCircle", timestamp, 45.0);
```

#### addBTWMarker

```cpp
void addBTWMarker(const GraphType &graphType, 
                  const QDateTime &timestamp, 
                  qreal range, 
                  qreal delta)
```

Adds a BTW marker to a graph. **Note:** This automatically adds magenta circles (BTW symbols) to all other graphs at the same timestamp.

**Example:**
```cpp
QDateTime timestamp = QDateTime::currentDateTime();
layout->addBTWMarker(GraphType::BTW, timestamp, 50.0, 2.5);
// This will also add magenta circles to RTW, BDW, etc. at the same timestamp
```

#### addRTWRMarker

```cpp
void addRTWRMarker(const GraphType &graphType, 
                   const QDateTime &timestamp, 
                   qreal range)
```

Adds an RTW R marker to a graph.

**Example:**
```cpp
QDateTime timestamp = QDateTime::currentDateTime();
layout->addRTWRMarker(GraphType::RTW, timestamp, 30.0);
```

### Removing Markers and Symbols

#### removeRTWSymbol

```cpp
bool removeRTWSymbol(const GraphType &graphType, 
                     const QString &symbolName, 
                     const QDateTime &timestamp, 
                     qreal range, 
                     qreal toleranceMs = 1000, 
                     qreal rangeTolerance = 0.1)
```

Removes an RTW symbol matching the given symbol name, timestamp, and range (within tolerance).

**Returns:** `true` if symbol was found and removed, `false` otherwise.

**Example:**
```cpp
QDateTime timestamp = QDateTime::currentDateTime();
bool removed = layout->removeRTWSymbol(GraphType::RTW, "Circle", timestamp, 25.0, 500, 0.05);
if (removed) {
    qDebug() << "RTW symbol removed successfully";
}
```

#### removeBTWMarker

```cpp
bool removeBTWMarker(const GraphType &graphType, 
                     const QDateTime &timestamp, 
                     qreal range, 
                     qreal toleranceMs = 1000, 
                     qreal rangeTolerance = 0.1)
```

Removes a BTW marker matching the given timestamp and range (within tolerance).

**Returns:** `true` if marker was found and removed, `false` otherwise.

**Example:**
```cpp
QDateTime timestamp = QDateTime::currentDateTime();
bool removed = layout->removeBTWMarker(GraphType::BTW, timestamp, 50.0, 500, 0.05);
if (removed) {
    qDebug() << "BTW marker removed successfully";
}
```

#### removeRTWRMarker

```cpp
bool removeRTWRMarker(const GraphType &graphType, 
                      const QDateTime &timestamp, 
                      qreal range, 
                      qreal toleranceMs = 1000, 
                      qreal rangeTolerance = 0.1)
```

Removes an RTW R marker matching the given timestamp and range (within tolerance).

**Returns:** `true` if marker was found and removed, `false` otherwise.

**Example:**
```cpp
QDateTime timestamp = QDateTime::currentDateTime();
bool removed = layout->removeRTWRMarker(GraphType::RTW, timestamp, 30.0);
```

### Clearing Markers and Symbols

#### clearRTWSymbols

```cpp
void clearRTWSymbols(const GraphType &graphType)
```

Clears all RTW symbols from a graph.

**Example:**
```cpp
layout->clearRTWSymbols(GraphType::RTW);
```

#### clearBTWSymbols

```cpp
void clearBTWSymbols(const GraphType &graphType)
```

Clears all BTW symbols from a graph.

**Example:**
```cpp
layout->clearBTWSymbols(GraphType::BTW);
```

#### clearBTWMarkers

```cpp
void clearBTWMarkers(const GraphType &graphType)
```

Clears all BTW markers (data source markers) from a graph.

**Example:**
```cpp
layout->clearBTWMarkers(GraphType::BTW);
```

#### clearRTWRMarkers

```cpp
void clearRTWRMarkers(const GraphType &graphType)
```

Clears all RTW R markers from a graph.

**Example:**
```cpp
layout->clearRTWRMarkers(GraphType::RTW);
```

#### clearBTWManualMarkers

```cpp
void clearBTWManualMarkers()
```

Clears all BTW manual markers (green interactive overlay markers) from all BTW graphs in all containers.

**Example:**
```cpp
layout->clearBTWManualMarkers();
```

#### clearAllGraphs

```cpp
void clearAllGraphs()
```

Clears all data, markers, and symbols from all graphs. Useful when all data tracks are unselected.

**Example:**
```cpp
layout->clearAllGraphs();
```

---

## Chevron Labels

### Global Chevron Labels

These methods operate on all visible containers.

#### setChevronLabel1 / setChevronLabel2 / setChevronLabel3

```cpp
void setChevronLabel1(const QString &label)
void setChevronLabel2(const QString &label)
void setChevronLabel3(const QString &label)
```

Sets chevron labels for all visible containers.

**Example:**
```cpp
layout->setChevronLabel1("Label 1");
layout->setChevronLabel2("Label 2");
layout->setChevronLabel3("Label 3");
```

#### getChevronLabel1 / getChevronLabel2 / getChevronLabel3

```cpp
QString getChevronLabel1() const
QString getChevronLabel2() const
QString getChevronLabel3() const
```

Gets chevron labels from the first visible container.

**Example:**
```cpp
QString label1 = layout->getChevronLabel1();
```

### Container-Specific Chevron Labels

These methods operate on a specific container.

#### setChevronLabel1 / setChevronLabel2 / setChevronLabel3 (container-specific)

```cpp
void setChevronLabel1(const QString &containerLabel, const QString &label)
void setChevronLabel2(const QString &containerLabel, const QString &label)
void setChevronLabel3(const QString &containerLabel, const QString &label)
```

Sets chevron labels for a specific container.

**Example:**
```cpp
layout->setChevronLabel1("Container 1", "Custom Label");
```

#### getChevronLabel1 / getChevronLabel2 / getChevronLabel3 (container-specific)

```cpp
QString getChevronLabel1(const QString &containerLabel) const
QString getChevronLabel2(const QString &containerLabel) const
QString getChevronLabel3(const QString &containerLabel) const
```

Gets chevron labels from a specific container.

**Example:**
```cpp
QString label = layout->getChevronLabel1("Container 1");
```

---

## Time Management

### setCurrentTime

```cpp
void setCurrentTime(const QTime &time)
```

Sets the current time for all timeline views.

**Example:**
```cpp
QTime currentTime = QTime::currentTime();
layout->setCurrentTime(currentTime);
```

### deleteInteractiveMarkers

```cpp
void deleteInteractiveMarkers()
```

Deletes all interactive markers from all graphs.

**Example:**
```cpp
layout->deleteInteractiveMarkers();
```

---

## Graph Redraw

### redrawGraph

```cpp
void redrawGraph(const GraphType &graphType)
```

Redraws a specific graph type in all containers that have it.

**Example:**
```cpp
layout->redrawGraph(GraphType::BTW);
```

### redrawAllGraphs

```cpp
void redrawAllGraphs()
```

Redraws all graphs in all containers.

**Example:**
```cpp
layout->redrawAllGraphs();
```

---

## Signals

### TimeSelectionCreated

```cpp
void TimeSelectionCreated(const TimeSelectionSpan &selection)
```

Emitted when a time selection is created.

**Example:**
```cpp
connect(layout, &GraphLayout::TimeSelectionCreated, 
        [](const TimeSelectionSpan &selection) {
    qDebug() << "Time selection created:" 
             << selection.start.toString() << "to" 
             << selection.end.toString();
});
```

### TimeSelectionsCleared

```cpp
void TimeSelectionsCleared()
```

Emitted when all time selections are cleared.

**Example:**
```cpp
connect(layout, &GraphLayout::TimeSelectionsCleared, []() {
    qDebug() << "All time selections cleared";
});
```

### RTWRMarkerTimestampCaptured

```cpp
void RTWRMarkerTimestampCaptured(const QDateTime &timestamp, const QPointF &position)
```

Emitted when an RTW R marker is clicked.

**Example:**
```cpp
connect(layout, &GraphLayout::RTWRMarkerTimestampCaptured,
        [](const QDateTime &timestamp, const QPointF &position) {
    qDebug() << "RTW R marker clicked at:" << timestamp.toString();
    qDebug() << "Position:" << position;
});
```

### BTWManualMarkerPlaced

```cpp
void BTWManualMarkerPlaced(const QDateTime &timestamp, const QPointF &position)
```

Emitted when a BTW manual marker is placed.

**Example:**
```cpp
connect(layout, &GraphLayout::BTWManualMarkerPlaced,
        [](const QDateTime &timestamp, const QPointF &position) {
    qDebug() << "BTW manual marker placed at:" << timestamp.toString();
    qDebug() << "Position:" << position;
});
```

### BTWManualMarkerClicked

```cpp
void BTWManualMarkerClicked(const QDateTime &timestamp, const QPointF &position)
```

Emitted when a BTW manual marker is clicked.

**Example:**
```cpp
connect(layout, &GraphLayout::BTWManualMarkerClicked,
        [](const QDateTime &timestamp, const QPointF &position) {
    qDebug() << "BTW manual marker clicked at:" << timestamp.toString();
});
```

---

## Slots

### onTimerTick

```cpp
void onTimerTick()
```

Internal slot called on timer tick. Typically not called directly.

### onTimeSelectionCreated

```cpp
void onTimeSelectionCreated(const TimeSelectionSpan &selection)
```

Slot called when a time selection is created. Can be connected to handle time selections.

**Example:**
```cpp
connect(someObject, &SomeClass::timeSelectionMade,
        layout, &GraphLayout::onTimeSelectionCreated);
```

### onTimeSelectionsCleared

```cpp
void onTimeSelectionsCleared()
```

Slot called when time selections are cleared.

**Example:**
```cpp
connect(clearButton, &QPushButton::clicked,
        layout, &GraphLayout::onTimeSelectionsCleared);
```

### onBTWManualMarkerPlaced

```cpp
void onBTWManualMarkerPlaced(const QDateTime &timestamp, const QPointF &position)
```

Internal slot called when a BTW manual marker is placed. Automatically adds magenta circles to all other graphs.

### onContainerIntervalChanged

```cpp
void onContainerIntervalChanged(TimeInterval interval)
```

Slot called when a container's time interval changes.

**Example:**
```cpp
connect(container, &GraphContainer::IntervalChanged,
        layout, &GraphLayout::onContainerIntervalChanged);
```

---

## Complete Example

Here's a complete example showing how to use GraphLayout:

```cpp
#include "graphlayout.h"
#include <QApplication>
#include <QTimer>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    
    // Create main window
    QWidget window;
    
    // Setup series labels and colors
    std::map<GraphType, std::vector<QPair<QString, QColor>>> seriesMap;
    seriesMap[GraphType::BTW] = {
        {"BTW-1", QColor(Qt::red)},
        {"BTW-2", QColor(Qt::green)},
        {"ADOPTED", QColor(Qt::yellow)}
    };
    seriesMap[GraphType::RTW] = {
        {"RTW-1", QColor(Qt::blue)}
    };
    
    // Create GraphLayout with 2x2 grid
    QTimer *timer = new QTimer(&window);
    GraphLayout *layout = new GraphLayout(&window, LayoutType::GPW4W, timer, seriesMap);
    
    // Set range limits
    layout->setHardRangeLimits(GraphType::BTW, 5.0, 75.0);
    layout->setHardRangeLimits(GraphType::RTW, 0.0, 50.0);
    
    // Add data points
    QDateTime baseTime = QDateTime::currentDateTime();
    for (int i = 0; i < 100; ++i) {
        QDateTime timestamp = baseTime.addSecs(i);
        layout->addDataPointToDataSource(GraphType::BTW, "BTW-1", 20.0 + i * 0.5, timestamp);
        layout->addDataPointToDataSource(GraphType::RTW, "RTW-1", 10.0 + i * 0.3, timestamp);
    }
    
    // Add a BTW marker (this will also add magenta circles to other graphs)
    QDateTime markerTime = baseTime.addSecs(50);
    layout->addBTWMarker(GraphType::BTW, markerTime, 45.0, 2.5);
    
    // Connect to signals
    connect(layout, &GraphLayout::BTWManualMarkerPlaced,
            [](const QDateTime &timestamp, const QPointF &position) {
        qDebug() << "BTW marker placed at:" << timestamp.toString();
    });
    
    // Setup window
    QVBoxLayout *mainLayout = new QVBoxLayout(&window);
    mainLayout->addWidget(layout);
    window.setLayout(mainLayout);
    window.resize(1600, 1200);
    window.show();
    
    return app.exec();
}
```

---

## Notes

- **GraphLayout is the central integration point**: All graph operations should go through GraphLayout rather than accessing GraphContainer or WaterfallGraph directly.

- **BTW Marker Behavior**: When a BTW marker is added via `addBTWMarker()`, it automatically adds magenta circles (BTW symbols) to all other graphs at the same timestamp. The magenta circles are positioned at each graph's data point at that timestamp, not at the BTW marker's range.

- **Data Source vs Data Option**: 
  - **Data Source**: The underlying `WaterfallData` object that stores data for a graph type
  - **Data Option**: A data source that has been added to a container and can be selected by the user

- **Manual Markers vs Data Source Markers**:
  - **Manual Markers**: Interactive green markers placed by clicking on BTW graphs (cleared with `clearBTWManualMarkers()`)
  - **Data Source Markers**: Markers stored in the data source (cleared with `clearBTWMarkers()`)

- **Thread Safety**: GraphLayout is not thread-safe. All operations should be performed from the main GUI thread.

