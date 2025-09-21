#ifndef WATERFALLGRAPH_H
#define WATERFALLGRAPH_H

#include <QWidget>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QMouseEvent>
#include <QGraphicsSceneMouseEvent>
#include <QResizeEvent>
#include <QShowEvent>
#include <QVBoxLayout>
#include <QPalette>
#include <QPainterPath>
#include <vector>
#include "drawutils.h"
#include "waterfalldata.h"

class waterfallgraph : public QWidget
{
    Q_OBJECT

public:
    explicit waterfallgraph(QWidget *parent = nullptr, bool enableGrid = true, int gridDivisions = 10);
    ~waterfallgraph();
    
    // Data source management
    void setDataSource(WaterfallData& dataSource);
    WaterfallData* getDataSource() const;
    
    // Data handling (delegates to data source)
    void setData(const std::vector<qreal>& yData, const std::vector<QDateTime>& timestamps);
    void setData(const WaterfallData& data);
    void clearData();
    
    // Incremental data addition methods (delegates to data source)
    void addDataPoint(qreal yValue, const QDateTime& timestamp);
    void addDataPoints(const std::vector<qreal>& yValues, const std::vector<QDateTime>& timestamps);
    
    // Data access methods (delegates to data source)
    WaterfallData getData() const;
    std::vector<std::pair<qreal, QDateTime>> getDataWithinYExtents(qreal yMin, qreal yMax) const;
    std::vector<std::pair<qreal, QDateTime>> getDataWithinTimeRange(const QDateTime& startTime, const QDateTime& endTime) const;
    
    // Direct access to data vectors (delegates to data source)
    const std::vector<qreal>& getYData() const;
    const std::vector<QDateTime>& getTimestamps() const;
    
    // Mouse event handlers (virtual so they can be overridden in derived classes)
    virtual void onMouseClick(const QPointF& scenePos);
    virtual void onMouseDrag(const QPointF& scenePos);

protected:
    // Override mouse events
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    
    // Override resize event
    void resizeEvent(QResizeEvent *event) override;
    
    // Override show event
    void showEvent(QShowEvent *event) override;

private:
    QGraphicsView *graphicsView;
    QGraphicsScene *graphicsScene;
    
    // Drawing area and grid
    QRectF drawingArea;
    bool gridEnabled;
    int gridDivisions;
    void draw();
    void setupDrawingArea();
    void drawGrid();
    void updateGraphicsDimensions();
    
    // Data plotting methods
    void drawDataLine();
    QPointF mapDataToScreen(qreal yValue, const QDateTime& timestamp) const;
    void updateDataRanges();
    
    // Data range tracking
    qreal yMin, yMax;
    QDateTime timeMin, timeMax;
    bool dataRangesValid;
    
    // Data source reference
    WaterfallData* dataSource;
    
    // Mouse tracking
    bool isDragging;
    QPointF lastMousePos;
};

#endif // WATERFALLGRAPH_H
