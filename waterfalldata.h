#ifndef WATERFALLDATA_H
#define WATERFALLDATA_H

#include <vector>
#include <utility>
#include <map>
#include <QDateTime>
#include <QTime>
#include <QDebug>
#include <QString>

// Forward declaration for RTW symbols
struct RTWSymbolData
{
    QString symbolName;
    QDateTime timestamp;
    qreal range;
};

// Forward declaration for BTW symbols
struct BTWSymbolData
{
    QString symbolName;
    QDateTime timestamp;
    qreal range;
};

class WaterfallData
{
public:
    WaterfallData(const QString& title = "");
    WaterfallData(const QString& title, const std::vector<QString>& seriesLabels);
    ~WaterfallData();

    // Data management methods
    void setData(const std::vector<qreal>& yData, const std::vector<QDateTime>& timestamps);
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
    void addDataSeries(const QString& seriesLabel, const std::vector<qreal>& yData, const std::vector<QDateTime>& timestamps);
    void addDataPointToSeries(const QString& seriesLabel, qreal yValue, const QDateTime& timestamp);
    void addDataPointsToSeries(const QString& seriesLabel, const std::vector<qreal>& yValues, const std::vector<QDateTime>& timestamps);
    void clearDataSeries(const QString& seriesLabel);
    void clearAllDataSeries();

    // Data series access methods
    std::vector<std::pair<qreal, QDateTime>> getDataSeries(const QString& seriesLabel) const;
    std::vector<std::pair<qreal, QDateTime>> getDataSeriesWithinYExtents(const QString& seriesLabel, qreal yMin, qreal yMax) const;
    std::vector<std::pair<qreal, QDateTime>> getDataSeriesWithinTimeRange(const QString& seriesLabel, const QDateTime& startTime, const QDateTime& endTime) const;

    // Direct access to data series vectors
    const std::vector<qreal>& getYDataSeries(const QString& seriesLabel) const;
    const std::vector<QDateTime>& getTimestampsSeries(const QString& seriesLabel) const;

    // Data series utility methods
    size_t getDataSeriesSize(const QString& seriesLabel) const;
    bool isDataSeriesEmpty(const QString& seriesLabel) const;
    bool hasDataSeries(const QString& seriesLabel) const;
    std::vector<QString> getDataSeriesLabels() const;

    // Data series range methods
    std::pair<qreal, qreal> getYRangeSeries(const QString& seriesLabel) const;
    std::pair<QDateTime, QDateTime> getTimeRangeSeries(const QString& seriesLabel) const;

    // Series-specific versions of legacy methods
    void setDataSeries(const QString& seriesLabel, const std::vector<qreal>& yData, const std::vector<QDateTime>& timestamps);
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

    // Data binning methods for sampling
    std::vector<std::pair<qreal, QDateTime>> getBinnedDataSeries(const QString& seriesLabel, const QTime& binDuration) const;
    
    // Static binning method that doesn't depend on class state
    static std::vector<std::pair<qreal, QDateTime>> binDataByTime(
        const std::vector<qreal>& yData, 
        const std::vector<QDateTime>& timestamps, 
        const QTime& binDuration
    );

    // RTW Symbol management methods (stored with track data)
    void addRTWSymbol(const QString& symbolName, const QDateTime& timestamp, qreal range);
    void clearRTWSymbols();
    std::vector<RTWSymbolData> getRTWSymbols() const;
    size_t getRTWSymbolsCount() const;

    // BTW Symbol management methods (stored with track data)
    void addBTWSymbol(const QString& symbolName, const QDateTime& timestamp, qreal range);
    void clearBTWSymbols();
    std::vector<BTWSymbolData> getBTWSymbols() const;
    size_t getBTWSymbolsCount() const;

private:

    // Multiple data series storage
    std::map<QString, std::vector<qreal>> dataSeriesYData;
    std::map<QString, std::vector<QDateTime>> dataSeriesTimestamps;

    // RTW Symbol storage (persists with track data)
    std::vector<RTWSymbolData> rtwSymbols;
    
    // BTW Symbol storage (persists with track data)
    std::vector<BTWSymbolData> btwSymbols;

    // Data title
    QString dataTitle;

    // Helper methods
    bool isValidIndex(size_t index) const;
    void validateDataConsistency() const;
    void validateDataSeriesConsistency(const QString& seriesLabel) const;
};

#endif // WATERFALLDATA_H
