# RTW Symbol Addition Guide

This guide explains how to add RTW (Rate Time Waterfall) symbols to your graphs programmatically.

## Table of Contents
1. [Overview](#overview)
2. [Symbol Types](#symbol-types)
3. [Adding Symbols - Two Methods](#adding-symbols---two-methods)
4. [Code Examples](#code-examples)
5. [Best Practices](#best-practices)
6. [Common Patterns](#common-patterns)

---

## Overview

RTW symbols are graphical markers that can be displayed on RTW graphs to indicate specific events, measurements, or annotations. Symbols are stored in the `WaterfallData` object, which means they persist with the track data and remain visible through zoom changes, track switches, and graph redraws.

### Key Concepts

- **Symbol Name**: A string identifier that maps to a specific symbol type (e.g., "TM", "DP", "LY")
- **Timestamp**: The X-axis position (time) where the symbol should be displayed
- **Range**: The Y-axis position (range value) where the symbol should be displayed
- **Persistence**: Symbols are stored in `WaterfallData`, not in the graph itself, ensuring they persist with the data

### Important: Symbol Persistence with Data Source Changes

**Yes, symbols are stored in the data source, and when a data source changes, the graph will redraw with the new data source's content.**

Here's how it works:

1. **Symbols are stored in `WaterfallData`**: When you add a symbol, it's stored in the `WaterfallData` object, not in the graph widget itself.

2. **Data source switching**: When you call `setDataSource()` or switch data options (e.g., via `setCurrentDataOption()`), the graph simply points to a different `WaterfallData` object and redraws.

3. **Symbols persist per data source**:
   - If `DataSource A` has symbols and you switch the graph to `DataSource B` (which has no symbols), the graph will show `DataSource B`'s data (no symbols visible).
   - If you switch back to `DataSource A`, the symbols will still be there because they're stored in `DataSource A`.
   - **Symbols are NOT automatically cleared** when you switch data sources.

4. **Explicit clearing required**: The only way symbols are removed is if you explicitly call `clearRTWSymbols()` on a `WaterfallData` object.

**Example Scenario:**
```cpp
// Create two data sources
WaterfallData* dataSourceA = new WaterfallData("Track A");
WaterfallData* dataSourceB = new WaterfallData("Track B");

// Add symbols to dataSourceA
rtwGraph->setDataSource(*dataSourceA);
rtwGraph->addRTWSymbol("TM", timestamp1, 10.0);
rtwGraph->addRTWSymbol("DP", timestamp2, 15.0);
// dataSourceA now has 2 symbols

// Switch to dataSourceB (no symbols)
rtwGraph->setDataSource(*dataSourceB);
// Graph shows dataSourceB's data (no symbols visible)
// dataSourceA still has 2 symbols stored

// Switch back to dataSourceA
rtwGraph->setDataSource(*dataSourceA);
// Graph shows dataSourceA's data AND its 2 symbols (they reappear!)
```

This design ensures that symbols are tied to the track data, not to the graph display, which is the correct behavior for track-based data management.

---

## Symbol Types

The following symbol types are available:

### Range Types
- `"TM"` - Target Mark
- `"DP"` - Detection Point
- `"LY"` - Launch Year
- `"CircleI"` - Circle I
- `"Triangle"` - Triangle
- `"RectR"` - Rectangle R
- `"EllipsePP"` - Ellipse PP
- `"RectX"` - Rectangle X
- `"RectK"` - Rectangle K
- `"CircleRYellow"` - Circle R (Yellow)
- `"DoubleBarYellow"` - Double Bar (Yellow)

### Adoption Types
- `"RectA"` - Rectangle A
- `"RectAPurple"` - Rectangle A (Purple)

### Methodology Types
- `"R"` - R marker
- `"L"` - L marker
- `"BOT"` - Bottom
- `"BOTC"` - Bottom C
- `"BOTF"` - Bottom F
- `"BOTD"` - Bottom D

---

## Adding Symbols - Two Methods

There are two ways to add RTW symbols:

### Method 1: Via RTWGraph (Recommended)

Use `RTWGraph::addRTWSymbol()` when you have direct access to the `RTWGraph` instance. This method:
- Automatically adds the symbol to the graph's data source
- Triggers a redraw automatically
- Is the preferred method when working with graph instances

**Signature:**
```cpp
void RTWGraph::addRTWSymbol(const QString &symbolName, const QDateTime &timestamp, qreal range);
```

### Method 2: Via WaterfallData (Direct)

Use `WaterfallData::addRTWSymbol()` when you have direct access to the data source. This method:
- Adds the symbol directly to the data source
- Requires manual redraw if needed
- Useful when you need to add symbols before the graph is created

**Signature:**
```cpp
void WaterfallData::addRTWSymbol(const QString& symbolName, const QDateTime& timestamp, qreal range);
```

---

## Code Examples

### Example 1: Adding Symbols with 10-Second Intervals

This example shows how to add 5 symbols with 10-second spacing:

```cpp
// Get the RTW graph instance
RTWGraph* rtwGraph = /* your RTW graph instance */;

// Get the current time range from the graph
auto timeRange = rtwGraph->getTimeRange();
QDateTime symbolTimeMin = timeRange.first;
QDateTime symbolTimeMax = timeRange.second;

// Calculate symbol timestamps with 10 second intervals
// Start from symbolTimeMin and add 0, 10, 20, 30, 40 seconds
QDateTime symbol1Time = symbolTimeMin.addSecs(0);
QDateTime symbol2Time = symbolTimeMin.addSecs(10);
QDateTime symbol3Time = symbolTimeMin.addSecs(20);
QDateTime symbol4Time = symbolTimeMin.addSecs(30);
QDateTime symbol5Time = symbolTimeMin.addSecs(40);

// Add symbols via RTWGraph (automatically triggers redraw)
rtwGraph->addRTWSymbol("TM", symbol1Time, 10.0);
rtwGraph->addRTWSymbol("DP", symbol2Time, 15.0);
rtwGraph->addRTWSymbol("LY", symbol3Time, 20.0);
rtwGraph->addRTWSymbol("CircleI", symbol4Time, 8.0);
rtwGraph->addRTWSymbol("Triangle", symbol5Time, 12.0);
```

### Example 2: Adding Symbols with 50-Second Intervals

This example shows how to add 5 symbols with 50-second spacing:

```cpp
// Get the RTW graph instance
RTWGraph* rtwGraph = /* your RTW graph instance */;

// Get the current time range from the graph
auto timeRange = rtwGraph->getTimeRange();
QDateTime symbolTimeMin = timeRange.first;

// Calculate symbol timestamps with 50 second intervals
QDateTime symbol1Time = symbolTimeMin.addSecs(0);
QDateTime symbol2Time = symbolTimeMin.addSecs(50);
QDateTime symbol3Time = symbolTimeMin.addSecs(100);
QDateTime symbol4Time = symbolTimeMin.addSecs(150);
QDateTime symbol5Time = symbolTimeMin.addSecs(200);

// Add symbols via RTWGraph
rtwGraph->addRTWSymbol("TM", symbol1Time, 10.0);
rtwGraph->addRTWSymbol("DP", symbol2Time, 15.0);
rtwGraph->addRTWSymbol("LY", symbol3Time, 20.0);
rtwGraph->addRTWSymbol("CircleI", symbol4Time, 8.0);
rtwGraph->addRTWSymbol("Triangle", symbol5Time, 12.0);
```

### Example 3: Adding Symbols via WaterfallData

This example shows how to add symbols directly to the data source:

```cpp
// Get the WaterfallData instance
WaterfallData* rtwData = /* your WaterfallData instance */;

// Calculate timestamps
QDateTime baseTime = QDateTime::currentDateTime();
QDateTime symbol1Time = baseTime.addSecs(0);
QDateTime symbol2Time = baseTime.addSecs(10);
QDateTime symbol3Time = baseTime.addSecs(20);

// Add symbols directly to data source
rtwData->addRTWSymbol("TM", symbol1Time, 10.0);
rtwData->addRTWSymbol("DP", symbol2Time, 15.0);
rtwData->addRTWSymbol("LY", symbol3Time, 20.0);

// If you have a graph, trigger a redraw
if (rtwGraph) {
    rtwGraph->update();  // or rtwGraph->draw() if accessible
}
```

### Example 4: Adding Symbols to GraphLayout RTW Graph

This example shows how to add symbols to an RTW graph within a `GraphLayout`:

```cpp
// Get the GraphLayout instance
GraphLayout* graphgrid = /* your GraphLayout instance */;

// Get the RTW data source from GraphLayout
WaterfallData* overviewRTWData = graphgrid->getDataSource(GraphType::RTW);

// Find the RTW graph instance
RTWGraph* rtwGraphToUse = nullptr;
QList<RTWGraph*> rtwGraphs = graphgrid->findChildren<RTWGraph*>();
for (RTWGraph* rtwGraph : rtwGraphs) {
    if (!rtwGraph) continue;
    
    WaterfallData* graphDataSource = rtwGraph->getDataSource();
    if (graphDataSource == overviewRTWData) {
        rtwGraphToUse = rtwGraph;
        break;
    }
}

// Get the current time range
QDateTime symbolTimeMin, symbolTimeMax;
if (rtwGraphToUse) {
    auto timeRange = rtwGraphToUse->getTimeRange();
    symbolTimeMin = timeRange.first;
    symbolTimeMax = timeRange.second;
} else if (!overviewRTWData->isEmpty()) {
    auto timeRange = overviewRTWData->getCombinedTimeRange();
    symbolTimeMin = timeRange.first;
    symbolTimeMax = timeRange.second;
} else {
    symbolTimeMax = QDateTime::currentDateTime();
    symbolTimeMin = symbolTimeMax.addSecs(-150);
}

// Calculate symbol timestamps with 10 second intervals
QDateTime symbol1Time = symbolTimeMin.addSecs(0);
QDateTime symbol2Time = symbolTimeMin.addSecs(10);
QDateTime symbol3Time = symbolTimeMin.addSecs(20);
QDateTime symbol4Time = symbolTimeMin.addSecs(30);
QDateTime symbol5Time = symbolTimeMin.addSecs(40);

// Add symbols
if (rtwGraphToUse) {
    // Preferred: via RTWGraph (automatically triggers redraw)
    rtwGraphToUse->addRTWSymbol("TM", symbol1Time, 10.0);
    rtwGraphToUse->addRTWSymbol("DP", symbol2Time, 15.0);
    rtwGraphToUse->addRTWSymbol("LY", symbol3Time, 20.0);
    rtwGraphToUse->addRTWSymbol("CircleI", symbol4Time, 8.0);
    rtwGraphToUse->addRTWSymbol("Triangle", symbol5Time, 12.0);
} else {
    // Fallback: directly to data source
    overviewRTWData->addRTWSymbol("TM", symbol1Time, 10.0);
    overviewRTWData->addRTWSymbol("DP", symbol2Time, 15.0);
    overviewRTWData->addRTWSymbol("LY", symbol3Time, 20.0);
    overviewRTWData->addRTWSymbol("CircleI", symbol4Time, 8.0);
    overviewRTWData->addRTWSymbol("Triangle", symbol5Time, 12.0);
}
```

### Example 5: Adding Multiple Symbols in a Loop

This example shows how to add multiple symbols programmatically:

```cpp
RTWGraph* rtwGraph = /* your RTW graph instance */;

// Get time range
auto timeRange = rtwGraph->getTimeRange();
QDateTime baseTime = timeRange.first;

// Define symbols to add
struct SymbolInfo {
    QString name;
    int secondsOffset;
    qreal range;
};

std::vector<SymbolInfo> symbols = {
    {"TM", 0, 10.0},
    {"DP", 10, 15.0},
    {"LY", 20, 20.0},
    {"CircleI", 30, 8.0},
    {"Triangle", 40, 12.0}
};

// Add all symbols
for (const auto& symbolInfo : symbols) {
    QDateTime symbolTime = baseTime.addSecs(symbolInfo.secondsOffset);
    rtwGraph->addRTWSymbol(symbolInfo.name, symbolTime, symbolInfo.range);
}
```

### Example 6: Adding Symbols at Specific Times

This example shows how to add symbols at specific absolute times:

```cpp
RTWGraph* rtwGraph = /* your RTW graph instance */;

// Define specific timestamps
QDateTime timestamp1 = QDateTime::fromString("2025-11-27T10:00:00", Qt::ISODate);
QDateTime timestamp2 = QDateTime::fromString("2025-11-27T10:00:10", Qt::ISODate);
QDateTime timestamp3 = QDateTime::fromString("2025-11-27T10:00:20", Qt::ISODate);

// Add symbols at specific times
rtwGraph->addRTWSymbol("TM", timestamp1, 10.0);
rtwGraph->addRTWSymbol("DP", timestamp2, 15.0);
rtwGraph->addRTWSymbol("LY", timestamp3, 20.0);
```

---

## Best Practices

### 1. Use RTWGraph::addRTWSymbol() When Possible

Prefer using `RTWGraph::addRTWSymbol()` over `WaterfallData::addRTWSymbol()` because it:
- Automatically triggers a redraw
- Ensures the symbol is added to the correct data source
- Is more convenient when you have graph access

### 2. Ensure Timestamps Are Within Visible Range

When adding symbols, ensure their timestamps fall within the graph's visible time range:

```cpp
// Get the graph's time range
auto timeRange = rtwGraph->getTimeRange();
QDateTime timeMin = timeRange.first;
QDateTime timeMax = timeRange.second;

// Ensure your symbol timestamp is within range
QDateTime symbolTime = /* your timestamp */;
if (symbolTime >= timeMin && symbolTime <= timeMax) {
    rtwGraph->addRTWSymbol("TM", symbolTime, 10.0);
}
```

### 3. Use Appropriate Range Values

Range values should correspond to the Y-axis (range) values in your graph. Ensure they fall within the visible Y-axis range:

```cpp
// Get the graph's Y range
auto yRange = rtwGraph->getYRange();
qreal yMin = yRange.first;
qreal yMax = yRange.second;

// Ensure your range value is within bounds
qreal symbolRange = 15.0;  // Your range value
if (symbolRange >= yMin && symbolRange <= yMax) {
    rtwGraph->addRTWSymbol("TM", timestamp, symbolRange);
}
```

### 4. Handle Invalid Time Ranges

When the graph doesn't have a valid time range yet, provide a fallback:

```cpp
QDateTime symbolTimeMin, symbolTimeMax;

if (rtwGraph) {
    auto timeRange = rtwGraph->getTimeRange();
    symbolTimeMin = timeRange.first;
    symbolTimeMax = timeRange.second;
    
    if (!symbolTimeMin.isValid() || !symbolTimeMax.isValid()) {
        // Fallback: use current time with a window
        symbolTimeMax = QDateTime::currentDateTime();
        symbolTimeMin = symbolTimeMax.addSecs(-150);  // 2.5 minutes window
    }
}
```

### 5. Verify Symbols Were Added

After adding symbols, verify they were added successfully:

```cpp
// Add symbols
rtwGraph->addRTWSymbol("TM", timestamp1, 10.0);
rtwGraph->addRTWSymbol("DP", timestamp2, 15.0);

// Verify
WaterfallData* dataSource = rtwGraph->getDataSource();
size_t symbolCount = dataSource->getRTWSymbolsCount();
qDebug() << "Total symbols in data source:" << symbolCount;
```

---

## Common Patterns

### Pattern 1: Adding Symbols with Fixed Intervals

```cpp
// Add 5 symbols with 10-second intervals
QDateTime baseTime = /* your base time */;
for (int i = 0; i < 5; i++) {
    QDateTime symbolTime = baseTime.addSecs(i * 10);
    QString symbolName = /* your symbol name */;
    qreal range = /* your range value */;
    rtwGraph->addRTWSymbol(symbolName, symbolTime, range);
}
```

### Pattern 2: Adding Symbols Based on Data Points

```cpp
// Add symbols at specific data point timestamps
WaterfallData* dataSource = rtwGraph->getDataSource();
auto timestamps = dataSource->getTimestampsSeries("RTW-1");

// Add a symbol at every 10th data point
for (size_t i = 0; i < timestamps.size(); i += 10) {
    QDateTime timestamp = timestamps[i];
    qreal range = /* calculate range from data */;
    rtwGraph->addRTWSymbol("TM", timestamp, range);
}
```

### Pattern 3: Adding Symbols in Response to Events

```cpp
// In an event handler or slot
void MyClass::onEventOccurred(const QDateTime& eventTime, qreal eventRange) {
    rtwGraph->addRTWSymbol("DP", eventTime, eventRange);
}
```

### Pattern 4: Clearing and Re-adding Symbols

```cpp
// Clear existing symbols
WaterfallData* dataSource = rtwGraph->getDataSource();
dataSource->clearRTWSymbols();

// Add new symbols
rtwGraph->addRTWSymbol("TM", timestamp1, 10.0);
rtwGraph->addRTWSymbol("DP", timestamp2, 15.0);
```

---

## Troubleshooting

### Symbols Not Appearing

1. **Check timestamp validity**: Ensure timestamps fall within the graph's visible time range
2. **Check range values**: Ensure range values fall within the graph's visible Y-axis range
3. **Verify data source**: Ensure the graph is using the correct data source
4. **Check symbol name**: Ensure the symbol name matches one of the available types
5. **Trigger redraw**: If using `WaterfallData::addRTWSymbol()`, ensure you trigger a redraw
6. **Check data source switch**: If you recently switched data sources, ensure you're viewing the data source that contains the symbols

### Symbols Disappearing After Zoom

Symbols should persist through zoom changes because they're stored in `WaterfallData`. If they disappear:
- Verify symbols are added to the correct data source
- Check that the graph's data source matches the one where symbols were added
- Ensure timestamps are still within the visible time range after zoom

### Symbols Disappearing After Data Source Change

**This is expected behavior!** When you switch to a different data source:
- The graph displays the new data source's content
- Symbols from the old data source are not visible (but they're still stored in that data source)
- To see the symbols again, switch back to the original data source

**To verify symbols are still stored:**
```cpp
WaterfallData* dataSource = rtwGraph->getDataSource();
size_t symbolCount = dataSource->getRTWSymbolsCount();
qDebug() << "Symbols in current data source:" << symbolCount;
```

If you want symbols to appear in a new data source, you need to add them to that data source:
```cpp
// Switch to new data source
rtwGraph->setDataSource(*newDataSource);

// Add symbols to the new data source
rtwGraph->addRTWSymbol("TM", timestamp1, 10.0);
rtwGraph->addRTWSymbol("DP", timestamp2, 15.0);
```

### Debugging

Enable debug output to trace symbol addition:

```cpp
qDebug() << "RTW: Adding symbol" << symbolName 
         << "at timestamp" << timestamp.toString() 
         << "with range" << range;
         
qDebug() << "RTW: Total symbols in data source:" 
         << dataSource->getRTWSymbolsCount();
```

---

## Summary

- **Use `RTWGraph::addRTWSymbol()`** when you have graph access (recommended)
- **Use `WaterfallData::addRTWSymbol()`** when you only have data source access
- **Ensure timestamps** fall within the graph's visible time range
- **Ensure range values** fall within the graph's visible Y-axis range
- **Symbols persist** with track data because they're stored in `WaterfallData`
- **Symbols automatically scale** with zoom panel customization
- **Symbols are tied to data sources**: When you switch data sources, the graph shows the new data source's content. Symbols from the old data source remain stored but are not visible until you switch back.
- **Data source changes trigger redraw**: When `setDataSource()` is called, the graph redraws with the new data source's data and symbols (if any).

For more information, see the source code in:
- `rtwgraph.h` / `rtwgraph.cpp` - RTWGraph class
- `waterfalldata.h` / `waterfalldata.cpp` - WaterfallData class
- `rtwsymbols.h` / `rtwsymbol.cpp` - Symbol drawing implementation

