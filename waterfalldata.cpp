#include "waterfalldata.h"
#include "debugutils.h"
#include <algorithm>
#include <limits>
#include <QStringList>

WaterfallData::WaterfallData(const QString& title)
{
    dataTitle = title;
    // Initialize empty circular buffers (unlimited capacity by default)
    dataSeriesYData[dataTitle] = CircularBuffer<qreal>(0);
    dataSeriesTimestamps[dataTitle] = CircularBuffer<QDateTime>(0);
    dataSeriesTimestampsEpoch[dataTitle] = CircularBuffer<qint64>(0);
}

WaterfallData::WaterfallData(const QString& title, const std::vector<QString>& seriesLabels)
{
    dataTitle = title;
    
    // Initialize empty circular buffers for each provided label (unlimited capacity by default)
    for (const QString& seriesLabel : seriesLabels)
    {
        dataSeriesYData[seriesLabel] = CircularBuffer<qreal>(0);
        dataSeriesTimestamps[seriesLabel] = CircularBuffer<QDateTime>(0);
        dataSeriesTimestampsEpoch[seriesLabel] = CircularBuffer<qint64>(0);
    }
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

void WaterfallData::setData(const std::vector<qreal>& yData, const std::vector<QDateTime>& timestamps)
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
}

void WaterfallData::clearData()
{
    dataSeriesYData[dataTitle].clear();
    dataSeriesTimestamps[dataTitle].clear();
    dataSeriesTimestampsEpoch[dataTitle].clear();
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
        
        // Iterate through circular buffer using indexing
        qreal seriesMin = pair.second[0];
        qreal seriesMax = pair.second[0];
        for (size_t i = 1; i < pair.second.size(); ++i)
        {
            qreal val = pair.second[i];
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
        
        // Find min using indexing
        qreal seriesMin = pair.second[0];
        for (size_t i = 1; i < pair.second.size(); ++i) {
            if (pair.second[i] < seriesMin) {
                seriesMin = pair.second[i];
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
        
        // Find max using indexing
        qreal seriesMax = pair.second[0];
        for (size_t i = 1; i < pair.second.size(); ++i) {
            if (pair.second[i] > seriesMax) {
                seriesMax = pair.second[i];
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

    // Convert float to double (qreal) for internal storage
    std::vector<qreal> yDataDouble(yData.begin(), yData.end());

    // Store the data series using circular buffers (preserve existing capacity)
    size_t existingCapacity = dataSeriesYData[seriesLabel].capacity();
    dataSeriesYData[seriesLabel].assign(yDataDouble);
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
}

void WaterfallData::addDataPointToSeries(const QString& seriesLabel, float yValue, const QDateTime& timestamp)
{
    // Convert float to double (qreal) for internal storage
    qreal yValueDouble = static_cast<qreal>(yValue);
    
    // Circular buffers automatically handle capacity limits
    dataSeriesYData[seriesLabel].push_back(yValueDouble);
    dataSeriesTimestamps[seriesLabel].push_back(timestamp);
    // Store epoch milliseconds in parallel (convert once, not in hot path)
    dataSeriesTimestampsEpoch[seriesLabel].push_back(timestamp.toMSecsSinceEpoch());

    validateDataSeriesConsistency(seriesLabel);
}

void WaterfallData::addDataPointsToSeries(const QString& seriesLabel, const std::vector<float>& yValues, const std::vector<QDateTime>& timestamps)
{
    // Validate that both vectors have the same size
    if (yValues.size() != timestamps.size()) {
        DEBUG_OUT() << "Error: yValues and timestamps must have the same size for series" << seriesLabel
            << ". yValues size:" << yValues.size() << "timestamps size:" << timestamps.size();
        return;
    }

    // Convert float to double (qreal) for internal storage
    std::vector<qreal> yValuesDouble(yValues.begin(), yValues.end());

    // Append the data to existing series (circular buffers handle capacity automatically)
    dataSeriesYData[seriesLabel].push_back(yValuesDouble);
    dataSeriesTimestamps[seriesLabel].push_back(timestamps);
    
    // Store epoch milliseconds in parallel (convert once, not in hot path)
    for (const QDateTime& ts : timestamps) {
        dataSeriesTimestampsEpoch[seriesLabel].push_back(ts.toMSecsSinceEpoch());
    }

    validateDataSeriesConsistency(seriesLabel);
}

void WaterfallData::clearDataSeries(const QString& seriesLabel)
{
    dataSeriesYData.erase(seriesLabel);
    dataSeriesTimestamps.erase(seriesLabel);
    dataSeriesTimestampsEpoch.erase(seriesLabel);
}

void WaterfallData::clearAllDataSeries()
{
    dataSeriesYData.clear();
    dataSeriesTimestamps.clear();
    dataSeriesTimestampsEpoch.clear();
}

std::vector<std::pair<qreal, QDateTime>> WaterfallData::getDataSeries(const QString& seriesLabel) const
{
    std::vector<std::pair<qreal, QDateTime>> result;

    auto yIt = dataSeriesYData.find(seriesLabel);
    auto tIt = dataSeriesTimestamps.find(seriesLabel);

    if (yIt != dataSeriesYData.end() && tIt != dataSeriesTimestamps.end()) {
        result.reserve(yIt->second.size());

        for (size_t i = 0; i < yIt->second.size(); ++i) {
            result.emplace_back(yIt->second[i], tIt->second[i]);
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
            if (yIt->second[i] >= yMin && yIt->second[i] <= yMax) {
                result.emplace_back(yIt->second[i], tIt->second[i]);
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
        std::vector<qreal> yData = yIt->second.toVector();
        
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
                result.emplace_back(yData[i], timestamps[i]);
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
    std::vector<qreal> yData = yIt->second.toVector();
    
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
        outValue = yData[bestIdx];
        outIndex = bestIdx;
        return true;
    }
    
    return false;
}

std::vector<qreal> WaterfallData::getYDataSeries(const QString& seriesLabel) const
{
    auto it = dataSeriesYData.find(seriesLabel);
    if (it != dataSeriesYData.end())
    {
        return it->second.toVector();
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

    // Find min/max using indexing
    qreal minY = it->second[0];
    qreal maxY = it->second[0];
    for (size_t i = 1; i < it->second.size(); ++i) {
        if (it->second[i] < minY) minY = it->second[i];
        if (it->second[i] > maxY) maxY = it->second[i];
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
    qreal globalMin = std::numeric_limits<qreal>::max();
    qreal globalMax = std::numeric_limits<qreal>::lowest();
    bool hasData = false;

    // Check all data series
    for (const auto& pair : dataSeriesYData) {
        if (!pair.second.empty()) {
            // Find min/max using indexing
            qreal seriesMin = pair.second[0];
            qreal seriesMax = pair.second[0];
            for (size_t i = 1; i < pair.second.size(); ++i) {
                if (pair.second[i] < seriesMin) seriesMin = pair.second[i];
                if (pair.second[i] > seriesMax) seriesMax = pair.second[i];
            }
            
            globalMin = std::min(globalMin, seriesMin);
            globalMax = std::max(globalMax, seriesMax);
            hasData = true;
        }
    }

    if (!hasData) {
        return std::make_pair(0.0, 0.0);
    }

    return std::make_pair(globalMin, globalMax);
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

void WaterfallData::setDataSeries(const QString& seriesLabel, const std::vector<qreal>& yData, const std::vector<QDateTime>& timestamps)
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
}

std::vector<std::pair<qreal, QDateTime>> WaterfallData::getAllDataSeries(const QString& seriesLabel) const
{
    std::vector<std::pair<qreal, QDateTime>> result;
    
    auto yIt = dataSeriesYData.find(seriesLabel);
    auto tIt = dataSeriesTimestamps.find(seriesLabel);
    
    if (yIt != dataSeriesYData.end() && tIt != dataSeriesTimestamps.end()) {
        result.reserve(yIt->second.size());
        
        for (size_t i = 0; i < yIt->second.size(); ++i) {
            result.emplace_back(yIt->second[i], tIt->second[i]);
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

    // Find min using indexing
    qreal minY = it->second[0];
    for (size_t i = 1; i < it->second.size(); ++i) {
        if (it->second[i] < minY) minY = it->second[i];
    }
    return minY;
}

qreal WaterfallData::getMaxYSeries(const QString& seriesLabel) const
{
    auto it = dataSeriesYData.find(seriesLabel);
    if (it == dataSeriesYData.end() || it->second.empty()) {
        return 0.0;
    }

    // Find max using indexing
    qreal maxY = it->second[0];
    for (size_t i = 1; i < it->second.size(); ++i) {
        if (it->second[i] > maxY) maxY = it->second[i];
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
    std::vector<qreal> yData = yIt->second.toVector();
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

void WaterfallData::addRTWSymbol(const QString& symbolName, const QDateTime& timestamp, qreal range)
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

bool WaterfallData::removeRTWSymbol(const QString& symbolName, const QDateTime& timestamp, qreal range, qreal toleranceMs, qreal rangeTolerance)
{
    // Use erase_if with predicate to remove matching symbol
    bool found = false;
    rtwSymbols.erase_if([&](const RTWSymbolData& symbol) {
        if (symbol.symbolName != symbolName) return false;
        
        qint64 timeDiff = qAbs(symbol.timestamp.msecsTo(timestamp));
        qreal rangeDiff = qAbs(symbol.range - range);
        
        if (timeDiff <= toleranceMs && rangeDiff <= rangeTolerance)
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
void WaterfallData::addBTWSymbol(const QString& symbolName, const QDateTime& timestamp, qreal range)
{
    BTWSymbolData symbolData;
    symbolData.symbolName = symbolName;
    symbolData.timestamp = timestamp;
    symbolData.range = range;
    
    btwSymbols.push_back(symbolData);
    
    DEBUG_OUT() << "WaterfallData: Added BTW symbol" << symbolName << "at timestamp" << timestamp.toString() << "with range" << range;
}

void WaterfallData::clearBTWSymbols()
{
    btwSymbols.clear();
    DEBUG_OUT() << "WaterfallData: Cleared all BTW symbols";
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
    
    // Create a sorted copy for binary search (symbols may not be sorted)
    // This is more efficient than sorting the original vector
    std::vector<BTWSymbolData> sortedSymbols = btwSymbols.toVector();
    std::sort(sortedSymbols.begin(), sortedSymbols.end(),
        [](const BTWSymbolData& a, const BTWSymbolData& b) {
            return a.timestamp < b.timestamp;
        });
    
    // Use binary search to find the range of symbols within the time window
    auto startIt = std::lower_bound(sortedSymbols.begin(), sortedSymbols.end(), startTime,
        [](const BTWSymbolData& symbol, const QDateTime& time) {
            return symbol.timestamp < time;
        });
    
    auto endIt = std::upper_bound(sortedSymbols.begin(), sortedSymbols.end(), endTime,
        [](const QDateTime& time, const BTWSymbolData& symbol) {
            return time < symbol.timestamp;
        });
    
    // Copy the range
    result.reserve(std::distance(startIt, endIt));
    result.assign(startIt, endIt);
    
    return result;
}

size_t WaterfallData::getBTWSymbolsCount() const
{
    return btwSymbols.size();
}

// BTW Marker management methods implementation

void WaterfallData::addBTWMarker(const QDateTime& timestamp, qreal range, qreal delta)
{
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

bool WaterfallData::removeBTWMarker(const QDateTime& timestamp, qreal range, qreal toleranceMs, qreal rangeTolerance)
{
    // Use erase_if with predicate to remove matching marker
    bool found = false;
    btwMarkers.erase_if([&](const BTWMarkerData& marker) {
        qint64 timeDiff = qAbs(marker.timestamp.msecsTo(timestamp));
        qreal rangeDiff = qAbs(marker.range - range);
        
        if (timeDiff <= toleranceMs && rangeDiff <= rangeTolerance)
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
    
    // Create a sorted copy for binary search (markers may not be sorted)
    // This is more efficient than sorting the original vector
    std::vector<BTWMarkerData> sortedMarkers = btwMarkers.toVector();
    std::sort(sortedMarkers.begin(), sortedMarkers.end(),
        [](const BTWMarkerData& a, const BTWMarkerData& b) {
            return a.timestamp < b.timestamp;
        });
    
    // Use binary search to find the range of markers within the time window
    auto startIt = std::lower_bound(sortedMarkers.begin(), sortedMarkers.end(), startTime,
        [](const BTWMarkerData& marker, const QDateTime& time) {
            return marker.timestamp < time;
        });
    
    auto endIt = std::upper_bound(sortedMarkers.begin(), sortedMarkers.end(), endTime,
        [](const QDateTime& time, const BTWMarkerData& marker) {
            return time < marker.timestamp;
        });
    
    // Copy the range
    result.reserve(std::distance(startIt, endIt));
    result.assign(startIt, endIt);
    
    return result;
}

size_t WaterfallData::getBTWMarkersCount() const
{
    return btwMarkers.size();
}

// RTW R Marker management methods implementation

void WaterfallData::addRTWRMarker(const QDateTime& timestamp, qreal range)
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

bool WaterfallData::removeRTWRMarker(const QDateTime& timestamp, qreal range, qreal toleranceMs, qreal rangeTolerance)
{
    // Use erase_if with predicate to remove matching marker
    bool found = false;
    rtwRMarkers.erase_if([&](const RTWRMarkerData& marker) {
        qint64 timeDiff = qAbs(marker.timestamp.msecsTo(timestamp));
        qreal rangeDiff = qAbs(marker.range - range);
        
        if (timeDiff <= toleranceMs && rangeDiff <= rangeTolerance)
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
