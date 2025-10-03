#ifndef WATERFALLGRAPH_H
#define WATERFALLGRAPH_H

#include "drawutils.h"
#include "timelineutils.h"
#include "waterfalldata.h"
#include <QColor>
#include <QCoreApplication>
#include <QFont>
#include <QGraphicsEllipseItem>
#include <QGraphicsLineItem>
#include <QGraphicsPolygonItem>
#include <QGraphicsRectItem>
#include <QGraphicsScene>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsTextItem>
#include <QGraphicsView>
#include <QMouseEvent>
#include <QPainterPath>
#include <QPalette>
#include <QPolygonF>
#include <QResizeEvent>
#include <QShowEvent>
#include <QTime>
#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>
#include <map>
#include <vector>

class WaterfallGraph : public QWidget
{
    Q_OBJECT

public:
    explicit WaterfallGraph(QWidget *parent = nullptr, bool enableGrid = false, int gridDivisions = 10, TimeInterval timeInterval = TimeInterval::FifteenMinutes);
    ~WaterfallGraph();

    // Data source management
    void setDataSource(WaterfallData &dataSource);
    WaterfallData *getDataSource() const;

    // Time interval configuration
    void setTimeInterval(TimeInterval interval);
    TimeInterval getTimeInterval() const;
    qint64 getTimeIntervalMs() const;

    // Grid configuration
    void setGridEnabled(bool enabled);
    bool isGridEnabled() const;
    void setGridDivisions(int divisions);
    int getGridDivisions() const;

    // Data handling (delegates to data source)
    void setData(const std::vector<qreal> &yData, const std::vector<QDateTime> &timestamps);
    void setData(const WaterfallData &data);
    void clearData();

    // Incremental data addition methods (delegates to data source)
    void addDataPoint(qreal yValue, const QDateTime &timestamp);
    void addDataPoints(const std::vector<qreal> &yValues, const std::vector<QDateTime> &timestamps);

    // Data access methods (delegates to data source)
    WaterfallData getData() const;
    std::vector<std::pair<qreal, QDateTime>> getDataWithinYExtents(qreal yMin, qreal yMax) const;
    std::vector<std::pair<qreal, QDateTime>> getDataWithinTimeRange(const QDateTime &startTime, const QDateTime &endTime) const;

    // Direct access to data vectors (delegates to data source)
    const std::vector<qreal> &getYData() const;
    const std::vector<QDateTime> &getTimestamps() const;

    // Mouse event handlers (virtual so they can be overridden in derived classes)
    virtual void onMouseClick(const QPointF &scenePos);
    virtual void onMouseDrag(const QPointF &scenePos);

    // Auto-update Y range methods
    void setAutoUpdateYRange(bool enabled);
    bool getAutoUpdateYRange() const;

protected:
    // Override mouse events
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

    // Override resize event
    void resizeEvent(QResizeEvent *event) override;

    // Override show event
    void showEvent(QShowEvent *event) override;

    // Auto-update Y range flag
    bool autoUpdateYRange;

protected:
    QGraphicsView *graphicsView;
    QGraphicsScene *graphicsScene;

    // Overlay scene for interactive elements
    QGraphicsView *overlayView;
    QGraphicsScene *overlayScene;

    // Drawing area and grid
    QRectF drawingArea;
    bool gridEnabled;
    int gridDivisions;
    void setupDrawingArea();
    virtual void drawGrid();
    void updateGraphicsDimensions();

    // Data plotting methods
    virtual void drawDataLine();
    virtual void drawAllDataSeries();
    virtual void drawDataSeries(const QString &seriesLabel);
    QPointF mapDataToScreen(qreal yValue, const QDateTime &timestamp) const;
    void updateDataRanges();
    void updateYRange();
    void updateYRangeFromData();
    void updateYRangeFromCustom();
    void forceRangeUpdate();

    // Data range tracking
    qreal yMin, yMax;
    QDateTime timeMin, timeMax;
    bool dataRangesValid;

    // Range limiting properties
    bool rangeLimitingEnabled;
    qreal customYMin, customYMax;

    // Time interval configuration
    TimeInterval timeInterval;

    // Data source reference
    WaterfallData *dataSource;

    // Multi-series support
    std::map<QString, QColor> seriesColors;
    std::map<QString, bool> seriesVisibility;

    // Mouse tracking
    bool isDragging;
    QPointF lastMousePos;

    // Mouse selection functionality
    bool mouseSelectionEnabled;
    QPointF selectionStartPos;
    QPointF selectionEndPos;
    QGraphicsRectItem *selectionRect;

    // Selection methods
    void startSelection(const QPointF &scenePos);
    void updateSelection(const QPointF &scenePos);
    void endSelection();
    void clearSelection();
    QTime mapScreenToTime(qreal yPos) const;

public:
    // Mouse selection control
    void setMouseSelectionEnabled(bool enabled);
    bool isMouseSelectionEnabled() const;

    // Test method to manually create a selection rectangle
    void testSelectionRectangle();

    // Range limiting methods
    void setRangeLimitingEnabled(bool enabled);
    bool isRangeLimitingEnabled() const;
    void setCustomYRange(const qreal yMin, const qreal yMax);
    std::pair<qreal,qreal> getCustomYRange() const;
    void unsetCustomYRange();

    // Time range update method
    void updateTimeRange();

    // Public draw method for external redraw triggers
    virtual void draw();

    // Drawing methods for custom elements
    void drawPoint(const QPointF &position, const QColor &color = Qt::white, qreal size = 2.0);
    void drawAxisLine(const QPointF &startPos, const QPointF &endPos, const QColor &color = QColor(255, 255, 255, 128));
    void drawCharacterLabel(const QString &text, const QPointF &position, const QColor &color = Qt::white, int fontSize = 12);
    void drawTriangleMarker(const QPointF &position, const QColor &fillColor = Qt::red, const QColor &outlineColor = Qt::black, qreal size = 8.0);
    void drawScatterplot(const QString &seriesLabel, const QColor &pointColor = Qt::white, qreal pointSize = 3.0, const QColor &outlineColor = Qt::black);
    void drawScatterplot(const QColor &pointColor = Qt::white, qreal pointSize = 3.0, const QColor &outlineColor = Qt::black);

    // Multi-series support methods
    void setSeriesColor(const QString &seriesLabel, const QColor &color);
    QColor getSeriesColor(const QString &seriesLabel) const;
    void setSeriesVisible(const QString &seriesLabel, bool visible);
    bool isSeriesVisible(const QString &seriesLabel) const;
    std::vector<QString> getVisibleSeries() const;

signals:
    void SelectionCreated(const TimeSelectionSpan &selection);
};

#endif // WATERFALLGRAPH_H
