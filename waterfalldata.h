#ifndef WATERFALLDATA_H
#define WATERFALLDATA_H

#include "circularbuffer.h"
#include <vector>
#include <utility>
#include <map>
#include <cstdint>
#include <QDateTime>
#include <QTime>
#include <QDebug>
#include <QString>

// Forward declaration for RTW symbols
struct RTWSymbolData
{
    QString symbolName;
    QDateTime timestamp;
    float range;
};

// Forward declaration for BTW symbols
struct BTWSymbolData
{
    QString symbolName;
    QDateTime timestamp;
    float range;
};

// Forward declaration for BTW markers
struct BTWMarkerData
{
    QDateTime timestamp;
    float range;  // Y-axis position (range value)
    float delta; // Delta value for angle calculation
};

// Forward declaration for RTW R markers
struct RTWRMarkerData
{
    QDateTime timestamp;
    float range; // Y-axis position (range value)
};

class WaterfallData
{
public:
    WaterfallData(const QString& title = "");
    WaterfallData(const QString& title, const std::vector<QString>& seriesLabels);
    ~WaterfallData();

    // Data management methods
    void setData(const std::vector<float>& yData, const std::vector<QDateTime>& timestamps);
    void clearData();

    QDateTime getLatestTime() const;
    QDateTime getEarliestTime() const;

    qreal getMinY() const;
    qreal getMaxY() const;
    
    // Utility methods
    bool isEmpty() const;

    // Data range methods
    std::pair<qreal, qreal> getYRange() const;
    std::pair<QDateTime, QDateTime> getTimeRange() const;

    

    // Time-based utility methods
    qint64 getTimeSpanMs() const;

    // Selection time span methods
    QDateTime getSelectionEarliestTime() const;
    QDateTime getSelectionLatestTime() const;
    qint64 getSelectionTimeSpanMs() const;
    bool isValidSelectionTime(const QDateTime& time) const;

    // Data title methods
    void setDataTitle(const QString& title) { dataTitle = title; }
    QString getDataTitle() const { return dataTitle; }

    // Multiple data series methods
    void addDataSeries(const QString& seriesLabel, const std::vector<float>& yData, const std::vector<QDateTime>& timestamps);
    void addDataPointToSeries(const QString& seriesLabel, float yValue, const QDateTime& timestamp);
    void addDataPointsToSeries(const QString& seriesLabel, const std::vector<float>& yValues, const std::vector<QDateTime>& timestamps);
    void clearDataSeries(const QString& seriesLabel);
    void clearAllDataSeries();

    // Data series access methods
    std::vector<std::pair<qreal, QDateTime>> getDataSeries(const QString& seriesLabel) const;
    std::vector<std::pair<qreal, QDateTime>> getDataSeriesWithinYExtents(const QString& seriesLabel, qreal yMin, qreal yMax) const;
    std::vector<std::pair<qreal, QDateTime>> getDataSeriesWithinTimeRange(const QString& seriesLabel, const QDateTime& startTime, const QDateTime& endTime) const;
    
    // Binary search helper: find closest data point to a timestamp within tolerance
    bool findClosestDataPoint(const QString& seriesLabel, const QDateTime& targetTime, qint64 toleranceMs, qreal& outValue, size_t& outIndex) const;

    // Direct access to data series vectors
    // Note: Returns vectors converted from circular buffers (chronological order, oldest first)
    std::vector<qreal> getYDataSeries(const QString& seriesLabel) const;
    std::vector<QDateTime> getTimestampsSeries(const QString& seriesLabel) const;
    
    // Reusable vector population methods (avoids toVector() allocations)
    // These methods populate a reusable vector passed by reference, avoiding temporary allocations
    void populateYDataSeries(const QString& seriesLabel, std::vector<qreal>& output) const;
    void populateTimestampsSeries(const QString& seriesLabel, std::vector<QDateTime>& output) const;
    void populateTimestampsEpochSeries(const QString& seriesLabel, std::vector<qint64>& output) const;
    std::vector<qint64> getTimestampsEpochSeries(const QString& seriesLabel) const; // Epoch milliseconds (no timezone conversion)

    // Data series utility methods
    size_t getDataSeriesSize(const QString& seriesLabel) const;
    bool isDataSeriesEmpty(const QString& seriesLabel) const;
    bool hasDataSeries(const QString& seriesLabel) const;
    std::vector<QString> getDataSeriesLabels() const;

    // Data series range methods
    std::pair<qreal, qreal> getYRangeSeries(const QString& seriesLabel) const;
    std::pair<QDateTime, QDateTime> getTimeRangeSeries(const QString& seriesLabel) const;

    // Series-specific versions of legacy methods
    void setDataSeries(const QString& seriesLabel, const std::vector<float>& yData, const std::vector<QDateTime>& timestamps);
    std::vector<std::pair<qreal, QDateTime>> getAllDataSeries(const QString& seriesLabel) const;
    qreal getMinYSeries(const QString& seriesLabel) const;
    qreal getMaxYSeries(const QString& seriesLabel) const;
    qint64 getTimeSpanMsSeries(const QString& seriesLabel) const;
    QDateTime getEarliestTimeSeries(const QString& seriesLabel) const;
    QDateTime getLatestTimeSeries(const QString& seriesLabel) const;
    bool isValidIndexSeries(const QString& seriesLabel, size_t index) const;
    bool isValidSelectionTimeSeries(const QString& seriesLabel, const QDateTime& time) const;

    // Combined range methods for all series
    std::pair<qreal, qreal> getCombinedYRange() const;
    std::pair<QDateTime, QDateTime> getCombinedTimeRange() const;
    
    // Phase 1: Get data version for cache invalidation
    uint64_t getDataVersion() const { return m_dataVersion; }

    // Data binning methods for sampling
    std::vector<std::pair<qreal, QDateTime>> getBinnedDataSeries(const QString& seriesLabel, const QTime& binDuration) const;
    
    // Static binning method that doesn't depend on class state
    static std::vector<std::pair<qreal, QDateTime>> binDataByTime(
        const std::vector<qreal>& yData, 
        const std::vector<QDateTime>& timestamps, 
        const QTime& binDuration
    );

    // RTW Symbol management methods (stored with track data)
    void addRTWSymbol(const QString& symbolName, const QDateTime& timestamp, float range);
    void clearRTWSymbols();
    bool removeRTWSymbol(const QString& symbolName, const QDateTime& timestamp, float range, float toleranceMs = 1000, float rangeTolerance = 0.1f);
    std::vector<RTWSymbolData> getRTWSymbols() const;
    size_t getRTWSymbolsCount() const;

    // BTW Symbol management methods (stored with track data)
    void addBTWSymbol(const QString& symbolName, const QDateTime& timestamp, float range);
    void clearBTWSymbols();
    std::vector<BTWSymbolData> getBTWSymbols() const;
    std::vector<BTWSymbolData> getBTWSymbolsWithinTimeRange(const QDateTime& startTime, const QDateTime& endTime) const;
    size_t getBTWSymbolsCount() const;

    // BTW Marker management methods (manually placed markers)
    void addBTWMarker(const QDateTime& timestamp, float range, float delta);
    void clearBTWMarkers();
    bool removeBTWMarker(const QDateTime& timestamp, float range, float toleranceMs = 1000, float rangeTolerance = 0.1f);
    std::vector<BTWMarkerData> getBTWMarkers() const;
    std::vector<BTWMarkerData> getBTWMarkersWithinTimeRange(const QDateTime& startTime, const QDateTime& endTime) const;
    size_t getBTWMarkersCount() const;

    // RTW R Marker management methods (manually placed markers)
    void addRTWRMarker(const QDateTime& timestamp, float range);
    void clearRTWRMarkers();
    bool removeRTWRMarker(const QDateTime& timestamp, float range, float toleranceMs = 1000, float rangeTolerance = 0.1f);
    std::vector<RTWRMarkerData> getRTWRMarkers() const;
    size_t getRTWRMarkersCount() const;

    // Capacity management methods - set circular buffer capacity to limit memory growth
    /**
     * @brief Set circular buffer capacity for data series vectors
     * @param seriesLabel The series label to set capacity for
     * @param capacity Maximum number of elements (0 = unlimited, not recommended)
     */
    void setDataSeriesCapacity(const QString& seriesLabel, size_t capacity);
    
    /**
     * @brief Set circular buffer capacity for all data series vectors
     * @param capacity Maximum number of elements for each series (0 = unlimited)
     */
    void setAllDataSeriesCapacity(size_t capacity);
    
    /**
     * @brief Set circular buffer capacity for RTW symbols vector
     * @param capacity Maximum number of elements (0 = unlimited)
     */
    void setRTWSymbolsCapacity(size_t capacity);
    
    /**
     * @brief Set circular buffer capacity for BTW symbols vector
     * @param capacity Maximum number of elements (0 = unlimited)
     */
    void setBTWSymbolsCapacity(size_t capacity);
    
    /**
     * @brief Set circular buffer capacity for BTW markers vector
     * @param capacity Maximum number of elements (0 = unlimited)
     */
    void setBTWMarkersCapacity(size_t capacity);
    
    /**
     * @brief Set circular buffer capacity for RTW R markers vector
     * @param capacity Maximum number of elements (0 = unlimited)
     */
    void setRTWRMarkersCapacity(size_t capacity);
    
    /**
     * @brief Set circular buffer capacity for all symbol and marker vectors
     * @param symbolsCapacity Maximum capacity for RTW and BTW symbols
     * @param markersCapacity Maximum capacity for BTW and RTW R markers
     */
    void setAllSymbolsAndMarkersCapacity(size_t symbolsCapacity, size_t markersCapacity);
    
    // Legacy reserve methods (now call setCapacity for circular buffers)
    void reserveDataSeriesCapacity(const QString& seriesLabel, size_t capacity) { setDataSeriesCapacity(seriesLabel, capacity); }
    void reserveAllDataSeriesCapacity(size_t capacity) { setAllDataSeriesCapacity(capacity); }
    void reserveRTWSymbolsCapacity(size_t capacity) { setRTWSymbolsCapacity(capacity); }
    void reserveBTWSymbolsCapacity(size_t capacity) { setBTWSymbolsCapacity(capacity); }
    void reserveBTWMarkersCapacity(size_t capacity) { setBTWMarkersCapacity(capacity); }
    void reserveRTWRMarkersCapacity(size_t capacity) { setRTWRMarkersCapacity(capacity); }
    void reserveAllSymbolsAndMarkersCapacity(size_t symbolsCapacity, size_t markersCapacity) { setAllSymbolsAndMarkersCapacity(symbolsCapacity, markersCapacity); }

private:

    // Multiple data series storage - using circular buffers to prevent unbounded growth
    std::map<QString, CircularBuffer<float>> dataSeriesYData;
    std::map<QString, CircularBuffer<QDateTime>> dataSeriesTimestamps;
    std::map<QString, CircularBuffer<qint64>> dataSeriesTimestampsEpoch; // Parallel storage for epoch milliseconds (performance optimization)

    // RTW Symbol storage (persists with track data) - using circular buffer
    CircularBuffer<RTWSymbolData> rtwSymbols;
    
    // BTW Symbol storage (persists with track data) - using circular buffer
    CircularBuffer<BTWSymbolData> btwSymbols;

    // BTW Marker storage (manually placed markers) - using circular buffer
    CircularBuffer<BTWMarkerData> btwMarkers;

    // RTW R Marker storage (manually placed markers) - using circular buffer
    CircularBuffer<RTWRMarkerData> rtwRMarkers;

    // Data title
    QString dataTitle;

    // Phase 1 Performance Optimization: Range calculation caching
    mutable std::pair<float, float> m_cachedCombinedYRange;  // Cached combined Y range (min, max)
    mutable bool m_cachedCombinedYRangeValid;                 // Cache validity flag
    std::map<QString, std::pair<float, float>> m_seriesMinMax; // Per-series min/max tracking
    uint64_t m_dataVersion;                                    // Version counter for cache invalidation

    // Helper methods
    bool isValidIndex(size_t index) const;
    void validateDataConsistency() const;
    void validateDataSeriesConsistency(const QString& seriesLabel) const;
    
    // Phase 1 Performance Optimization: Helper methods for range caching
    void invalidateRangeCache();
    void updateSeriesMinMax(const QString& seriesLabel);
    void recomputeCombinedYRange() const;
};

#endif // WATERFALLDATA_H
