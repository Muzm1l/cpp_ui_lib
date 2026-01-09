<!-- event-driven-architecture-redesign-2024 -->
# Plan: Event-Driven Architecture Redesign for WaterfallGraph

## Problem

Current architecture has significant performance bottlenecks identified by callgrind analysis:

1. **Multiple Data Copies**: Data flows through multiple layers with copies at each step:
   - CircularBuffer → `populateYDataSeriesFloat()` → vector → CircularBuffer → `getVisibleDataVector()` → vector
   - 34,071 calls to `populateYDataSeriesFloat` = 194M instructions
   - 34,071 calls to `populateTimestampsEpochSeries` = 194M instructions

2. **Reactive Validation Overhead**: Cache validation happens on every access:
   - `ensureVisibleDataCacheValid()` called 45,689 times
   - Each call does map lookups (45,241 `std::map::find` operations)
   - Total: 431M instructions in cache validation

3. **Tight Coupling**: Data storage, filtering, and rendering are tightly coupled:
   - No separation of concerns
   - Difficult to optimize individual layers
   - Cache validation mixed with rendering logic

4. **Inefficient Data Access**: 
   - `populateYDataSeriesFloat()` copies entire CircularBuffer to vector every time
   - No zero-copy access pattern
   - Redundant data transformations

5. **Map Lookup Overhead**:
   - Many `std::map::find` operations (O(log n))
   - Should use `unordered_map` for O(1) lookups where order isn't required

## Solution

Redesign to event-driven architecture with zero-copy data access:

### Architecture Overview

```
┌─────────────────┐
│  WaterfallData  │  (Data Storage Layer)
│  (CircularBuffer │
│   per series)    │
└────────┬────────┘
         │ Direct access (no copy)
         ▼
┌─────────────────┐
│  DataView       │  (View Layer - zero-copy)
│  - Iterator     │
│  - Range filter │
└────────┬────────┘
         │ Event-driven updates
         ▼
┌─────────────────┐
│  RenderCache    │  (Cache Layer)
│  - Versioned    │
│  - Event-driven │
└────────┬────────┘
         │ On-demand rendering
         ▼
┌─────────────────┐
│  Renderer       │  (Rendering Layer)
│  - Direct draw  │
│  - Batched ops  │
└─────────────────┘
```

### Key Components

1. **DataView**: Iterator-based, zero-copy access to CircularBuffer
2. **RenderCache**: Event-driven cache with version tracking
3. **Enhanced GraphEngine**: Granular event emission
4. **Refactored WaterfallGraph**: Event handlers replace reactive validation

## Implementation Steps

### Phase 1: DataView Layer (Zero-Copy Access)

**File**: `waterfalldataview.h` (new)

```cpp
class WaterfallDataView {
public:
    class ConstIterator {
    private:
        const CircularBuffer<float>* m_yData;
        const CircularBuffer<qint64>* m_timestamps;
        size_t m_index;
        
    public:
        ConstIterator(const CircularBuffer<float>* y, 
                     const CircularBuffer<qint64>* t, 
                     size_t idx)
            : m_yData(y), m_timestamps(t), m_index(idx) {}
        
        std::pair<float, qint64> operator*() const {
            return {(*m_yData)[m_index], (*m_timestamps)[m_index]};
        }
        
        ConstIterator& operator++() { ++m_index; return *this; }
        bool operator!=(const ConstIterator& other) const {
            return m_index != other.m_index;
        }
    };
    
    WaterfallDataView(const WaterfallData* data, const QString& seriesLabel);
    
    ConstIterator begin() const;
    ConstIterator end() const;
    size_t size() const;
    
    // Binary search for time range (O(log n))
    std::pair<ConstIterator, ConstIterator> 
    findTimeRange(qint64 minEpoch, qint64 maxEpoch) const;
    
private:
    const CircularBuffer<float>* m_yData;
    const CircularBuffer<qint64>* m_timestamps;
    size_t m_size;
};
```

**Benefits**:
- Eliminates `populateYDataSeriesFloat()` calls
- Zero-copy data access
- Iterator-based API for efficient iteration

### Phase 2: Event-Driven Cache System

**File**: `rendercache.h` (new)

```cpp
class RenderCache {
public:
    struct SeriesCache {
        uint64_t dataVersion;      // From WaterfallData
        QDateTime cachedTimeMin, cachedTimeMax;
        size_t cachedDataSize;
        CircularBuffer<QPointF> screenPoints;  // Pre-computed screen coords
        bool needsUpdate;
    };
    
    // Event-driven invalidation (no polling)
    void notifyDataChanged(const QString& series, uint64_t newVersion);
    void notifyTimeRangeChanged(const QDateTime& min, const QDateTime& max);
    void notifySeriesAdded(const QString& series);
    void notifySeriesRemoved(const QString& series);
    
    // Direct access (no validation overhead)
    SeriesCache& getOrCreate(const QString& series);
    const SeriesCache* find(const QString& series) const;
    
    // Batch operations
    void invalidateAll();
    void updateAll(const std::function<void(const QString&, SeriesCache&)>& updater);
    
private:
    std::unordered_map<QString, SeriesCache> m_caches;  // O(1) lookup
    QDateTime m_currentTimeMin, m_currentTimeMax;
};
```

**Benefits**:
- Eliminates `ensureVisibleDataCacheValid()` polling
- Event-driven updates only when data changes
- O(1) map lookups instead of O(log n)

### Phase 3: Enhanced GraphEngine

**File**: `graphengine.h` (enhance existing)

Add new methods and signals:

```cpp
class GraphEngine : public QObject
{
    Q_OBJECT

public:
    // NEW: Direct data access (zero-copy)
    WaterfallDataView getDataView(const QString& seriesLabel) const;
    
    // NEW: Version tracking per series
    uint64_t getSeriesDataVersion(const QString& seriesLabel) const;
    uint64_t getGlobalDataVersion() const;
    
    // NEW: Batch operations with single event
    void addDataPointsBatch(const QString& seriesLabel, 
                           const std::vector<float>& yValues,
                           const std::vector<QDateTime>& timestamps);
    
signals:
    // Existing signals (kept for backward compatibility)
    void dataAppended(const QString &seriesLabel);
    void dataRangeChanged();
    void symbolsChanged();
    void markersChanged();
    
    // NEW: Enhanced granular signals
    void seriesDataChanged(const QString& seriesLabel, uint64_t newVersion);
    void seriesAdded(const QString& seriesLabel);
    void seriesRemoved(const QString& seriesLabel);
    void timeRangeChanged(const QDateTime& min, const QDateTime& max);
    
    // NEW: Batch update signal (for high-frequency updates)
    void batchDataAppended(const QString& seriesLabel, size_t pointCount);
};
```

**File**: `graphengine.cpp` (enhance existing)

```cpp
void GraphEngine::addDataPoint(const QString &seriesLabel, 
                               float yValue, 
                               const QDateTime &timestamp)
{
    m_data.addDataPointToSeries(seriesLabel, yValue, timestamp);
    
    // Increment version counter (in WaterfallData)
    uint64_t newVersion = m_data.getDataVersion();
    
    // Emit granular event with version
    emit seriesDataChanged(seriesLabel, newVersion);
    
    // Emit legacy signal (backward compatibility)
    emit dataAppended(seriesLabel);
    emit dataRangeChanged();
}

WaterfallDataView GraphEngine::getDataView(const QString& seriesLabel) const
{
    return WaterfallDataView(&m_data, seriesLabel);
}
```

**Benefits**:
- Granular events reduce unnecessary work
- Version tracking enables efficient cache updates
- Batch operations for high-frequency updates

### Phase 4: Refactored WaterfallGraph

**File**: `waterfallgraph.h` (refactor existing)

Replace reactive validation with event handlers:

```cpp
class WaterfallGraph : public QWidget
{
private:
    // Event-driven cache (replaces reactive validation)
    RenderCache m_renderCache;
    
    // Event handlers (replaces ensureVisibleDataCacheValid)
    void onSeriesDataChanged(const QString& series, uint64_t newVersion);
    void onSeriesAdded(const QString& series);
    void onSeriesRemoved(const QString& series);
    void onTimeRangeChanged(const QDateTime& min, const QDateTime& max);
    void onBatchDataAppended(const QString& series, size_t pointCount);
    
    // Direct data access (replaces populateYDataSeriesFloat)
    WaterfallDataView getDataView(const QString& series) const;
    
    // Rendering (direct, no intermediate copies)
    void renderSeriesDirect(const QString& series);
    void updateCacheFromView(const QString& series, 
                            const WaterfallDataView& view,
                            RenderCache::SeriesCache& cache);
    
public slots:
    // Connected to GraphEngine signals
    void handleSeriesDataChanged(const QString& series, uint64_t newVersion);
    void handleBatchDataAppended(const QString& series, size_t pointCount);
};
```

**File**: `waterfallgraph.cpp` (refactor existing)

Event-driven `attachEngine()`:

```cpp
void WaterfallGraph::attachEngine(GraphEngine *engine)
{
    if (m_engine == engine) {
        return;
    }
    
    // Disconnect old engine
    if (m_engine) {
        disconnect(m_engine, nullptr, this, nullptr);
        m_renderCache.invalidateAll();
    }
    
    m_engine = engine;
    
    if (m_engine) {
        dataSource = m_engine->dataMutable();
        
        // Connect to enhanced granular signals
        connect(m_engine, &GraphEngine::seriesDataChanged,
                this, &WaterfallGraph::handleSeriesDataChanged,
                Qt::QueuedConnection);  // Batch updates in event loop
        
        connect(m_engine, &GraphEngine::batchDataAppended,
                this, &WaterfallGraph::handleBatchDataAppended,
                Qt::QueuedConnection);
        
        connect(m_engine, &GraphEngine::seriesAdded,
                this, [this](const QString& series) {
                    m_renderCache.notifySeriesAdded(series);
                    markSeriesDirty(series);
                    scheduleRender();
                });
        
        // Legacy signals (backward compatibility)
        connect(m_engine, &GraphEngine::dataRangeChanged,
                this, [this]() {
                    if (!isVisible()) return;
                    m_renderCache.notifyTimeRangeChanged(timeMin, timeMax);
                    scheduleRender();
                });
        
        // Initialize cache with current data
        initializeCacheFromEngine();
        
        // Initial render
        setRenderState(RenderState::FULL_REDRAW);
        scheduleRender();
    }
}

void WaterfallGraph::handleSeriesDataChanged(const QString& series, 
                                             uint64_t newVersion)
{
    if (!isVisible()) return;
    
    // Update cache directly (no validation needed - event tells us it changed)
    auto& cache = m_renderCache.getOrCreate(series);
    
    if (cache.dataVersion != newVersion) {
        cache.needsUpdate = true;
        cache.dataVersion = newVersion;
        
        // Mark series for rendering
        markSeriesDirty(series);
        scheduleRender();
    }
}

void WaterfallGraph::renderSeriesDirect(const QString& series)
{
    auto view = getDataView(series);
    auto& cache = m_renderCache.getOrCreate(series);
    
    if (cache.needsUpdate) {
        updateCacheFromView(series, view, cache);
    }
    
    renderCachedPoints(cache.screenPoints, getSeriesColor(series));
}

void WaterfallGraph::updateCacheFromView(const QString& series, 
                                        const WaterfallDataView& view,
                                        RenderCache::SeriesCache& cache)
{
    // Direct iteration, no copies
    cache.screenPoints.clear();
    auto range = view.findTimeRange(m_cachedTimeMinEpoch, m_cachedTimeMaxEpoch);
    
    for (auto it = range.first; it != range.second; ++it) {
        auto [yValue, timestamp] = *it;
        QPointF screen = mapDataToScreen(yValue, timestamp);
        if (isValidScreenPoint(screen)) {
            cache.screenPoints.push_back(screen);
        }
    }
    
    cache.needsUpdate = false;
    cache.dataVersion = dataSource->getDataVersion();
}
```

**Benefits**:
- Eliminates reactive validation overhead
- Zero-copy data access
- Event-driven cache updates
- Cleaner separation of concerns

## Migration Strategy

### Step 1: Add New Components (Non-Breaking)
- Create `waterfalldataview.h` and implementation
- Create `rendercache.h` and implementation
- Add new signals to `GraphEngine` (keep legacy signals)
- Test new components in isolation

### Step 2: Parallel Implementation
- Add `RenderCache` to `WaterfallGraph` alongside existing cache
- Connect to new signals alongside legacy signals
- Gradually migrate rendering functions to use new cache

### Step 3: Refactor Rendering
- Replace `drawDataLine()` to use `renderSeriesDirect()`
- Replace `drawDataSeries()` to use `renderSeriesDirect()`
- Update `paintEvent()` to use cached screen points

### Step 4: Remove Legacy Code
- Remove `ensureVisibleDataCacheValid()`
- Remove `populateYDataSeriesFloat()` calls
- Remove `updateVisibleDataCacheIncremental()` / `Full()`
- Remove old cache validation logic

### Step 5: Optimize Data Structures
- Replace `std::map` with `std::unordered_map` where order isn't required
- Optimize cache update algorithms
- Add batch rendering optimizations

## Expected Performance Improvements

Based on callgrind analysis:

1. **Data Access**: 
   - Eliminate 34K+ `populateYDataSeriesFloat()` calls
   - **Savings**: ~388M instructions (194M + 194M)

2. **Cache Validation**:
   - Eliminate 45K+ `ensureVisibleDataCacheValid()` calls
   - **Savings**: ~431M instructions

3. **Map Lookups**:
   - O(1) vs O(log n) for series lookups
   - **Savings**: ~50% faster lookups

4. **Memory**:
   - Eliminate redundant data copies
   - **Savings**: Reduced memory allocations

**Total Expected Improvement**: ~819M instructions saved (9% of total 9.1B instructions)

## Event Flow Diagram

```
┌─────────────────────────────────────────────────────────┐
│ External Code (e.g., GraphLayout)                       │
│   engine->addDataPoint("series1", 1.0, timestamp)       │
└────────────────────┬────────────────────────────────────┘
                    │
                    ▼
┌─────────────────────────────────────────────────────────┐
│ GraphEngine                                              │
│   1. m_data.addDataPointToSeries(...)                   │
│   2. m_data.incrementVersion()                          │
│   3. emit seriesDataChanged("series1", newVersion)      │
└────────────────────┬────────────────────────────────────┘
                    │ Qt Signal/Slot
                    ▼
┌─────────────────────────────────────────────────────────┐
│ WaterfallGraph::handleSeriesDataChanged()               │
│   1. Check visibility                                    │
│   2. Update RenderCache (event-driven, no validation)  │
│   3. markSeriesDirty("series1")                          │
│   4. scheduleRender()                                    │
└────────────────────┬────────────────────────────────────┘
                    │
                    ▼
┌─────────────────────────────────────────────────────────┐
│ RenderCache                                              │
│   - Cache knows it needs update (from event)            │
│   - No validation overhead                               │
└────────────────────┬────────────────────────────────────┘
                    │
                    ▼
┌─────────────────────────────────────────────────────────┐
│ renderSeriesDirect()                                     │
│   1. getDataView("series1") - zero-copy access          │
│   2. Update cache from view (if needed)                 │
│   3. Render cached screen points                        │
└─────────────────────────────────────────────────────────┘
```

## Notes

- **Backward Compatibility**: Legacy signals and methods maintained during migration
- **Gradual Migration**: Can be implemented incrementally without breaking existing code
- **Testing**: Each phase should be tested before moving to next phase
- **Performance Monitoring**: Use callgrind to verify improvements at each phase

## Status

**Status**: Planned - To be implemented when all features are in place

**Priority**: High (addresses major performance bottlenecks)

**Estimated Impact**: 9% reduction in total instructions, significant memory savings

