#ifndef WATERFALLDATA_H
#define WATERFALLDATA_H

#include <vector>
#include <utility>
#include <QDateTime>
#include <QDebug>

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

    // Data title methods
    void setDataTitle(const QString &title) { dataTitle = title; }
    QString getDataTitle() const { return dataTitle; }

private:
    std::vector<qreal> yData;
    std::vector<QDateTime> timestamps;

    // Data title
    QString dataTitle;

    // Helper methods
    bool isValidIndex(size_t index) const;
    void validateDataConsistency() const;
};

#endif // WATERFALLDATA_H
