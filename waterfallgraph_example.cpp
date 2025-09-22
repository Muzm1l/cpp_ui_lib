#include "waterfallgraph_example.h"
#include <QDebug>
#include <QGraphicsEllipseItem>
#include <QPen>
#include <QBrush>
#include <QDateTime>
#include "timelineutils.h"

WaterfallGraphExample::WaterfallGraphExample(QWidget *parent)
    : waterfallgraph(parent, false, 10, TimeInterval::FiveMinutes) // Grid disabled by default
{
    // Enable grid for this example
    setGridEnabled(true);
    
    // Enable mouse selection for this example
    setMouseSelectionEnabled(true);
    
    // Example: Set some test data with more interesting values
    std::vector<qreal> yData = {10.0, 25.0, 15.0, 35.0, 20.0, 40.0, 30.0, 45.0, 50.0};
    std::vector<QDateTime> timestamps;
    
    // Create timestamps spanning the time interval (current time going backwards)
    QDateTime currentTime = QDateTime::currentDateTime();
    for (int i = 0; i < 9; ++i) {
        // Spread data points across the 5-minute interval
        timestamps.append(currentTime.addMSecs(-i * 30 * 1000)); // 30-second intervals going backwards
    }
    
    setData(yData, timestamps);
    
    // Example: Add a single data point incrementally (2 minutes ago)
    addDataPoint(60.0, currentTime.addMSecs(-2 * 60 * 1000));
}

void WaterfallGraphExample::onMouseClick(const QPointF& scenePos)
{
    qDebug() << "Example: Mouse clicked at scene position:" << scenePos;
    
    // Example: Add a small circle at the click position
    QGraphicsEllipseItem* circle = new QGraphicsEllipseItem(scenePos.x() - 5, scenePos.y() - 5, 10, 10);
    circle->setPen(QPen(Qt::blue, 2));
    circle->setBrush(QBrush(Qt::lightBlue));
    graphicsScene->addItem(circle);
}

void WaterfallGraphExample::onMouseDrag(const QPointF& scenePos)
{
    qDebug() << "Example: Mouse dragged to scene position:" << scenePos;
    
    // Example: Add a small square at the drag position
    QGraphicsEllipseItem* square = new QGraphicsEllipseItem(scenePos.x() - 3, scenePos.y() - 3, 6, 6);
    square->setPen(QPen(Qt::red, 1));
    square->setBrush(QBrush(Qt::lightGray));
    graphicsScene->addItem(square);
}
