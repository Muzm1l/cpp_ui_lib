#ifndef TIMELINEVIEW_H
#define TIMELINEVIEW_H

#include <QWidget>
#include <QPushButton>
#include <QVBoxLayout>
#include <QPainter>
#include <QPaintEvent>
#include <QResizeEvent>
#include <QMouseEvent>
#include <QEnterEvent>
#include <QTime>
#include <QList>
#include <QObject>
#include <QTimer>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QGraphicsRectItem>
#include <QPair>
#include <QRect>
#include <QPoint>
#include <vector>
#include "timelineutils.h"
#include "timelinedrawingobjects.h"

// Compile-time parameters
#define TIMELINE_VIEW_BUTTON_SIZE 64
#define TIMELINE_VIEW_GRAPHICS_VIEW_WIDTH 80

// Forward declarations
class TimelineVisualizerWidget;

// Slider Geometry Helper - Centralized calculations for slider dimensions and positions
class SliderGeometry
{
public:
    // Calculate slider rectangle based on position and dimensions
    static QRect calculateSliderRect(int widgetHeight, int widgetWidth, 
                                     const QTime& timeInterval,
                                     int sliderYPosition);
    
    // Calculate slider Y position from time window
    static int calculateSliderYFromTime(const TimeSelectionSpan& timeWindow,
                                        int widgetHeight);
    
    // Calculate time window from slider Y position
    static TimeSelectionSpan calculateTimeWindowFromY(int sliderY,
                                                     const QTime& timeInterval,
                                                     int widgetHeight);
    
    // Get slider bounds (min/max Y) given widget dimensions
    static QPair<int, int> getSliderBounds(int widgetHeight, int sliderHeight);
    
    // Get minimum slider height
    static int getMinimumSliderHeight();
    
    // Calculate slider height for given time interval
    static int calculateSliderHeight(const QTime& timeInterval, int widgetHeight);
    
    // Get twelve hours in minutes constant
    static int getTwelveHoursInMinutes() { return TWELVE_HOURS_IN_MINUTES; }
    
private:
    static const int TWELVE_HOURS_IN_MINUTES;
    static const int MINIMUM_SLIDER_HEIGHT;
};

// Slider State Manager - Encapsulates slider position, time window, and drag state
class SliderState
{
public:
    SliderState();
    
    // Position management
    void setYPosition(int y, int widgetHeight, const QTime& interval);
    int getYPosition() const;
    
    // Time window management
    TimeSelectionSpan getTimeWindow() const;
    void setTimeWindow(const TimeSelectionSpan& window, int widgetHeight, const QTime& interval);
    
    // Drag state management
    void startDrag(const QPoint& mousePos);
    void updateDrag(const QPoint& mousePos, int widgetHeight, const QTime& interval);
    void endDrag(int widgetHeight, const QTime& interval);
    bool isDragging() const;
    
    // Validation and synchronization
    void clampToBounds(int widgetHeight, const QTime& interval);
    void syncTimeWindowFromPosition(int widgetHeight, const QTime& interval);
    void syncPositionFromTimeWindow(int widgetHeight);
    
private:
    int m_yPosition = 0;              // Current pixel position (source of truth)
    TimeSelectionSpan m_timeWindow;    // Time window (derived from position)
    bool m_isDragging = false;
    QPoint m_dragStartMousePos;
    int m_dragStartSliderY;
};

class TimelineVisualizerWidget : public QWidget
{
    Q_OBJECT

public:
    explicit TimelineVisualizerWidget(QWidget* parent = nullptr);
    ~TimelineVisualizerWidget();

    // Properties
    void setTimeLineLength(const QTime& length);
    void setCurrentTime(const QTime& currentTime);
    void setNumberOfDivisions(int divisions);
    void setIsAbsoluteTime(bool isAbsoluteTime);
    QTime getTimeLineLength() const { return m_timeLineLength; }
    QTime getCurrentTime() const { return m_currentTime; }
    int getNumberOfDivisions() const { return m_numberOfDivisions; }

    // Time interval specific methods
    void setTimeInterval(TimeInterval interval);
    TimeInterval getTimeInterval() const { return m_timeInterval; }
    int calculateOptimalDivisions() const;
    int calculateOptimalDivisionsForArea(int areaHeight) const;
    double calculateSegmentDurationSeconds() const;
    double getMinimumSegmentHeight() const;
    int getFixedNumberOfSegments() const;

    // Update and draw loop method
    void updateAndDraw();

    // Label mode control
    void setShowRelativeLabels(bool showRelative);
    bool getShowRelativeLabels() const { return m_showRelativeLabels; }

    // Chevron label control
    void setChevronLabel1(const QString& label);
    void setChevronLabel2(const QString& label);
    void setChevronLabel3(const QString& label);
    QString getChevronLabel1() const;
    QString getChevronLabel2() const;
    QString getChevronLabel3() const;

    // Slider visible window access
    TimeSelectionSpan getVisibleTimeWindow() const { return m_sliderVisibleWindow; }

protected:
    void paintEvent(QPaintEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void enterEvent(QEnterEvent* event) override;

private:
    QTime m_timeLineLength = QTime(0, 15, 0); // Default to 15 minutes
    QTime m_currentTime;
    int m_numberOfDivisions = 15; // Default to 15 segments
    bool m_isAbsoluteTime = true;
    TimeInterval m_timeInterval = TimeInterval::FifteenMinutes; // Default time interval

    // Smooth shifting state
    QTime m_lastCurrentTime;
    double m_pixelSpeed; // pixels per second
    double m_accumulatedOffset; // accumulated pixel offset

    // Drawing objects (only segments and chevron)
    TimelineChevronDrawer* m_chevronDrawer;
    std::vector<TimelineSegmentDrawer*> m_segmentDrawers;

    // Label mode control
    bool m_showRelativeLabels = false;

    // Chevron label storage
    QString m_chevronLabel1 = "1";
    QString m_chevronLabel2 = "2";
    QString m_chevronLabel3 = "3";

    // Slider state - using new state manager
    SliderState m_sliderState;
    
    // Legacy member kept for backward compatibility with getVisibleTimeWindow()
    // This will be removed and replaced by m_sliderState.getTimeWindow()
    TimeSelectionSpan m_sliderVisibleWindow;
    QGraphicsRectItem* m_sliderIndicator = nullptr; // Kept for now but may be removed

    void updateVisualization();
    double calculateTimeOffset();
    void updatePixelSpeed();
    double calculateSmoothOffset();

    // Drawing object management
    void createDrawingObjects();
    void clearDrawingObjects();

    // Helper methods for drawing with QPainter
    void drawSegmentWithPainter(QPainter& painter, TimelineSegmentDrawer* segmentDrawer);
    void drawChevronWithPainter(QPainter& painter, TimelineChevronDrawer* chevronDrawer);
    
    // Slider methods (following zoom slider pattern)
    void createSliderIndicator();
    void updateSliderIndicator();
    void updateSliderFromMousePosition(const QPoint& currentPos);
    void emitTimeScopeChanged();

signals:
    void visibleTimeWindowChanged(const TimeSelectionSpan& selection);

};


class TimelineView : public QWidget
{
    Q_OBJECT

public:
    explicit TimelineView(QWidget* parent = nullptr, QTimer* timer = nullptr);
    ~TimelineView();

    // No time selection methods needed for TimelineView
    void setTimeLineLength(TimeInterval interval) {
        m_visualizerWidget->setTimeInterval(interval);
        updateButtonText(interval);
    }
    void setCurrentTime(const QTime& currentTime) { m_visualizerWidget->setCurrentTime(currentTime); }
    void setNumberOfDivisions(int divisions) { m_visualizerWidget->setNumberOfDivisions(divisions); }

    // Update and draw loop method
    void updateAndDraw() { if (m_visualizerWidget) m_visualizerWidget->updateAndDraw(); }

    // Chevron label control
    void setChevronLabel1(const QString& label);
    void setChevronLabel2(const QString& label);
    void setChevronLabel3(const QString& label);
    QString getChevronLabel1() const;
    QString getChevronLabel2() const;
    QString getChevronLabel3() const;

signals:
    void TimeIntervalChanged(TimeInterval currentInterval);
    void TimeScopeChanged(const TimeSelectionSpan& selection);

private:
    QPushButton* m_intervalChangeButton;
    QPushButton* m_timeModeChangeButton;
    TimelineVisualizerWidget* m_visualizerWidget;
    QVBoxLayout* m_layout;
    int intervalIndex = 0;
    bool m_isAbsoluteTime = true;

    // Timer management
    QTimer* m_timer;
    bool m_ownsTimer;

    void updateButtonText(TimeInterval interval);
    void updateTimeModeButtonText(bool isAbsoluteTime);
    void setupTimer();
    void onVisibleTimeWindowChanged(const TimeSelectionSpan& selection);

private slots:
    void onIntervalButtonClicked();
    void onTimeModeButtonClicked();
    void onTimerTick();

};

#endif // TIMELINEVIEW_H
