#ifndef TWOAXISGRAPH_H
#define TWOAXISGRAPH_H

#include <QWidget>
#include <QPainter>
#include <QDebug>
#include <QGraphicsScene>
#include "twoaxisdata.h"

class TwoAxisGraph : public QWidget
{
    Q_OBJECT

public:
    explicit TwoAxisGraph(QWidget* parent = nullptr);
    ~TwoAxisGraph();

    // Set the percentage of events to drop (0-100)
    void setEventDropPercentage(int percentage) {
        dropPercentage = qBound(0, percentage, 100);
    }

    // Data handling
    bool setData(const std::vector<double>& x,
        const std::vector<double>& y1,
        const std::vector<double>& y2);

    void setAxesLabels(const QString& xLabel,
        const QString& y1Label,
        const QString& y2Label);

protected:
    void paintEvent(QPaintEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;

private:
    // Drawing functions
    void draw();
    void drawBackground();
    void drawGraphArea();
    void drawAxes();  // New method for drawing axes
    void drawTestPattern();
    void drawInfoArea();
    void drawAxesLabels();
    void drawCursor();
    void drawData();

private:
    QGraphicsScene* scene;
    QPoint currentMousePos;  // Store current mouse position
    TwoAxisData data;       // Store the plotting data

    // Debounce settings
    int eventCount = 0;
    int dropPercentage = 50;  // Drop 80% of events by default
    bool shouldProcessEvent();

    // Utility functions
    QRectF getGraphDrawArea() const;
    qreal getSceneCoordinates(const QPoint& widgetPos) const;

    // Text rendering
    QGraphicsTextItem* createAxisLabel(const QString& text,
        const QColor& textColor,
        const QColor& borderColor,
        const QColor& backgroundColor);

    // Axis labels
    QString leftAxisLabelText;
    QString rightAxisLabelText;
    QString bottomAxisLabelText;

};
#endif // TWOAXISGRAPH_H
