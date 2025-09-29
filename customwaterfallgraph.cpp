#include "customwaterfallgraph.h"
#include <QDebug>
#include <QPainterPath>
#include <QGraphicsRectItem>
#include <QGraphicsPathItem>

CustomWaterfallGraph::CustomWaterfallGraph(QWidget *parent, bool enableGrid, int gridDivisions, TimeInterval timeInterval)
    : WaterfallGraph(parent, enableGrid, gridDivisions, timeInterval)
    , drawingStyle("default")
    , customDataColor(Qt::green)
    , customGridColor(Qt::white)
    , customBackgroundColor(Qt::black)
    , useCustomColors(false)
    , barWidth(8.0)
    , pointSize(4.0)
    , showDataPoints(true)
    , showDataBars(false)
    , showDataArea(false)
{
    qDebug() << "CustomWaterfallGraph constructor - Style:" << drawingStyle;
}

CustomWaterfallGraph::~CustomWaterfallGraph()
{
    qDebug() << "CustomWaterfallGraph destructor";
}

void CustomWaterfallGraph::draw()
{
    if (!graphicsScene)
        return;

    // Clear existing items
    graphicsScene->clear();

    // Update the drawing area
    setupDrawingArea();

    // Draw custom grid if enabled
    if (gridEnabled) {
        drawGrid();
    }

    // Draw the custom data visualization if we have data
    if (dataSource && !dataSource->isEmpty()) {
        updateDataRanges();
        drawDataLine();
    }
    
    qDebug() << "CustomWaterfallGraph::draw() completed with style:" << drawingStyle;
}

void CustomWaterfallGraph::drawDataLine()
{
    if (!graphicsScene || !dataSource || dataSource->isEmpty() || !dataRangesValid) {
        return;
    }
    
    const auto& yData = dataSource->getYData();
    const auto& timestamps = dataSource->getTimestamps();
    
    // Filter data points to only include those within the current time range
    std::vector<std::pair<qreal, QDateTime>> visibleData;
    for (size_t i = 0; i < yData.size(); ++i) {
        if (timestamps[i] >= timeMin && timestamps[i] <= timeMax) {
            visibleData.push_back({yData[i], timestamps[i]});
        }
    }
    
    if (visibleData.empty()) {
        qDebug() << "No data points within current time range for custom drawing";
        return;
    }
    
    // Choose drawing style based on current style setting
    if (drawingStyle == "area") {
        drawCustomDataArea();
    } else if (drawingStyle == "points") {
        drawCustomDataPoints();
    } else {
        // Default style - draw both line and points
        drawCustomDataPoints();
        drawCustomDataLine();
    }
    
    qDebug() << "CustomWaterfallGraph::drawDataLine() completed with" << visibleData.size() << "points";
}

void CustomWaterfallGraph::drawGrid()
{
    if (!graphicsScene || !gridEnabled || drawingArea.isEmpty() || gridDivisions <= 0) return;
    
    QColor gridColor = useCustomColors ? customGridColor : Qt::white;
    QPen gridPen(gridColor, 1, Qt::DashLine);
    
    // Draw vertical grid lines (for x-axis - variable)
    double stepX = drawingArea.width() / gridDivisions;
    for (int i = 0; i <= gridDivisions; ++i) {
        double x = drawingArea.left() + i * stepX;
        graphicsScene->addLine(x, drawingArea.top(), x, drawingArea.bottom(), gridPen);
    }
    
    // Draw horizontal grid lines (for y-axis - time)
    double stepY = drawingArea.height() / gridDivisions;
    for (int i = 0; i <= gridDivisions; ++i) {
        double y = drawingArea.top() + i * stepY;
        graphicsScene->addLine(drawingArea.left(), y, drawingArea.right(), y, gridPen);
    }
    
    // Draw border with custom color
    QColor borderColor = useCustomColors ? customGridColor : Qt::white;
    QPen borderPen(borderColor, 2);
    graphicsScene->addRect(drawingArea, borderPen);
    
    qDebug() << "CustomWaterfallGraph::drawGrid() completed with custom colors:" << useCustomColors;
}

void CustomWaterfallGraph::setDrawingStyle(const QString& style)
{
    if (drawingStyle != style) {
        drawingStyle = style;
        draw(); // Redraw with new style
        qDebug() << "Drawing style changed to:" << style;
    }
}

QString CustomWaterfallGraph::getDrawingStyle() const
{
    return drawingStyle;
}

void CustomWaterfallGraph::setCustomColors(const QColor& dataColor, const QColor& gridColor, const QColor& backgroundColor)
{
    customDataColor = dataColor;
    customGridColor = gridColor;
    customBackgroundColor = backgroundColor;
    useCustomColors = true;
    
    // Update background color
    QPalette pal = palette();
    pal.setColor(QPalette::Window, backgroundColor);
    setPalette(pal);
    setAutoFillBackground(true);
    
    // Update graphics scene background
    if (graphicsScene) {
        graphicsScene->setBackgroundBrush(QBrush(backgroundColor));
    }
    
    draw(); // Redraw with new colors
    qDebug() << "Custom colors set - Data:" << dataColor.name() << "Grid:" << gridColor.name() << "Background:" << backgroundColor.name();
}

void CustomWaterfallGraph::drawCustomDataArea()
{
    if (!dataSource || dataSource->isEmpty()) return;
    
    const auto& yData = dataSource->getYData();
    const auto& timestamps = dataSource->getTimestamps();
    
    // Filter visible data
    std::vector<std::pair<qreal, QDateTime>> visibleData;
    for (size_t i = 0; i < yData.size(); ++i) {
        if (timestamps[i] >= timeMin && timestamps[i] <= timeMax) {
            visibleData.push_back({yData[i], timestamps[i]});
        }
    }
    
    if (visibleData.size() < 2) return;
    
    QColor areaColor = useCustomColors ? customDataColor : QColor(0, 255, 0, 128);
    
    // Create area path
    QPainterPath areaPath;
    QPointF firstPoint = mapDataToScreen(visibleData[0].first, visibleData[0].second);
    areaPath.moveTo(firstPoint.x(), drawingArea.bottom()); // Start from bottom
    areaPath.lineTo(firstPoint); // Go to first data point
    
    // Add lines connecting all visible data points
    for (size_t i = 1; i < visibleData.size(); ++i) {
        QPointF point = mapDataToScreen(visibleData[i].first, visibleData[i].second);
        areaPath.lineTo(point);
    }
    
    // Close the area by going back to bottom
    QPointF lastPoint = mapDataToScreen(visibleData.back().first, visibleData.back().second);
    areaPath.lineTo(lastPoint.x(), drawingArea.bottom());
    areaPath.closeSubpath();
    
    // Draw the area
    QGraphicsPathItem* areaItem = new QGraphicsPathItem(areaPath);
    areaItem->setPen(QPen(areaColor, 2));
    areaItem->setBrush(QBrush(areaColor));
    areaItem->setZValue(90);
    
    graphicsScene->addItem(areaItem);
    
    qDebug() << "Custom data area drawn with" << visibleData.size() << "points";
}

void CustomWaterfallGraph::drawCustomDataPoints()
{
    if (!dataSource || dataSource->isEmpty()) return;
    
    const auto& yData = dataSource->getYData();
    const auto& timestamps = dataSource->getTimestamps();
    
    QColor pointColor = useCustomColors ? customDataColor : Qt::yellow;
    
    // Draw data points as circles
    for (size_t i = 0; i < yData.size(); ++i) {
        if (timestamps[i] >= timeMin && timestamps[i] <= timeMax) {
            QPointF screenPoint = mapDataToScreen(yData[i], timestamps[i]);
            
            QGraphicsEllipseItem* point = new QGraphicsEllipseItem();
            point->setRect(screenPoint.x() - pointSize/2, screenPoint.y() - pointSize/2, pointSize, pointSize);
            point->setPen(QPen(pointColor, 2));
            point->setBrush(QBrush(pointColor));
            point->setZValue(110);
            
            graphicsScene->addItem(point);
        }
    }
    
    qDebug() << "Custom data points drawn with" << yData.size() << "points";
}

void CustomWaterfallGraph::drawCustomDataLine()
{
    if (!dataSource || dataSource->isEmpty()) return;
    
    const auto& yData = dataSource->getYData();
    const auto& timestamps = dataSource->getTimestamps();
    
    // Filter visible data
    std::vector<std::pair<qreal, QDateTime>> visibleData;
    for (size_t i = 0; i < yData.size(); ++i) {
        if (timestamps[i] >= timeMin && timestamps[i] <= timeMax) {
            visibleData.push_back({yData[i], timestamps[i]});
        }
    }
    
    if (visibleData.size() < 2) return;
    
    QColor lineColor = useCustomColors ? customDataColor : Qt::green;
    
    // Create line path
    QPainterPath linePath;
    QPointF firstPoint = mapDataToScreen(visibleData[0].first, visibleData[0].second);
    linePath.moveTo(firstPoint);
    
    // Add lines connecting all visible data points
    for (size_t i = 1; i < visibleData.size(); ++i) {
        QPointF point = mapDataToScreen(visibleData[i].first, visibleData[i].second);
        linePath.lineTo(point);
    }
    
    // Draw the line
    QGraphicsPathItem* lineItem = new QGraphicsPathItem(linePath);
    lineItem->setPen(QPen(lineColor, 3));
    lineItem->setZValue(100);
    
    graphicsScene->addItem(lineItem);
    
    qDebug() << "Custom data line drawn with" << visibleData.size() << "points";
}
