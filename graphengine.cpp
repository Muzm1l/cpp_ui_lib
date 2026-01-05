#include "graphengine.h"
#include "graphtype.h"
#include "debugutils.h"

GraphEngine::GraphEngine(GraphType graphType, QObject *parent)
    : QObject(parent), m_graphType(graphType), m_data(graphTypeToString(graphType))
{
    DEBUG_OUT() << "GraphEngine: Created for graph type" << static_cast<int>(graphType);
}

GraphEngine::GraphEngine(GraphType graphType, const std::vector<QString>& seriesLabels, QObject *parent)
    : QObject(parent), m_graphType(graphType), m_data(graphTypeToString(graphType), seriesLabels)
{
    DEBUG_OUT() << "GraphEngine: Created for graph type" << static_cast<int>(graphType) << "with" << seriesLabels.size() << "series";
}

GraphEngine::~GraphEngine()
{
    DEBUG_OUT() << "GraphEngine: Destroyed for graph type" << static_cast<int>(m_graphType);
}

const WaterfallData& GraphEngine::data() const
{
    return m_data;
}

WaterfallData* GraphEngine::dataMutable()
{
    return &m_data;
}

GraphType GraphEngine::getGraphType() const
{
    return m_graphType;
}

// Data modification methods
void GraphEngine::addDataPoint(const QString &seriesLabel, qreal yValue, const QDateTime &timestamp)
{
    m_data.addDataPointToSeries(seriesLabel, yValue, timestamp);
    emit dataAppended(seriesLabel);
    emit dataRangeChanged();
}

void GraphEngine::addDataPoints(const QString &seriesLabel, const std::vector<qreal> &yValues, const std::vector<QDateTime> &timestamps)
{
    m_data.addDataPointsToSeries(seriesLabel, yValues, timestamps);
    emit dataAppended(seriesLabel);
    emit dataRangeChanged();
}

void GraphEngine::setDataSeries(const QString &seriesLabel, const std::vector<qreal> &yData, const std::vector<QDateTime> &timestamps)
{
    m_data.setDataSeries(seriesLabel, yData, timestamps);
    emit dataAppended(seriesLabel);
    emit dataRangeChanged();
}

void GraphEngine::clearDataSeries(const QString &seriesLabel)
{
    m_data.clearDataSeries(seriesLabel);
    emit dataRangeChanged();
}

void GraphEngine::clearAllDataSeries()
{
    m_data.clearAllDataSeries();
    emit dataRangeChanged();
}

// Symbol management
void GraphEngine::addRTWSymbol(const QString &symbolName, const QDateTime &timestamp, qreal range)
{
    m_data.addRTWSymbol(symbolName, timestamp, range);
    emit symbolsChanged();
}

void GraphEngine::addBTWSymbol(const QString &symbolName, const QDateTime &timestamp, qreal range)
{
    m_data.addBTWSymbol(symbolName, timestamp, range);
    emit symbolsChanged();
}

void GraphEngine::clearRTWSymbols()
{
    m_data.clearRTWSymbols();
    emit symbolsChanged();
}

void GraphEngine::clearBTWSymbols()
{
    m_data.clearBTWSymbols();
    emit symbolsChanged();
}

bool GraphEngine::removeRTWSymbol(const QString &symbolName, const QDateTime &timestamp, qreal range, qreal toleranceMs, qreal rangeTolerance)
{
    bool removed = m_data.removeRTWSymbol(symbolName, timestamp, range, toleranceMs, rangeTolerance);
    if (removed) {
        emit symbolsChanged();
    }
    return removed;
}

std::vector<RTWSymbolData> GraphEngine::getRTWSymbols() const
{
    return m_data.getRTWSymbols();
}

std::vector<BTWSymbolData> GraphEngine::getBTWSymbols() const
{
    return m_data.getBTWSymbols();
}

size_t GraphEngine::getRTWSymbolsCount() const
{
    return m_data.getRTWSymbolsCount();
}

size_t GraphEngine::getBTWSymbolsCount() const
{
    return m_data.getBTWSymbolsCount();
}

// Marker management
void GraphEngine::addBTWMarker(const QDateTime &timestamp, qreal range, qreal delta)
{
    m_data.addBTWMarker(timestamp, range, delta);
    emit markersChanged();
}

void GraphEngine::addRTWRMarker(const QDateTime &timestamp, qreal range)
{
    m_data.addRTWRMarker(timestamp, range);
    emit markersChanged();
}

void GraphEngine::clearBTWMarkers()
{
    m_data.clearBTWMarkers();
    emit markersChanged();
}

void GraphEngine::clearRTWRMarkers()
{
    m_data.clearRTWRMarkers();
    emit markersChanged();
}

bool GraphEngine::removeBTWMarker(const QDateTime &timestamp, qreal range, qreal toleranceMs, qreal rangeTolerance)
{
    bool removed = m_data.removeBTWMarker(timestamp, range, toleranceMs, rangeTolerance);
    if (removed) {
        emit markersChanged();
    }
    return removed;
}

bool GraphEngine::removeRTWRMarker(const QDateTime &timestamp, qreal range, qreal toleranceMs, qreal rangeTolerance)
{
    bool removed = m_data.removeRTWRMarker(timestamp, range, toleranceMs, rangeTolerance);
    if (removed) {
        emit markersChanged();
    }
    return removed;
}

std::vector<BTWMarkerData> GraphEngine::getBTWMarkers() const
{
    return m_data.getBTWMarkers();
}

std::vector<RTWRMarkerData> GraphEngine::getRTWRMarkers() const
{
    return m_data.getRTWRMarkers();
}

// Data access methods
bool GraphEngine::hasDataSeries(const QString &seriesLabel) const
{
    return m_data.hasDataSeries(seriesLabel);
}

std::vector<QString> GraphEngine::getDataSeriesLabels() const
{
    return m_data.getDataSeriesLabels();
}

size_t GraphEngine::getDataSeriesSize(const QString &seriesLabel) const
{
    return m_data.getDataSeriesSize(seriesLabel);
}

bool GraphEngine::isEmpty() const
{
    return m_data.isEmpty();
}

std::pair<qreal, qreal> GraphEngine::getYRangeSeries(const QString &seriesLabel) const
{
    return m_data.getYRangeSeries(seriesLabel);
}

std::pair<QDateTime, QDateTime> GraphEngine::getTimeRangeSeries(const QString &seriesLabel) const
{
    return m_data.getTimeRangeSeries(seriesLabel);
}

bool GraphEngine::findClosestDataPoint(const QString &seriesLabel, const QDateTime &targetTime, qint64 toleranceMs, qreal& outValue, size_t& outIndex) const
{
    return m_data.findClosestDataPoint(seriesLabel, targetTime, toleranceMs, outValue, outIndex);
}

std::vector<qreal> GraphEngine::getYDataSeries(const QString &seriesLabel) const
{
    return m_data.getYDataSeries(seriesLabel);
}

std::vector<QDateTime> GraphEngine::getTimestampsSeries(const QString &seriesLabel) const
{
    return m_data.getTimestampsSeries(seriesLabel);
}

