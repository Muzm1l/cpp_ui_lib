#ifndef TIMESELECTIONVISUALIZER_H
#define TIMESELECTIONVISUALIZER_H

#include <QWidget>
#include <QPushButton>
#include <QVBoxLayout>
#include <QPainter>
#include <QPaintEvent>
#include <QTime>
#include <QDateTime>
#include <QTimer>
#include <QList>
#include <QMouseEvent>
#include "timelineutils.h"

// Compile-time parameters
#define BUTTON_SIZE 32
#define GRAPHICS_VIEW_WIDTH 32
#define MAX_TIME_SELECTIONS 5

class TimeVisualizerWidget : public QWidget
{
    Q_OBJECT

public:
    explicit TimeVisualizerWidget(QWidget* parent = nullptr);

    // Time selection management
    void addTimeSelection(TimeSelectionSpan span);
    void clearTimeSelections();

    // Valid selection range
    void setValidSelectionRange(const QTime& start, const QTime& end);
    void setValidSelectionRange(const TimeSelectionSpan& span) { setValidSelectionRange(span.startTime.time(), span.endTime.time()); }

    // Properties
    void setTimeLineLength(const QTime& length);
    void setTimeLineLength(TimeInterval interval);
    void setCurrentTime(const QTime& currentTime);

    QTime getTimeLineLength() const { return m_timeLineLength; }
    QTime getCurrentTime() const { return m_currentTime; }

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void mouseDoubleClickEvent(QMouseEvent* event) override;

signals:
    void timeSelectionMade(const TimeSelectionSpan& span);

private:
    QList<TimeSelectionSpan> m_timeSelections;
    QTime m_timeLineLength;
    QTime m_currentTime;

    // Valid selection range (inclusive). If start or end is null, no range enforcement
    QTime m_validStartTime;
    QTime m_validEndTime;

    // Mouse selection state
    bool m_isSelecting;
    int m_selectionStartY;
    int m_selectionEndY;

    void updateVisualization();
    void drawSelection(QPainter& painter, const TimeSelectionSpan& span);
    void drawCurrentSelection(QPainter& painter);
    QTime yCoordinateToTime(int y) const;
    TimeSelectionSpan calculateSelectionSpan(int startY, int endY) const;
    bool hasValidRange() const { return !m_validStartTime.isNull() && !m_validEndTime.isNull(); }
    TimeSelectionSpan clampToValidRange(const TimeSelectionSpan& span) const;
};

class TimeSelectionVisualizer : public QWidget
{
    Q_OBJECT

public:
    explicit TimeSelectionVisualizer(QWidget* parent = nullptr, QTimer* timer = nullptr, int clearButtonHeight = BUTTON_SIZE);
    ~TimeSelectionVisualizer();

    // Delegate methods to the visualizer widget
    void addTimeSelection(TimeSelectionSpan span) { m_visualizerWidget->addTimeSelection(span); }
    void clearTimeSelections() { m_visualizerWidget->clearTimeSelections(); }
    void setTimeLineLength(const QTime& length) { m_visualizerWidget->setTimeLineLength(length); }
    void setTimeLineLength(TimeInterval interval) { m_visualizerWidget->setTimeLineLength(timeIntervalToQTime(interval)); }
    void setCurrentTime(const QTime& currentTime) { m_visualizerWidget->setCurrentTime(currentTime); }
    void setValidSelectionRange(const QTime& start, const QTime& end) { m_visualizerWidget->setValidSelectionRange(start, end); }
    void setValidSelectionRange(const TimeSelectionSpan& span) { m_visualizerWidget->setValidSelectionRange(span); }

signals:
    void timeSelectionsCleared();
    void timeSelectionMade(const TimeSelectionSpan& span);

private slots:
    void onButtonClicked();
    void onTimerTick();

private:
    QPushButton* m_button;
    TimeVisualizerWidget* m_visualizerWidget;
    QVBoxLayout* m_layout;

    // Timer management
    QTimer* m_timer;
    bool m_ownsTimer;

    void setupTimer();
};

#endif // TIMESELECTIONVISUALIZER_H
