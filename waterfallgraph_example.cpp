#include "waterfallgraph_example.h"
#include <QDebug>
#include <QGraphicsEllipseItem>
#include <QPen>
#include <QBrush>

WaterfallGraphExample::WaterfallGraphExample(QWidget *parent)
    : waterfallgraph(parent)
{
    // Example: Set some test data
    std::vector<double> xData = {1.0, 2.0, 3.0, 4.0, 5.0};
    std::vector<double> yData = {10.0, 20.0, 30.0, 40.0, 50.0};
    setData(xData, yData);
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
