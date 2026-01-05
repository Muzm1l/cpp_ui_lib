#ifndef GRAPHENGINE_H
#define GRAPHENGINE_H

#include "waterfalldata.h"
#include "graphtype.h"
#include <QObject>
#include <QDateTime>
#include <vector>
#include <QString>

/**
 * @brief GraphEngine - Non-UI persistent state manager for graph data
 * 
 * Acts as single source of truth for a graph type. Owns WaterfallData
 * and emits semantic change signals. Does NOT use any QGraphics* or
 * QWidget classes.
 * 
 * This follows Qt game engine architecture: separates simulation state
 * (engine) from rendering state (view).
 */
class GraphEngine : public QObject
{
    Q_OBJECT

public:
    explicit GraphEngine(GraphType graphType, QObject *parent = nullptr);
    explicit GraphEngine(GraphType graphType, const std::vector<QString>& seriesLabels, QObject *parent = nullptr);
    ~GraphEngine();

    // Read-only data access (for views)
    const WaterfallData& data() const;
    
    // Mutable access (for GraphLayout backward compatibility)
    WaterfallData* dataMutable();
    
    GraphType getGraphType() const;

    // Data modification methods (delegate to WaterfallData)
    void addDataPoint(const QString &seriesLabel, qreal yValue, const QDateTime &timestamp);
    void addDataPoints(const QString &seriesLabel, const std::vector<qreal> &yValues, const std::vector<QDateTime> &timestamps);
    void setDataSeries(const QString &seriesLabel, const std::vector<qreal> &yData, const std::vector<QDateTime> &timestamps);
    void clearDataSeries(const QString &seriesLabel);
    void clearAllDataSeries();
    
    // Symbol management (delegate to WaterfallData)
    void addRTWSymbol(const QString &symbolName, const QDateTime &timestamp, qreal range);
    void addBTWSymbol(const QString &symbolName, const QDateTime &timestamp, qreal range);
    void clearRTWSymbols();
    void clearBTWSymbols();
    bool removeRTWSymbol(const QString &symbolName, const QDateTime &timestamp, qreal range, qreal toleranceMs = 1000, qreal rangeTolerance = 0.1);
    std::vector<RTWSymbolData> getRTWSymbols() const;
    std::vector<BTWSymbolData> getBTWSymbols() const;
    size_t getRTWSymbolsCount() const;
    size_t getBTWSymbolsCount() const;
    
    // Marker management (delegate to WaterfallData)
    void addBTWMarker(const QDateTime &timestamp, qreal range, qreal delta);
    void addRTWRMarker(const QDateTime &timestamp, qreal range);
    void clearBTWMarkers();
    void clearRTWRMarkers();
    bool removeBTWMarker(const QDateTime &timestamp, qreal range, qreal toleranceMs = 1000, qreal rangeTolerance = 0.1);
    bool removeRTWRMarker(const QDateTime &timestamp, qreal range, qreal toleranceMs = 1000, qreal rangeTolerance = 0.1);
    std::vector<BTWMarkerData> getBTWMarkers() const;
    std::vector<RTWRMarkerData> getRTWRMarkers() const;
    
    // Data access methods (delegate to WaterfallData)
    bool hasDataSeries(const QString &seriesLabel) const;
    std::vector<QString> getDataSeriesLabels() const;
    size_t getDataSeriesSize(const QString &seriesLabel) const;
    bool isEmpty() const;
    std::pair<qreal, qreal> getYRangeSeries(const QString &seriesLabel) const;
    std::pair<QDateTime, QDateTime> getTimeRangeSeries(const QString &seriesLabel) const;
    bool findClosestDataPoint(const QString &seriesLabel, const QDateTime &targetTime, qint64 toleranceMs, qreal& outValue, size_t& outIndex) const;
    std::vector<qreal> getYDataSeries(const QString &seriesLabel) const;
    std::vector<QDateTime> getTimestampsSeries(const QString &seriesLabel) const;

signals:
    void dataAppended(const QString &seriesLabel);
    void dataRangeChanged();
    void symbolsChanged();
    void markersChanged();

private:
    GraphType m_graphType;
    WaterfallData m_data; // Owned by engine, not view
};

#endif // GRAPHENGINE_H

