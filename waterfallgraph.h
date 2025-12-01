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
#include <QGraphicsPathItem>
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
#include <QEnterEvent>
#include <QResizeEvent>
#include <QShowEvent>
#include <QTime>
#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>
#include <QCursor>
#include <map>
#include <set>
#include <vector>
#include <functional>
#include "sharedsyncstate.h"

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
    void setData(const QString &seriesLabel, const std::vector<qreal> &yData, const std::vector<QDateTime> &timestamps);
    void setData(const WaterfallData &data);
    void clearData();

    // Incremental data addition methods (delegates to data source)
    void addDataPoint(const QString &seriesLabel, qreal yValue, const QDateTime &timestamp);
    void addDataPoints(const QString &seriesLabel, const std::vector<qreal> &yValues, const std::vector<QDateTime> &timestamps);

    // Data access methods (delegates to data source)
    WaterfallData getData(const QString &seriesLabel) const;
    std::vector<std::pair<qreal, QDateTime>> getDataWithinYExtents(const QString &seriesLabel, qreal yMin, qreal yMax) const;
    std::vector<std::pair<qreal, QDateTime>> getDataWithinTimeRange(const QString &seriesLabel, const QDateTime &startTime, const QDateTime &endTime) const;

    // Direct access to data vectors (delegates to data source)
    const std::vector<qreal> &getYData(const QString &seriesLabel) const;
    const std::vector<QDateTime> &getTimestamps(const QString &seriesLabel) const;

    // Mouse event handlers (virtual so they can be overridden in derived classes)
    virtual void onMouseClick(const QPointF &scenePos);
    virtual void onMouseDrag(const QPointF &scenePos);

    // Auto-update Y range methods
    void setAutoUpdateYRange(bool enabled);
    bool getAutoUpdateYRange() const;

protected:
    // Override mouse events
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    // Override mouse move to track cursor for crosshair
    void enterEvent(QEnterEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;

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

    // Cursor layer for dedicated cursor rendering
    QGraphicsView *cursorView;
    QGraphicsScene *cursorScene;
    QTimer *cursorUpdateTimer;
    QGraphicsLineItem *cursorCrosshairHorizontal;
    QGraphicsLineItem *cursorCrosshairVertical;
    QGraphicsLineItem *cursorTimeAxisLine;
    GraphContainerSyncState *m_cursorSyncState;
    QPointF m_lastMousePos;
    bool m_cursorLayerEnabled;

    // Drawing area and grid
    QRectF drawingArea;
    bool gridEnabled;
    int gridDivisions;
    void setupDrawingArea();
    virtual void drawGrid();
    void updateGraphicsDimensions();

    // Data plotting methods
    virtual void drawDataLine(const QString &seriesLabel, bool plotPoints = true);
    virtual void drawAllDataSeries();
    virtual void drawDataSeries(const QString &seriesLabel);
    void drawIncremental();
    QPointF mapDataToScreen(qreal yValue, const QDateTime &timestamp) const;

    // State machine for rendering
    enum class RenderState {
        CLEAN,
        RANGE_UPDATE_ONLY,
        INCREMENTAL_UPDATE,
        FULL_REDRAW
    };
    void setRenderState(RenderState newState);
    void markSeriesDirty(const QString &seriesLabel);
    void markAllSeriesDirty();
    void markRangeUpdateNeeded();
    void transitionToAppropriateState();
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

    // Time range management
    bool customTimeRangeEnabled;
    QDateTime customTimeMin, customTimeMax;

    // Time interval configuration
    TimeInterval timeInterval;

    // Data source reference
    WaterfallData *dataSource;

    // Multi-series support
    std::map<QString, QColor> seriesColors;
    std::map<QString, bool> seriesVisibility;

    // Incremental rendering support
    RenderState m_renderState;
    bool m_rangeUpdateNeeded;
    std::set<QString> m_dirtySeries;
    std::map<QString, QGraphicsPathItem*> m_seriesPathItems;
    std::map<QString, std::vector<QGraphicsEllipseItem*>> m_seriesPointItems;

    // Mouse tracking
    bool isDragging;
    QPointF lastMousePos;

    // Crosshair functionality
    void setupCrosshair();
    void updateCrosshair(const QPointF &mousePos);
    void showCrosshair();
    void hideCrosshair();
    QGraphicsLineItem *crosshairHorizontal;
    QGraphicsLineItem *crosshairVertical;
    bool crosshairEnabled;

    // Cursor callback helpers
    void notifyCursorTimeChanged(const QDateTime &time);
    std::function<void(const QDateTime &)> cursorTimeChangedCallback;
    QDateTime lastNotifiedCursorTime;

    // Time axis cursor functionality
    QGraphicsLineItem *timeAxisCursor;
    qreal mapTimeToY(const QDateTime &time) const;

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
    QDateTime mapScreenToTime(qreal yPos) const;

private slots:
    // Cursor layer update method
    void updateCursorLayer();

public:
    // Mouse selection control
    void setMouseSelectionEnabled(bool enabled);
    bool isMouseSelectionEnabled() const;

    // Test method to manually create a selection rectangle
    void testSelectionRectangle();
    
    // Crosshair control
    void setCrosshairEnabled(bool enabled);
    bool isCrosshairEnabled() const;
    
    // Time axis cursor control
    void setTimeAxisCursor(const QDateTime &time);
    void clearTimeAxisCursor();
    void setCursorTimeChangedCallback(const std::function<void(const QDateTime &)> &callback);
    
    // Cursor layer control
    void setCursorSyncState(GraphContainerSyncState *syncState);
    void setCursorLayerEnabled(bool enabled);
    bool isCursorLayerEnabled() const;
    
    // Public access to overlay scene for interactive elements
    QGraphicsScene* getOverlayScene() const { return overlayScene; }

    // Range limiting methods
    void setRangeLimitingEnabled(bool enabled);
    bool isRangeLimitingEnabled() const;
    void setCustomYRange(const qreal yMin, const qreal yMax);
    std::pair<qreal,qreal> getCustomYRange() const;
    void unsetCustomYRange();

    // Time range update method
    void updateTimeRange();

    // Time range management methods
    void setTimeRange(const QDateTime &timeMin, const QDateTime &timeMax);
    void setTimeMax(const QDateTime &timeMax);
    void setTimeMin(const QDateTime &timeMin);
    QDateTime getTimeMax() const;
    QDateTime getTimeMin() const;
    std::pair<QDateTime, QDateTime> getTimeRange() const;
    void setTimeRangeFromData();
    void setTimeRangeFromDataWithInterval(qint64 intervalMs);
    void unsetCustomTimeRange();

    // Public draw method for external redraw triggers
    virtual void draw();

    // Drawing methods for custom elements
    void drawPoint(const QPointF &position, const QColor &color = Qt::white, qreal size = 2.0);
    void drawAxisLine(const QPointF &startPos, const QPointF &endPos, const QColor &color = QColor(255, 255, 255, 128));
    void drawCharacterLabel(const QString &text, const QPointF &position, const QColor &color = Qt::white, int fontSize = 12);
    void drawTriangleMarker(const QPointF &position, const QColor &fillColor = Qt::red, const QColor &outlineColor = Qt::black, qreal size = 8.0);
    void drawScatterplot(const QString &seriesLabel, const QColor &pointColor = Qt::white, qreal pointSize = 3.0, const QColor &outlineColor = Qt::black);

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
