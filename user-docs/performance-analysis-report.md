# Performance Analysis Report
## Callgrind Profile Analysis - BTW Graph, RTW Graph, and Rendering Functions

**Date:** December 2025  
**Profile File:** `callgrind.out.79693`  
**Total Instructions:** 4,076,647,238  
**Analysis Tool:** Valgrind Callgrind

---

## Executive Summary

This report analyzes the performance characteristics of the UI rendering system, focusing on BTW Graph, RTW Graph, and core rendering functions. The analysis reveals significant improvements from previous optimizations, with `drawScatterplot()` remaining as the primary performance hotspot.

### Key Findings

- **Total execution cost:** 4.08 billion instructions
- **Largest hotspot:** `drawScatterplot()` at 28.19% of total cost
- **BTW Graph:** 16.19% of total cost, called 1,043 times
- **RTW Graph:** 1.68% of total cost, called 191 times
- **Render state optimization:** Highly effective - `graphicsScene->clear()` reduced by 99.97%

---

## Table of Contents

1. [Performance Metrics](#performance-metrics)
2. [BTW Graph Analysis](#btw-graph-analysis)
3. [RTW Graph Analysis](#rtw-graph-analysis)
4. [Core Rendering Functions](#core-rendering-functions)
5. [Performance Comparison](#performance-comparison)
6. [Key Insights](#key-insights)
7. [Recommendations](#recommendations)
8. [Technical Details](#technical-details)

---

## Performance Metrics

### Overall Statistics

| Metric | Value |
|-------|-------|
| Total Instructions | 4,076,647,238 |
| Profiling Period | Full program execution |
| Primary Event Type | Instruction Reads (Ir) |

### Top 5 Hotspots by Total Cost

| Function | Instructions | Percentage | Calls | Avg/Call |
|----------|-------------|------------|-------|----------|
| `drawScatterplot()` | 1,149,193,844 | 28.19% | 5,502 | 209,000 |
| `BTWGraph::draw()` | 659,945,119 | 16.19% | 1,043 | 632,000 |
| `RTWGraph::draw()` | 68,412,248 | 1.68% | 191 | 358,000 |
| `drawShadedRegions()` | 37,527,345 | 0.92% | 373 | 101,000 |
| `drawDataLine()` | ~15,000,000 | 0.29% | 387 | 39,000 |

---

## BTW Graph Analysis

### `BTWGraph::draw()`

**Performance Characteristics:**
- **Total Cost:** 659,945,119 instructions (16.19% of total)
- **Call Count:** 1,043 calls
- **Average per Call:** ~632,000 instructions
- **Efficiency:** Moderate - high call frequency but reasonable per-call cost

**Call Sources:**
- `GraphContainer::onTimeScopeChanged()`: 1,043 calls (660M instructions)
- `GraphContainer::onDataChanged()`: 271 calls (184M instructions)
- `GraphContainer::redrawWaterfallGraph()`: 156 calls (133M instructions)

**Internal Breakdown:**
- Calls `drawScatterplot()`: 5,502 times (1,149M instructions)
- Calls `drawShadedRegions()`: 373 times (38M instructions)
- Calls `drawDataLine()`: 339 times (3.1M instructions)

### `BTWGraph::drawShadedRegions()`

**Performance Characteristics:**
- **Total Cost:** 37,527,345 instructions (0.92% of total)
- **Call Count:** 373 calls
- **Average per Call:** ~100,600 instructions
- **Efficiency:** Good - only called during full redraws

**Optimization Status:**
- Only executed when `m_renderState == RenderState::FULL_REDRAW`
- Uses QMap iteration for shaded region management
- Minimal overhead compared to scatterplot rendering

---

## RTW Graph Analysis

### `RTWGraph::draw()`

**Performance Characteristics:**
- **Total Cost:** 68,412,248 instructions (1.68% of total)
- **Call Count:** 191 calls
- **Average per Call:** ~358,000 instructions
- **Efficiency:** Good - lower per-call cost than BTW Graph

**Call Sources:**
- `GraphContainer::onDataChanged()`: 89 calls (32M instructions)
- `GraphContainer::redrawWaterfallGraph()`: 52 calls (19M instructions)
- `GraphContainer::redrawWaterfallGraph(GraphType)`: 4 calls (36K instructions)

**Internal Breakdown:**
- Calls `drawScatterplot()`: 394 times (103M instructions)
- Calls `drawDataLine()`: 48 times (12M instructions)
- Calls `drawRTWSymbols()`: Minimal cost (90K instructions)

**Comparison with BTW:**
- RTW is **43% more efficient** per call (358K vs 632K instructions)
- RTW has **82% fewer calls** (191 vs 1,043)
- RTW has **90% lower total cost** (68M vs 660M instructions)

---

## Core Rendering Functions

### `WaterfallGraph::drawScatterplot()`

**Performance Characteristics:**
- **Total Cost:** 1,149,193,844 instructions (28.19% of total)
- **Call Count:** 5,502 calls from BTW, 394 calls from RTW
- **Average per Call:** ~209,000 instructions
- **Status:** **PRIMARY PERFORMANCE HOTSPOT**

**Call Distribution:**
- From `BTWGraph::draw()`: 5,502 calls (1,149M instructions)
- From `RTWGraph::draw()`: 394 calls (103M instructions)

**Optimization Opportunities:**
1. **Caching:** Already uses visible data cache, but could be more aggressive
2. **Incremental Updates:** State machine is working, but could reduce full rebuilds
3. **Position Calculations:** `mapDataToScreen()` called for every point
4. **Item Management:** Pixmap item creation/updates could be optimized

### `WaterfallGraph::drawDataLine()`

**Performance Characteristics:**
- **Total Cost:** ~15,000,000 instructions (0.29% of total)
- **Call Count:** 387 calls total
  - From RTW: 48 calls (12M instructions)
  - From BTW: 339 calls (3.1M instructions)
- **Average per Call:** ~39,000 instructions
- **Efficiency:** Good - relatively low cost

**Usage:**
- RTW uses for "ADOPTED" series line rendering
- BTW uses for data series line rendering
- Only called when `needsFullClear` is true (full redraw)

### `QGraphicsScene::clear()`

**Performance Characteristics:**
- **Total Cost:** 340,490 instructions (0.01% of total)
- **Status:** **HIGHLY OPTIMIZED** ✅
- **Optimization Impact:** 99.97% reduction from previous profile

**Analysis:**
- Only called when `m_renderState == RenderState::FULL_REDRAW`
- Render state machine is working effectively
- Minimal overhead compared to other operations

---

## Performance Comparison

### Comparison with Previous Profile (`callgrind.out.73241`)

| Function | Previous | Current | Improvement |
|----------|----------|---------|-------------|
| **Total Instructions** | 7.69B | 4.08B | **47% reduction** |
| **BTWGraph::draw()** | 4.27B (48.5%) | 660M (16.19%) | **85% reduction** |
| **graphicsScene->clear()** | 1.1B (14.3%) | 340K (0.01%) | **99.97% reduction** |
| **drawScatterplot()** | 3.03B (39.4%) | 1.15B (28.19%) | **62% reduction** |
| **drawShadedRegions()** | 286M (3.7%) | 38M (0.92%) | **87% reduction** |

### Key Improvements

1. **Render State Machine:** Successfully reduced unnecessary full redraws
2. **Scene Clearing:** Dramatically reduced by only clearing on FULL_REDRAW
3. **Overall Efficiency:** 47% reduction in total instruction count
4. **BTW Graph:** 85% reduction in draw() cost

### Remaining Challenges

1. **drawScatterplot():** Still the largest hotspot (28.19%)
2. **BTW Call Frequency:** 1,043 calls is still high
3. **Incremental Updates:** Could be more aggressive

---

## Key Insights

### 1. Render State Optimization Success

The render state machine implementation has been highly successful:
- `graphicsScene->clear()` reduced from 1.1B to 340K instructions
- Only called when truly necessary (FULL_REDRAW state)
- Prevents unnecessary scene clearing on incremental updates

### 2. drawScatterplot() Dominance

`drawScatterplot()` is now the primary performance bottleneck:
- 28.19% of total execution cost
- Called 5,502 times from BTW Graph alone
- Average 209K instructions per call
- **Next optimization target**

### 3. BTW vs RTW Efficiency

RTW Graph is significantly more efficient:
- 43% lower per-call cost
- 82% fewer calls
- 90% lower total cost
- Suggests BTW has optimization opportunities

### 4. Call Frequency Patterns

- BTW Graph: High call frequency (1,043 calls) suggests potential for debouncing/throttling
- RTW Graph: Lower call frequency (191 calls) indicates better update management
- Consider implementing update batching for BTW Graph

---

## Recommendations

### Priority 1: Optimize drawScatterplot()

**Current Status:** 28.19% of total cost, 5,502 calls from BTW

**Recommended Actions:**
1. **Profile Internals:** Use detailed profiling to identify specific bottlenecks within `drawScatterplot()`
2. **Aggressive Caching:** 
   - Cache `mapDataToScreen()` results when time range hasn't changed
   - Pre-calculate screen positions for visible data points
3. **Reduce Item Creation:**
   - Reuse existing pixmap items when possible
   - Batch item updates instead of individual updates
4. **Optimize Data Filtering:**
   - Improve visible data cache hit rate
   - Use more efficient data structures for time range queries

### Priority 2: Reduce BTW Graph Call Frequency

**Current Status:** 1,043 calls, 16.19% of total cost

**Recommended Actions:**
1. **Debouncing/Throttling:**
   - Implement update batching for rapid successive calls
   - Use QTimer to batch multiple updates into single redraw
2. **Smarter Update Triggers:**
   - Only trigger redraw when data actually changes
   - Use dirty flags to track what needs updating
3. **Incremental Updates:**
   - Increase use of INCREMENTAL_UPDATE state
   - Reduce FULL_REDRAW frequency

### Priority 3: Optimize BTW Graph Per-Call Cost

**Current Status:** 632K instructions per call (vs 358K for RTW)

**Recommended Actions:**
1. **Compare with RTW:** Identify why RTW is 43% more efficient
2. **Reduce Redundant Work:**
   - Cache calculations that don't change between calls
   - Skip unnecessary operations when state hasn't changed
3. **Optimize Shaded Regions:**
   - Review `drawShadedRegions()` for optimization opportunities
   - Consider caching region polygons

### Priority 4: Further Incremental Update Optimization

**Current Status:** Render state machine working, but could be more aggressive

**Recommended Actions:**
1. **Expand INCREMENTAL_UPDATE Usage:**
   - Use for more scenarios (e.g., time range shifts)
   - Reduce FULL_REDRAW requirements
2. **Position-Only Updates:**
   - When only positions change, update items instead of recreating
   - Use RANGE_UPDATE_ONLY state more frequently

---

## Technical Details

### Render State Machine

The render state machine uses four states:
- `CLEAN`: No updates needed
- `RANGE_UPDATE_ONLY`: Only position updates needed
- `INCREMENTAL_UPDATE`: Partial updates (add/remove items)
- `FULL_REDRAW`: Complete scene clear and rebuild

**Current Effectiveness:**
- Successfully prevents unnecessary `graphicsScene->clear()` calls
- Reduces total instruction count by 47% compared to previous profile
- Could be expanded to cover more update scenarios

### Call Patterns

**BTW Graph:**
- High frequency: 1,043 calls during execution
- Primarily triggered by time scope changes
- Each call triggers multiple `drawScatterplot()` calls

**RTW Graph:**
- Lower frequency: 191 calls during execution
- More selective update triggers
- More efficient per-call execution

### Memory Management

**Item Cleanup:**
- `m_seriesScatterplotItems.clear()` before `graphicsScene->clear()`
- `m_seriesPathItems.clear()` before `graphicsScene->clear()`
- `m_seriesPointItems.clear()` before `graphicsScene->clear()`
- Prevents use-after-free crashes

**Optimization Status:**
- Proper cleanup prevents memory issues
- No significant memory-related performance issues detected

---

## Conclusion

The performance analysis reveals significant improvements from previous optimizations, particularly in the render state machine implementation. The `graphicsScene->clear()` operation has been reduced by 99.97%, and overall instruction count has decreased by 47%.

However, `drawScatterplot()` remains the primary performance bottleneck at 28.19% of total cost. This function should be the next focus for optimization, with particular attention to:

1. Internal profiling to identify specific bottlenecks
2. More aggressive caching of calculations
3. Reduced item creation overhead
4. Optimized data filtering and position calculations

The BTW Graph also shows opportunities for optimization, with 1,043 calls compared to RTW's 191 calls, suggesting potential for debouncing/throttling mechanisms.

---

## Appendix: Profile Statistics

### Function Call Summary

| Function | Calls | Total Cost | Avg/Call | % of Total |
|----------|-------|------------|----------|------------|
| `drawScatterplot()` | 5,896 | 1,149M | 209K | 28.19% |
| `BTWGraph::draw()` | 1,043 | 660M | 632K | 16.19% |
| `RTWGraph::draw()` | 191 | 68M | 358K | 1.68% |
| `drawShadedRegions()` | 373 | 38M | 101K | 0.92% |
| `drawDataLine()` | 387 | 15M | 39K | 0.29% |
| `graphicsScene->clear()` | ~373 | 340K | 912 | 0.01% |

### Call Chain Analysis

**BTW Graph Draw Chain:**
```
GraphContainer::onTimeScopeChanged()
  → BTWGraph::draw() [1,043 calls, 660M instructions]
    → drawScatterplot() [5,502 calls, 1,149M instructions]
    → drawShadedRegions() [373 calls, 38M instructions]
    → drawDataLine() [339 calls, 3.1M instructions]
```

**RTW Graph Draw Chain:**
```
GraphContainer::onDataChanged()
  → RTWGraph::draw() [191 calls, 68M instructions]
    → drawScatterplot() [394 calls, 103M instructions]
    → drawDataLine() [48 calls, 12M instructions]
    → drawRTWSymbols() [minimal cost]
```

---

**Report Generated:** December 2025  
**Profile File:** `callgrind.out.79693`  
**Analysis Tool:** Valgrind Callgrind 3.22.0






