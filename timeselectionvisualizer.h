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
#define RESIZE_EDGE_THRESHOLD 4   // pixels from top/bottom edge for resize vs center drag
#define MIN_SELECTION_SECONDS 1   // minimum duration when resizing

#include <utility>

// Hit zone when clicking on an existing selection
enum class SelectionHitZone { None, TopEdge, BottomEdge, Center };

// Interaction mode after mouse press
enum class InteractionMode { None, Creating, ResizingTop, ResizingBottom, Dragging };

class TimeVisualizerWidget : public QWidget
{
    Q_OBJECT

public:
    explicit TimeVisualizerWidget(QWidget* parent = nullptr);

    // Time selection management
    bool addTimeSelection(TimeSelectionSpan span);
    void setTimeSelection(int index, const TimeSelectionSpan& span);  // replace at index (for sync from other containers)
    void clearTimeSelections();
    bool hasTimeSelections() const { return !m_timeSelections.isEmpty(); }
    void createFullSelection();

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
    void timeSelectionModified(int index, const TimeSelectionSpan& newSpan);

private:
    QList<TimeSelectionSpan> m_timeSelections;
    QTime m_timeLineLength;
    QTime m_currentTime;

    // Valid selection range (inclusive). If start or end is null, no range enforcement
    QTime m_validStartTime;
    QTime m_validEndTime;

    // Mouse selection state (creating new selection)
    bool m_isSelecting;
    int m_selectionStartY;
    int m_selectionEndY;

    // Resize/drag state (interacting with existing selection)
    InteractionMode m_interactionMode;
    int m_interactionSelectionIndex;
    int m_dragStartY;
    TimeSelectionSpan m_dragStartSpan;

    void updateVisualization();
    void drawSelection(QPainter& painter, const TimeSelectionSpan& span);
    void drawCurrentSelection(QPainter& painter);
    QTime yCoordinateToTime(int y) const;
    QDateTime timeAtY(int y) const;
    TimeSelectionSpan calculateSelectionSpan(int startY, int endY) const;
    bool hasValidRange() const { return !m_validStartTime.isNull() && !m_validEndTime.isNull(); }
    TimeSelectionSpan clampToValidRange(const TimeSelectionSpan& span) const;
    QRect getSelectionRect(int index) const;
    std::pair<int, SelectionHitZone> hitTest(int x, int y) const;
};

class TimeSelectionVisualizer : public QWidget
{
    Q_OBJECT

public:
    explicit TimeSelectionVisualizer(QWidget* parent = nullptr, QTimer* timer = nullptr, int clearButtonHeight = BUTTON_SIZE);
    ~TimeSelectionVisualizer();

    // Delegate methods to the visualizer widget
    bool addTimeSelection(TimeSelectionSpan span) { return m_visualizerWidget->addTimeSelection(span); }
    void setTimeSelection(int index, const TimeSelectionSpan& span) { m_visualizerWidget->setTimeSelection(index, span); }
    void clearTimeSelections() { m_visualizerWidget->clearTimeSelections(); }
    void createFullSelection() { m_visualizerWidget->createFullSelection(); }
    bool hasTimeSelections() const { return m_visualizerWidget->hasTimeSelections(); }
    void setTimeLineLength(const QTime& length) { m_visualizerWidget->setTimeLineLength(length); }
    void setTimeLineLength(TimeInterval interval) { m_visualizerWidget->setTimeLineLength(timeIntervalToQTime(interval)); }
    void setCurrentTime(const QTime& currentTime) { m_visualizerWidget->setCurrentTime(currentTime); }
    void setValidSelectionRange(const QTime& start, const QTime& end) { m_visualizerWidget->setValidSelectionRange(start, end); }
    void setValidSelectionRange(const TimeSelectionSpan& span) { m_visualizerWidget->setValidSelectionRange(span); }

signals:
    void timeSelectionsCleared();
    void timeSelectionMade(const TimeSelectionSpan& span);
    void timeSelectionModified(int index, const TimeSelectionSpan& newSpan);
    /** Emitted when the user clicks the H button with no selections; container can create selection from real time to BTW line or fall back to full range */
    void fullSelectionRequested();

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
