#include "waterfallgraph_example.h"
#include <QDebug>
#include <QGraphicsEllipseItem>
#include <QPen>
#include <QBrush>
#include <QDateTime>

WaterfallGraphExample::WaterfallGraphExample(QWidget *parent)
    : waterfallgraph(parent)
{
    // Example: Set some test data with more interesting values
    std::vector<qreal> yData = {10.0, 25.0, 15.0, 35.0, 20.0, 40.0, 30.0, 45.0, 50.0};
    std::vector<QDateTime> timestamps;
    
    // Create timestamps (current time + intervals)
    QDateTime baseTime = QDateTime::currentDateTime();
    for (int i = 0; i < 9; ++i) {
        timestamps.append(baseTime.addMSecs(i * 500)); // 0.5 second intervals
    }
    
    setData(yData, timestamps);
    
    // Example: Add a single data point incrementally
    addDataPoint(60.0, baseTime.addMSecs(4500));
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
