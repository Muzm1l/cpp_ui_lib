#include "waterfalldata.h"
#include <algorithm>
#include <limits>

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
