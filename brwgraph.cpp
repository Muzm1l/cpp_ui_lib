#include "brwgraph.h"
#include <QDebug>

/**
 * @brief Construct a new BRWGraph::BRWGraph object
 *
 * @param parent Parent widget
 * @param enableGrid Whether to enable grid display
 * @param gridDivisions Number of grid divisions
 * @param timeInterval Time interval for the waterfall display
 */
BRWGraph::BRWGraph(QWidget *parent, bool enableGrid, int gridDivisions, TimeInterval timeInterval)
    : WaterfallGraph(parent, enableGrid, gridDivisions, timeInterval)
{
    qDebug() << "BRWGraph constructor called";
}

/**
 * @brief Destroy the BRWGraph::BRWGraph object
 *
 */
BRWGraph::~BRWGraph()
{
    qDebug() << "BRWGraph destructor called";
}

/**
 * @brief Override draw method to create scatterplots by default
 *
 */
void BRWGraph::draw()
{
    if (!graphicsScene)
        return;

    graphicsScene->clear();
    setupDrawingArea();

    if (gridEnabled)
    {
        drawGrid();
    }

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
}

/**
 * @brief Handle mouse click events specific to BRW graph
 *
 * @param scenePos Scene position of the click
 */
void BRWGraph::onMouseClick(const QPointF &scenePos)
{
    qDebug() << "BRWGraph mouse clicked at scene position:" << scenePos;
    // Call parent implementation
    WaterfallGraph::onMouseClick(scenePos);
}

/**
 * @brief Handle mouse drag events specific to BRW graph
 *
 * @param scenePos Scene position of the drag
 */
void BRWGraph::onMouseDrag(const QPointF &scenePos)
{
    qDebug() << "BRWGraph mouse dragged to scene position:" << scenePos;
    // Call parent implementation
    WaterfallGraph::onMouseDrag(scenePos);
}

/**
 * @brief Draw BRW-specific scatterplot
 *
 */
void BRWGraph::drawBRWScatterplot()
{
    // By default, create a scatterplot using the parent's scatterplot functionality
    drawScatterplot(QString("BRW-1"), Qt::yellow, 4.0, Qt::black);

    qDebug() << "BRW scatterplot drawn";
}

/**
 * @brief Override drawDataLine to use dashed lines for BRW graph
 *
 */
void BRWGraph::drawDataLine(const QString &seriesLabel, bool plotPoints)
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

    qDebug() << "BRW data line drawn (dashed) for series" << seriesLabel << "with" << visibleData.size() << "visible points";
}
