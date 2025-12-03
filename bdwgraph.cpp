#include "bdwgraph.h"
#include <QDebug>

/**
 * @brief Construct a new BDWGraph::BDWGraph object
 *
 * @param parent Parent widget
 * @param enableGrid Whether to enable grid display
 * @param gridDivisions Number of grid divisions
 * @param timeInterval Time interval for the waterfall display
 */
BDWGraph::BDWGraph(QWidget *parent, bool enableGrid, int gridDivisions, TimeInterval timeInterval)
    : WaterfallGraph(parent, enableGrid, gridDivisions, timeInterval)
{
    qDebug() << "BDWGraph constructor called";
}

/**
 * @brief Destroy the BDWGraph::BDWGraph object
 *
 */
BDWGraph::~BDWGraph()
{
    qDebug() << "BDWGraph destructor called";
}

/**
 * @brief Override draw method to create scatterplots by default
 *
 */
void BDWGraph::draw()
{
    if (!graphicsScene)
        return;
    
    // Prevent concurrent drawing to avoid marker duplication
    if (isDrawing) {
        qDebug() << "BDWGraph: draw() already in progress, skipping";
        return;
    }
    
    isDrawing = true;

    graphicsScene->clear();
    graphicsScene->update(); // Force immediate update to ensure clearing is visible
    setupDrawingArea();

    if (gridEnabled)
    {
        drawGrid();
    }

    // Draw dashed grey vertical axis at 0 value
    drawZeroAxis();
    
    if (dataSource && !dataSource->isEmpty())
    {
        updateDataRanges();
        
        // Draw scatterplots for each series with their respective colors
        std::vector<QString> seriesLabels = dataSource->getDataSeriesLabels();
        for (const QString &seriesLabel : seriesLabels)
        {
            if (isSeriesVisible(seriesLabel))
            {
                QColor seriesColor = getSeriesColor(seriesLabel);
                
                if (seriesLabel == "ADOPTED")
                {
                    // Draw curve for ADOPTED series without points
                    drawDataLine(seriesLabel, false);
                }
                else
                {
                    // Draw scatterplot for other series
                    drawScatterplot(seriesLabel, seriesColor, 3.0, Qt::black);
                }
            }
        }
    }
    
    // Draw BTW symbols (magenta circles) if any exist in data source
    drawBTWSymbols();
    
    isDrawing = false;
}

/**
 * @brief Handle mouse click events specific to BDW graph
 *
 * @param scenePos Scene position of the click
 */
void BDWGraph::onMouseClick(const QPointF &scenePos)
{
    qDebug() << "BDWGraph mouse clicked at scene position:" << scenePos;
    // Call parent implementation
    WaterfallGraph::onMouseClick(scenePos);
}

/**
 * @brief Handle mouse drag events specific to BDW graph
 *
 * @param scenePos Scene position of the drag
 */
void BDWGraph::onMouseDrag(const QPointF &scenePos)
{
    qDebug() << "BDWGraph mouse dragged to scene position:" << scenePos;
    // Call parent implementation
    WaterfallGraph::onMouseDrag(scenePos);
}

/**
 * @brief Draw BDW-specific scatterplot
 *
 */
void BDWGraph::drawBDWScatterplot()
{
    // By default, create a scatterplot using the parent's scatterplot functionality
    // TODO: Change
    drawScatterplot(QString("BDW-1"), Qt::magenta, 4.0, Qt::white);

    qDebug() << "BDW scatterplot drawn";
}

/**
 * @brief Draw dashed grey vertical axis at 0 value
 *
 */
void BDWGraph::drawZeroAxis()
{
    if (!graphicsScene) {
        return;
    }

    // Map zero axis value (zoom panel middle sticker value) to screen coordinates using current time as timestamp
    QDateTime currentTime = QDateTime::currentDateTime();
    QPointF zeroPoint = mapDataToScreen(m_zeroAxisValue, currentTime);
    
    // Create vertical line from top to bottom of drawing area at x = 0
    QPointF topPoint(zeroPoint.x(), drawingArea.top());
    QPointF bottomPoint(zeroPoint.x(), drawingArea.bottom());
    
    // Create dashed white pen with more spacing
    QPen zeroAxisPen(QColor(255, 255, 255), 1.0, Qt::DashLine); // White dashed line
    zeroAxisPen.setDashPattern({8, 4}); // Custom dash pattern: 8px dash, 4px gap
    
    // Draw the vertical line
    graphicsScene->addLine(QLineF(topPoint, bottomPoint), zeroAxisPen);
    
    qDebug() << "BDW zero axis drawn at x:" << zeroPoint.x();
}

/**
 * @brief Override drawDataLine to use dashed lines for BDW graph
 *
 */
void BDWGraph::drawDataLine(const QString &seriesLabel, bool plotPoints)
{
    if (!graphicsScene || !dataSource || dataSource->isEmpty() || !dataRangesValid)
    {
        return;
    }

    const auto &yData = dataSource->getYDataSeries(seriesLabel);
    const auto &timestamps = dataSource->getTimestampsSeries(seriesLabel);

    // Filter data points to only include those within the current time range
    std::vector<std::pair<qreal, QDateTime>> visibleData;
    for (size_t i = 0; i < yData.size(); ++i)
    {
        if (timestamps[i] >= timeMin && timestamps[i] <= timeMax)
        {
            visibleData.push_back({yData[i], timestamps[i]});
        }
    }

    if (visibleData.empty())
    {
        qDebug() << "No data points within current time range";
        return;
    }

    if (visibleData.size() < 2)
    {
        // Draw a single point if we only have one data point
        QPointF screenPoint = mapDataToScreen(visibleData[0].first, visibleData[0].second);
        QPen pointPen(Qt::green, 0); // No stroke (width 0)
        graphicsScene->addEllipse(screenPoint.x() - 2, screenPoint.y() - 2, 4, 4, pointPen);
        qDebug() << "Data line drawn with 1 visible point";
        return;
    }

    // Create a path for the line
    QPainterPath path;
    QPointF firstPoint = mapDataToScreen(visibleData[0].first, visibleData[0].second);
    path.moveTo(firstPoint);

    // Add lines connecting all visible data points
    for (size_t i = 1; i < visibleData.size(); ++i)
    {
        QPointF point = mapDataToScreen(visibleData[i].first, visibleData[i].second);
        path.lineTo(point);
    }

    // Draw the line with dashed style
    QColor seriesColor = getSeriesColor(seriesLabel);
    QPen linePen(seriesColor, 2);
    linePen.setStyle(Qt::DashLine);
    linePen.setDashPattern({8, 4}); // Custom dash pattern: 8px dash, 4px gap
    graphicsScene->addPath(path, linePen);

    // Draw data points if enabled
    if (plotPoints)
    {
        // Draw data points
        QPen pointPen(seriesColor, 0); // No stroke (width 0)
        for (size_t i = 0; i < visibleData.size(); ++i)
        {
            QPointF point = mapDataToScreen(visibleData[i].first, visibleData[i].second);
            graphicsScene->addEllipse(point.x() - 1, point.y() - 1, 2, 2, pointPen);
        }
    }

    qDebug() << "BDW data line drawn (dashed) for series" << seriesLabel << "with" << visibleData.size() << "visible points";
}
