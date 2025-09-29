#ifndef WATERFALLDATA_H
#define WATERFALLDATA_H

#include <vector>
#include <utility>
#include <map>
#include <QDateTime>
#include <QDebug>
#include <QString>

class WaterfallData
{
public:
    WaterfallData(const QString &title = "");
    ~WaterfallData();

    // Data management methods
    void setData(const std::vector<qreal> &yData, const std::vector<QDateTime> &timestamps);
    void clearData();

    // Incremental data addition methods
    void addDataPoint(qreal yValue, const QDateTime &timestamp);
    void addDataPoints(const std::vector<qreal> &yValues, const std::vector<QDateTime> &timestamps);

    // Data access methods
    std::vector<std::pair<qreal, QDateTime>> getAllData() const;
    std::vector<std::pair<qreal, QDateTime>> getDataWithinYExtents(qreal yMin, qreal yMax) const;
    std::vector<std::pair<qreal, QDateTime>> getDataWithinTimeRange(const QDateTime &startTime, const QDateTime &endTime) const;

    // Direct access to data vectors
    const std::vector<qreal> &getYData() const;
    const std::vector<QDateTime> &getTimestamps() const;

    // Utility methods
    size_t getDataSize() const;
    bool isEmpty() const;

    // Data range methods
    std::pair<qreal, qreal> getYRange() const;
    std::pair<QDateTime, QDateTime> getTimeRange() const;

    // Individual min/max methods
    qreal getMinY() const;
    qreal getMaxY() const;

    // Time-based utility methods
    qint64 getTimeSpanMs() const;
    QDateTime getEarliestTime() const;
    QDateTime getLatestTime() const;
    
    // Selection time span methods
    QDateTime getSelectionEarliestTime() const;
    QDateTime getSelectionLatestTime() const;
    qint64 getSelectionTimeSpanMs() const;
    bool isValidSelectionTime(const QDateTime& time) const;

    // Data title methods
    void setDataTitle(const QString &title) { dataTitle = title; }
    QString getDataTitle() const { return dataTitle; }

    // Multiple data series methods
    void addDataSeries(const QString &seriesLabel, const std::vector<qreal> &yData, const std::vector<QDateTime> &timestamps);
    void addDataPointToSeries(const QString &seriesLabel, qreal yValue, const QDateTime &timestamp);
    void addDataPointsToSeries(const QString &seriesLabel, const std::vector<qreal> &yValues, const std::vector<QDateTime> &timestamps);
    void clearDataSeries(const QString &seriesLabel);
    void clearAllDataSeries();
    
    // Data series access methods
    std::vector<std::pair<qreal, QDateTime>> getDataSeries(const QString &seriesLabel) const;
    std::vector<std::pair<qreal, QDateTime>> getDataSeriesWithinYExtents(const QString &seriesLabel, qreal yMin, qreal yMax) const;
    std::vector<std::pair<qreal, QDateTime>> getDataSeriesWithinTimeRange(const QString &seriesLabel, const QDateTime &startTime, const QDateTime &endTime) const;
    
    // Direct access to data series vectors
    const std::vector<qreal> &getYDataSeries(const QString &seriesLabel) const;
    const std::vector<QDateTime> &getTimestampsSeries(const QString &seriesLabel) const;
    
    // Data series utility methods
    size_t getDataSeriesSize(const QString &seriesLabel) const;
    bool isDataSeriesEmpty(const QString &seriesLabel) const;
    bool hasDataSeries(const QString &seriesLabel) const;
    std::vector<QString> getDataSeriesLabels() const;
    
    // Data series range methods
    std::pair<qreal, qreal> getYRangeSeries(const QString &seriesLabel) const;
    std::pair<QDateTime, QDateTime> getTimeRangeSeries(const QString &seriesLabel) const;
    
    // Combined range methods for all series
    std::pair<qreal, qreal> getCombinedYRange() const;
    std::pair<QDateTime, QDateTime> getCombinedTimeRange() const;

private:
    // Legacy single series data (for backward compatibility)
    std::vector<qreal> yData;
    std::vector<QDateTime> timestamps;

    // Multiple data series storage
    std::map<QString, std::vector<qreal>> dataSeriesYData;
    std::map<QString, std::vector<QDateTime>> dataSeriesTimestamps;

    // Data title
    QString dataTitle;

    // Helper methods
    bool isValidIndex(size_t index) const;
    void validateDataConsistency() const;
    void validateDataSeriesConsistency(const QString &seriesLabel) const;
};

#endif // WATERFALLDATA_H
