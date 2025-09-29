#include "waterfalldata.h"
#include <algorithm>
#include <limits>
#include <QStringList>

WaterfallData::WaterfallData(const QString& title) 
{
    dataTitle = title;
    // Initialize empty vectors
    yData.clear();
    timestamps.clear();
}

WaterfallData::~WaterfallData() 
{
    // Vectors will be automatically cleaned up
}

void WaterfallData::setData(const std::vector<qreal>& yData, const std::vector<QDateTime>& timestamps)
{
    // Validate that both vectors have the same size
    if (yData.size() != timestamps.size()) {
        qDebug() << "Error: yData and timestamps must have the same size. yData size:" << yData.size() << "timestamps size:" << timestamps.size();
        return;
    }
    
    // Store the data
    this->yData = yData;
    this->timestamps = timestamps;
    
    validateDataConsistency();
}

void WaterfallData::clearData()
{
    yData.clear();
    timestamps.clear();
}

void WaterfallData::addDataPoint(qreal yValue, const QDateTime& timestamp)
{
    yData.push_back(yValue);
    timestamps.push_back(timestamp);
}

void WaterfallData::addDataPoints(const std::vector<qreal>& yValues, const std::vector<QDateTime>& timestamps)
{
    // Validate that both vectors have the same size
    if (yValues.size() != timestamps.size()) {
        qDebug() << "Error: yValues and timestamps must have the same size. yValues size:" << yValues.size() << "timestamps size:" << timestamps.size();
        return;
    }
    
    // Append the data
    this->yData.insert(this->yData.end(), yValues.begin(), yValues.end());
    this->timestamps.insert(this->timestamps.end(), timestamps.begin(), timestamps.end());
    
    validateDataConsistency();
}

std::vector<std::pair<qreal, QDateTime>> WaterfallData::getAllData() const
{
    std::vector<std::pair<qreal, QDateTime>> result;
    result.reserve(yData.size());
    
    for (size_t i = 0; i < yData.size(); ++i) {
        result.emplace_back(yData[i], timestamps[i]);
    }
    
    return result;
}

std::vector<std::pair<qreal, QDateTime>> WaterfallData::getDataWithinYExtents(qreal yMin, qreal yMax) const
{
    std::vector<std::pair<qreal, QDateTime>> result;
    
    for (size_t i = 0; i < yData.size(); ++i) {
        if (yData[i] >= yMin && yData[i] <= yMax) {
            result.emplace_back(yData[i], timestamps[i]);
        }
    }
    
    return result;
}

std::vector<std::pair<qreal, QDateTime>> WaterfallData::getDataWithinTimeRange(const QDateTime& startTime, const QDateTime& endTime) const
{
    std::vector<std::pair<qreal, QDateTime>> result;
    
    for (size_t i = 0; i < timestamps.size(); ++i) {
        if (timestamps[i] >= startTime && timestamps[i] <= endTime) {
            result.emplace_back(yData[i], timestamps[i]);
        }
    }
    
    return result;
}

const std::vector<qreal>& WaterfallData::getYData() const
{
    return yData;
}

const std::vector<QDateTime>& WaterfallData::getTimestamps() const
{
    return timestamps;
}

size_t WaterfallData::getDataSize() const
{
    return yData.size();
}

bool WaterfallData::isEmpty() const
{
    return yData.empty() && timestamps.empty();
}

std::pair<qreal, qreal> WaterfallData::getYRange() const
{
    if (yData.empty()) {
        return std::make_pair(0.0, 0.0);
    }
    
    auto minMax = std::minmax_element(yData.begin(), yData.end());
    return std::make_pair(*minMax.first, *minMax.second);
}

std::pair<QDateTime, QDateTime> WaterfallData::getTimeRange() const
{
    if (timestamps.empty()) {
        return std::make_pair(QDateTime(), QDateTime());
    }
    
    auto minMax = std::minmax_element(timestamps.begin(), timestamps.end());
    return std::make_pair(*minMax.first, *minMax.second);
}

qreal WaterfallData::getMinY() const
{
    if (yData.empty()) {
        return 0.0;
    }
    
    return *std::min_element(yData.begin(), yData.end());
}

qreal WaterfallData::getMaxY() const
{
    if (yData.empty()) {
        return 0.0;
    }
    
    return *std::max_element(yData.begin(), yData.end());
}

qint64 WaterfallData::getTimeSpanMs() const
{
    if (timestamps.size() < 2) {
        return 0;
    }
    
    auto timeRange = getTimeRange();
    return timeRange.first.msecsTo(timeRange.second);
}

QDateTime WaterfallData::getEarliestTime() const
{
    if (timestamps.empty()) {
        return QDateTime();
    }
    
    return *std::min_element(timestamps.begin(), timestamps.end());
}

QDateTime WaterfallData::getLatestTime() const
{
    if (timestamps.empty()) {
        return QDateTime();
    }
    
    return *std::max_element(timestamps.begin(), timestamps.end());
}

bool WaterfallData::isValidIndex(size_t index) const
{
    return index < yData.size() && index < timestamps.size();
}

void WaterfallData::validateDataConsistency() const
{
    if (yData.size() != timestamps.size()) {
        qDebug() << "Warning: Data inconsistency detected - yData size:" << yData.size() << "timestamps size:" << timestamps.size();
    }
}

void WaterfallData::validateDataSeriesConsistency(const QString &seriesLabel) const
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

void WaterfallData::addDataSeries(const QString &seriesLabel, const std::vector<qreal> &yData, const std::vector<QDateTime> &timestamps)
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

void WaterfallData::addDataPointToSeries(const QString &seriesLabel, qreal yValue, const QDateTime &timestamp)
{
    dataSeriesYData[seriesLabel].push_back(yValue);
    dataSeriesTimestamps[seriesLabel].push_back(timestamp);
    
    validateDataSeriesConsistency(seriesLabel);
}

void WaterfallData::addDataPointsToSeries(const QString &seriesLabel, const std::vector<qreal> &yValues, const std::vector<QDateTime> &timestamps)
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

void WaterfallData::clearDataSeries(const QString &seriesLabel)
{
    dataSeriesYData.erase(seriesLabel);
    dataSeriesTimestamps.erase(seriesLabel);
}

void WaterfallData::clearAllDataSeries()
{
    dataSeriesYData.clear();
    dataSeriesTimestamps.clear();
}

std::vector<std::pair<qreal, QDateTime>> WaterfallData::getDataSeries(const QString &seriesLabel) const
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

std::vector<std::pair<qreal, QDateTime>> WaterfallData::getDataSeriesWithinYExtents(const QString &seriesLabel, qreal yMin, qreal yMax) const
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

std::vector<std::pair<qreal, QDateTime>> WaterfallData::getDataSeriesWithinTimeRange(const QString &seriesLabel, const QDateTime &startTime, const QDateTime &endTime) const
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

const std::vector<qreal> &WaterfallData::getYDataSeries(const QString &seriesLabel) const
{
    static const std::vector<qreal> emptyVector;
    auto it = dataSeriesYData.find(seriesLabel);
    return (it != dataSeriesYData.end()) ? it->second : emptyVector;
}

const std::vector<QDateTime> &WaterfallData::getTimestampsSeries(const QString &seriesLabel) const
{
    static const std::vector<QDateTime> emptyVector;
    auto it = dataSeriesTimestamps.find(seriesLabel);
    return (it != dataSeriesTimestamps.end()) ? it->second : emptyVector;
}

size_t WaterfallData::getDataSeriesSize(const QString &seriesLabel) const
{
    auto it = dataSeriesYData.find(seriesLabel);
    return (it != dataSeriesYData.end()) ? it->second.size() : 0;
}

bool WaterfallData::isDataSeriesEmpty(const QString &seriesLabel) const
{
    auto it = dataSeriesYData.find(seriesLabel);
    return (it == dataSeriesYData.end()) || it->second.empty();
}

bool WaterfallData::hasDataSeries(const QString &seriesLabel) const
{
    return dataSeriesYData.find(seriesLabel) != dataSeriesYData.end();
}

std::vector<QString> WaterfallData::getDataSeriesLabels() const
{
    std::vector<QString> labels;
    labels.reserve(dataSeriesYData.size());
    
    for (const auto &pair : dataSeriesYData) {
        labels.push_back(pair.first);
    }
    
    return labels;
}

std::pair<qreal, qreal> WaterfallData::getYRangeSeries(const QString &seriesLabel) const
{
    auto it = dataSeriesYData.find(seriesLabel);
    if (it == dataSeriesYData.end() || it->second.empty()) {
        return std::make_pair(0.0, 0.0);
    }
    
    auto minMax = std::minmax_element(it->second.begin(), it->second.end());
    return std::make_pair(*minMax.first, *minMax.second);
}

std::pair<QDateTime, QDateTime> WaterfallData::getTimeRangeSeries(const QString &seriesLabel) const
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
    
    // Check legacy data
    if (!yData.empty()) {
        auto minMax = std::minmax_element(yData.begin(), yData.end());
        globalMin = std::min(globalMin, *minMax.first);
        globalMax = std::max(globalMax, *minMax.second);
        hasData = true;
    }
    
    // Check all data series
    for (const auto &pair : dataSeriesYData) {
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
    
    // Check legacy data
    if (!timestamps.empty()) {
        auto minMax = std::minmax_element(timestamps.begin(), timestamps.end());
        globalMin = *minMax.first;
        globalMax = *minMax.second;
        hasData = true;
    }
    
    // Check all data series
    for (const auto &pair : dataSeriesTimestamps) {
        if (!pair.second.empty()) {
            auto minMax = std::minmax_element(pair.second.begin(), pair.second.end());
            if (!hasData) {
                globalMin = *minMax.first;
                globalMax = *minMax.second;
                hasData = true;
            } else {
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
