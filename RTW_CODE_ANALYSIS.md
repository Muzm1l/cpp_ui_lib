# RTW Code Analysis

## Overview
The RTW (Rate Time Waterfall) graph implementation extends `WaterfallGraph` to provide specialized functionality for displaying RTW data with custom R markers and RTW symbols.

## Architecture

### Class Structure
- **RTWGraph**: Inherits from `WaterfallGraph`
- **RTWSymbolDrawing**: Utility class for generating symbol pixmaps
- **WaterfallData**: Stores RTW symbols alongside data series

### Key Components

1. **Constructor** (`rtwgraph.cpp:17-25`)
   - Initializes `symbols(40)` which triggers `generateAll()` to populate pixmap cache
   - Sets hard Y-range limits: 0.0 to 25.0
   - Enables range limiting

2. **Draw Method** (`rtwgraph.cpp:40-105`)
   - Clears scene and sets up drawing area
   - Draws grid if enabled
   - Updates data ranges and draws data series
   - Draws ADOPTED series as line
   - Draws other series as R markers
   - Draws RTW symbols

3. **R Markers** (`rtwgraph.cpp:136-260`)
   - Uses time-based binning (300ms intervals)
   - Filters by visible time range
   - Draws yellow "R" text markers
   - Has fallback to raw data if binning fails

4. **RTW Symbols** (`rtwgraph.cpp:341-442`)
   - Reads symbols from `dataSource` (persistent storage)
   - Filters by time range and drawing area
   - Converts symbol names to enum types
   - Validates pixmaps before use
   - Draws pixmap items on graph

## Issues Found

### 1. Syntax Error (Line 186)
```cpp
<< "- Sampling interval:" << samplingIntervalMs << "ms)";
```
**Problem**: Mismatched parenthesis - should be `"ms"` not `"ms)"`
**Fix**: Remove the closing parenthesis

### 2. Data Range Update (Line 60)
```cpp
updateDataRanges();
```
**Issue**: Always calls `updateDataRanges()` without checking `dataRangesValid`
**Impact**: May cause unnecessary recalculations
**Note**: This might be intentional to ensure ranges are always up-to-date

### 3. Excessive Debug Logging
**Issue**: Very verbose debug output in production code
**Recommendation**: Use conditional compilation or log levels

### 4. Memory Management
**Good**: QGraphicsItems are added to scene, which manages their lifetime
**Good**: Proper validation before creating items

## Code Quality

### Strengths
✅ **Pattern Consistency**: Symbols follow same pattern as R markers (stored in dataSource)
✅ **Error Handling**: Comprehensive validation of pixmaps and items
✅ **Safe Cache Access**: Uses `constFind()` in `RTWSymbolDrawing::get()`
✅ **Time Range Filtering**: Properly filters symbols by visible time range
✅ **Drawing Area Check**: Validates screen position before drawing

### Areas for Improvement
⚠️ **Debug Logging**: Too verbose for production
⚠️ **Error Recovery**: Could provide better fallback for missing symbols
⚠️ **Performance**: Could cache symbol type conversions

## Data Flow

1. **Symbol Addition**:
   ```
   addRTWSymbol() → dataSource->addRTWSymbol() → draw()
   ```

2. **Symbol Drawing**:
   ```
   draw() → drawRTWSymbols() → dataSource->getRTWSymbols() 
   → filter by time → filter by area → get pixmap → draw item
   ```

3. **R Marker Drawing**:
   ```
   draw() → drawCustomRMarkers() → dataSource->getYDataSeries() 
   → bin by time → filter by time → draw markers
   ```

## Recommendations

1. **Fix syntax error** on line 186
2. **Reduce debug logging** or use log levels
3. **Consider caching** symbol name to type conversions
4. **Add unit tests** for symbol drawing logic
5. **Document** the binning algorithm and sampling interval choice

## Testing Checklist

- [ ] Symbols persist through redraws
- [ ] Symbols scale correctly with zoom
- [ ] Symbols filter correctly by time range
- [ ] Invalid symbol names default to "R"
- [ ] Missing pixmaps are handled gracefully
- [ ] R markers display correctly with binning
- [ ] Fallback R markers work when binning fails

