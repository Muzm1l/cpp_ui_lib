#include "waterfalldata.h"
#include <algorithm>
#include <limits>
#include <QStringList>

WaterfallData::WaterfallData(const QString& title)
{
    dataTitle = title;
    // Initialize empty vectors
    dataSeriesYData[dataTitle] = std::vector<qreal>();
    dataSeriesTimestamps[dataTitle] = std::vector<QDateTime>();
}

WaterfallData::WaterfallData(const QString& title, const std::vector<QString>& seriesLabels)
{
    dataTitle = title;
    
    // Initialize empty series for each provided label
    for (const QString& seriesLabel : seriesLabels)
    {
        dataSeriesYData[seriesLabel] = std::vector<qreal>();
        dataSeriesTimestamps[seriesLabel] = std::vector<QDateTime>();
    }
    
}

WaterfallData::~WaterfallData()
{
    // Vectors will be automatically cleaned up
    dataSeriesYData.clear();
    dataSeriesTimestamps.clear();
    rtwSymbols.clear();
}

void WaterfallData::setData(const std::vector<qreal>& yData, const std::vector<QDateTime>& timestamps)
{
    // Validate that both vectors have the same size
    if (yData.size() != timestamps.size()) {
        qDebug() << "Error: yData and timestamps must have the same size. yData size:" << yData.size() << "timestamps size:" << timestamps.size();
        return;
    }

    // Store the data
    this->dataSeriesYData[dataTitle] = yData;
    this->dataSeriesTimestamps[dataTitle] = timestamps;

    validateDataConsistency();
}

void WaterfallData::clearData()
{
    dataSeriesYData[dataTitle].clear();
    dataSeriesTimestamps[dataTitle].clear();
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
        auto minmax = std::minmax_element(pair.second.begin(), pair.second.end());
        if (!found) {
            minY = *minmax.first;
            maxY = *minmax.second;
            found = true;
        } else {
            if (*minmax.first < minY) minY = *minmax.first;
            if (*minmax.second > maxY) maxY = *minmax.second;
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
        for (const auto& t : pair.second) {
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
        qreal seriesMin = *std::min_element(pair.second.begin(), pair.second.end());
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
        qreal seriesMax = *std::max_element(pair.second.begin(), pair.second.end());
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
            QDateTime seriesEarliest = *std::min_element(pair.second.begin(), pair.second.end());
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
            QDateTime seriesLatest = *std::max_element(pair.second.begin(), pair.second.end());
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
            qDebug() << "Warning: Data inconsistency detected for series" << dataTitle
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
            qDebug() << "Warning: Data series inconsistency detected for series" << seriesLabel
                << "- yData size:" << yIt->second.size() << "timestamps size:" << tIt->second.size();
        }
    }
}

// Multiple data series methods implementation

void WaterfallData::addDataSeries(const QString& seriesLabel, const std::vector<qreal>& yData, const std::vector<QDateTime>& timestamps)
{
    // Validate that both vectors have the same size
    if (yData.size() != timestamps.size()) {
        qDebug() << "Error: yData and timestamps must have the same size for series" << seriesLabel
            << ". yData size:" << yData.size() << "timestamps size:" << timestamps.size();
        return;
    }

    // Store the data series
    dataSeriesYData[seriesLabel] = yData;
    dataSeriesTimestamps[seriesLabel] = timestamps;

    validateDataSeriesConsistency(seriesLabel);
}

void WaterfallData::addDataPointToSeries(const QString& seriesLabel, qreal yValue, const QDateTime& timestamp)
{
    dataSeriesYData[seriesLabel].push_back(yValue);
    dataSeriesTimestamps[seriesLabel].push_back(timestamp);

    validateDataSeriesConsistency(seriesLabel);
}

void WaterfallData::addDataPointsToSeries(const QString& seriesLabel, const std::vector<qreal>& yValues, const std::vector<QDateTime>& timestamps)
{
    // Validate that both vectors have the same size
    if (yValues.size() != timestamps.size()) {
        qDebug() << "Error: yValues and timestamps must have the same size for series" << seriesLabel
            << ". yValues size:" << yValues.size() << "timestamps size:" << timestamps.size();
        return;
    }

    // Append the data to existing series
    dataSeriesYData[seriesLabel].insert(dataSeriesYData[seriesLabel].end(), yValues.begin(), yValues.end());
    dataSeriesTimestamps[seriesLabel].insert(dataSeriesTimestamps[seriesLabel].end(), timestamps.begin(), timestamps.end());

    validateDataSeriesConsistency(seriesLabel);
}

void WaterfallData::clearDataSeries(const QString& seriesLabel)
{
    dataSeriesYData.erase(seriesLabel);
    dataSeriesTimestamps.erase(seriesLabel);
}

void WaterfallData::clearAllDataSeries()
{
    dataSeriesYData.clear();
    dataSeriesTimestamps.clear();
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
        for (size_t i = 0; i < tIt->second.size(); ++i) {
            if (tIt->second[i] >= startTime && tIt->second[i] <= endTime) {
                result.emplace_back(yIt->second[i], tIt->second[i]);
            }
        }
    }

    return result;
}

const std::vector<qreal>& WaterfallData::getYDataSeries(const QString& seriesLabel) const
{
    static const std::vector<qreal> emptyVector;
    auto it = dataSeriesYData.find(seriesLabel);
    return (it != dataSeriesYData.end()) ? it->second : emptyVector;
}

const std::vector<QDateTime>& WaterfallData::getTimestampsSeries(const QString& seriesLabel) const
{
    static const std::vector<QDateTime> emptyVector;
    auto it = dataSeriesTimestamps.find(seriesLabel);
    return (it != dataSeriesTimestamps.end()) ? it->second : emptyVector;
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

    auto minMax = std::minmax_element(it->second.begin(), it->second.end());
    return std::make_pair(*minMax.first, *minMax.second);
}

std::pair<QDateTime, QDateTime> WaterfallData::getTimeRangeSeries(const QString& seriesLabel) const
{
    auto it = dataSeriesTimestamps.find(seriesLabel);
    if (it == dataSeriesTimestamps.end() || it->second.empty()) {
        return std::make_pair(QDateTime(), QDateTime());
    }

    auto minMax = std::minmax_element(it->second.begin(), it->second.end());
    return std::make_pair(*minMax.first, *minMax.second);
}

std::pair<qreal, qreal> WaterfallData::getCombinedYRange() const
{
    qreal globalMin = std::numeric_limits<qreal>::max();
    qreal globalMax = std::numeric_limits<qreal>::lowest();
    bool hasData = false;

    // Check all data series
    for (const auto& pair : dataSeriesYData) {
        if (!pair.second.empty()) {
            auto minMax = std::minmax_element(pair.second.begin(), pair.second.end());
            globalMin = std::min(globalMin, *minMax.first);
            globalMax = std::max(globalMax, *minMax.second);
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
            auto minMax = std::minmax_element(pair.second.begin(), pair.second.end());
            if (!hasData) {
                globalMin = *minMax.first;
                globalMax = *minMax.second;
                hasData = true;
            }
            else {
                globalMin = std::min(globalMin, *minMax.first);
                globalMax = std::max(globalMax, *minMax.second);
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
        qDebug() << "Error: yData and timestamps must have the same size for series" << seriesLabel
            << ". yData size:" << yData.size() << "timestamps size:" << timestamps.size();
        return;
    }

    // Store the data series
    dataSeriesYData[seriesLabel] = yData;
    dataSeriesTimestamps[seriesLabel] = timestamps;

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

    return *std::min_element(it->second.begin(), it->second.end());
}

qreal WaterfallData::getMaxYSeries(const QString& seriesLabel) const
{
    auto it = dataSeriesYData.find(seriesLabel);
    if (it == dataSeriesYData.end() || it->second.empty()) {
        return 0.0;
    }

    return *std::max_element(it->second.begin(), it->second.end());
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

    return *std::min_element(it->second.begin(), it->second.end());
}

QDateTime WaterfallData::getLatestTimeSeries(const QString& seriesLabel) const
{
    auto it = dataSeriesTimestamps.find(seriesLabel);
    if (it == dataSeriesTimestamps.end() || it->second.empty()) {
        return QDateTime();
    }

    return *std::max_element(it->second.begin(), it->second.end());
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
    
    const auto& yData = yIt->second;
    const auto& timestamps = tIt->second;
    
    if (timestamps.empty()) {
        return result;
    }
    
    // Convert QTime duration to milliseconds
    qint64 binSizeMs = QTime(0, 0, 0).msecsTo(binDuration);
    
    if (binSizeMs <= 0) {
        qDebug() << "Warning: Invalid bin duration provided for series" << seriesLabel;
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
        qDebug() << "WaterfallData::binDataByTime: Invalid input data - sizes don't match or data is empty";
        return result;
    }
    
    // Convert QTime duration to milliseconds
    qint64 binSizeMs = QTime(0, 0, 0).msecsTo(binDuration);
    
    if (binSizeMs <= 0) {
        qDebug() << "WaterfallData::binDataByTime: Invalid bin duration provided";
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
    
    qDebug() << "WaterfallData::binDataByTime: Binned" << yData.size() << "points into" << result.size() << "bins with duration" << binSizeMs << "ms";
    
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
    
    qDebug() << "WaterfallData: Added RTW symbol" << symbolName << "at timestamp" << timestamp.toString() << "with range" << range;
}

void WaterfallData::clearRTWSymbols()
{
    rtwSymbols.clear();
    qDebug() << "WaterfallData: Cleared all RTW symbols";
}

std::vector<RTWSymbolData> WaterfallData::getRTWSymbols() const
{
    return rtwSymbols;
}

size_t WaterfallData::getRTWSymbolsCount() const
{
    return rtwSymbols.size();
}
