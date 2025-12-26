# Callgrind Profile Analysis: callgrind.out.269853

## Executive Summary

**Program:** `./ui-sandbox`  
**Total Instructions:** 74,550,279,313 (74.55 billion)  
**Profiling Event:** Instruction reads (Ir)  
**Profile Trigger:** Program termination

## Top Performance Hotspots

### Application Code (ui-sandbox)

The following functions in your application code consume the most CPU time:

1. **`WaterfallGraph::mapDataToScreen(double, long long)`** - **2.26%** (1.69 billion instructions)
   - This is the single most expensive function in your application code
   - Called frequently for coordinate transformations

2. **`WaterfallGraph::updateScatterplotItemPositions(...)`** - **1.30%** (967 million instructions)
   - Updates positions of scatterplot graphics items
   - Significant overhead in graphics item manipulation

3. **`BTWGraph::drawBTWSymbols()`** - **0.38%** (285 million instructions)
   - Symbol drawing operations for BTW graphs

4. **`WaterfallGraph::updateScatterplotItemsIncremental(...)`** - **0.38%** (281 million instructions)
   - Incremental updates to scatterplot items

5. **`WaterfallGraph::drawBTWSymbols()`** - **0.33%** (245 million instructions)
   - Additional symbol drawing overhead

6. **`WaterfallGraph::updateVisibleDataCacheFull(QString const&)`** - **0.21%** (156 million instructions)
   - Cache updates for visible data ranges

7. **`WaterfallGraph::drawDataSeries(QString const&)`** - **0.18%** (134 million instructions)
   - Drawing data series on the graph

### Library Code (Qt/System)

The majority of time is spent in Qt and system libraries:

1. **Qt5Widgets.so** - **13.48%** (10.05 billion instructions)
   - Widget rendering and graphics operations

2. **Qt5Gui.so** - Multiple functions totaling **~15%**
   - Graphics rendering, painting operations
   - Transform operations

3. **libc.so.6** - Memory operations
   - `__memcpy_avx_unaligned_erms` - **2.38%** (1.77 billion)
   - `_int_malloc` - **1.19%** (887 million)
   - `_int_free` - **0.62%** (463 million)

## Key Observations

### 1. Coordinate Transformation Overhead
- `mapDataToScreen()` is the biggest bottleneck in application code
- This function is called extremely frequently (1.69 billion instructions)
- Consider caching transformation results or optimizing the algorithm

### 2. Graphics Item Management
- Significant overhead in updating scatterplot item positions
- Multiple calls to `QGraphicsItem::setPos()` (350M+ instructions)
- Consider batching position updates or using more efficient update strategies

### 3. STL Algorithm Usage
- Heavy use of `std::minmax_element` on vectors (492M+ instructions for doubles, 125M+ for QDateTime)
- Iterator operations consume significant time
- Consider pre-computing min/max values or caching results

### 4. Symbol Drawing
- BTW symbol drawing operations are expensive (284M + 245M = 529M total)
- May benefit from caching or reducing redraw frequency

### 5. Memory Allocation
- Notable overhead in memory management (~2% total)
- Consider object pooling or reducing allocations in hot paths

## Recommendations

### High Priority

1. **Optimize `mapDataToScreen()`**
   - Profile this function specifically to identify bottlenecks
   - Consider caching transformation matrices
   - Check if it's being called more than necessary

2. **Batch Graphics Updates**
   - Reduce individual `setPos()` calls
   - Use `QGraphicsScene::update()` with regions instead of per-item updates
   - Consider using `QGraphicsItem::prepareGeometryChange()` more efficiently

3. **Cache Min/Max Calculations**
   - Pre-compute min/max values for data ranges
   - Avoid recalculating in hot loops
   - Store results in data structures

### Medium Priority

4. **Optimize Symbol Drawing**
   - Cache symbol graphics items when possible
   - Reduce redraw frequency for static symbols
   - Consider using pixmap caching for frequently drawn symbols

5. **Reduce Iterator Overhead**
   - Use raw pointers or indices in hot loops instead of iterators
   - Minimize iterator comparisons and dereferences

6. **Data Structure Optimization**
   - Consider using more efficient containers for frequently accessed data
   - Pre-allocate vectors to avoid reallocations

### Low Priority

7. **Memory Pooling**
   - Consider object pools for frequently allocated/deallocated objects
   - Reduce temporary object creation in hot paths

## Performance Metrics

- **Total Instructions:** 74.55 billion
- **Application Code Share:** ~5-6% of total
- **Qt Library Share:** ~30-35% of total
- **System Library Share:** ~5-10% of total
- **Unidentified Code:** ~50-60% (likely Qt internals)

## Next Steps

1. Use `kcachegrind` or `qcachegrind` for visual analysis:
   ```bash
   kcachegrind callgrind.out.269853
   ```

2. Focus profiling on specific operations:
   - Run callgrind with `--instr-atstart=no` to skip initialization
   - Profile specific user interactions separately

3. Consider using `perf` for hardware-level profiling to identify:
   - Cache misses
   - Branch mispredictions
   - CPU pipeline stalls

