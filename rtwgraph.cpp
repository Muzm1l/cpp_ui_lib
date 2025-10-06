#include "rtwgraph.h"
#include <QDebug>
#include <QGraphicsTextItem>
#include <QFont>
#include <QTime>
#include <QtMath>

/**
 * @brief Construct a new RTWGraph::RTWGraph object
 *
 * @param parent Parent widget
 * @param enableGrid Whether to enable grid display
 * @param gridDivisions Number of grid divisions
 * @param timeInterval Time interval for the waterfall display
 */
RTWGraph::RTWGraph(QWidget *parent, bool enableGrid, int gridDivisions, TimeInterval timeInterval)
    : WaterfallGraph(parent, enableGrid, gridDivisions, timeInterval)
{
    // Set hard limits for RTW graph: 0 to 25
    setCustomYRange(0.0, 25.0);
    setRangeLimitingEnabled(true);
    
    qDebug() << "RTWGraph constructor called with hard limits 0-25";
}

/**
 * @brief Destroy the RTWGraph::RTWGraph object
 *
 */
RTWGraph::~RTWGraph()
{
    qDebug() << "RTWGraph destructor called";
}

/**
 * @brief Override draw method to create scatterplots by default
 *
 */
void RTWGraph::draw()
{
    qDebug() << "RTW: draw() called";
    
    if (!graphicsScene) {
        qDebug() << "RTW: draw() early return - no graphicsScene";
        return;
    }

    graphicsScene->clear();
    setupDrawingArea();

    if (gridEnabled)
    {
        drawGrid();
    }

    if (dataSource && !dataSource->isEmpty())
    {
        qDebug() << "RTW: draw() - dataSource available, updating ranges and drawing series";
        updateDataRanges();
        
        // Debug: Show current Y range
        qDebug() << "RTW: Current Y range:" << yMin << "to" << yMax;
        
        // Debug: Show data source info
        qDebug() << "RTW: Data source title:" << dataSource->getDataTitle();
        qDebug() << "RTW: Data source empty?" << dataSource->isEmpty();
        
        // RTW should only have 1 series - get the first (and only) series
        std::vector<QString> seriesLabels = dataSource->getDataSeriesLabels();
        qDebug() << "RTW: draw() - found" << seriesLabels.size() << "series labels";
        
        // Debug: Show all series labels
        for (const QString& label : seriesLabels) {
            qDebug() << "RTW: Series label:" << label << "size:" << dataSource->getDataSeriesSize(label);
        }
        
        // Draw all series - ADOPTED as line, others as R markers
        for (const QString &seriesLabel : seriesLabels)
        {
            if (isSeriesVisible(seriesLabel))
            {
                if (seriesLabel == "ADOPTED")
                {
                    // Draw ADOPTED series as line
                    qDebug() << "RTW: draw() - drawing ADOPTED series as line";
                    drawDataLine(seriesLabel, false);
                }
                else
                {
                    // Draw custom yellow "R" markers with 1/5 interval sampling for other series
                    qDebug() << "RTW: draw() - drawing custom R markers for series:" << seriesLabel;
                    drawCustomRMarkers(seriesLabel);
                }
            }
        }
    }
    else
    {
        qDebug() << "RTW: draw() - no dataSource or dataSource is empty";
    }
}

/**
 * @brief Handle mouse click events specific to RTW graph
 *
 * @param scenePos Scene position of the click
 */
void RTWGraph::onMouseClick(const QPointF &scenePos)
{
    qDebug() << "RTWGraph mouse clicked at scene position:" << scenePos;
    // Call parent implementation
    WaterfallGraph::onMouseClick(scenePos);
}

/**
 * @brief Handle mouse drag events specific to RTW graph
 *
 * @param scenePos Scene position of the drag
 */
void RTWGraph::onMouseDrag(const QPointF &scenePos)
{
    qDebug() << "RTWGraph mouse dragged to scene position:" << scenePos;
    // Call parent implementation
    WaterfallGraph::onMouseDrag(scenePos);
}

/**
 * @brief Draw custom yellow "R" markers for RTW graph with 1/5 interval sampling
 *
 * @param seriesLabel The series label to draw markers for
 */
void RTWGraph::drawCustomRMarkers(const QString &seriesLabel)
{
    if (!dataSource || !graphicsScene) {
        qDebug() << "RTW: drawCustomRMarkers early return - no dataSource or graphicsScene";
        return;
    }

    // Get total data size for comparison
    size_t totalDataSize = dataSource->getDataSeriesSize(seriesLabel);
    qDebug() << "RTW: drawCustomRMarkers called for series" << seriesLabel << "with total data size:" << totalDataSize;

    if (totalDataSize == 0) {
        qDebug() << "RTW: No data available for series" << seriesLabel;
        return;
    }

    // Use the static binning method to sample data
    qint64 samplingIntervalMs = 300000; // 3 seconds

    // Convert to QTime for the binning method
    QTime binDuration = QTime(0, 0, 0).addMSecs(samplingIntervalMs);
    
    // Get raw data and use static binning method
    const std::vector<qreal>& yData = dataSource->getYDataSeries(seriesLabel);
    const std::vector<QDateTime>& timestamps = dataSource->getTimestampsSeries(seriesLabel);
    
    // Debug: Show sample raw data
    if (!yData.empty()) {
        qDebug() << "RTW: Raw data sample - First value:" << yData[0] << "Last value:" << yData.back();
        qDebug() << "RTW: Raw data range:" << *std::min_element(yData.begin(), yData.end()) << "to" << *std::max_element(yData.begin(), yData.end());
    }
    
    std::vector<std::pair<qreal, QDateTime>> binnedData = WaterfallData::binDataByTime(yData, timestamps, binDuration);
    
    // Filter binned data to only include points within the visible time range
    std::vector<std::pair<qreal, QDateTime>> visibleBinnedData;
    for (const auto& point : binnedData) {
        if (point.second >= timeMin && point.second <= timeMax) {
            visibleBinnedData.push_back(point);
        }
    }
    
    qDebug() << "RTW: Time range filtering - Total binned:" << binnedData.size() 
             << "- Visible binned:" << visibleBinnedData.size()
             << "- Time range:" << timeMin.toString() << "to" << timeMax.toString();

    qDebug() << "RTW: Binning completed for series" << seriesLabel 
             << "- Total data:" << totalDataSize 
             << "- Binned data:" << binnedData.size()
             << "- Visible binned data:" << visibleBinnedData.size()
             << "- Sampling interval:" << samplingIntervalMs << "ms)";

    if (visibleBinnedData.empty()) {
        qDebug() << "RTW: No visible binned data available for series" << seriesLabel;
        qDebug() << "RTW: Trying fallback - drawing markers for raw data";
        
        // Fallback: draw markers for raw data if binning produces no visible results
        int fallbackMarkersDrawn = 0;
        for (size_t i = 0; i < yData.size() && i < 10; ++i) { // Limit to first 10 points
            qreal yValue = yData[i];
            QDateTime timestamp = timestamps[i];
            QPointF screenPos = mapDataToScreen(yValue, timestamp);
            
            if (drawingArea.contains(screenPos)) {
                QSize windowSize = this->size();
                qreal markerSize = std::min(0.08 * windowSize.width(), 24.0);
                
                QGraphicsTextItem *rMarker = new QGraphicsTextItem("R");
                QFont font = rMarker->font();
                font.setPointSizeF(markerSize);
                font.setBold(true);
                rMarker->setFont(font);
                rMarker->setDefaultTextColor(Qt::red); // Use red for fallback markers
                
                QRectF textRect = rMarker->boundingRect();
                rMarker->setPos(screenPos.x() - textRect.width()/2, screenPos.y() - textRect.height()/2);
                rMarker->setZValue(1000);
                
                graphicsScene->addItem(rMarker);
                fallbackMarkersDrawn++;
            }
        }
        qDebug() << "RTW: Fallback drew" << fallbackMarkersDrawn << "red R markers";
        return;
    }

    // Draw yellow "R" markers for each visible binned point
    int markersDrawn = 0;
    qDebug() << "RTW: Drawing area:" << drawingArea;
    for (const auto& point : visibleBinnedData) {
        qreal yValue = point.first;
        QDateTime timestamp = point.second;
        QPointF screenPos = mapDataToScreen(yValue, timestamp);
        
        // Only debug first few points to avoid spam
        if (markersDrawn < 3) {
            qDebug() << "RTW: Point" << markersDrawn << "- Y:" << yValue << "Time:" << timestamp.toString() << "Screen:" << screenPos << "In area:" << drawingArea.contains(screenPos);
        }
        
        // Check if point is within visible area
        if (drawingArea.contains(screenPos)) {
            // Calculate marker size based on window size - make it larger
            QSize windowSize = this->size();
            qreal markerSize = std::min(0.08 * windowSize.width(), 24.0); // Increased size, cap at 24 pixels
            
            // Create yellow "R" text marker
            QGraphicsTextItem *rMarker = new QGraphicsTextItem("R");
            QFont font = rMarker->font();
            font.setPointSizeF(markerSize);
            font.setBold(true);
            rMarker->setFont(font);
            rMarker->setDefaultTextColor(Qt::yellow);
            
            // Center the marker on the data point
            QRectF textRect = rMarker->boundingRect();
            rMarker->setPos(screenPos.x() - textRect.width()/2, screenPos.y() - textRect.height()/2);
            rMarker->setZValue(1000); // Very high z-value to ensure visibility
            
            graphicsScene->addItem(rMarker);
            markersDrawn++;
        }
    }
    
    qDebug() << "RTW: Successfully drew" << markersDrawn << "yellow R markers for series" << seriesLabel;
}

/**
 * @brief Draw RTW-specific scatterplot
 *
 */
void RTWGraph::drawRTWScatterplot()
{
    // By default, create a scatterplot using the parent's scatterplot functionality
    drawScatterplot(QString("RTW-1"), Qt::blue, 4.0, Qt::white);

    qDebug() << "RTW scatterplot drawn";
}
