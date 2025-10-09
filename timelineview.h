#ifndef TIMELINEVIEW_H
#define TIMELINEVIEW_H

#include <QWidget>
#include <QPushButton>
#include <QVBoxLayout>
#include <QPainter>
#include <QPaintEvent>
#include <QResizeEvent>
#include <QTime>
#include <QList>
#include <QObject>
#include <QTimer>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <vector>
#include "timelineutils.h"
#include "timelinedrawingobjects.h"

// Compile-time parameters
#define TIMELINE_VIEW_BUTTON_SIZE 64
#define TIMELINE_VIEW_GRAPHICS_VIEW_WIDTH 80

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

protected:
    void paintEvent(QPaintEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;

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

private slots:
    void onIntervalButtonClicked();
    void onTimeModeButtonClicked();
    void onTimerTick();

};

#endif // TIMELINEVIEW_H
