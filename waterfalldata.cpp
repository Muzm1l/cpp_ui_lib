#include "waterfalldata.h"
#include "btwsymboldrawing.h"
#include "debugutils.h"
#include <algorithm>
#include <limits>
#include <QStringList>

WaterfallData::WaterfallData(const QString& title)
{
    dataTitle = title;
    // Initialize empty circular buffers (unlimited capacity by default)
    dataSeriesYData[dataTitle] = CircularBuffer<float>(0);
    dataSeriesTimestamps[dataTitle] = CircularBuffer<QDateTime>(0);
    dataSeriesTimestampsEpoch[dataTitle] = CircularBuffer<qint64>(0);
    
    // Phase 1: Initialize range cache
    m_cachedCombinedYRange = std::make_pair(0.0f, 0.0f);
    m_cachedCombinedYRangeValid = false;
    m_dataVersion = 1;
}

WaterfallData::WaterfallData(const QString& title, const std::vector<QString>& seriesLabels)
{
    dataTitle = title;
    
    // Initialize empty circular buffers for each provided label (unlimited capacity by default)
    for (const QString& seriesLabel : seriesLabels)
    {
        dataSeriesYData[seriesLabel] = CircularBuffer<float>(0);
        dataSeriesTimestamps[seriesLabel] = CircularBuffer<QDateTime>(0);
        dataSeriesTimestampsEpoch[seriesLabel] = CircularBuffer<qint64>(0);
    }
    
    // Phase 1: Initialize range cache
    m_cachedCombinedYRange = std::make_pair(0.0f, 0.0f);
    m_cachedCombinedYRangeValid = false;
    m_dataVersion = 1;
}

WaterfallData::~WaterfallData()
{
    // Explicitly clear all containers to free QArrayData allocations
    // This helps prevent QArrayData leaks (15.0 MB leak identified by heaptrack)
    dataSeriesYData.clear();
    dataSeriesTimestamps.clear();
    dataSeriesTimestampsEpoch.clear(); // Was missing - now included
    rtwSymbols.clear();
    btwSymbols.clear();
    btwMarkers.clear();
    rtwRMarkers.clear();
    dataTitle.clear(); // Clear QString to free QArrayData
}

void WaterfallData::setData(const std::vector<float>& yData, const std::vector<QDateTime>& timestamps)
{
    // Validate that both vectors have the same size
    if (yData.size() != timestamps.size()) {
        DEBUG_OUT() << "Error: yData and timestamps must have the same size. yData size:" << yData.size() << "timestamps size:" << timestamps.size();
        return;
    }

    // Store the data using circular buffers (preserve existing capacity)
    size_t existingCapacity = dataSeriesYData[dataTitle].capacity();
    dataSeriesYData[dataTitle].assign(yData);
    dataSeriesTimestamps[dataTitle].assign(timestamps);
    
    // Restore capacity if it was set
    if (existingCapacity > 0)
    {
        dataSeriesYData[dataTitle].setCapacity(existingCapacity);
        dataSeriesTimestamps[dataTitle].setCapacity(existingCapacity);
    }
    
    // Store epoch milliseconds in parallel (convert once, not in hot path)
    dataSeriesTimestampsEpoch[dataTitle].clear();
    existingCapacity = dataSeriesTimestampsEpoch[dataTitle].capacity();
    for (const QDateTime& ts : timestamps) {
        dataSeriesTimestampsEpoch[dataTitle].push_back(ts.toMSecsSinceEpoch());
    }
    if (existingCapacity > 0)
    {
        dataSeriesTimestampsEpoch[dataTitle].setCapacity(existingCapacity);
    }

    validateDataConsistency();
    
    // Phase 1: Invalidate range cache and update series min/max
    invalidateRangeCache();
    updateSeriesMinMax(dataTitle);
}

void WaterfallData::clearData()
{
    dataSeriesYData[dataTitle].clear();
    dataSeriesTimestamps[dataTitle].clear();
    dataSeriesTimestampsEpoch[dataTitle].clear();
    
    // Phase 1: Remove series min/max and invalidate cache
    m_seriesMinMax.erase(dataTitle);
    invalidateRangeCache();
}


bool WaterfallData::isEmpty() const
{
    // Check if any series has data
    for (const auto& pair : dataSeriesYData) {
        if (!pair.second.empty()) {
            return false; // Found at least one series with data
        }
    }
    return true; // No series has data
}

std::pair<qreal, qreal> WaterfallData::getYRange() const
{
    bool found = false;
    qreal minY = 0.0, maxY = 0.0;

    for (const auto &pair : dataSeriesYData)
    {
        if (pair.second.empty()) continue;
        
        // Iterate through circular buffer using indexing, convert float to qreal for calculations
        qreal seriesMin = static_cast<qreal>(pair.second[0]);
        qreal seriesMax = static_cast<qreal>(pair.second[0]);
        for (size_t i = 1; i < pair.second.size(); ++i)
        {
            qreal val = static_cast<qreal>(pair.second[i]);
            if (val < seriesMin) seriesMin = val;
            if (val > seriesMax) seriesMax = val;
        }
        
        if (!found) {
            minY = seriesMin;
            maxY = seriesMax;
            found = true;
        } else {
            if (seriesMin < minY) minY = seriesMin;
            if (seriesMax > maxY) maxY = seriesMax;
        }
    }

    if (!found) {
        return std::make_pair(0.0, 0.0);
    }
    return std::make_pair(minY, maxY);
}

std::pair<QDateTime, QDateTime> WaterfallData::getTimeRange() const
{
    QDateTime minTime, maxTime;

    bool hasValue = false;
    for (const auto& pair : dataSeriesTimestamps) {
        if (pair.second.empty()) continue;
        
        // Iterate through circular buffer using indexing
        for (size_t i = 0; i < pair.second.size(); ++i) {
            const QDateTime& t = pair.second[i];
            if (!hasValue) {
                minTime = maxTime = t;
                hasValue = true;
            } else {
                if (t < minTime) minTime = t;
                if (t > maxTime) maxTime = t;
            }
        }
    }

    if (!hasValue) {
        return std::make_pair(QDateTime(), QDateTime());
    }
    return std::make_pair(minTime, maxTime);
}

qreal WaterfallData::getMinY() const
{
    bool found = false;
    qreal minY = 0.0;
    for (const auto& pair : dataSeriesYData) {
        if (pair.second.empty()) continue;
        
        // Find min using indexing, convert float to qreal for calculations
        qreal seriesMin = static_cast<qreal>(pair.second[0]);
        for (size_t i = 1; i < pair.second.size(); ++i) {
            qreal val = static_cast<qreal>(pair.second[i]);
            if (val < seriesMin) {
                seriesMin = val;
            }
        }
        
        if (!found) {
            minY = seriesMin;
            found = true;
        } else {
            if (seriesMin < minY) minY = seriesMin;
        }
    }
    if (!found) {
        return 0.0;
    }
    return minY;
}

qreal WaterfallData::getMaxY() const
{
    bool found = false;
    qreal maxY = 0.0;
    for (const auto& pair : dataSeriesYData) {
        if (pair.second.empty()) continue;
        
        // Find max using indexing, convert float to qreal for calculations
        qreal seriesMax = static_cast<qreal>(pair.second[0]);
        for (size_t i = 1; i < pair.second.size(); ++i) {
            qreal val = static_cast<qreal>(pair.second[i]);
            if (val > seriesMax) {
                seriesMax = val;
            }
        }
        
        if (!found) {
            maxY = seriesMax;
            found = true;
        } else {
            if (seriesMax > maxY) maxY = seriesMax;
        }
    }
    if (!found) {
        return 0.0;
    }
    return maxY;
}

qint64 WaterfallData::getTimeSpanMs() const
{
    auto it = dataSeriesTimestamps.find(dataTitle);
    if (it == dataSeriesTimestamps.end() || it->second.size() < 2) {
        return 0;
    }

    auto timeRange = getTimeRange();
    return timeRange.first.msecsTo(timeRange.second);
}

QDateTime WaterfallData::getEarliestTime() const
{
    QDateTime earliestTime;
    bool hasValue = false;

    for (const auto& pair : dataSeriesTimestamps) {
        if (!pair.second.empty()) {
            // Find earliest using indexing
            QDateTime seriesEarliest = pair.second[0];
            for (size_t i = 1; i < pair.second.size(); ++i) {
                if (pair.second[i] < seriesEarliest) {
                    seriesEarliest = pair.second[i];
                }
            }
            
            if (!hasValue) {
                earliestTime = seriesEarliest;
                hasValue = true;
            } else {
                if (seriesEarliest < earliestTime) {
                    earliestTime = seriesEarliest;
                }
            }
        }
    }

    if (!hasValue) {
        return QDateTime();
    }

    return earliestTime;
}

QDateTime WaterfallData::getLatestTime() const
{
    QDateTime latestTime;
    bool hasValue = false;

    for (const auto& pair : dataSeriesTimestamps) {
        if (!pair.second.empty()) {
            // Find latest using indexing
            QDateTime seriesLatest = pair.second[0];
            for (size_t i = 1; i < pair.second.size(); ++i) {
                if (pair.second[i] > seriesLatest) {
                    seriesLatest = pair.second[i];
                }
            }
            
            if (!hasValue) {
                latestTime = seriesLatest;
                hasValue = true;
            } else {
                if (seriesLatest > latestTime) {
                    latestTime = seriesLatest;
                }
            }
        }
    }

    if (!hasValue) {
        return QDateTime();
    }
    return latestTime;
}

bool WaterfallData::isValidIndex(size_t index) const
{
    auto yIt = dataSeriesYData.find(dataTitle);
    auto tIt = dataSeriesTimestamps.find(dataTitle);
    return (yIt != dataSeriesYData.end() && index < yIt->second.size()) && 
           (tIt != dataSeriesTimestamps.end() && index < tIt->second.size());
}

void WaterfallData::validateDataConsistency() const
{
    auto yIt = dataSeriesYData.find(dataTitle);
    auto tIt = dataSeriesTimestamps.find(dataTitle);
    
    if (yIt != dataSeriesYData.end() && tIt != dataSeriesTimestamps.end()) {
        if (yIt->second.size() != tIt->second.size()) {
            DEBUG_OUT() << "Warning: Data inconsistency detected for series" << dataTitle
                << "- yData size:" << yIt->second.size() << "timestamps size:" << tIt->second.size();
        }
    }
}

void WaterfallData::validateDataSeriesConsistency(const QString& seriesLabel) const
{
    auto yIt = dataSeriesYData.find(seriesLabel);
    auto tIt = dataSeriesTimestamps.find(seriesLabel);

    if (yIt != dataSeriesYData.end() && tIt != dataSeriesTimestamps.end()) {
        if (yIt->second.size() != tIt->second.size()) {
            DEBUG_OUT() << "Warning: Data series inconsistency detected for series" << seriesLabel
                << "- yData size:" << yIt->second.size() << "timestamps size:" << tIt->second.size();
        }
    }
}

// Multiple data series methods implementation

void WaterfallData::addDataSeries(const QString& seriesLabel, const std::vector<float>& yData, const std::vector<QDateTime>& timestamps)
{
    // Validate that both vectors have the same size
    if (yData.size() != timestamps.size()) {
        DEBUG_OUT() << "Error: yData and timestamps must have the same size for series" << seriesLabel
            << ". yData size:" << yData.size() << "timestamps size:" << timestamps.size();
        return;
    }

    // Store directly as float (no conversion needed)
    size_t existingCapacity = dataSeriesYData[seriesLabel].capacity();
    dataSeriesYData[seriesLabel].assign(yData);
    dataSeriesTimestamps[seriesLabel].assign(timestamps);
    
    // Restore capacity if it was set
    if (existingCapacity > 0)
    {
        dataSeriesYData[seriesLabel].setCapacity(existingCapacity);
        dataSeriesTimestamps[seriesLabel].setCapacity(existingCapacity);
    }
    
    // Store epoch milliseconds in parallel (convert once, not in hot path)
    dataSeriesTimestampsEpoch[seriesLabel].clear();
    existingCapacity = dataSeriesTimestampsEpoch[seriesLabel].capacity();
    for (const QDateTime& ts : timestamps) {
        dataSeriesTimestampsEpoch[seriesLabel].push_back(ts.toMSecsSinceEpoch());
    }
    if (existingCapacity > 0)
    {
        dataSeriesTimestampsEpoch[seriesLabel].setCapacity(existingCapacity);
    }

    validateDataSeriesConsistency(seriesLabel);
    
    // Phase 1: Invalidate range cache and update series min/max
    invalidateRangeCache();
    updateSeriesMinMax(seriesLabel);
}

void WaterfallData::addDataPointToSeries(const QString& seriesLabel, float yValue, const QDateTime& timestamp)
{
    CircularBuffer<float> &yBuffer = dataSeriesYData[seriesLabel];
    const bool evictsOldest = yBuffer.full();

    // Store directly as float (no conversion needed)
    yBuffer.push_back(yValue);
    dataSeriesTimestamps[seriesLabel].push_back(timestamp);
    // Store epoch milliseconds in parallel (convert once, not in hot path)
    dataSeriesTimestampsEpoch[seriesLabel].push_back(timestamp.toMSecsSinceEpoch());

    validateDataSeriesConsistency(seriesLabel);
    
    // Phase 1: Keep min/max cache correct even when circular buffers evict oldest values.
    // If an element is evicted, incremental min/max updates are not sufficient.
    if (evictsOldest)
    {
        updateSeriesMinMax(seriesLabel);
        invalidateRangeCache();
        return;
    }

    auto minMaxIt = m_seriesMinMax.find(seriesLabel);
    if (minMaxIt != m_seriesMinMax.end()) {
        bool rangeChanged = false;
        if (yValue < minMaxIt->second.first) {
            minMaxIt->second.first = yValue;
            rangeChanged = true;
        }
        if (yValue > minMaxIt->second.second) {
            minMaxIt->second.second = yValue;
            rangeChanged = true;
        }
        if (rangeChanged)
            invalidateRangeCache();
    } else {
        // First point in series - initialize min/max
        updateSeriesMinMax(seriesLabel);
        invalidateRangeCache();
    }
}

void WaterfallData::addDataPointsToSeries(const QString& seriesLabel, const std::vector<float>& yValues, const std::vector<QDateTime>& timestamps)
{
    // Validate that both vectors have the same size
    if (yValues.size() != timestamps.size()) {
        DEBUG_OUT() << "Error: yValues and timestamps must have the same size for series" << seriesLabel
            << ". yValues size:" << yValues.size() << "timestamps size:" << timestamps.size();
        return;
    }

    // Store directly as float (no conversion needed)
    dataSeriesYData[seriesLabel].push_back(yValues);
    dataSeriesTimestamps[seriesLabel].push_back(timestamps);
    
    // Store epoch milliseconds in parallel (convert once, not in hot path)
    for (const QDateTime& ts : timestamps) {
        dataSeriesTimestampsEpoch[seriesLabel].push_back(ts.toMSecsSinceEpoch());
    }

    validateDataSeriesConsistency(seriesLabel);
    
    // Phase 1: Update series min/max (recompute for accuracy)
    updateSeriesMinMax(seriesLabel);
    invalidateRangeCache();
}

void WaterfallData::clearDataSeries(const QString& seriesLabel)
{
    dataSeriesYData.erase(seriesLabel);
    dataSeriesTimestamps.erase(seriesLabel);
    dataSeriesTimestampsEpoch.erase(seriesLabel);
    
    // Phase 1: Remove series min/max and invalidate cache
    m_seriesMinMax.erase(seriesLabel);
    invalidateRangeCache();
}

void WaterfallData::clearAllDataSeries()
{
    dataSeriesYData.clear();
    dataSeriesTimestamps.clear();
    dataSeriesTimestampsEpoch.clear();
    
    // Phase 1: Clear all min/max tracking and invalidate cache
    m_seriesMinMax.clear();
    invalidateRangeCache();
}

std::vector<std::pair<qreal, QDateTime>> WaterfallData::getDataSeries(const QString& seriesLabel) const
{
    std::vector<std::pair<qreal, QDateTime>> result;

    auto yIt = dataSeriesYData.find(seriesLabel);
    auto tIt = dataSeriesTimestamps.find(seriesLabel);

    if (yIt != dataSeriesYData.end() && tIt != dataSeriesTimestamps.end()) {
        result.reserve(yIt->second.size());

        for (size_t i = 0; i < yIt->second.size(); ++i) {
            // Convert float to qreal when returning
            result.emplace_back(static_cast<qreal>(yIt->second[i]), tIt->second[i]);
        }
    }

    return result;
}

std::vector<std::pair<qreal, QDateTime>> WaterfallData::getDataSeriesWithinYExtents(const QString& seriesLabel, qreal yMin, qreal yMax) const
{
    std::vector<std::pair<qreal, QDateTime>> result;

    auto yIt = dataSeriesYData.find(seriesLabel);
    auto tIt = dataSeriesTimestamps.find(seriesLabel);

    if (yIt != dataSeriesYData.end() && tIt != dataSeriesTimestamps.end()) {
        for (size_t i = 0; i < yIt->second.size(); ++i) {
            float val = yIt->second[i];
            if (val >= static_cast<float>(yMin) && val <= static_cast<float>(yMax)) {
                // Convert float to qreal when returning
                result.emplace_back(static_cast<qreal>(val), tIt->second[i]);
            }
        }
    }

    return result;
}

std::vector<std::pair<qreal, QDateTime>> WaterfallData::getDataSeriesWithinTimeRange(const QString& seriesLabel, const QDateTime& startTime, const QDateTime& endTime) const
{
    std::vector<std::pair<qreal, QDateTime>> result;

    auto yIt = dataSeriesYData.find(seriesLabel);
    auto tIt = dataSeriesTimestamps.find(seriesLabel);

    if (yIt != dataSeriesYData.end() && tIt != dataSeriesTimestamps.end()) {
        // Convert circular buffers to vectors for binary search
        std::vector<QDateTime> timestamps = tIt->second.toVector();
        std::vector<float> yData = yIt->second.toVector();
        
        if (timestamps.empty() || timestamps.size() != yData.size()) {
            return result;
        }
        
        // Use binary search to find the start and end indices
        // Find first timestamp >= startTime
        auto startIt = std::lower_bound(timestamps.begin(), timestamps.end(), startTime);
        // Find first timestamp > endTime
        auto endIt = std::upper_bound(timestamps.begin(), timestamps.end(), endTime);
        
        // Calculate indices
        size_t startIdx = std::distance(timestamps.begin(), startIt);
        size_t endIdx = std::distance(timestamps.begin(), endIt);
        
        // Reserve space for efficiency
        if (startIdx < endIdx) {
            result.reserve(endIdx - startIdx);
            for (size_t i = startIdx; i < endIdx; ++i) {
                // Convert float to qreal when returning
                result.emplace_back(static_cast<qreal>(yData[i]), timestamps[i]);
            }
        }
    }

    return result;
}

bool WaterfallData::findClosestDataPoint(const QString& seriesLabel, const QDateTime& targetTime, qint64 toleranceMs, qreal& outValue, size_t& outIndex) const
{
    auto yIt = dataSeriesYData.find(seriesLabel);
    auto tIt = dataSeriesTimestamps.find(seriesLabel);
    
    if (yIt == dataSeriesYData.end() || tIt == dataSeriesTimestamps.end()) {
        return false;
    }
    
    // Convert circular buffers to vectors for binary search
    std::vector<QDateTime> timestamps = tIt->second.toVector();
    std::vector<float> yData = yIt->second.toVector();
    
    if (timestamps.empty() || timestamps.size() != yData.size()) {
        return false;
    }
    
    // Use binary search to find the closest timestamp
    auto it = std::lower_bound(timestamps.begin(), timestamps.end(), targetTime);
    
    qint64 bestDiff = toleranceMs + 1;
    size_t bestIdx = timestamps.size();
    
    // Check the element at or before the insertion point
    if (it != timestamps.begin()) {
        auto prevIt = std::prev(it);
        size_t prevIdx = std::distance(timestamps.begin(), prevIt);
        qint64 diff = qAbs(prevIt->msecsTo(targetTime));
        if (diff < bestDiff) {
            bestDiff = diff;
            bestIdx = prevIdx;
        }
    }
    
    // Check the element at the insertion point
    if (it != timestamps.end()) {
        size_t currIdx = std::distance(timestamps.begin(), it);
        qint64 diff = qAbs(it->msecsTo(targetTime));
        if (diff < bestDiff) {
            bestDiff = diff;
            bestIdx = currIdx;
        }
    }
    
    if (bestIdx < timestamps.size() && bestDiff <= toleranceMs) {
        // Convert float to qreal when returning
        outValue = static_cast<qreal>(yData[bestIdx]);
        outIndex = bestIdx;
        return true;
    }
    
    return false;
}

bool WaterfallData::interpolateSeriesRangeAtTime(const QString& seriesLabel, const QDateTime& targetTime, qreal& outRange) const
{
    auto yIt = dataSeriesYData.find(seriesLabel);
    auto tIt = dataSeriesTimestamps.find(seriesLabel);

    if (yIt == dataSeriesYData.end() || tIt == dataSeriesTimestamps.end()) {
        return false;
    }

    const std::vector<QDateTime> timestamps = tIt->second.toVector();
    const std::vector<float> yData = yIt->second.toVector();

    if (timestamps.empty() || timestamps.size() != yData.size()) {
        return false;
    }

    auto it = std::lower_bound(timestamps.begin(), timestamps.end(), targetTime);

    if (it == timestamps.begin()) {
        outRange = static_cast<qreal>(yData.front());
        return true;
    }

    if (it == timestamps.end()) {
        outRange = static_cast<qreal>(yData.back());
        return true;
    }

    const size_t idx = static_cast<size_t>(std::distance(timestamps.begin(), it));
    if (*it == targetTime) {
        outRange = static_cast<qreal>(yData[idx]);
        return true;
    }

    const size_t prevIdx = idx - 1;
    const qint64 t0 = timestamps[prevIdx].toMSecsSinceEpoch();
    const qint64 t1 = timestamps[idx].toMSecsSinceEpoch();
    const qint64 tt = targetTime.toMSecsSinceEpoch();

    if (t1 == t0) {
        outRange = static_cast<qreal>(yData[prevIdx]);
        return true;
    }

    const qreal alpha = static_cast<qreal>(tt - t0) / static_cast<qreal>(t1 - t0);
    outRange = (1.0 - alpha) * static_cast<qreal>(yData[prevIdx]) + alpha * static_cast<qreal>(yData[idx]);
    return true;
}

std::vector<qreal> WaterfallData::getYDataSeries(const QString& seriesLabel) const
{
    auto it = dataSeriesYData.find(seriesLabel);
    if (it != dataSeriesYData.end())
    {
        // Convert float to qreal when returning
        std::vector<float> floatData = it->second.toVector();
        std::vector<qreal> result;
        result.reserve(floatData.size());
        for (float val : floatData) {
            result.push_back(static_cast<qreal>(val));
        }
        return result;
    }
    return std::vector<qreal>();
}

std::vector<QDateTime> WaterfallData::getTimestampsSeries(const QString& seriesLabel) const
{
    auto it = dataSeriesTimestamps.find(seriesLabel);
    if (it != dataSeriesTimestamps.end())
    {
        return it->second.toVector();
    }
    return std::vector<QDateTime>();
}

std::vector<qint64> WaterfallData::getTimestampsEpochSeries(const QString& seriesLabel) const
{
    auto it = dataSeriesTimestampsEpoch.find(seriesLabel);
    if (it != dataSeriesTimestampsEpoch.end())
    {
        return it->second.toVector();
    }
    return std::vector<qint64>();
}

void WaterfallData::populateYDataSeries(const QString& seriesLabel, std::vector<qreal>& output) const
{
    output.clear();
    auto it = dataSeriesYData.find(seriesLabel);
    if (it != dataSeriesYData.end())
    {
        const CircularBuffer<float>& buffer = it->second;
        output.reserve(buffer.size());
        for (size_t i = 0; i < buffer.size(); ++i)
        {
            // Convert float to qreal when populating
            output.push_back(static_cast<qreal>(buffer[i]));
        }
    }
}

void WaterfallData::populateYDataSeriesFloat(const QString& seriesLabel, std::vector<float>& output) const
{
    output.clear();
    auto it = dataSeriesYData.find(seriesLabel);
    if (it != dataSeriesYData.end())
    {
        const CircularBuffer<float>& buffer = it->second;
        output.reserve(buffer.size());
        for (size_t i = 0; i < buffer.size(); ++i)
        {
            // NO CONVERSION - direct float copy (optimization to eliminate float-to-double overhead)
            output.push_back(buffer[i]);
        }
    }
}

void WaterfallData::populateTimestampsSeries(const QString& seriesLabel, std::vector<QDateTime>& output) const
{
    output.clear();
    auto it = dataSeriesTimestamps.find(seriesLabel);
    if (it != dataSeriesTimestamps.end())
    {
        const CircularBuffer<QDateTime>& buffer = it->second;
        output.reserve(buffer.size());
        for (size_t i = 0; i < buffer.size(); ++i)
        {
            output.push_back(buffer[i]);
        }
    }
}

void WaterfallData::populateTimestampsEpochSeries(const QString& seriesLabel, std::vector<qint64>& output) const
{
    output.clear();
    auto it = dataSeriesTimestampsEpoch.find(seriesLabel);
    if (it != dataSeriesTimestampsEpoch.end())
    {
        const CircularBuffer<qint64>& buffer = it->second;
        output.reserve(buffer.size());
        for (size_t i = 0; i < buffer.size(); ++i)
        {
            output.push_back(buffer[i]);
        }
    }
}

bool WaterfallData::findVisibleEpochRange(const QString& seriesLabel, qint64 timeMinEpoch, qint64 timeMaxEpoch,
                                          size_t& firstIdx, size_t& lastIdx) const
{
    firstIdx = 0;
    lastIdx = 0;

    auto epochIt = dataSeriesTimestampsEpoch.find(seriesLabel);
    if (epochIt == dataSeriesTimestampsEpoch.end())
    {
        return false;
    }

    const CircularBuffer<qint64>& epochs = epochIt->second;
    const size_t n = epochs.size();
    if (n == 0)
    {
        return false;
    }

    // lower_bound on circular buffer logical indices (0 = oldest, n-1 = newest)
    size_t lo = 0;
    size_t hi = n;
    while (lo < hi)
    {
        const size_t mid = lo + (hi - lo) / 2;
        if (epochs[mid] < timeMinEpoch)
        {
            lo = mid + 1;
        }
        else
        {
            hi = mid;
        }
    }
    firstIdx = lo;

    // upper_bound on circular buffer logical indices
    lo = firstIdx;
    hi = n;
    while (lo < hi)
    {
        const size_t mid = lo + (hi - lo) / 2;
        if (epochs[mid] <= timeMaxEpoch)
        {
            lo = mid + 1;
        }
        else
        {
            hi = mid;
        }
    }
    lastIdx = lo;

    return firstIdx < lastIdx;
}

void WaterfallData::populateSeriesRangeFloatEpoch(const QString& seriesLabel, size_t firstIdx, size_t lastIdx,
                                                  std::vector<std::pair<float, qint64>>& output) const
{
    output.clear();

    auto yIt = dataSeriesYData.find(seriesLabel);
    auto epochIt = dataSeriesTimestampsEpoch.find(seriesLabel);
    if (yIt == dataSeriesYData.end() || epochIt == dataSeriesTimestampsEpoch.end())
    {
        return;
    }

    const CircularBuffer<float>& yBuffer = yIt->second;
    const CircularBuffer<qint64>& epochBuffer = epochIt->second;
    const size_t n = std::min(yBuffer.size(), epochBuffer.size());
    if (n == 0 || firstIdx >= n)
    {
        return;
    }

    const size_t endIdx = std::min(lastIdx, n);
    if (firstIdx >= endIdx)
    {
        return;
    }

    output.reserve(endIdx - firstIdx);
    for (size_t i = firstIdx; i < endIdx; ++i)
    {
        output.push_back(std::make_pair(yBuffer[i], epochBuffer[i]));
    }
}

bool WaterfallData::getSeriesPointAtIndexEpoch(const QString& seriesLabel, size_t index, float& yValue, qint64& timestampEpoch) const
{
    auto yIt = dataSeriesYData.find(seriesLabel);
    auto epochIt = dataSeriesTimestampsEpoch.find(seriesLabel);
    if (yIt == dataSeriesYData.end() || epochIt == dataSeriesTimestampsEpoch.end())
    {
        return false;
    }

    const CircularBuffer<float>& yBuffer = yIt->second;
    const CircularBuffer<qint64>& epochBuffer = epochIt->second;
    const size_t n = std::min(yBuffer.size(), epochBuffer.size());
    if (index >= n)
    {
        return false;
    }

    yValue = yBuffer[index];
    timestampEpoch = epochBuffer[index];
    return true;
}

size_t WaterfallData::getDataSeriesSize(const QString& seriesLabel) const
{
    auto it = dataSeriesYData.find(seriesLabel);
    return (it != dataSeriesYData.end()) ? it->second.size() : 0;
}

bool WaterfallData::isDataSeriesEmpty(const QString& seriesLabel) const
{
    auto it = dataSeriesYData.find(seriesLabel);
    return (it == dataSeriesYData.end()) || it->second.empty();
}

bool WaterfallData::hasDataSeries(const QString& seriesLabel) const
{
    return dataSeriesYData.find(seriesLabel) != dataSeriesYData.end();
}

std::vector<QString> WaterfallData::getDataSeriesLabels() const
{
    std::vector<QString> labels;
    labels.reserve(dataSeriesYData.size());

    for (const auto& pair : dataSeriesYData) {
        labels.push_back(pair.first);
    }

    return labels;
}

std::pair<qreal, qreal> WaterfallData::getYRangeSeries(const QString& seriesLabel) const
{
    auto it = dataSeriesYData.find(seriesLabel);
    if (it == dataSeriesYData.end() || it->second.empty()) {
        return std::make_pair(0.0, 0.0);
    }

    // Find min/max using indexing, convert float to qreal for calculations
    qreal minY = static_cast<qreal>(it->second[0]);
    qreal maxY = static_cast<qreal>(it->second[0]);
    for (size_t i = 1; i < it->second.size(); ++i) {
        qreal val = static_cast<qreal>(it->second[i]);
        if (val < minY) minY = val;
        if (val > maxY) maxY = val;
    }
    return std::make_pair(minY, maxY);
}

std::pair<QDateTime, QDateTime> WaterfallData::getTimeRangeSeries(const QString& seriesLabel) const
{
    auto it = dataSeriesTimestamps.find(seriesLabel);
    if (it == dataSeriesTimestamps.end() || it->second.empty()) {
        return std::make_pair(QDateTime(), QDateTime());
    }

    // Find min/max using indexing
    QDateTime minTime = it->second[0];
    QDateTime maxTime = it->second[0];
    for (size_t i = 1; i < it->second.size(); ++i) {
        if (it->second[i] < minTime) minTime = it->second[i];
        if (it->second[i] > maxTime) maxTime = it->second[i];
    }
    return std::make_pair(minTime, maxTime);
}

std::pair<qreal, qreal> WaterfallData::getCombinedYRange() const
{
    // Phase 1: Return cached value if valid
    if (m_cachedCombinedYRangeValid) {
        return std::make_pair(static_cast<qreal>(m_cachedCombinedYRange.first),
                              static_cast<qreal>(m_cachedCombinedYRange.second));
    }
    
    // Cache miss - recompute and cache
    recomputeCombinedYRange();
    
    return std::make_pair(static_cast<qreal>(m_cachedCombinedYRange.first),
                          static_cast<qreal>(m_cachedCombinedYRange.second));
}

std::pair<QDateTime, QDateTime> WaterfallData::getCombinedTimeRange() const
{
    QDateTime globalMin = QDateTime();
    QDateTime globalMax = QDateTime();
    bool hasData = false;

    // Check all data series
    for (const auto& pair : dataSeriesTimestamps) {
        if (!pair.second.empty()) {
            // Find min/max using indexing
            QDateTime seriesMin = pair.second[0];
            QDateTime seriesMax = pair.second[0];
            for (size_t i = 1; i < pair.second.size(); ++i) {
                if (pair.second[i] < seriesMin) seriesMin = pair.second[i];
                if (pair.second[i] > seriesMax) seriesMax = pair.second[i];
            }
            
            if (!hasData) {
                globalMin = seriesMin;
                globalMax = seriesMax;
                hasData = true;
            }
            else {
                globalMin = std::min(globalMin, seriesMin);
                globalMax = std::max(globalMax, seriesMax);
            }
        }
    }

    if (!hasData) {
        return std::make_pair(QDateTime(), QDateTime());
    }

    return std::make_pair(globalMin, globalMax);
}

// Selection time span methods implementation

QDateTime WaterfallData::getSelectionEarliestTime() const
{
    // For selection purposes, we want the earliest time available in the data
    // This is the oldest timestamp (furthest in the past)
    return getEarliestTime();
}

QDateTime WaterfallData::getSelectionLatestTime() const
{
    // For selection purposes, we want the latest time available in the data
    // This is the newest timestamp (closest to current time)
    return getLatestTime();
}

qint64 WaterfallData::getSelectionTimeSpanMs() const
{
    // Return the total time span available for selection
    return getTimeSpanMs();
}

bool WaterfallData::isValidSelectionTime(const QDateTime& time) const
{
    auto it = dataSeriesTimestamps.find(dataTitle);
    if (it == dataSeriesTimestamps.end() || it->second.empty()) {
        return false;
    }

    QDateTime earliest = getSelectionEarliestTime();
    QDateTime latest = getSelectionLatestTime();

    // Check if the time is within the available data range
    return (time >= earliest && time <= latest);
}

// Series-specific versions of legacy methods implementation

void WaterfallData::setDataSeries(const QString& seriesLabel, const std::vector<float>& yData, const std::vector<QDateTime>& timestamps)
{
    // Validate that both vectors have the same size
    if (yData.size() != timestamps.size()) {
        DEBUG_OUT() << "Error: yData and timestamps must have the same size for series" << seriesLabel
            << ". yData size:" << yData.size() << "timestamps size:" << timestamps.size();
        return;
    }

    // Store the data series using circular buffers (preserve existing capacity)
    size_t existingCapacity = dataSeriesYData[seriesLabel].capacity();
    dataSeriesYData[seriesLabel].assign(yData);
    dataSeriesTimestamps[seriesLabel].assign(timestamps);
    
    // Restore capacity if it was set
    if (existingCapacity > 0)
    {
        dataSeriesYData[seriesLabel].setCapacity(existingCapacity);
        dataSeriesTimestamps[seriesLabel].setCapacity(existingCapacity);
    }
    
    // Store epoch milliseconds in parallel (convert once, not in hot path)
    dataSeriesTimestampsEpoch[seriesLabel].clear();
    existingCapacity = dataSeriesTimestampsEpoch[seriesLabel].capacity();
    for (const QDateTime& ts : timestamps) {
        dataSeriesTimestampsEpoch[seriesLabel].push_back(ts.toMSecsSinceEpoch());
    }
    if (existingCapacity > 0)
    {
        dataSeriesTimestampsEpoch[seriesLabel].setCapacity(existingCapacity);
    }

    validateDataSeriesConsistency(seriesLabel);
    
    // Phase 1: Invalidate range cache and update series min/max
    invalidateRangeCache();
    updateSeriesMinMax(seriesLabel);
}

std::vector<std::pair<qreal, QDateTime>> WaterfallData::getAllDataSeries(const QString& seriesLabel) const
{
    std::vector<std::pair<qreal, QDateTime>> result;
    
    auto yIt = dataSeriesYData.find(seriesLabel);
    auto tIt = dataSeriesTimestamps.find(seriesLabel);
    
    if (yIt != dataSeriesYData.end() && tIt != dataSeriesTimestamps.end()) {
        result.reserve(yIt->second.size());
        
        for (size_t i = 0; i < yIt->second.size(); ++i) {
            // Convert float to qreal when returning
            result.emplace_back(static_cast<qreal>(yIt->second[i]), tIt->second[i]);
        }
    }
    
    return result;
}

qreal WaterfallData::getMinYSeries(const QString& seriesLabel) const
{
    auto it = dataSeriesYData.find(seriesLabel);
    if (it == dataSeriesYData.end() || it->second.empty()) {
        return 0.0;
    }

    // Find min using indexing, convert float to qreal for calculations
    qreal minY = static_cast<qreal>(it->second[0]);
    for (size_t i = 1; i < it->second.size(); ++i) {
        qreal val = static_cast<qreal>(it->second[i]);
        if (val < minY) minY = val;
    }
    return minY;
}

qreal WaterfallData::getMaxYSeries(const QString& seriesLabel) const
{
    auto it = dataSeriesYData.find(seriesLabel);
    if (it == dataSeriesYData.end() || it->second.empty()) {
        return 0.0;
    }

    // Find max using indexing, convert float to qreal for calculations
    qreal maxY = static_cast<qreal>(it->second[0]);
    for (size_t i = 1; i < it->second.size(); ++i) {
        qreal val = static_cast<qreal>(it->second[i]);
        if (val > maxY) maxY = val;
    }
    return maxY;
}

qint64 WaterfallData::getTimeSpanMsSeries(const QString& seriesLabel) const
{
    auto it = dataSeriesTimestamps.find(seriesLabel);
    if (it == dataSeriesTimestamps.end() || it->second.size() < 2) {
        return 0;
    }

    auto timeRange = getTimeRangeSeries(seriesLabel);
    return timeRange.first.msecsTo(timeRange.second);
}

QDateTime WaterfallData::getEarliestTimeSeries(const QString& seriesLabel) const
{
    auto it = dataSeriesTimestamps.find(seriesLabel);
    if (it == dataSeriesTimestamps.end() || it->second.empty()) {
        return QDateTime();
    }

    // Find earliest using indexing
    QDateTime earliest = it->second[0];
    for (size_t i = 1; i < it->second.size(); ++i) {
        if (it->second[i] < earliest) {
            earliest = it->second[i];
        }
    }
    return earliest;
}

QDateTime WaterfallData::getLatestTimeSeries(const QString& seriesLabel) const
{
    auto it = dataSeriesTimestamps.find(seriesLabel);
    if (it == dataSeriesTimestamps.end() || it->second.empty()) {
        return QDateTime();
    }

    // Find latest using indexing
    QDateTime latest = it->second[0];
    for (size_t i = 1; i < it->second.size(); ++i) {
        if (it->second[i] > latest) {
            latest = it->second[i];
        }
    }
    return latest;
}

bool WaterfallData::isValidIndexSeries(const QString& seriesLabel, size_t index) const
{
    auto yIt = dataSeriesYData.find(seriesLabel);
    auto tIt = dataSeriesTimestamps.find(seriesLabel);
    return (yIt != dataSeriesYData.end() && index < yIt->second.size()) && 
           (tIt != dataSeriesTimestamps.end() && index < tIt->second.size());
}

bool WaterfallData::isValidSelectionTimeSeries(const QString& seriesLabel, const QDateTime& time) const
{
    auto it = dataSeriesTimestamps.find(seriesLabel);
    if (it == dataSeriesTimestamps.end() || it->second.empty()) {
        return false;
    }

    QDateTime earliest = getEarliestTimeSeries(seriesLabel);
    QDateTime latest = getLatestTimeSeries(seriesLabel);

    // Check if the time is within the available data range
    return (time >= earliest && time <= latest);
}

// Data binning methods implementation

std::vector<std::pair<qreal, QDateTime>> WaterfallData::getBinnedDataSeries(const QString& seriesLabel, const QTime& binDuration) const
{
    std::vector<std::pair<qreal, QDateTime>> result;
    
    auto yIt = dataSeriesYData.find(seriesLabel);
    auto tIt = dataSeriesTimestamps.find(seriesLabel);
    
    if (yIt == dataSeriesYData.end() || tIt == dataSeriesTimestamps.end() || yIt->second.empty()) {
        return result; // Return empty vector if series doesn't exist or is empty
    }
    
    // Convert circular buffers to vectors for iteration
    std::vector<float> yData = yIt->second.toVector();
    std::vector<QDateTime> timestamps = tIt->second.toVector();
    
    if (timestamps.empty()) {
        return result;
    }
    
    // Convert QTime duration to milliseconds
    qint64 binSizeMs = QTime(0, 0, 0).msecsTo(binDuration);
    
    if (binSizeMs <= 0) {
        DEBUG_OUT() << "Warning: Invalid bin duration provided for series" << seriesLabel;
        return result;
    }
    
    // Find the earliest timestamp to use as reference for binning
    QDateTime earliestTime = *std::min_element(timestamps.begin(), timestamps.end());
    
    // Create a map to store the first value in each bin
    std::map<qint64, std::pair<qreal, QDateTime>> bins;
    
    for (size_t i = 0; i < timestamps.size(); ++i) {
        // Calculate which bin this timestamp belongs to
        qint64 timeDiffMs = earliestTime.msecsTo(timestamps[i]);
        qint64 binIndex = timeDiffMs / binSizeMs;
        
        // If this is the first value in this bin, store it
        if (bins.find(binIndex) == bins.end()) {
            bins[binIndex] = std::make_pair(yData[i], timestamps[i]);
        }
    }
    
    // Convert the map to a vector, maintaining chronological order
    result.reserve(bins.size());
    for (const auto& bin : bins) {
        result.push_back(bin.second);
    }
    
    // Sort by timestamp to ensure chronological order
    std::sort(result.begin(), result.end(), 
              [](const std::pair<qreal, QDateTime>& a, const std::pair<qreal, QDateTime>& b) {
                  return a.second < b.second;
              });
    
    return result;
}

// Static binning method implementation

std::vector<std::pair<qreal, QDateTime>> WaterfallData::binDataByTime(
    const std::vector<qreal>& yData, 
    const std::vector<QDateTime>& timestamps, 
    const QTime& binDuration)
{
    std::vector<std::pair<qreal, QDateTime>> result;
    
    // Validate input data
    if (yData.empty() || timestamps.empty() || yData.size() != timestamps.size()) {
        DEBUG_OUT() << "WaterfallData::binDataByTime: Invalid input data - sizes don't match or data is empty";
        return result;
    }
    
    // Convert QTime duration to milliseconds
    qint64 binSizeMs = QTime(0, 0, 0).msecsTo(binDuration);
    
    if (binSizeMs <= 0) {
        DEBUG_OUT() << "WaterfallData::binDataByTime: Invalid bin duration provided";
        return result;
    }
    
    // Find the earliest timestamp to use as reference for binning
    QDateTime earliestTime = *std::min_element(timestamps.begin(), timestamps.end());
    
    // Create a map to store the first value in each bin
    std::map<qint64, std::pair<qreal, QDateTime>> bins;
    
    for (size_t i = 0; i < timestamps.size(); ++i) {
        // Calculate which bin this timestamp belongs to
        qint64 timeDiffMs = earliestTime.msecsTo(timestamps[i]);
        qint64 binIndex = timeDiffMs / binSizeMs;
        
        // If this is the first value in this bin, store it
        if (bins.find(binIndex) == bins.end()) {
            bins[binIndex] = std::make_pair(yData[i], timestamps[i]);
        }
    }
    
    // Convert the map to a vector, maintaining chronological order
    result.reserve(bins.size());
    for (const auto& bin : bins) {
        result.push_back(bin.second);
    }
    
    // Sort by timestamp to ensure chronological order
    std::sort(result.begin(), result.end(), 
              [](const std::pair<qreal, QDateTime>& a, const std::pair<qreal, QDateTime>& b) {
                  return a.second < b.second;
              });
    
    DEBUG_OUT() << "WaterfallData::binDataByTime: Binned" << yData.size() << "points into" << result.size() << "bins with duration" << binSizeMs << "ms";
    
    return result;
}

// RTW Symbol management methods implementation

void WaterfallData::addRTWSymbol(const QString& symbolName, const QDateTime& timestamp, float range)
{
    RTWSymbolData symbolData;
    symbolData.symbolName = symbolName;
    symbolData.timestamp = timestamp;
    symbolData.range = range;
    
    rtwSymbols.push_back(symbolData);
    
    DEBUG_OUT() << "WaterfallData: Added RTW symbol" << symbolName << "at timestamp" << timestamp.toString() << "with range" << range;
}

void WaterfallData::clearRTWSymbols()
{
    rtwSymbols.clear();
    DEBUG_OUT() << "WaterfallData: Cleared all RTW symbols";
}

bool WaterfallData::removeRTWSymbol(const QString& symbolName, const QDateTime& timestamp, float range, float toleranceMs, float rangeTolerance)
{
    // Use erase_if with predicate to remove matching symbol
    bool found = false;
    rtwSymbols.erase_if([&](const RTWSymbolData& symbol) {
        if (symbol.symbolName != symbolName) return false;
        
        qint64 timeDiff = qAbs(symbol.timestamp.msecsTo(timestamp));
        float rangeDiff = qAbs(symbol.range - range);
        
        if (timeDiff <= static_cast<qint64>(toleranceMs) && rangeDiff <= rangeTolerance)
        {
            found = true;
            return true; // Erase this element
        }
        return false;
    });
    
    if (found)
    {
        DEBUG_OUT() << "WaterfallData: Removed RTW symbol" << symbolName << "at timestamp" << timestamp.toString() << "with range" << range;
        return true;
    }
    
    DEBUG_OUT() << "WaterfallData: RTW symbol not found:" << symbolName << "at timestamp" << timestamp.toString() << "with range" << range;
    return false;
}

std::vector<RTWSymbolData> WaterfallData::getRTWSymbols() const
{
    return rtwSymbols.toVector();
}

size_t WaterfallData::getRTWSymbolsCount() const
{
    return rtwSymbols.size();
}

// BTW Symbol management methods
void WaterfallData::addBTWSymbol(const QString& symbolName, const QDateTime& timestamp, float range, bool isSynced)
{
    BTWSymbolData symbolData;
    symbolData.symbolName = symbolName;
    symbolData.timestamp = timestamp;
    // OPTIMIZATION: Cache epoch milliseconds to avoid repeated toMSecsSinceEpoch() calls in hot paths
    symbolData.timestampEpoch = timestamp.isValid() ? timestamp.toMSecsSinceEpoch() : 0;
    symbolData.range = range;
    symbolData.isSynced = isSynced;
    
    btwSymbols.push_back(symbolData);
    
    DEBUG_OUT() << "WaterfallData: Added BTW symbol" << symbolName << "at timestamp" << timestamp.toString() << "with range" << range << "isSynced:" << isSynced;
}

void WaterfallData::clearBTWSymbols()
{
    btwSymbols.clear();
    DEBUG_OUT() << "WaterfallData: Cleared all BTW symbols";
}

namespace {

bool btwSymbolNamesMatch(const QString &query, const QString &stored)
{
    if (query == stored)
        return true;
    const BTWSymbolDrawing::SymbolType queryType = BTWSymbolDrawing::symbolNameToType(query);
    const BTWSymbolDrawing::SymbolType storedType = BTWSymbolDrawing::symbolNameToType(stored);
    return queryType == storedType;
}

} // namespace

bool WaterfallData::removeBTWSymbol(const QString& symbolName, const QDateTime& timestamp, float range, float toleranceMs, float rangeTolerance)
{
    bool found = false;
    btwSymbols.erase_if([&](const BTWSymbolData& symbol) {
        if (!btwSymbolNamesMatch(symbolName, symbol.symbolName))
            return false;

        const qint64 timeDiff = qAbs(symbol.timestamp.msecsTo(timestamp));
        const float rangeDiff = qAbs(symbol.range - range);

        if (timeDiff <= static_cast<qint64>(toleranceMs) && rangeDiff <= rangeTolerance)
        {
            found = true;
            return true;
        }
        return false;
    });

    if (found)
    {
        DEBUG_OUT() << "WaterfallData: Removed BTW symbol" << symbolName << "at timestamp" << timestamp.toString() << "with range" << range;
        return true;
    }

    DEBUG_OUT() << "WaterfallData: BTW symbol not found:" << symbolName << "at timestamp" << timestamp.toString() << "with range" << range;
    return false;
}

std::vector<BTWSymbolData> WaterfallData::getBTWSymbols() const
{
    return btwSymbols.toVector();
}

std::vector<BTWSymbolData> WaterfallData::getBTWSymbolsWithinTimeRange(const QDateTime& startTime, const QDateTime& endTime) const
{
    std::vector<BTWSymbolData> result;

    if (btwSymbols.empty()) {
        return result;
    }

    const qint64 startEpoch = startTime.isValid() ? startTime.toMSecsSinceEpoch() : std::numeric_limits<qint64>::min();
    const qint64 endEpoch = endTime.isValid() ? endTime.toMSecsSinceEpoch() : std::numeric_limits<qint64>::max();
    if (startEpoch > endEpoch)
    {
        return result;
    }

    result.reserve(btwSymbols.size());
    for (size_t i = 0; i < btwSymbols.size(); ++i)
    {
        const BTWSymbolData &symbol = btwSymbols[i];
        if (symbol.timestampEpoch >= startEpoch && symbol.timestampEpoch <= endEpoch)
        {
            result.push_back(symbol);
        }
    }

    return result;
}

size_t WaterfallData::getBTWSymbolsCount() const
{
    return btwSymbols.size();
}

// BTW Marker management methods implementation

void WaterfallData::addBTWMarker(const QDateTime& timestamp, float range, float delta)
{
    // Deduplication: when integrated in main system, the same automatic marker can be
    // reported twice (e.g. once with delta 0 and once with the real delta). If a marker
    // already exists at the same (timestamp, range) within tolerance, update its delta
    // instead of appending a duplicate. Prefer non-zero delta when one is 0.
    const qint64 timeToleranceMs = 500;
    const float rangeTolerance = 0.5f;
    for (size_t i = 0; i < btwMarkers.size(); ++i) {
        const BTWMarkerData& existing = btwMarkers[i];
        qint64 timeDiff = qAbs(existing.timestamp.msecsTo(timestamp));
        float rangeDiff = qAbs(existing.range - range);
        if (timeDiff <= timeToleranceMs && rangeDiff <= rangeTolerance) {
            // Prefer non-zero delta: don't overwrite existing non-zero with 0
            float newDelta = (existing.delta != 0.0f && delta == 0.0f) ? existing.delta : delta;
            btwMarkers[i].delta = newDelta;
            DEBUG_OUT() << "WaterfallData: Updated existing BTW marker at timestamp" << timestamp.toString() << "range" << range << "delta" << newDelta << "(deduplicated)";
            return;
        }
    }

    BTWMarkerData markerData;
    markerData.timestamp = timestamp;
    markerData.range = range;
    markerData.delta = delta;
    
    btwMarkers.push_back(markerData);
    
    DEBUG_OUT() << "WaterfallData: Added BTW marker at timestamp" << timestamp.toString() << "with range" << range << "and delta" << delta;
}

void WaterfallData::clearBTWMarkers()
{
    btwMarkers.clear();
    DEBUG_OUT() << "WaterfallData: Cleared all BTW markers";
}

bool WaterfallData::removeBTWMarker(const QDateTime& timestamp, float range, float toleranceMs, float rangeTolerance)
{
    // Use erase_if with predicate to remove matching marker
    bool found = false;
    btwMarkers.erase_if([&](const BTWMarkerData& marker) {
        qint64 timeDiff = qAbs(marker.timestamp.msecsTo(timestamp));
        float rangeDiff = qAbs(marker.range - range);
        
        if (timeDiff <= static_cast<qint64>(toleranceMs) && rangeDiff <= rangeTolerance)
        {
            found = true;
            return true; // Erase this element
        }
        return false;
    });
    
    if (found)
    {
        DEBUG_OUT() << "WaterfallData: Removed BTW marker at timestamp" << timestamp.toString() << "with range" << range;
        return true;
    }
    
    DEBUG_OUT() << "WaterfallData: BTW marker not found at timestamp" << timestamp.toString() << "with range" << range;
    return false;
}

std::vector<BTWMarkerData> WaterfallData::getBTWMarkers() const
{
    return btwMarkers.toVector();
}

std::vector<BTWMarkerData> WaterfallData::getBTWMarkersWithinTimeRange(const QDateTime& startTime, const QDateTime& endTime) const
{
    std::vector<BTWMarkerData> result;

    if (btwMarkers.empty()) {
        return result;
    }

    if (startTime.isValid() && endTime.isValid() && startTime > endTime)
    {
        return result;
    }

    result.reserve(btwMarkers.size());
    for (size_t i = 0; i < btwMarkers.size(); ++i)
    {
        const BTWMarkerData &marker = btwMarkers[i];
        if ((!startTime.isValid() || marker.timestamp >= startTime) &&
            (!endTime.isValid() || marker.timestamp <= endTime))
        {
            result.push_back(marker);
        }
    }

    return result;
}

size_t WaterfallData::getBTWMarkersCount() const
{
    return btwMarkers.size();
}

// RTW R Marker management methods implementation

void WaterfallData::addRTWRMarker(const QDateTime& timestamp, float range)
{
    RTWRMarkerData markerData;
    markerData.timestamp = timestamp;
    markerData.range = range;
    
    rtwRMarkers.push_back(markerData);
    
    DEBUG_OUT() << "WaterfallData: Added RTW R marker at timestamp" << timestamp.toString() << "with range" << range;
}

void WaterfallData::clearRTWRMarkers()
{
    rtwRMarkers.clear();
    DEBUG_OUT() << "WaterfallData: Cleared all RTW R markers";
}

bool WaterfallData::removeRTWRMarker(const QDateTime& timestamp, float range, float toleranceMs, float rangeTolerance)
{
    // Use erase_if with predicate to remove matching marker
    bool found = false;
    rtwRMarkers.erase_if([&](const RTWRMarkerData& marker) {
        qint64 timeDiff = qAbs(marker.timestamp.msecsTo(timestamp));
        float rangeDiff = qAbs(marker.range - range);
        
        if (timeDiff <= static_cast<qint64>(toleranceMs) && rangeDiff <= rangeTolerance)
        {
            found = true;
            return true; // Erase this element
        }
        return false;
    });
    
    if (found)
    {
        DEBUG_OUT() << "WaterfallData: Removed RTW R marker at timestamp" << timestamp.toString() << "with range" << range;
        return true;
    }
    
    DEBUG_OUT() << "WaterfallData: RTW R marker not found at timestamp" << timestamp.toString() << "with range" << range;
    return false;
}

std::vector<RTWRMarkerData> WaterfallData::getRTWRMarkers() const
{
    return rtwRMarkers.toVector();
}

size_t WaterfallData::getRTWRMarkersCount() const
{
    return rtwRMarkers.size();
}

// Capacity management methods implementation

void WaterfallData::setDataSeriesCapacity(const QString& seriesLabel, size_t capacity)
{
    // Set circular buffer capacity for Y data
    dataSeriesYData[seriesLabel].setCapacity(capacity);
    
    // Set circular buffer capacity for timestamps
    dataSeriesTimestamps[seriesLabel].setCapacity(capacity);
    
    // Set circular buffer capacity for epoch timestamps
    dataSeriesTimestampsEpoch[seriesLabel].setCapacity(capacity);
}

void WaterfallData::setAllDataSeriesCapacity(size_t capacity)
{
    // Set circular buffer capacity for all existing series
    for (auto& pair : dataSeriesYData)
    {
        pair.second.setCapacity(capacity);
    }
    
    for (auto& pair : dataSeriesTimestamps)
    {
        pair.second.setCapacity(capacity);
    }
    
    for (auto& pair : dataSeriesTimestampsEpoch)
    {
        pair.second.setCapacity(capacity);
    }
}

void WaterfallData::setRTWSymbolsCapacity(size_t capacity)
{
    rtwSymbols.setCapacity(capacity);
}

void WaterfallData::setBTWSymbolsCapacity(size_t capacity)
{
    btwSymbols.setCapacity(capacity);
}

void WaterfallData::setBTWMarkersCapacity(size_t capacity)
{
    btwMarkers.setCapacity(capacity);
}

void WaterfallData::setRTWRMarkersCapacity(size_t capacity)
{
    rtwRMarkers.setCapacity(capacity);
}

void WaterfallData::setAllSymbolsAndMarkersCapacity(size_t symbolsCapacity, size_t markersCapacity)
{
    setRTWSymbolsCapacity(symbolsCapacity);
    setBTWSymbolsCapacity(symbolsCapacity);
    setBTWMarkersCapacity(markersCapacity);
    setRTWRMarkersCapacity(markersCapacity);
}

// Phase 1 Performance Optimization: Range cache helper methods

void WaterfallData::invalidateRangeCache()
{
    m_cachedCombinedYRangeValid = false;
    m_dataVersion++;  // Increment version for cache invalidation
}

void WaterfallData::updateSeriesMinMax(const QString& seriesLabel)
{
    auto it = dataSeriesYData.find(seriesLabel);
    if (it == dataSeriesYData.end() || it->second.empty()) {
        m_seriesMinMax.erase(seriesLabel);
        return;
    }
    
    const CircularBuffer<float>& buffer = it->second;
    float seriesMin = buffer[0];
    float seriesMax = buffer[0];
    
    for (size_t i = 1; i < buffer.size(); ++i) {
        float val = buffer[i];
        if (val < seriesMin) seriesMin = val;
        if (val > seriesMax) seriesMax = val;
    }
    
    m_seriesMinMax[seriesLabel] = std::make_pair(seriesMin, seriesMax);
}

void WaterfallData::recomputeCombinedYRange() const
{
    float globalMin = std::numeric_limits<float>::max();
    float globalMax = std::numeric_limits<float>::lowest();
    bool hasData = false;

    // Use cached per-series min/max if available, otherwise compute on-the-fly
    for (const auto& pair : dataSeriesYData) {
        if (!pair.second.empty()) {
            float seriesMin, seriesMax;
            
            // Check if we have cached min/max for this series
            auto minMaxIt = m_seriesMinMax.find(pair.first);
            if (minMaxIt != m_seriesMinMax.end()) {
                seriesMin = minMaxIt->second.first;
                seriesMax = minMaxIt->second.second;
            } else {
                // Compute min/max for this series
                seriesMin = pair.second[0];
                seriesMax = pair.second[0];
                for (size_t i = 1; i < pair.second.size(); ++i) {
                    float val = pair.second[i];
                    if (val < seriesMin) seriesMin = val;
                    if (val > seriesMax) seriesMax = val;
                }
            }
            
            globalMin = std::min(globalMin, seriesMin);
            globalMax = std::max(globalMax, seriesMax);
            hasData = true;
        }
    }

    if (!hasData) {
        m_cachedCombinedYRange = std::make_pair(0.0f, 0.0f);
    } else {
        m_cachedCombinedYRange = std::make_pair(globalMin, globalMax);
    }
    m_cachedCombinedYRangeValid = true;
}
