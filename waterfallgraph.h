#ifndef WATERFALLGRAPH_H
#define WATERFALLGRAPH_H

#include <QWidget>
#include <QGraphicsScene>
#include <QMouseEvent>
#include <QGraphicsSceneMouseEvent>
#include <QResizeEvent>
#include <QShowEvent>
#include <QPainter>
#include <QPalette>
#include <vector>
#include "drawutils.h"

namespace Ui {
class waterfallgraph;
}

class waterfallgraph : public QWidget
{
    Q_OBJECT

public:
    explicit waterfallgraph(QWidget *parent = nullptr, bool enableGrid = true, int gridDivisions = 10);
    ~waterfallgraph();
    
    // Data handling
    void setData(const std::vector<double>& xData, const std::vector<double>& yData);
    
    // Mouse event handlers (virtual so they can be overridden in derived classes)
    virtual void onMouseClick(const QPointF& scenePos);
    virtual void onMouseDrag(const QPointF& scenePos);

protected:
    // Override paint event
    void paintEvent(QPaintEvent *event) override;
    
    // Override mouse events
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    
    // Override resize event
    void resizeEvent(QResizeEvent *event) override;
    
    // Override show event
    void showEvent(QShowEvent *event) override;

private:
    Ui::waterfallgraph *ui;
    QGraphicsScene *graphicsScene;
    
    // Drawing area and grid
    QRectF drawingArea;
    bool gridEnabled;
    int gridDivisions;
    void draw();
    void setupDrawingArea();
    void drawGrid();
    
    // Data storage
    std::vector<double> xData;
    std::vector<double> yData;
    
    // Mouse tracking
    bool isDragging;
    QPointF lastMousePos;
};

#endif // WATERFALLGRAPH_H
