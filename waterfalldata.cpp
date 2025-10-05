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

WaterfallData::~WaterfallData()
{
    // Vectors will be automatically cleaned up
    dataSeriesYData.clear();
    dataSeriesTimestamps.clear();
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

void WaterfallData::addDataPoint(qreal yValue, const QDateTime& timestamp)
{
    dataSeriesYData[dataTitle].push_back(yValue);
    dataSeriesTimestamps[dataTitle].push_back(timestamp);
}

void WaterfallData::addDataPoints(const std::vector<qreal>& yValues, const std::vector<QDateTime>& timestamps)
{
    // Validate that both vectors have the same size
    if (yValues.size() != timestamps.size()) {
        qDebug() << "Error: yValues and timestamps must have the same size. yValues size:" << yValues.size() << "timestamps size:" << timestamps.size();
        return;
    }

    // Append the data
    this->dataSeriesYData[dataTitle].insert(this->dataSeriesYData[dataTitle].end(), yValues.begin(), yValues.end());
    this->dataSeriesTimestamps[dataTitle].insert(this->dataSeriesTimestamps[dataTitle].end(), timestamps.begin(), timestamps.end());

    validateDataConsistency();
}

std::vector<std::pair<qreal, QDateTime>> WaterfallData::getAllData() const
{
    std::vector<std::pair<qreal, QDateTime>> result;
    
    auto yIt = dataSeriesYData.find(dataTitle);
    auto tIt = dataSeriesTimestamps.find(dataTitle);
    
    if (yIt != dataSeriesYData.end() && tIt != dataSeriesTimestamps.end()) {
        result.reserve(yIt->second.size());
        
        for (size_t i = 0; i < yIt->second.size(); ++i) {
            result.emplace_back(yIt->second[i], tIt->second[i]);
        }
    }
    
    return result;
}

std::vector<std::pair<qreal, QDateTime>> WaterfallData::getDataWithinYExtents(qreal yMin, qreal yMax) const
{
    std::vector<std::pair<qreal, QDateTime>> result;
    
    auto yIt = dataSeriesYData.find(dataTitle);
    auto tIt = dataSeriesTimestamps.find(dataTitle);
    
    if (yIt != dataSeriesYData.end() && tIt != dataSeriesTimestamps.end()) {
        for (size_t i = 0; i < yIt->second.size(); ++i) {
            if (yIt->second[i] >= yMin && yIt->second[i] <= yMax) {
                result.emplace_back(yIt->second[i], tIt->second[i]);
            }
        }
    }
    
    return result;
}

std::vector<std::pair<qreal, QDateTime>> WaterfallData::getDataWithinTimeRange(const QDateTime& startTime, const QDateTime& endTime) const
{
    std::vector<std::pair<qreal, QDateTime>> result;
    
    auto yIt = dataSeriesYData.find(dataTitle);
    auto tIt = dataSeriesTimestamps.find(dataTitle);
    
    if (yIt != dataSeriesYData.end() && tIt != dataSeriesTimestamps.end()) {
        for (size_t i = 0; i < tIt->second.size(); ++i) {
            if (tIt->second[i] >= startTime && tIt->second[i] <= endTime) {
                result.emplace_back(yIt->second[i], tIt->second[i]);
            }
        }
    }
    
    return result;
}

const std::vector<qreal>& WaterfallData::getYData() const
{
    static const std::vector<qreal> emptyVector;
    auto it = dataSeriesYData.find(dataTitle);
    return (it != dataSeriesYData.end()) ? it->second : emptyVector;
}

const std::vector<QDateTime>& WaterfallData::getTimestamps() const
{
    static const std::vector<QDateTime> emptyVector;
    auto it = dataSeriesTimestamps.find(dataTitle);
    return (it != dataSeriesTimestamps.end()) ? it->second : emptyVector;
}

size_t WaterfallData::getDataSize() const
{
    auto it = dataSeriesYData.find(dataTitle);
    return (it != dataSeriesYData.end()) ? it->second.size() : 0;
}

bool WaterfallData::isEmpty() const
{
    auto yIt = dataSeriesYData.find(dataTitle);
    auto tIt = dataSeriesTimestamps.find(dataTitle);
    return (yIt == dataSeriesYData.end() || yIt->second.empty()) && 
           (tIt == dataSeriesTimestamps.end() || tIt->second.empty());
}

std::pair<qreal, qreal> WaterfallData::getYRange() const
{
    auto it = dataSeriesYData.find(dataTitle);
    if (it == dataSeriesYData.end() || it->second.empty()) {
        return std::make_pair(0.0, 0.0);
    }

    auto minMax = std::minmax_element(it->second.begin(), it->second.end());
    return std::make_pair(*minMax.first, *minMax.second);
}

std::pair<QDateTime, QDateTime> WaterfallData::getTimeRange() const
{
    auto it = dataSeriesTimestamps.find(dataTitle);
    if (it == dataSeriesTimestamps.end() || it->second.empty()) {
        return std::make_pair(QDateTime(), QDateTime());
    }

    auto minMax = std::minmax_element(it->second.begin(), it->second.end());
    return std::make_pair(*minMax.first, *minMax.second);
}

qreal WaterfallData::getMinY() const
{
    auto it = dataSeriesYData.find(dataTitle);
    if (it == dataSeriesYData.end() || it->second.empty()) {
        return 0.0;
    }

    return *std::min_element(it->second.begin(), it->second.end());
}

qreal WaterfallData::getMaxY() const
{
    auto it = dataSeriesYData.find(dataTitle);
    if (it == dataSeriesYData.end() || it->second.empty()) {
        return 0.0;
    }

    return *std::max_element(it->second.begin(), it->second.end());
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
    auto it = dataSeriesTimestamps.find(dataTitle);
    if (it == dataSeriesTimestamps.end() || it->second.empty()) {
        return QDateTime();
    }

    return *std::min_element(it->second.begin(), it->second.end());
}

QDateTime WaterfallData::getLatestTime() const
{
    auto it = dataSeriesTimestamps.find(dataTitle);
    if (it == dataSeriesTimestamps.end() || it->second.empty()) {
        return QDateTime();
    }

    return *std::max_element(it->second.begin(), it->second.end());
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
